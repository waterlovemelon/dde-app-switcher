#pragma once

#include <QList>
#include <QString>

namespace deepswitch {

using WindowId = quint64;

struct MatchEvidence {
    QString source;
    QString expected;
    QString actual;
    int score = 0;
    QString ruleType;
    QString value;
    int scoreDelta = 0;
    bool matched = false;
    QString effect = "include";
};

struct WindowInfo {
    WindowId id = 0;
    QString title;
    QString wmClass;
    QString instanceName;
    int pid = 0;
    int desktop = -1;
    bool minimized = false;
    bool active = false;
    int lastActiveOrder = 0;
    QString windowType;
    QString appId;
    bool skipTaskbar = false;
    int matchScore = 0;
    QList<MatchEvidence> matchEvidence;
};

}
