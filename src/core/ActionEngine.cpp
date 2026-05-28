#include "core/ActionEngine.h"

#include <algorithm>

namespace deepswitch {
namespace {

QList<WindowInfo> windowsByActivationOrder(QList<WindowInfo> windows)
{
    std::stable_sort(windows.begin(), windows.end(), [](const WindowInfo& lhs, const WindowInfo& rhs) {
        if (lhs.active != rhs.active) {
            return lhs.active;
        }
        return lhs.lastActiveOrder < rhs.lastActiveOrder;
    });
    return windows;
}

}

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

    const QList<WindowInfo> orderedWindows = windowsByActivationOrder(windows);
    if (!orderedWindows.isEmpty() && orderedWindows.first().active && orderedWindows.size() > 1) {
        return { ActionType::Cycle, orderedWindows.at(1).id, {}, {} };
    }
    return { ActionType::Cycle, orderedWindows.first().id, {}, {} };
}

}
