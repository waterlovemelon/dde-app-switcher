#pragma once

#include "core/AppInfo.h"
#include "core/Config.h"
#include "core/WindowInfo.h"
#include <QList>

namespace oopsjump {

enum class ActionType {
    Launch,
    Focus,
    Cycle,
    Ignore,
    Fail
};

struct ActionDecision {
    ActionType type = ActionType::Ignore;
    WindowId windowId = 0;
    QString errorCode;
    QString message;
};

class ActionEngine {
public:
    static ActionDecision decide(const Binding& binding, const AppInfo& app, const QList<WindowInfo>& windows);
};

}
