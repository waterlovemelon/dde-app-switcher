#include "core/ActionEngine.h"

namespace deepswitch {

ActionDecision ActionEngine::decide(const Binding& binding, const AppInfo& app, const QList<WindowInfo>& windows)
{
    if (!binding.enabled) {
        return { ActionType::Ignore, 0, "binding_disabled", "Binding is disabled." };
    }
    if (app.desktopId.isEmpty() && binding.command.isEmpty()) {
        return { ActionType::Fail, 0, "app_not_found", "Binding has no application target." };
    }
    if (windows.isEmpty()) {
        if (binding.launchIfNotRunning) {
            return { ActionType::Launch, 0, {}, {} };
        }
        return { ActionType::Fail, 0, "window_not_found", "No matching window found." };
    }
    if (windows.size() == 1) {
        return { ActionType::Focus, windows.first().id, {}, {} };
    }
    return { ActionType::Cycle, windows.first().id, {}, {} };
}

}
