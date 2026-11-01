#include "../src/api.h"
#include <QCoreApplication>
#include <QJsonObject>
#include <cassert>
#include <iostream>
static bool throws(const QJsonObject &config){try{diaryEndpoint(config);}catch(const std::exception&){return true;}return false;}
int main(int argc,char **argv){
 QCoreApplication app(argc,argv);
 // The endpoint is base_url (default OpenRouter) plus exactly one /chat/completions.
 QJsonObject c;
 assert(diaryEndpoint(c).toString()=="https://openrouter.ai/api/v1/chat/completions");
 c["base_url"]="https://openrouter.ai/api/v1/";
 assert(diaryEndpoint(c).toString()=="https://openrouter.ai/api/v1/chat/completions");
 c["base_url"]="  https://example.com/v1  ";
 assert(diaryEndpoint(c).toString()=="https://example.com/v1/chat/completions");
 // Plain http is only for the local network, so a key never crosses the internet unencrypted.
 for(auto local:{"http://192.168.1.20:11434/v1","http://10.0.0.5/v1","http://172.20.1.1/v1","http://127.0.0.1:1234/v1","http://localhost:8787/v1","http://mac.local:8787/v1"}){
  c["base_url"]=local;assert(!throws(c));assert(diaryEndpoint(c).toString()==QString(local)+"/chat/completions");
 }
 for(auto remote:{"http://example.com/v1","http://172.32.0.1/v1","http://8.8.8.8/v1","ftp://example.com/v1","not a url",""}){
  c["base_url"]=remote;assert(throws(c));
 }
 // Errors: readable on the page, with free-tier limits spelled out and never the key.
 assert(diaryApiError(429,R"({"error":{"message":"Rate limit exceeded: free-models-per-day"}})").contains("Daily free limit"));
 assert(diaryApiError(429,R"({"error":{"message":"Rate limit exceeded: free-models-per-min"}})").contains("try again in a minute"));
 assert(diaryApiError(401,R"({"error":{"message":"No auth credentials found"}})").contains("401"));
 auto generic=diaryApiError(500,R"({"error":{"message":")"+QByteArray(300,'x')+R"("}})");
 assert(generic.startsWith("API request failed (HTTP 500)"));assert(generic.count('x')==160);
 assert(diaryApiError(502,"not json").contains("HTTP 502"));
 DiaryHttpError e(429,"x");assert(e.code==429);
 std::cout<<"PASS: endpoint from base_url, https-only outside the local network, readable 401/429 errors\n";
}
