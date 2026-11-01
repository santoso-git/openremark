#pragma once
#include <QJsonArray>
#include <QJsonObject>
#include <QRegularExpression>
#include <QString>
#include <QStringList>
#include <stdexcept>
// Chat completions: choices[0].message.content, only after a clean stop.
inline QString responseText(const QJsonObject &root) {
    auto choices=root.value("choices").toArray();
    if(choices.isEmpty())throw std::runtime_error("Invalid API response");
    auto choice=choices.first().toObject();
    auto message=choice.value("message").toObject();
    QString finish=choice.value("finish_reason").toString();
    if(finish=="length")throw std::runtime_error("Answer incomplete; try a shorter question");
    if(finish=="content_filter"||!message.value("refusal").toString().isEmpty())throw std::runtime_error("The model declined this request");
    if(finish!="stop")throw std::runtime_error(finish.isEmpty() ? "Invalid API response" : QString("Answer stopped early (%1); please try again").arg(finish).toStdString());
    QString text=message.value("content").toString();
    if(text.trimmed().isEmpty() || text.size()>16000)
        throw std::runtime_error("Empty or oversized answer");
    return text.trimmed();
}
// The generation id lets the logged prompt and answer be looked up at OpenRouter; only plain ids are kept.
inline void diaryKeepGenerationId(QStringList &ids,const QJsonObject &root) {
    static const QRegularExpression plain("^[A-Za-z0-9_-]{1,100}$");
    QString id=root.value("id").toString();
    if(plain.match(id).hasMatch())ids.append(id);
}
// Free models often wrap JSON in a code fence or a sentence: keep the outermost object.
inline QString diaryExtractJson(const QString &text) {
    int start=text.indexOf('{'),end=text.lastIndexOf('}');
    if(start<0||end<start)return text;
    return text.mid(start,end-start+1);
}
