#include "../src/webanswer.h"
#include <QFileInfo>
#include "../src/api.h"
#include "../src/reading.h"
#include "../src/plot.h"
#include "../src/freshness.h"
#include <QGuiApplication>
#include <QFile>
#include <QFontDatabase>
#include <cassert>
#include <iostream>
int main(int argc,char **argv){
 QGuiApplication app(argc,argv);
 auto example=QDateTime::fromString("2026-10-20T01:42:00Z",Qt::ISODate);
 auto local=diaryLocalClock(example);assert(local.contains("2026-10-19 09:42 PM EDT"));
 assert(local.contains("-04:00"));
 auto winter=diaryLocalClock(QDateTime::fromString("2026-12-20T02:42:00Z",Qt::ISODate));assert(winter.contains("2026-12-19 09:42 PM EST"));assert(winter.contains("-05:00"));
 auto spring=diaryLocalClock(QDateTime::fromString("2026-03-08T07:00:00Z",Qt::ISODate));assert(spring.contains("03:00 AM EDT"));
 auto fall=diaryLocalClock(QDateTime::fromString("2026-11-01T06:00:00Z",Qt::ISODate));assert(fall.contains("01:00 AM EST"));
 assert(diaryCurrentContext(example,"America/New_York").contains("2026-10-19 09:42 PM EDT"));
 for(const auto &question:{"Who is the current president?","Who is the US president today?","Who is the president of France?","What happened recently?","Search lunar eclipses"}) {
  DiaryAnswer a;a.command=question;a.text="An old answer";diaryRequireFreshSearch(a);assert(a.search&&a.text==question);
 }
 for(const auto &question:{"Who was president in 1990?","Explain electric current","Why does the Moon shine?"}) {
  DiaryAnswer a;a.command=question;diaryRequireFreshSearch(a);assert(!a.search);
 }
 DiaryAnswer graph;graph.plot=true;graph.command="plot y=x today";diaryRequireFreshSearch(graph);assert(!graph.search);
 auto timedPayload=diarySearchPayload("Weather in Lowell MA","google/gemma-4-31b-it:free","America/New_York",example);
 auto timedMessages=timedPayload["messages"].toArray();
 assert(timedMessages[0].toObject()["role"]=="system"&&timedMessages[0].toObject()["content"].toString().contains("2026-10-19 09:42 PM EDT"));
 // OpenRouter's web plugin does the search; no tools, no structured output.
 auto payload=diarySearchPayload("Search today's weather in Boston","google/gemma-4-31b-it:free");
 auto plugin=payload["plugins"].toArray()[0].toObject();assert(plugin["id"]=="web"&&plugin["max_results"].toInt()==3);
 // OpenRouter's default search prompt asks for markdown source links; the app lists sources itself (2026-10-31: sources appeared twice).
 assert(!plugin["search_prompt"].toString().isEmpty());assert(!plugin["search_prompt"].toString().contains("markdown link",Qt::CaseInsensitive));
 assert(!payload.contains("tools")&&!payload.contains("response_format"));
 assert(payload["max_tokens"].toInt()==16000);
 assert(payload["messages"].toArray()[1].toObject()["content"]=="Search today's weather in Boston");
 auto route=decodeDiaryAnswer(R"JSON({"kind":"search","text":"weather in Boston today"})JSON");assert(route.search&&!route.plot);
 // Sources come from url_citation annotations, de-duplicated; Markdown links in the text lose their URL.
 auto loadFixture=[](const char *path){QFile f(path);assert(f.open(QIODevice::ReadOnly));return QJsonDocument::fromJson(f.readAll()).object();};
 auto result=parseDiarySearch(loadFixture("tests/fixtures/search-openrouter.json"));
 assert(result.sources.size()==2);assert(result.sources[0].title=="Burj Khalifa - Wikipedia");assert(result.sources[1].title=="example.org");
 assert(result.text=="The **Burj Khalifa** in Dubai is the tallest building at 828 m, per Wikipedia.");
 assert(!result.text.contains("](http"));
 // Sources stay off the page; the saved .png.html file keeps them with the time of the check.
 assert(!result.text.contains("[1]"));
 result.checkedAt=local;
 auto html=diarySourcesHtml(result);assert(html.contains("09:42 PM EDT"));assert(html.contains("href=\"https://example.org/a?x=1&amp;y=2\""));
 assert(html.contains("https://en.wikipedia.org/wiki/Burj_Khalifa"));
 // A trailing list of source links written by the model is dropped; a link inside a sentence keeps its label.
 {auto listed=parseDiarySearch(QJsonDocument::fromJson(R"J({"choices":[{"finish_reason":"stop","message":{"content":"Jönköping is the largest, per [scb.se](https://scb.se/x).\n\n[visitsmaland.se](https://www.visitsmaland.se/a)\n- [en.wikipedia.org](https://en.wikipedia.org/wiki/B)\nSources: [a.se](https://a.se), [b.com](https://b.com)\nhttps://example.org/c","annotations":[{"type":"url_citation","url_citation":{"url":"https://www.visitsmaland.se/a","title":"Visit Småland"}}]}}]})J").object());
  assert(listed.text=="Jönköping is the largest, per scb.se.");}
 // The model's own [n] markers go: no source list is shown on the page.
 {auto marked=parseDiarySearch(QJsonDocument::fromJson(R"J({"choices":[{"finish_reason":"stop","message":{"content":"Kalmar och Växjö [1][2]. Gränna [3].","annotations":[{"type":"url_citation","url_citation":{"url":"https://a.se/x","title":"A"}}]}}]})J").object());
  assert(marked.text=="Kalmar och Växjö. Gränna.");}
 // Anthropic's native search: the line written before the search is glued to the answer without a space (2026-10-31).
 {auto glued=parseDiarySearch(QJsonDocument::fromJson(R"J({"choices":[{"finish_reason":"stop","message":{"content":"Jag söker efter aktuella uppgifter.Frankrikes största vinregion är Languedoc-Roussillon.\n\n- **Bordeaux:** störst i kvalitetsvin."}}]})J").object());
  assert(glued.text=="Frankrikes största vinregion är Languedoc-Roussillon.\n\n- **Bordeaux:** störst i kvalitetsvin.");}
 {auto umlaut=parseDiarySearch(QJsonDocument::fromJson(R"J({"choices":[{"finish_reason":"stop","message":{"content":"Let me search for that.Älvsborg is the answer."}}]})J").object());
  assert(umlaut.text=="Älvsborg is the answer.");}
 // Ordinary sentences, abbreviations and a lone sentence stay.
 {auto plain=parseDiarySearch(QJsonDocument::fromJson(R"J({"choices":[{"finish_reason":"stop","message":{"content":"Kalmar. Växjö.Gränna"}}]})J").object());
  assert(plain.text=="Kalmar. Växjö.Gränna");}
 {auto lone=parseDiarySearch(QJsonDocument::fromJson(R"J({"choices":[{"finish_reason":"stop","message":{"content":"Sunny, 12 °C in Lund."}}]})J").object());
  assert(lone.text=="Sunny, 12 °C in Lund.");}
 // Native search ignored the 80-word limit and quoted English sources word for word.
 {auto system=diarySearchPayload("x","m")["messages"].toArray()[0].toObject()["content"].toString();
  assert(system.contains("Write nothing before or during the search"));
  assert(system.contains("Never quote a source word for word"));
  assert(system.contains("Hard limit: 60 words"));}
 // No annotations: the answer stays and the saved HTML says that links are missing.
 auto none=parseDiarySearch(QJsonDocument::fromJson(R"({"choices":[{"finish_reason":"stop","message":{"content":"Sunny"}}]})").object());
 assert(none.text=="Sunny"&&none.sources.isEmpty());
 assert(diarySourcesHtml(none).contains("no source links"));
 // Unsafe or malformed citation URLs are rejected rather than shown.
 bool rejected=false;
 try{parseDiarySearch(QJsonDocument::fromJson(R"J({"choices":[{"finish_reason":"stop","message":{"content":"x","annotations":[{"type":"url_citation","url_citation":{"url":"javascript:alert(1)","title":"t"}}]}}]})J").object());}catch(...){rejected=true;}
 assert(rejected);
 // web_search:false answers search requests without a search; the default is on.
 {DiaryAnswer a;a.search=true;a.text="weather";diaryApplySearchSetting(a,QJsonObject{{"web_search",false}});
  assert(!a.search&&a.text=="Web search is turned off in config.json.");
  DiaryAnswer b;b.search=true;b.text="weather";diaryApplySearchSetting(b,QJsonObject{});assert(b.search&&b.text=="weather");
  DiaryAnswer c;c.text="plain";diaryApplySearchSetting(c,QJsonObject{{"web_search",false}});assert(!c.search&&c.text=="plain");}
 // Refusals and truncation surface as errors, not as answers.
 for(auto finish:{"length","content_filter"}){rejected=false;
  try{parseDiarySearch(QJsonDocument::fromJson(QString(R"({"choices":[{"finish_reason":"%1","message":{"content":"x"}}]})").arg(finish).toUtf8()).object());}catch(...){rejected=true;}
  assert(rejected);}
 // The reading request is a chat completion with the page as an image and the answer schema as response_format.
 {auto p=diaryReadingPayload("PNG",QJsonObject{{"model","google/gemma-4-31b-it:free"}});
  assert(p["model"]=="google/gemma-4-31b-it:free");// Reasoning models spend most of the budget thinking (2000 of 2000 tokens on 2026-10-31); leave room for the answer.
  assert(p["max_tokens"].toInt()==16000);
  auto messages=p["messages"].toArray();assert(messages.size()==2&&messages[0].toObject()["role"]=="system");
  auto image=messages[1].toObject()["content"].toArray()[1].toObject();assert(image["type"]=="image_url");
  assert(image["image_url"].toObject()["url"].toString()=="data:image/png;base64,"+QString::fromLatin1(QByteArray("PNG").toBase64()));
  auto format=p["response_format"].toObject();assert(format["type"]=="json_schema");
  auto schema=format["json_schema"].toObject();assert(schema["name"]=="diary_answer"&&schema["strict"].toBool());
  assert(schema["schema"].toObject()==diaryAnswerSchema());
  assert(!p.contains("system")&&!p.contains("output_config"));
  // Without structured outputs the schema moves into the prompt.
  auto loose=diaryWithoutFormat(p);assert(!loose.contains("response_format"));
  auto system=loose["messages"].toArray()[0].toObject()["content"].toString();
  assert(system.startsWith(messages[0].toObject()["content"].toString()));
  assert(system.contains("Return only a JSON object matching this schema")&&system.contains("\"kind\""));}
 // Tolerant decoding: fences, a lead-in sentence and a missing plot range.
 assert(decodeDiaryAnswer("Here you go: {\"kind\":\"text\",\"text\":\"Hi\",\"command\":\"q\"}").text=="Hi");
 assert(decodeDiaryAnswer("```json\n{\"kind\":\"text\",\"text\":\"Hi\"}\n```").text=="Hi");
 {auto plot=decodeDiaryAnswer(R"({"kind":"plot","command":"plot y=x^2","expression":"x^2"})");
  assert(plot.plot&&plot.xmin==-5&&plot.xmax==5);}
 // Email was removed: the prompt no longer routes to it.
 assert(!diaryReadingPrompt().contains("email"));
 // A circled word plus "tell me about these" must scope to the marked words (2026-10-25 regression:
 // without this rule 4 of 5 live runs also answered about the unmarked basking shark).
 {auto reading=diaryReadingPayload(QByteArray("png"),QJsonObject{{"model","google/gemma-4-31b-it:free"}})["messages"].toArray()[0].toObject()["content"].toString();
  assert(reading.contains("Handwritten circles, underlines, brackets and lines drawn from text"));assert(reading.contains("answer only about the marked part"));
  // An earlier Search line that already has an answer beneath it must not turn a new question into a search.
  assert(reading.contains("A Search line that already has an answer beneath it belongs to an earlier question"));}
 if(argc>1){
  QFile input(QString::fromLocal8Bit(argv[1]));assert(input.open(QIODevice::ReadOnly));
  auto live=parseDiarySearch(QJsonDocument::fromJson(input.readAll()).object());live.checkedAt=local;
  int id=QFontDatabase::addApplicationFont(QFileInfo("fonts/Caveat.ttf").absoluteFilePath());assert(id>=0);
  QFont font(QFontDatabase::applicationFontFamilies(id).first());font.setPixelSize(48);font.setWeight(QFont::Medium);QFontMetrics fm(font);
  for(auto ch:live.text)assert(ch.isSpace()||fm.inFont(ch));
  QRect box=fm.boundingRect(QRect(0,0,1150,10000),Qt::TextWordWrap,live.text);
  QImage image(1174,box.height()+24,QImage::Format_ARGB32);image.fill(Qt::white);QPainter p(&image);p.setPen(Qt::black);p.setFont(font);p.drawText(QRect(12,8,1150,box.height()+8),Qt::TextWordWrap,live.text);p.end();assert(image.save(argc>2?QString::fromLocal8Bit(argv[2]):QString("web-answer-preview.png")));
 }
 // The second button: the latest Haiku without reasoning, for the page reading and the web search alike.
 {QJsonObject base{{"api_key","k"},{"model","nvidia/nemotron-3.5-super-vl:free"},{"effort","high"}};
  auto premium=diaryPremiumConfig(base);
  assert(premium["model"]=="anthropic/claude-haiku-5.5"&&premium["effort"]=="none"&&premium["api_key"]=="k");
  auto reading=diaryReadingPayload(QByteArray("png"),premium);
  assert(reading["model"]=="anthropic/claude-haiku-5.5"&&reading["reasoning"].toObject()["effort"]=="none");
  auto search=diaryWithEffort(diarySearchPayload("Weather in Lund",premium["model"].toString()),premium);
  assert(search["model"]=="anthropic/claude-haiku-5.5"&&search["reasoning"].toObject()["effort"]=="none");
  // config.json can name another model and effort for the button.
  QJsonObject chosen=base;chosen["premium_model"]="anthropic/claude-sonnet-5.5";chosen["premium_effort"]="low";
  assert(diaryPremiumConfig(chosen)["model"]=="anthropic/claude-sonnet-5.5"&&diaryPremiumConfig(chosen)["effort"]=="low");
  // The Ask button is unchanged: its search has no reasoning unless effort is set.
  assert(!diaryWithEffort(diarySearchPayload("x","m"),QJsonObject{{"model","m"}}).contains("reasoning"));}
 // The tools button sends to the MCP proxy with its own key; Ask and the Haiku button keep OpenRouter.
 {QJsonObject base{{"api_key","sk-or"},{"base_url","https://openrouter.ai/api/v1"},{"model","nvidia/nemotron-3.5-super-vl:free"},
                   {"tools_base_url","https://proxy.example.com/v1"},{"tools_api_key","proxy-key"}};
  auto tools=diaryToolsConfig(base);
  assert(tools["base_url"]=="https://proxy.example.com/v1"&&tools["api_key"]=="proxy-key");
  // Tool calls need a capable model: the premium model unless tools_model names another.
  assert(tools["model"]=="anthropic/claude-haiku-5.5"&&tools["effort"]=="none");
  QJsonObject chosen=base;chosen["tools_model"]="anthropic/claude-sonnet-5.5";
  assert(diaryToolsConfig(chosen)["model"]=="anthropic/claude-sonnet-5.5");
  // Without the proxy settings the button explains what is missing and never falls back to OpenRouter.
  for(const char *missing:{"tools_base_url","tools_api_key"}) {
   QJsonObject without=base;without.remove(missing);bool rejected=false;
   try{diaryToolsConfig(without);}catch(const std::runtime_error &e){rejected=QString(e.what()).contains("tools_base_url");}
   assert(rejected);
  }}
 // Email was removed: its kinds are unknown and the schema no longer mentions it.
 {bool threw=false;
  try{decodeDiaryAnswer(R"({"kind":"email_check","text":"","command":"check mail"})");}catch(const std::exception&){threw=true;}
  assert(threw);}
 assert(!QJsonDocument(diaryAnswerSchema()).toJson().contains("email"));
 std::cout<<"PASS: Eastern date rollover and DST, checked-at timestamp, search routing, search payload, no citation markers, escaped links, url_citation sources, unsafe URLs, missing-source notice, refusals\n";
}
