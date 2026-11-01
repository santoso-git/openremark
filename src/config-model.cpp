#include <QCoreApplication>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QSaveFile>
#include <iostream>
int main(int argc,char **argv) {
 QCoreApplication app(argc,argv);
 // config-model MODEL [file]        set the model
 // config-model --set KEY [file]    set KEY to one line read from stdin (keeps secrets out of argv)
 bool setMode=argc>=3&&QByteArray(argv[1])=="--set";
 if(argc<2||argc>(setMode?4:3)){std::cerr<<"Usage: config-model MODEL [config-file] | config-model --set KEY [config-file]\n";return 1;}
 QString key=setMode?QString::fromLocal8Bit(argv[2]):QString("model"),value;
 if(setMode) {
  static const QStringList allowed{"timezone","effort","base_url","premium_model","premium_effort","tools_base_url","tools_api_key","tools_model"};
  if(!allowed.contains(key)){std::cerr<<"Unknown setting; allowed: "<<allowed.join(", ").toStdString()<<"\n";return 1;}
  std::string line;std::getline(std::cin,line);value=QString::fromStdString(line).trimmed();
  if(value.isEmpty()||value.size()>200){std::cerr<<"Empty or too long value; nothing changed\n";return 1;}
  if((key=="base_url"||key=="tools_base_url")&&!value.startsWith("https://")&&!value.startsWith("http://")){std::cerr<<key.toStdString()<<" must start with https:// or http://\n";return 1;}
 } else {
  value=QString::fromLocal8Bit(argv[1]);
  // Provider-prefixed ids such as google/gemma-4-31b-it:free are allowed; a single slash at most.
  if(value.size()>128||!QRegularExpression("^[A-Za-z0-9][A-Za-z0-9_.:-]*(/[A-Za-z0-9_.:-]+)?$").match(value).hasMatch()){std::cerr<<"Invalid model name\n";return 1;}
 }
 int fileArg=setMode?3:2;
 QString path=argc>fileArg?QString::fromLocal8Bit(argv[fileArg]):QString("/home/root/.config/ink-diary/config.json");
 QFile input(path);
 if(!input.open(QIODevice::ReadOnly)){std::cerr<<"Configure an API key first\n";return 1;}
 auto doc=QJsonDocument::fromJson(input.readAll());input.close();
 if(!doc.isObject()||doc.object().value("api_key").toString().trimmed().isEmpty()){std::cerr<<"Invalid configuration or missing API key; nothing changed\n";return 1;}
 auto config=doc.object();config.insert(key,value);
 QSaveFile output(path);output.setDirectWriteFallback(false);
 if(!output.open(QIODevice::WriteOnly)||!output.setPermissions(QFileDevice::ReadOwner|QFileDevice::WriteOwner)){std::cerr<<"Cannot securely save configuration\n";return 1;}
 auto bytes=QJsonDocument(config).toJson();
 if(output.write(bytes)!=bytes.size()||!output.commit()){std::cerr<<"Cannot save configuration; original preserved\n";return 1;}
 if(setMode) std::cout<<key.toStdString()<<" saved. Other settings preserved. No restart needed.\n";
 else std::cout<<"Model updated to "<<value.toStdString()<<". API key and other settings preserved. No restart needed.\n";
}
