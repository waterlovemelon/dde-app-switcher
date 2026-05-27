#include "agent/AgentController.h"

#include "backends/x11/X11Connection.h"
#include "backends/x11/X11HotkeyBackend.h"
#include "backends/x11/X11WindowBackend.h"
#include "core/ActionEngine.h"
#include "core/AppMatcher.h"
#include "core/AppRegistry.h"
#include "core/ConfigManager.h"
#include "core/Hotkey.h"
#include "core/Launcher.h"

#include <memory>
#include <optional>

namespace deepswitch {

AgentController::AgentController(QString configPath, BackendMode backendMode, LauncherFn launcher, HotkeyTestFn hotkeyTester)
    : m_configPath(std::move(configPath))
    , m_backendMode(backendMode)
    , m_config(Config::defaults())
    , m_launcher(std::move(launcher))
    , m_hotkeyTester(std::move(hotkeyTester))
{
    if (!m_launcher) {
        m_launcher = [](const AppInfo& appInfo) {
            return Launcher::launch(appInfo);
        };
    }
    m_status.state = AgentControllerState::Starting;
    m_status.activeBackend = backendMode == BackendMode::X11 ? "x11" : "disabled";
}

VoidResult AgentController::reloadConfig()
{
    ConfigManager manager(m_configPath);
    const auto loaded = manager.load();
    if (!loaded.ok) {
        updateState(AgentControllerState::Error, loaded.message);
        return VoidResult::failure(loaded.errorCode, loaded.message);
    }

    m_config = loaded.value;
    m_status.enabled = m_config.general.enabled;
    updateState(m_config.general.enabled ? AgentControllerState::Running : AgentControllerState::Paused);
    return VoidResult::success();
}

void AgentController::pause()
{
    updateState(AgentControllerState::Paused);
}

void AgentController::resume()
{
    updateState(m_config.general.enabled ? AgentControllerState::Running : AgentControllerState::Paused);
}

Result<QString> AgentController::triggerAction(const QString& actionId)
{
    return triggerAction(actionId, TriggerMode::Strict);
}

Result<QString> AgentController::triggerHotkeyAction(const QString& actionId)
{
    return triggerAction(actionId, TriggerMode::TolerateWindowListFailure);
}

Result<QString> AgentController::triggerAction(const QString& actionId, TriggerMode mode)
{
    if (m_status.state == AgentControllerState::Paused) {
        return Result<QString>::failure("agent_paused", "Agent is paused.");
    }

    const std::optional<Binding> binding = findBinding(actionId);
    if (!binding.has_value()) {
        updateState(AgentControllerState::Degraded, "Binding not found.");
        return Result<QString>::failure("app_not_found", "binding not found");
    }

    const auto appInfo = findApplication(binding.value());
    if (!appInfo.ok) {
        updateState(AgentControllerState::Degraded, appInfo.message);
        return Result<QString>::failure(appInfo.errorCode, appInfo.message);
    }

    QList<WindowInfo> matches;
    std::unique_ptr<X11Connection> connection;
    std::unique_ptr<X11WindowBackend> windows;
    if (m_backendMode == BackendMode::X11) {
        connection = std::make_unique<X11Connection>();
        const auto opened = connection->open();
        if (opened.ok) {
            windows = std::make_unique<X11WindowBackend>(*connection);
            const auto listed = windows->listWindows();
            if (listed.ok) {
                for (const WindowInfo& window : listed.value) {
                    const MatchResult match = AppMatcher::match(appInfo.value, window, binding->matchRules);
                    if (match.matched) {
                        matches.append(window);
                    }
                }
            } else if (mode == TriggerMode::Strict) {
                updateState(AgentControllerState::Degraded, listed.message);
                return Result<QString>::failure(listed.errorCode, listed.message);
            } else {
                updateState(AgentControllerState::Degraded, listed.message);
            }
        } else if (mode == TriggerMode::Strict) {
            updateState(AgentControllerState::Degraded, opened.message);
            return Result<QString>::failure(opened.errorCode, opened.message);
        } else {
            updateState(AgentControllerState::Degraded, opened.message);
        }
    } else if (mode == TriggerMode::Strict) {
        updateState(AgentControllerState::Degraded, "Window backend is unavailable.");
        return Result<QString>::failure("backend_unavailable", "Window backend is unavailable.");
    } else {
        updateState(AgentControllerState::Degraded, "Window backend is unavailable.");
    }

    const ActionDecision decision = ActionEngine::decide(binding.value(), appInfo.value, matches);
    if (decision.type == ActionType::Launch) {
        const auto launched = m_launcher(appInfo.value);
        if (!launched.ok) {
            updateState(AgentControllerState::Degraded, launched.message);
            return Result<QString>::failure(launched.errorCode, launched.message);
        }
        updateState(AgentControllerState::Running);
        return Result<QString>::success("launched " + appInfo.value.desktopId);
    }

    if (decision.type == ActionType::Focus || decision.type == ActionType::Cycle) {
        if (windows == nullptr) {
            updateState(AgentControllerState::Degraded, "Window backend is unavailable.");
            return Result<QString>::failure("backend_unavailable", "Window backend is unavailable.");
        }

        const auto activated = windows->activateWindow(decision.windowId);
        if (!activated.ok) {
            updateState(AgentControllerState::Degraded, activated.message);
            return Result<QString>::failure(activated.errorCode, activated.message);
        }
        updateState(AgentControllerState::Running);
        return Result<QString>::success("activated " + QString::number(decision.windowId));
    }

    updateState(AgentControllerState::Degraded, decision.message);
    return Result<QString>::failure(decision.errorCode, decision.message);
}

QList<Binding> AgentController::listBindings() const
{
    return m_config.bindings;
}

Result<QList<AppInfo>> AgentController::listApplications()
{
    AppRegistry registry;
    if (!m_applicationDirs.isEmpty()) {
        registry.setApplicationDirs(m_applicationDirs);
    }

    const auto scanned = registry.scan();
    if (!scanned.ok) {
        updateState(AgentControllerState::Degraded, scanned.message);
        return Result<QList<AppInfo>>::failure(scanned.errorCode, scanned.message);
    }

    m_applications = registry.listApplications();
    return Result<QList<AppInfo>>::success(m_applications);
}

Result<QList<WindowInfo>> AgentController::listWindows(const QString& filter) const
{
    if (m_backendMode == BackendMode::Disabled) {
        return Result<QList<WindowInfo>>::failure("backend_unavailable", "Window backend is unavailable.");
    }

    X11Connection connection;
    const auto opened = connection.open();
    if (!opened.ok) {
        return Result<QList<WindowInfo>>::failure(opened.errorCode, opened.message);
    }

    X11WindowBackend windows(connection);
    const auto listed = windows.listWindows();
    if (!listed.ok || filter.trimmed().isEmpty()) {
        return listed;
    }

    QList<WindowInfo> filtered;
    const QString needle = filter.trimmed();
    for (const WindowInfo& window : listed.value) {
        if (window.appId == needle || window.wmClass == needle || window.instanceName == needle || window.title.contains(needle, Qt::CaseInsensitive)) {
            filtered.append(window);
        }
    }
    return Result<QList<WindowInfo>>::success(filtered);
}

VoidResult AgentController::testHotkey(const QString& hotkey, const QString& excludeActionId) const
{
    const auto parsed = Hotkey::parse(hotkey);
    if (!parsed.ok) {
        return VoidResult::failure("hotkey_invalid", parsed.message);
    }

    bool matchesExcludedBinding = false;
    for (const Binding& binding : m_config.bindings) {
        if (!binding.enabled || binding.hotkey.trimmed().isEmpty()) {
            continue;
        }

        const auto candidate = Hotkey::parse(binding.hotkey);
        if (!candidate.ok) {
            continue;
        }

        if (binding.id == excludeActionId) {
            matchesExcludedBinding = candidate.value.sequence == parsed.value.sequence;
            continue;
        }

        if (candidate.value.sequence == parsed.value.sequence) {
            return VoidResult::failure(
                "hotkey_conflict",
                "Hotkey conflicts with binding '" + binding.id + "'.");
        }
    }

    if (matchesExcludedBinding) {
        return VoidResult::success();
    }

    if (m_hotkeyTester) {
        return m_hotkeyTester(parsed.value);
    }

    if (m_backendMode == BackendMode::Disabled) {
        return VoidResult::failure("hotkey_backend_unavailable", "Hotkey backend is unavailable.");
    }

    X11Connection connection;
    const auto opened = connection.open();
    if (!opened.ok) {
        return VoidResult::failure("hotkey_backend_unavailable", opened.message);
    }

    X11HotkeyBackend hotkeys(connection);
    const auto registered = hotkeys.registerHotkey(parsed.value, "__deepswitch_test__");
    if (!registered.ok) {
        return registered;
    }

    hotkeys.unregisterHotkey(parsed.value);
    return VoidResult::success();
}

AgentControllerStatus AgentController::status() const
{
    return m_status;
}

void AgentController::setApplicationDirs(QStringList dirs)
{
    m_applicationDirs = std::move(dirs);
    m_applications.clear();
}

Result<int> AgentController::registerHotkeys(X11HotkeyBackend& hotkeys, QStringList* messages)
{
    int registered = 0;
    for (const Binding& binding : m_config.bindings) {
        if (!binding.enabled || binding.hotkey.isEmpty()) {
            continue;
        }
        const auto parsed = Hotkey::parse(binding.hotkey);
        if (!parsed.ok) {
            if (messages != nullptr) {
                messages->append("hotkey_invalid: " + binding.id + " - " + parsed.message);
            }
            continue;
        }
        const auto result = hotkeys.registerHotkey(parsed.value, binding.id);
        if (!result.ok) {
            if (messages != nullptr) {
                messages->append(result.errorCode + ": " + binding.id + " - " + result.message);
            }
            continue;
        }
        if (messages != nullptr) {
            messages->append("registered " + binding.hotkey + " -> " + binding.id);
        }
        ++registered;
    }

    if (registered == 0) {
        updateState(AgentControllerState::Error, "No hotkeys registered.");
        return Result<int>::failure("hotkey_registration_failed", "no hotkeys registered");
    }

    updateState(AgentControllerState::Running);
    return Result<int>::success(registered);
}

std::optional<Binding> AgentController::findBinding(const QString& actionId) const
{
    for (const Binding& candidate : m_config.bindings) {
        if (candidate.id == actionId) {
            return candidate;
        }
    }
    return std::nullopt;
}

Result<AppInfo> AgentController::findApplication(const Binding& binding)
{
    if (m_applications.isEmpty()) {
        const auto listed = listApplications();
        if (!listed.ok) {
            return Result<AppInfo>::failure(listed.errorCode, listed.message);
        }
    }

    for (const AppInfo& appInfo : m_applications) {
        if (appInfo.desktopId == binding.desktopId) {
            return Result<AppInfo>::success(appInfo);
        }
    }

    return Result<AppInfo>::failure("app_not_found", "desktop id not found");
}

void AgentController::updateState(AgentControllerState state, QString message)
{
    m_status.state = state;
    m_status.message = std::move(message);
}

}
