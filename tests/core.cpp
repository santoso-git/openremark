#include <cassert>
#include <QFileInfo>
#include <QGuiApplication>
#include <QJsonDocument>
#include <QFontDatabase>
#include <QFontMetrics>
#include <QPainter>
#include <iostream>
#include "../src/layout.h"
#include "../src/response.h"
#include "../src/letterpaths.h"
int main(int argc,char **argv) {
    QGuiApplication app(argc,argv);
    // Top handwriting used to be dropped by the old y=170 crop. Verify the
    // exact production crop, including its unchanged bottom/right boundaries.
    const QRect captureArea=diaryCaptureArea();
    assert(captureArea==QRect(150,0,1174,1752));
    // The Ask button sits under the toolbar's close button; it is masked out of every capture.
    assert(diaryButtonRect()==QRect(1292,112,112,112));
    {QImage withButton(1404,1872,QImage::Format_RGB32);withButton.fill(Qt::white);
     {QPainter p(&withButton);p.fillRect(diaryButtonRect().adjusted(40,40,-40,-40),Qt::black);}
     diaryMaskControls(withButton);assert(lastInkRow(withButton.copy(captureArea))==-1);
     assert(withButton.pixelColor(1348,168)==QColor(Qt::white));
     // Email was removed: writing where the Send button used to be is real ink and stays in the capture.
     {QPainter p(&withButton);p.fillRect(QRect(1200,142,84,52),Qt::black);}
     diaryMaskControls(withButton);assert(withButton.pixelColor(1240,168)==QColor(Qt::black));}
    // The Haiku button sits one toolbar cell below Ask and is masked the same way.
    assert(diaryPremiumButtonRect()==QRect(1292,224,112,112));
    {QImage withButton(1404,1872,QImage::Format_RGB32);withButton.fill(Qt::white);
     {QPainter p(&withButton);p.fillRect(diaryPremiumButtonRect().adjusted(40,40,-40,-40),Qt::black);}
     diaryMaskControls(withButton);assert(lastInkRow(withButton.copy(captureArea))==-1);}
    // The tools button (MCP proxy) sits one cell below the Haiku button and is masked too.
    assert(diaryToolsButtonRect()==QRect(1292,336,112,112));
    {QImage withButton(1404,1872,QImage::Format_RGB32);withButton.fill(Qt::white);
     {QPainter p(&withButton);p.fillRect(diaryToolsButtonRect().adjusted(40,40,-40,-40),Qt::black);}
     diaryMaskControls(withButton);assert(lastInkRow(withButton.copy(captureArea))==-1);}
    QImage screen(1404,1872,QImage::Format_RGB32);screen.fill(Qt::white);
    screen.setPixelColor(500,0,Qt::black);
    screen.setPixelColor(500,90,Qt::black);
    auto captured=screen.copy(captureArea);
    assert(captured.pixelColor(350,0)==Qt::black);
    assert(captured.pixelColor(350,90)==Qt::black);
    assert(lastInkRow(captured)+captureArea.y()==90);
    screen.setPixelColor(500,1700,Qt::black);
    screen.setPixelColor(1350,1830,Qt::black); // Bottom-right button is excluded.
    assert(lastInkRow(screen.copy(captureArea))+captureArea.y()==1700);
    QImage page(1174,1582,QImage::Format_RGB32); page.fill(Qt::white);
    assert(lastInkRow(page)==-1);
    page.setPixelColor(500,420,Qt::black); assert(lastInkRow(page)==420);
    page.setPixelColor(0,1581,QColor(180,180,180)); assert(lastInkRow(page)==1581);
    QImage ruled(1174,1582,QImage::Format_RGB32); ruled.fill(Qt::white);
    for(int y=20;y<ruled.height();y+=62)
        for(int x=0;x<ruled.width();++x) ruled.setPixelColor(x,y,QColor(180,180,180));
    for(int y=0;y<ruled.height();++y) ruled.setPixelColor(80,y,QColor(180,180,180));
    for(int y=20;y<ruled.height();y+=62) ruled.setPixelColor(80,y,QColor(127,127,127));
    assert(lastInkRow(ruled)==-1);
    ruled.setPixelColor(400,330,Qt::black); assert(lastInkRow(ruled)==330);
    // A stroke crossing a rule or margin must still reserve space.
    ruled.setPixelColor(400,392,Qt::black); assert(lastInkRow(ruled)==392);
    ruled.setPixelColor(80,450,Qt::black); assert(lastInkRow(ruled)==450);
    ruled.setPixelColor(600,600,QColor(170,170,170)); assert(lastInkRow(ruled)==600);
    ruled.setPixelColor(1000,1570,Qt::black); assert(lastInkRow(ruled)==1570);
    // A white plot interrupts pale margin lines. Their lower fragments are
    // still template, not writing extending to the bottom of the viewport.
    QImage plotted(1174,1582,QImage::Format_RGB32);plotted.fill(Qt::white);
    for(int y=0;y<plotted.height();++y)for(int x:{79,80,89,90})plotted.setPixelColor(x,y,QColor(191,191,191));
    for(int y=20;y<plotted.height();y+=62)for(int x=0;x<plotted.width();++x) {
        bool crossing=x==79||x==80||x==89||x==90;
        plotted.setPixelColor(x,y,crossing?QColor(143,143,143):QColor(191,191,191));
    }
    {QPainter p(&plotted);p.fillRect(QRect(0,120,1174,800),Qt::white);p.fillRect(QRect(105,200,1020,2),Qt::black);}
    plotted.setPixelColor(500,1060,Qt::black);assert(lastInkRow(plotted)==1060);
    // Faint writing and dark marks on the interrupted rules must survive.
    plotted.setPixelColor(700,1160,QColor(180,180,180));assert(lastInkRow(plotted)==1160);
    plotted.setPixelColor(80,1300,Qt::black);assert(lastInkRow(plotted)==1300);
    plotted.setPixelColor(700,1581,Qt::black);assert(lastInkRow(plotted)==1581);
    if(argc>1) {
        QImage actual(QString::fromLocal8Bit(argv[1])); assert(!actual.isNull());
        int bottom=lastInkRow(actual.copy(diaryCaptureArea()))+diaryCaptureArea().y();
        std::cout<<"Captured question bottom: "<<bottom<<"\n";
        assert(bottom>=480 && bottom<550);
    }
    // Chat completions: the answer is choices[0].message.content; anything but a clean stop is an error.
    auto parse=[](const char *json) {return responseText(QJsonDocument::fromJson(json).object());};
    // Each request's generation id is kept so its logged prompt and answer can be looked up at OpenRouter.
    {QStringList ids;
     diaryKeepGenerationId(ids,QJsonDocument::fromJson(R"({"id":"gen-1761-abcXYZ_09","choices":[]})").object());
     diaryKeepGenerationId(ids,QJsonDocument::fromJson(R"({"choices":[]})").object());
     diaryKeepGenerationId(ids,QJsonDocument::fromJson(R"({"id":"gen-1\n../x"})").object());
     assert(ids==QStringList{"gen-1761-abcXYZ_09"});}
    assert(parse(R"({"choices":[{"finish_reason":"stop","message":{"role":"assistant","content":" Hello "}}]})")=="Hello");
    for(auto invalid:{R"({"choices":[{"finish_reason":"length","message":{"content":"cut"}}]})",
                      R"({"choices":[{"finish_reason":"stop","message":{"content":""}}]})",
                      R"({"choices":[{"finish_reason":"stop","message":{"content":null,"refusal":"no"}}]})",
                      R"({"choices":[{"finish_reason":"content_filter","message":{"content":"x"}}]})",
                      R"({"choices":[{"finish_reason":"tool_calls","message":{"content":null}}]})",
                      R"({"choices":[]})",R"({"error":{"message":"invalid key"}})"}) {
        bool failed=false; try {parse(invalid);}catch(const std::runtime_error&){failed=true;} assert(failed);
    }
    {bool cut=false;try{parse(R"({"choices":[{"finish_reason":"length","message":{"content":"cut"}}]})");}
     catch(const std::runtime_error &e){cut=QString(e.what()).contains("Answer incomplete");}assert(cut);}
    // Free models often fence their JSON or put a sentence before it.
    assert(diaryExtractJson("```json\n{\"kind\":\"text\",\"text\":\"Hi\"}\n```")=="{\"kind\":\"text\",\"text\":\"Hi\"}");
    assert(diaryExtractJson("Here you go: {\"a\":{\"b\":1}} done")=="{\"a\":{\"b\":1}}");
    assert(diaryExtractJson("{\"a\":1}")=="{\"a\":1}");
    assert(diaryExtractJson("no json here")=="no json here");
    int id=QFontDatabase::addApplicationFont(QFileInfo("fonts/Caveat.ttf").absoluteFilePath()); assert(id>=0);
    QFont font(QFontDatabase::applicationFontFamilies(id).first()); font.setPixelSize(48); font.setWeight(QFont::Medium);
    QFontMetrics fm(font);
    QString answer="The Moon shines because it reflects sunlight. It does not produce its own light.";
    QRect bounds=fm.boundingRect(QRect(0,0,1150,10000),Qt::TextWordWrap,answer);
    assert(bounds.height()<400);
    QImage sample(1174,bounds.height()+24,QImage::Format_ARGB32);sample.fill(Qt::white);
    QPainter painter(&sample);painter.setFont(font);painter.setPen(Qt::black);
    painter.drawText(QRect(12,8,1150,bounds.height()+8),Qt::TextWordWrap,answer);painter.end();
    assert(lastInkRow(sample)>0);assert(sample.save("handwriting-preview.png"));
    QImage transparent(sample.size(),QImage::Format_ARGB32);transparent.fill(Qt::transparent);
    QPainter imagePainter(&transparent);imagePainter.setFont(font);imagePainter.setPen(Qt::black);
    imagePainter.drawText(QRect(12,8,1150,bounds.height()+8),Qt::TextWordWrap,answer);imagePainter.end();
    assert(qAlpha(transparent.pixel(0,0))==0);
    bool hasInk=false;for(int y=0;y<transparent.height();++y)for(int x=0;x<transparent.width();++x)if(qAlpha(transparent.pixel(x,y))>0)hasInk=true;
    assert(hasInk);assert(transparent.save("image-answer-preview.png"));
    QImage restored("image-answer-preview.png");assert(restored==transparent);
    auto paths=letterPaths(sample);
    assert(!paths.empty());
    size_t points=0;for(const auto &path:paths) {assert(!path.isEmpty());points+=path.size();for(auto p:path)assert(sample.rect().contains(p.toPoint()));}
    QImage oldRaster=sample.scaled(768,768,Qt::KeepAspectRatio,Qt::SmoothTransformation);
    int oldRuns=0;
    for(int y=0;y<oldRaster.height();++y){bool in=false;for(int x=0;x<=oldRaster.width();++x){bool dark=x<oldRaster.width()&&qGray(oldRaster.pixel(x,y))<170;if(dark&&!in){++oldRuns;in=true;}if(!dark)in=false;}}
    std::cout<<"Scanline strokes: "<<oldRuns<<"; letter strokes: "<<paths.size()<<"; letter points: "<<points<<"\n";
    assert(paths.size()*3<size_t(oldRuns));
    QImage preview(sample.width(),sample.height(),QImage::Format_RGB32);preview.fill(Qt::white);
    QPainter pen(&preview);pen.setRenderHint(QPainter::Antialiasing);pen.setPen(QPen(Qt::black,1.8,Qt::SolidLine,Qt::RoundCap,Qt::RoundJoin));
    for(const auto &path:paths)pen.drawPolyline(path);pen.end();assert(preview.save("letter-path-preview.png"));
    QImage ring(20,20,QImage::Format_RGB32);ring.fill(Qt::white);
    for(int i=5;i<=14;++i){ring.setPixelColor(i,5,Qt::black);ring.setPixelColor(i,14,Qt::black);ring.setPixelColor(5,i,Qt::black);ring.setPixelColor(14,i,Qt::black);}
    auto loop=letterPaths(ring);assert(loop.size()==1);assert(loop[0].first()==loop[0].last());
    QImage dot(8,8,QImage::Format_RGB32);dot.fill(Qt::white);dot.setPixelColor(4,4,Qt::black);assert(letterPaths(dot).size()==1);
    QImage blank(8,8,QImage::Format_RGB32);blank.fill(Qt::white);assert(letterPaths(blank).empty());
    std::cout<<"PASS: placement boundaries, response parsing, incomplete/error/refusal handling, font rendering\n";
}
