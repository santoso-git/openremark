// Ink Diary prototype. GPL-3.0-only. See THIRD_PARTY.md.
#include <atomic>
#include <thread>
#include <stdexcept>
#include <QBuffer>
#include <QDir>
#include <QEventLoop>
#include <QFontDatabase>
#include <QFontMetrics>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPointer>
#include <QTemporaryDir>
#include <QUuid>
#include <QMetaMethod>
#include <QTimer>
#include <QFileInfo>
#include <functional>
#include <QVersionNumber>
#include "inserter.inc"
#include "layout.h"
#include "response.h"
#include "plot.h"
#include "api.h"
#include "diagrams.h"
#include "webanswer.h"
#include "markdown.h"
#include "freshness.h"
#include "reading.h"
#include "letterpaths.h"
#include "pathwriter.inc"

namespace {
std::atomic<bool> busy{false}, cancelled{false};
QMutex stateMutex;
QString message = "Ask";
QPointer<QObject> originController, originView;
QString originPage;
const QString base = "/home/root/.config/ink-diary";
std::atomic<bool> lastError{false};
// Status shown by the button. Errors stay visible until the next tap (see inkDiary.qmd).
void state(const QString &text) { QMutexLocker lock(&stateMutex); message = text; lastError=false; }
void fail(const QString &text) { state(text); lastError=true; }
void require(bool valid, const char *why) { if (!valid) throw std::runtime_error(why); }
QImage capture(const QString &path) {
    QFile::remove(path);
    QByteArray arg = (path + ",0,0").toUtf8();
    char *result = rm_shot$screenshotHandler(arg.data());
    bool started = result && QByteArray(result) == "success";
    free(result);
    require(started, "Screenshot failed to start");
    // rm-shot writes asynchronously. Decode rather than accepting file existence.
    for (int i=0; i<100; ++i) {
        require(diaryGuard(), "Cancelled or page changed");
        QThread::msleep(100);
        QImage image(path);
        if (!image.isNull()) { diaryMaskControls(image); return image; }
    }
    throw std::runtime_error("Screenshot timed out");
}

QString answer(const QImage &image, const QJsonObject &config, QStringList &ids) {
    QString key = config.value("api_key").toString().trimmed();
    QString model = config.value("model").toString().trimmed();
    require(!key.isEmpty() && !model.isEmpty(), "Set api_key and model in config.json");
    QByteArray png;
    QBuffer buffer(&png); buffer.open(QIODevice::WriteOnly); image.save(&buffer,"PNG");
    auto payload=diaryReadingPayload(png,config);
    try {auto root=diaryApiRequest(payload,config,diaryGuard);diaryKeepGenerationId(ids,root);return responseText(root);}
    catch(const DiaryHttpError &error) {
        // Models without structured outputs reject response_format; ask for the JSON in the prompt instead.
        if(error.code!=400&&error.code!=422)throw;
        auto root=diaryApiRequest(diaryWithoutFormat(payload),config,diaryGuard);diaryKeepGenerationId(ids,root);return responseText(root);
    }
}

void work(const QByteArray &button) {
    try {
        QTemporaryDir temp("/tmp/ink-diary-XXXXXX");
        require(temp.isValid(), "Cannot create private temporary directory");
        QFile file(base+"/config.json");
        require(file.open(QIODevice::ReadOnly), "Create ink-diary/config.json first");
        QJsonObject config=QJsonDocument::fromJson(file.readAll()).object();
        require(!config.isEmpty(), "Invalid config.json");
        if(button=="premium") config=diaryPremiumConfig(config);
        else if(button=="tools") config=diaryToolsConfig(config);
        state("Reading page...");
        // Let the button switch to its stable busy appearance before capture.
        QThread::msleep(800);
        auto screen=capture(temp.path()+"/question.png");
        require(screen.width()==1404 && screen.height()==1872,
                "First version requires rM2 portrait (1404 x 1872)");
        QSize canvas;
        runOnGuiSync([&] {canvas=activeCanvasSize();});
        require(canvas==screen.size(), "Screen/input coordinates differ; refusing to draw");
        const QRect area=diaryCaptureArea();
        QImage question=screen.copy(area);
        int last=lastInkRow(question);
        require(last>=0, "Write a question on a blank page first");
        int top=area.y()+last+45;
        // Native scene images can extend below the current viewport. Render at
        // a fixed readable size, independent of empty space on screen.
        int width=area.width(), available=1582;
        DiaryAnswer resultAnswer;
        QStringList generationIds;
        state("Thinking...");
        bool demo=config.value("demo_mode").toBool(false);
        if(demo) resultAnswer.text="This is a handwriting test. Live answers will work after you configure an API key.";
        else resultAnswer=decodeDiaryAnswer(answer(question,config,generationIds));
        if(!demo)diaryRequireFreshSearch(resultAnswer);
        diaryApplySearchSetting(resultAnswer,config);
        DiaryWebAnswer web;
        if(resultAnswer.search) {
            state("Searching the web...");
            auto payload=diaryWithEffort(diarySearchPayload(resultAnswer.text,config.value("model").toString(),config.value("timezone").toString("America/New_York")),config);
            auto searchRoot=diaryApiRequest(payload,config,diaryGuard);diaryKeepGenerationId(generationIds,searchRoot);
            web=parseDiarySearch(searchRoot);
            web.checkedAt=diaryLocalClock(QDateTime::currentDateTimeUtc(),config.value("timezone").toString("America/New_York"));
            resultAnswer.text=web.text;
        }
        QString text=resultAnswer.text;
        QImage ink; QString renderError, renderedHtml;
        runOnGuiSync([&] {
            if(!resultAnswer.diagram.isEmpty()) {
                try {ink=renderDiaryDiagram(resultAnswer.diagram,text,width);}
                catch(const std::exception &error){renderError=QString::fromUtf8(error.what());}
                return;
            }
            if(resultAnswer.plot) {
                try {ink=renderPlot(resultAnswer,width,available);}
                catch(const std::exception &error){renderError=QString::fromUtf8(error.what());}
                return;
            }
            static int fontId=QFontDatabase::addApplicationFont(base+"/Caveat.ttf");
            static int monoId=QFontDatabase::addApplicationFont(base+"/DejaVuSansMono.ttf");
            if(monoId<0) {renderError="Code font missing; reinstall Ink Diary";return;}
            if(fontId<0) {renderError="Caveat font missing";return;}
            try {
                // Web sources are saved in the .png.html file, not printed on the page.
                auto rendered=renderDiaryMarkdown(text,QFontDatabase::applicationFontFamilies(fontId).first(),width,
                    QString(),{},resultAnswer.cellLines);
                ink=rendered.image;renderedHtml=resultAnswer.search?diarySourcesHtml(web):rendered.html;
            } catch(const std::exception &error) {renderError=QString::fromUtf8(error.what());}
        });
        if(!renderError.isEmpty()) throw std::runtime_error(renderError.toStdString());
        state("Checking page...");
        auto current=capture(temp.path()+"/check.png");
        require(current.copy(area)==question, "Page changed while thinking; tap Ask again");
        require(diaryGuard(), "Cancelled or page changed");
        const QString answerDir="/home/root/.local/share/ink-diary/answers";
        require(QDir().mkpath(answerDir), "Cannot create answer image directory");
        QFile::setPermissions(answerDir,QFileDevice::ReadOwner|QFileDevice::WriteOwner|QFileDevice::ExeOwner);
        QString imagePath=answerDir+"/"+QUuid::createUuid().toString(QUuid::WithoutBraces)+".png";
        require(ink.save(imagePath), "Cannot save answer image");
        QFile::setPermissions(imagePath,QFileDevice::ReadOwner|QFileDevice::WriteOwner);
        if(!generationIds.isEmpty()) {
            // OpenRouter generation ids, one per request, for looking up the logged prompt and answer.
            QFile ids(imagePath+".ids");
            require(ids.open(QIODevice::WriteOnly),"Cannot save generation ids");
            ids.setPermissions(QFileDevice::ReadOwner|QFileDevice::WriteOwner);
            QByteArray bytes=(generationIds.join('\n')+'\n').toUtf8();
            require(ids.write(bytes)==bytes.size(),"Cannot save generation ids");
        }
        QString htmlPath=imagePath+".html";
        if(!renderedHtml.isEmpty()) {
            QFile html(htmlPath);
            require(html.open(QIODevice::WriteOnly),"Cannot save source links");
            html.setPermissions(QFileDevice::ReadOwner|QFileDevice::WriteOwner);
            QByteArray bytes=renderedHtml.toUtf8();
            require(html.write(bytes)==bytes.size(),"Cannot save source links");
        }
        QString sourcePath=imagePath+(!resultAnswer.diagram.isEmpty() ? ".source.txt" : ".md");
        if(!resultAnswer.plot) {
            QFile raw(sourcePath);
            require(raw.open(QIODevice::WriteOnly),"Cannot save answer Markdown");
            raw.setPermissions(QFileDevice::ReadOwner|QFileDevice::WriteOwner);
            auto bytes=text.toUtf8();require(raw.write(bytes)==bytes.size(),"Cannot save answer Markdown");
        }
        require(diaryGuard(), "Cancelled or page changed");
        state("Inserting image...");
        auto result=insertSceneImage(QJsonObject{{"path",imagePath},{"coordinateSpace","screen"},
            {"x",area.x()+width/2.0},{"y",top+ink.height()/2.0},{"appendBelowContent",true}});
        if(!result.value("ok").toBool()) {
            QFile::remove(imagePath);
            QFile::remove(htmlPath);
            QFile::remove(sourcePath);
            throw std::runtime_error(result.value("message").toString().toStdString());
        }
        state(!resultAnswer.diagram.isEmpty() ? "Diagram added; scroll down if needed" : resultAnswer.plot ? "Plot added; scroll down if needed" : resultAnswer.search ? (web.sources.isEmpty() ? "Web answer added; source links unavailable" : "Web answer + sources added") : "Answer added; scroll down if needed");

    } catch(const std::exception &error) { fail(QString::fromUtf8(error.what())); }
    busy=false;
}
}

