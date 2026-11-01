#include "../src/plot.h"
#include <QFileInfo>
#include "../src/markdown.h"
#include <QGuiApplication>
#include <QFontDatabase>
#include <cassert>
#include <iostream>
int main(int argc,char **argv){QGuiApplication app(argc,argv);
 QJsonObject t{{"title",""},{"count",10},{"labels",QJsonArray()},{"columns",QJsonArray()},{"rows",QJsonArray()}};
 auto text=diaryTemplateMarkdown("checklist",t);assert(text.count("- [ ]")==10);
 auto decode=[&](QString kind){return decodeDiaryAnswer(QJsonDocument(QJsonObject{{"kind",kind},{"template",t}}).toJson()).text;};assert(decode("checklist")==text);
 t["count"]=2;t["labels"]=QJsonArray{"Use | pipes","**literal**"};text=decode("checklist");assert(text.contains("\\|"));assert(text.contains("\\*\\*literal\\*\\*"));
 bool failed=false;t["count"]=3;try{decode("checklist");}catch(...){failed=true;}assert(failed);
 t["labels"]=QJsonArray();t["columns"]=QJsonArray{"Pros","Cons"};t["count"]=10;text=decode("table");assert(text.count('\n')==12);
 DiaryMarkdownDocument doc;doc.setMarkdown(text,QTextDocument::MarkdownDialectGitHub);auto table=dynamic_cast<QTextTable*>(doc.rootFrame()->childFrames().first());assert(table&&table->rows()==11&&table->columns()==2);
 t["rows"]=QJsonArray{QJsonArray{"one"}};failed=false;try{decode("table");}catch(...){failed=true;}assert(failed);
 t["rows"]=QJsonArray();t["count"]=1e100;failed=false;try{decode("table");}catch(...){failed=true;}assert(failed);
 t["count"]=5;t["title"]="Pros and cons";int id=QFontDatabase::addApplicationFont(QFileInfo("fonts/Caveat.ttf").absoluteFilePath());assert(id>=0);auto rendered=renderDiaryMarkdown(decode("table"),QFontDatabase::applicationFontFamilies(id).first(),1174);
 QImage preview(rendered.image.size(),QImage::Format_RGB32);preview.fill(Qt::white);QPainter p(&preview);p.drawImage(0,0,rendered.image);p.end();assert(preview.save("template-table-preview.png"));
 t["cell_lines"]=3;auto answer=decodeDiaryAnswer(QJsonDocument(QJsonObject{{"kind","table"},{"template",t}}).toJson());assert(answer.cellLines==3);
 auto tall=renderDiaryMarkdown(answer.text,QFontDatabase::applicationFontFamilies(id).first(),1174,QString(),{},answer.cellLines);
 assert(tall.image.height()>=rendered.image.height()+5*115);assert(tall.html.count("<tr")==rendered.html.count("<tr"));

 t["cell_lines"]=11;failed=false;try{diaryTemplateCellLines(t);}catch(...){failed=true;}assert(failed);
 auto schema=diaryAnswerSchema();assert(schema["required"].toArray().contains("template"));
 std::cout<<"PASS: exact checklist/table counts, literal labels, blank rows, malformed templates rejected\n";
}
