#pragma once

#include "core/AppInfo.h"
#include "core/Config.h"
#include "core/WindowInfo.h"
#include <QList>

namespace deepswitch {

struct MatchEvidence {
    QString source;
    QString expected;
    QString actual;
    int score = 0;
};

struct MatchResult {
    bool matched = false;
    int totalScore = 0;
    QList<MatchEvidence> evidence;
};

class AppMatcher {
public:
    static MatchResult match(const AppInfo& app, const WindowInfo& window, const QList<MatchRule>& rules);

private:
    static bool ruleMatches(const MatchRule& rule, const WindowInfo& window);
};

}