static bool diaryGuard() {
    if(cancelled) return false;
    bool valid=false;
    runOnGuiSync([&] {
        auto now=findActivePage();
        valid=originController && originView && now.controller==originController.data()
            && now.documentView==originView.data() && now.pageId==originPage;
    });
    if(!valid) cancelled=true;
    return valid;
}
extern "C" char *diary_start(const char *argument) {
    QByteArray button=argument?QByteArray(argument):QByteArray();
    if(busy.exchange(true)) return response(QJsonObject{{"ok",false},{"message","Already busy"}});
    cancelled=false;
    bool valid=false;
    runOnGuiSync([&] {
        auto context=findActivePage();
        originController=context.controller; originView=context.documentView; originPage=context.pageId;
        valid=originController && originView && !originPage.isEmpty();
    });
    if(!valid) {busy=false; fail("Open a notebook page first"); return response(QJsonObject{{"ok",false}});}
    state("Starting...");
    std::thread([button]{work(button);}).detach();
    return response(QJsonObject{{"ok",true}});
}
extern "C" char *diary_cancel(const char *) { cancelled=true; return response(QJsonObject{{"ok",true}}); }
extern "C" char *diary_status(const char *) {
    QFile config(base+"/config.json");
    bool demo=config.open(QIODevice::ReadOnly) && QJsonDocument::fromJson(config.readAll()).object().value("demo_mode").toBool();
    QMutexLocker lock(&stateMutex);
    return response(QJsonObject{{"busy",busy.load()},{"error",lastError.load()},{"message",message=="Ask" && demo ? "Ask (demo)" : message}});
}
// Load on any firmware whose Qt is binary compatible with the SDK we were built
// against (same major, same or newer minor). Notebook-app internals are probed at
// runtime instead; missing ones produce a status message, never a guess.
extern "C" char _xovi_shouldLoad() {
    auto runtime=QVersionNumber::fromString(QString::fromLatin1(qVersion()));
    auto built=QVersionNumber::fromString(QStringLiteral(QT_VERSION_STR));
    if(runtime.majorVersion()!=built.majorVersion() || runtime.minorVersion()<built.minorVersion()) {
        qWarning("[ink-diary] not loading: built for Qt %s, firmware has Qt %s", QT_VERSION_STR, qVersion());
        return false;
    }
    return dlsym(RTLD_DEFAULT,"_Z21qRegisterResourceDataiPKhS0_S0_")!=nullptr;
}
extern "C" char *diary_probe(const char *arg) {
    if(arg && QByteArray(arg)=="checklist") {
        QJsonObject check;
        runOnGuiSync([&] {
            try {
                int id=QFontDatabase::addApplicationFont(base+"/Caveat.ttf");
                QString text;for(int i=0;i<10;++i)text+="- [ ]\n";
                auto rendered=renderDiaryMarkdown(text,QFontDatabase::applicationFontFamilies(id).first(),1174);
                check.insert("checkboxes",rendered.html.count("class=\"unchecked\""));
                check.insert("height",rendered.image.height());
            } catch(const std::exception &e){check.insert("error",QString::fromUtf8(e.what()));}
        });
        return response(check);
    }
    auto result=capabilities();
    bool imageSupport=false,mapping=false;
    runOnGuiSync([&] {
        auto context=findActivePage();
        if(!context.controller)return;
        for(int i=0;i<context.controller->metaObject()->methodCount();++i)
            if(context.controller->metaObject()->method(i).name()=="insertImageFileAsSceneItem")imageSupport=true;
        for(auto object:runtimeObjects()) {
            if(!qobject_cast<QQuickItem *>(object) || !isVisibleObject(object) || controllerProperty(object,"controller")!=context.controller)continue;
            for(int i=0;i<object->metaObject()->methodCount();++i)
                if(object->metaObject()->method(i).name()=="viewToScene")mapping=true;
        }
    });
    result.insert("nativeImageInsertAvailable",imageSupport);result.insert("screenMappingAvailable",mapping);
    return response(result);
}
