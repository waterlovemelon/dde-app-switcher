#include <QCoreApplication>
#include <QCommandLineParser>
#include <QDir>
#include <QTextStream>
#include <QThread>
#include <optional>

#include "backends/x11/X11Connection.h"
#include "backends/x11/X11HotkeyBackend.h"
#include "backends/x11/X11WindowBackend.h"
#include "core/ActionEngine.h"
#include "core/AppInfo.h"
#include "core/AppMatcher.h"
#include "core/AppRegistry.h"
#include "core/ConfigManager.h"
#include "core/Hotkey.h"
#include "core/Launcher.h"
#include "core/WindowInfo.h"

using namespace deepswitch;

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("deepswitch-agent");
    QCoreApplication::setApplicationVersion("0.1.0");

    QCommandLineParser parser;
    parser.setApplicationDescription("DeepSwitch command-line agent");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addOption({ "validate-config", "Validate the config file and exit." });
    parser.addOption({ "list-bindings", "List configured bindings and exit." });
    parser.addOption({ "list-apps", "List desktop applications and exit." });
    parser.addOption({ "list-windows", "List X11 windows and exit." });
    parser.addOption({ "trigger", "Trigger an action id and exit.", "action-id" });
    parser.addOption({ "config", "Use a specific config file.", "path" });
    parser.process(app);

    QTextStream out(stdout);
    QTextStream err(stderr);

    const QString configPath = parser.value("config").isEmpty()
        ? QDir::homePath() + "/.config/deepswitch/config.json"
        : parser.value("config");

    if (parser.isSet("validate-config")) {
        ConfigManager manager(configPath);
        const auto loaded = manager.load();
        if (!loaded.ok) {
            err << loaded.errorCode << ": " << loaded.message << "\n";
            return 2;
        }
        out << "config valid\n";
        return 0;
    }

    if (parser.isSet("list-bindings")) {
        ConfigManager manager(configPath);
        const auto loaded = manager.load();
        if (!loaded.ok) {
            err << loaded.errorCode << ": " << loaded.message << "\n";
            return 2;
        }
        for (const Binding& binding : loaded.value.bindings) {
            out << binding.id << "\t" << binding.hotkey << "\t" << binding.desktopId << "\n";
        }
        return 0;
    }

    if (parser.isSet("list-apps")) {
        AppRegistry registry;
        const auto scanned = registry.scan();
        if (!scanned.ok) {
            err << scanned.errorCode << ": " << scanned.message << "\n";
            return 2;
        }
        for (const AppInfo& appInfo : registry.listApplications()) {
            out << appInfo.desktopId << "\t" << appInfo.localizedName << "\t" << appInfo.exec << "\n";
        }
        return 0;
    }

    if (parser.isSet("list-windows")) {
        X11Connection connection;
        const auto opened = connection.open();
        if (!opened.ok) {
            err << opened.errorCode << ": " << opened.message << "\n";
            return 2;
        }

        X11WindowBackend windows(connection);
        const auto listed = windows.listWindows();
        if (!listed.ok) {
            err << listed.errorCode << ": " << listed.message << "\n";
            return 2;
        }

        for (const WindowInfo& window : listed.value) {
            out << QString::number(window.id)
                << "\t" << window.wmClass
                << "\t" << window.title
                << "\n";
        }
        return 0;
    }

    if (parser.isSet("trigger")) {
        ConfigManager manager(configPath);
        const auto loaded = manager.load();
        if (!loaded.ok) {
            err << loaded.errorCode << ": " << loaded.message << "\n";
            return 2;
        }

        const QString actionId = parser.value("trigger");
        std::optional<Binding> binding;
        for (const Binding& candidate : loaded.value.bindings) {
            if (candidate.id == actionId) {
                binding = candidate;
                break;
            }
        }
        if (!binding.has_value()) {
            err << "app_not_found: binding not found\n";
            return 2;
        }

        AppRegistry registry;
        registry.scan();
        const auto appInfo = registry.findByDesktopId(binding->desktopId);
        if (!appInfo.has_value()) {
            err << "app_not_found: desktop id not found\n";
            return 2;
        }

        X11Connection connection;
        const auto opened = connection.open();
        if (!opened.ok) {
            err << opened.errorCode << ": " << opened.message << "\n";
            return 2;
        }

        X11WindowBackend windows(connection);
        const auto listed = windows.listWindows();
        if (!listed.ok) {
            err << listed.errorCode << ": " << listed.message << "\n";
            return 2;
        }

        QList<WindowInfo> matches;
        for (const WindowInfo& window : listed.value) {
            const MatchResult match = AppMatcher::match(appInfo.value(), window, binding->matchRules);
            if (match.matched) {
                matches.append(window);
            }
        }

        const ActionDecision decision = ActionEngine::decide(binding.value(), appInfo.value(), matches);
        if (decision.type == ActionType::Launch) {
            const auto launched = Launcher::launch(appInfo.value());
            if (!launched.ok) {
                err << launched.errorCode << ": " << launched.message << "\n";
                return 2;
            }
            out << "launched " << appInfo->desktopId << "\n";
            return 0;
        }
        if (decision.type == ActionType::Focus || decision.type == ActionType::Cycle) {
            const auto activated = windows.activateWindow(decision.windowId);
            if (!activated.ok) {
                err << activated.errorCode << ": " << activated.message << "\n";
                return 2;
            }
            out << "activated " << decision.windowId << "\n";
            return 0;
        }

        err << decision.errorCode << ": " << decision.message << "\n";
        return 2;
    }

    // Default: run hotkey loop
    X11Connection connection;
    const auto opened = connection.open();
    if (!opened.ok) {
        err << opened.errorCode << ": " << opened.message << "\n";
        return 2;
    }

    ConfigManager manager(configPath);
    const auto loaded = manager.load();
    if (!loaded.ok) {
        err << loaded.errorCode << ": " << loaded.message << "\n";
        return 2;
    }

    AppRegistry registry;
    registry.scan();

    X11HotkeyBackend hotkeys(connection);
    X11WindowBackend windowBackend(connection);

    int registered = 0;
    for (const Binding& binding : loaded.value.bindings) {
        if (!binding.enabled || binding.hotkey.isEmpty()) {
            continue;
        }
        const auto parsed = Hotkey::parse(binding.hotkey);
        if (!parsed.ok) {
            err << "hotkey_invalid: " << binding.id << " - " << parsed.message << "\n";
            continue;
        }
        const auto result = hotkeys.registerHotkey(parsed.value, binding.id);
        if (!result.ok) {
            err << result.errorCode << ": " << binding.id << " - " << result.message << "\n";
            continue;
        }
        out << "registered " << binding.hotkey << " -> " << binding.id << "\n";
        ++registered;
    }

    if (registered == 0) {
        err << "no hotkeys registered\n";
        return 2;
    }

    out << "listening for hotkeys; press Ctrl+C to exit\n";
    out.flush();

    while (true) {
        const QString action = hotkeys.pollTriggeredAction();
        if (!action.isEmpty()) {
            std::optional<Binding> binding;
            for (const Binding& candidate : loaded.value.bindings) {
                if (candidate.id == action) {
                    binding = candidate;
                    break;
                }
            }
            if (!binding.has_value()) {
                continue;
            }

            const auto appInfo = registry.findByDesktopId(binding->desktopId);
            if (!appInfo.has_value()) {
                err << "app_not_found: " << binding->desktopId << "\n";
                continue;
            }

            const auto listed = windowBackend.listWindows();
            QList<WindowInfo> matches;
            if (listed.ok) {
                for (const WindowInfo& window : listed.value) {
                    const MatchResult match = AppMatcher::match(appInfo.value(), window, binding->matchRules);
                    if (match.matched) {
                        matches.append(window);
                    }
                }
            }

            const ActionDecision decision = ActionEngine::decide(binding.value(), appInfo.value(), matches);
            if (decision.type == ActionType::Launch) {
                const auto launched = Launcher::launch(appInfo.value());
                if (launched.ok) {
                    out << "launched " << appInfo->desktopId << "\n";
                } else {
                    err << launched.errorCode << ": " << launched.message << "\n";
                }
            } else if (decision.type == ActionType::Focus || decision.type == ActionType::Cycle) {
                const auto activated = windowBackend.activateWindow(decision.windowId);
                if (activated.ok) {
                    out << "activated " << decision.windowId << "\n";
                } else {
                    err << activated.errorCode << ": " << activated.message << "\n";
                }
            } else {
                err << decision.errorCode << ": " << decision.message << "\n";
            }
            out.flush();
        }
        QThread::msleep(20);
    }

    return 0;
}
