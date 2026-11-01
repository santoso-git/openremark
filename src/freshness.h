#pragma once
#include "webanswer.h"
#include "plot.h"

inline QString diaryCurrentContext(const QDateTime &now,const QString &zone) {
    return QString("Current user-local date and time: %1. Treat this application clock as the present, "
        "not as a hypothetical future date. Your training cutoff is not today's date. "
        "Resolve today, now, current and recent relative to this clock. "
        "For facts that may have changed since training, route to web search instead of answering from memory "
        "or saying that the present is in the future. Earlier notebook answers may be stale or wrong; "
        "do not treat them as evidence. Preserve explicit historical dates in the user's question. ").arg(diaryLocalClock(now,zone));
}
// A conservative English backstop for a model that incorrectly returns kind=text.
// Only the latest transcribed question is inspected, never old page text or an answer.
inline void diaryRequireFreshSearch(DiaryAnswer &answer) {
    if(answer.calendar || !answer.diagram.isEmpty() || answer.plot || answer.search || answer.command.trimmed().isEmpty())return;
    static const QRegularExpression current(
        R"(\b(today|tonight|currently|latest|recent|recently|right now|this week|this month|this year)\b|\bcurrent\s+(president|prime minister|leader|governor|mayor|ceo|price|weather|news|version|rate|schedule)\b|\bwho\s+is\s+(?:the\s+)?(?:\w+\s+){0,4}(president|prime minister|governor|mayor|ceo)\b|^\s*search\b)",
        QRegularExpression::CaseInsensitiveOption);
    if(current.match(answer.command).hasMatch()) {
        if(answer.command.size()>2000)throw std::runtime_error("Search question too long; please shorten it");
        answer.search=true;answer.text=answer.command;
    }
}
// web_search:false in config.json answers search requests without searching (searches cost money even on free models).
inline void diaryApplySearchSetting(DiaryAnswer &answer,const QJsonObject &config) {
    if(!answer.search||config.value("web_search").toBool(true))return;
    answer.search=false;answer.text="Web search is turned off in config.json.";
}
