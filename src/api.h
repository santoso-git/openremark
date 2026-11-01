#pragma once
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QHostAddress>
#include <QUrl>
#include <QEventLoop>
#include <QTimer>
#include <functional>
#include <stdexcept>
struct DiaryHttpError : std::runtime_error {
    int code;
    DiaryHttpError(int status,const QString &message):std::runtime_error(message.toStdString()),code(status) {}
};
// Plain http only on the local network, so the key never crosses the internet unencrypted.
inline bool diaryIsLocalHost(const QString &host) {
    if(host=="localhost"||host.endsWith(".local"))return true;
    QHostAddress address(host);
    if(address.isNull())return false;
    for(auto subnet:{"127.0.0.0/8","10.0.0.0/8","172.16.0.0/12","192.168.0.0/16"})
        if(address.isInSubnet(QHostAddress::parseSubnet(subnet)))return true;
    return false;
}
// base_url (default OpenRouter) plus exactly one /chat/completions.
inline QUrl diaryEndpoint(const QJsonObject &config) {
    QString base=config.value("base_url").toString("https://openrouter.ai/api/v1").trimmed();
    while(base.endsWith('/'))base.chop(1);
    QUrl url(base+"/chat/completions");
    if(!url.isValid()||url.host().isEmpty())throw std::runtime_error("Invalid base_url in config.json");
    if(url.scheme()=="http"&&diaryIsLocalHost(url.host()))return url;
    if(url.scheme()!="https")throw std::runtime_error("base_url must use https");
    return url;
}
inline QString diaryApiError(int code,const QByteArray &body) {
    // OpenAI-style errors are {"error":{"message":...}}; never echo the key.
    QString detail=QJsonDocument::fromJson(body).object().value("error").toObject().value("message").toString();
    if(code==401)return "API key rejected (HTTP 401); run configure.sh";
    if(code==429)return detail.contains("per-day")||detail.contains("daily",Qt::CaseInsensitive)
        ? "Daily free limit reached; try tomorrow or add credits" : "Rate limited; try again in a minute";
    detail=detail.left(160);
    if(detail.isEmpty())detail="check Wi-Fi, key and model";
    return QString("API request failed (HTTP %1): %2").arg(code).arg(detail);
}
inline QJsonObject diaryApiRequest(const QJsonObject &payload,const QJsonObject &config,
                                  const std::function<bool()> &valid) {
    const QUrl endpoint=diaryEndpoint(config);
    QNetworkAccessManager manager;
    QNetworkRequest request(endpoint);
    request.setHeader(QNetworkRequest::ContentTypeHeader,"application/json");
    request.setRawHeader("Authorization", "Bearer " + config.value("api_key").toString().trimmed().toUtf8());
    if(endpoint.host()=="openrouter.ai") {
        request.setRawHeader("HTTP-Referer", "https://github.com/santoso-git/openremark");
        request.setRawHeader("X-Title", "openremark");
    }
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,QNetworkRequest::ManualRedirectPolicy);
    auto *reply=manager.post(request,QJsonDocument(payload).toJson(QJsonDocument::Compact));
    QEventLoop loop; QTimer timeout,cancelPoll;
    timeout.setSingleShot(true);
    QObject::connect(reply,&QNetworkReply::finished,&loop,&QEventLoop::quit);
    QObject::connect(&timeout,&QTimer::timeout,reply,&QNetworkReply::abort);
    QObject::connect(&cancelPoll,&QTimer::timeout,reply,[&]{if(!valid())reply->abort();});
    const qint64 limit=4*1024*1024;
    QByteArray bytes;bool oversized=false;
    auto drain=[&] { auto chunk=reply->readAll();if(bytes.size()+chunk.size()>limit){oversized=true;reply->abort();}else bytes+=chunk; };
    reply->setReadBufferSize(65536);
    QObject::connect(reply,&QNetworkReply::readyRead,reply,drain);
    timeout.start(180000);cancelPoll.start(300);loop.exec();
    if(!oversized)drain();
    if(oversized)throw std::runtime_error("Response too large");
    if(!valid())throw std::runtime_error("Cancelled or page changed");
    int code=reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if(code!=200||reply->error()!=QNetworkReply::NoError)
        throw DiaryHttpError(code,diaryApiError(code,bytes));
    auto doc=QJsonDocument::fromJson(bytes);
    if(!doc.isObject())throw std::runtime_error("Invalid API response");
    return doc.object();
}
