#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDir>
#include <QTextStream>
#include <QThread>

#include "agent/AgentController.h"
#include "backends/x11/X11Connection.h"
#include "backends/x11/X11HotkeyBackend.h"
#include "core/AppInfo.h"
#include "core/Config.h"
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

    AgentController controller(configPath);

    if (parser.isSet("validate-config")) {
        const auto loaded = controller.reloadConfig();
        if (!loaded.ok) {
            err << loaded.errorCode << ": " << loaded.message << "\n";
            return 2;
        }
        out << "config valid\n";
        return 0;
    }

    if (parser.isSet("list-bindings")) {
        const auto loaded = controller.reloadConfig();
        if (!loaded.ok) {
            err << loaded.errorCode << ": " << loaded.message << "\n";
            return 2;
        }
        for (const Binding& binding : controller.listBindings()) {
            out << binding.id << "\t" << binding.hotkey << "\t" << binding.desktopId << "\n";
        }
        return 0;
    }

    if (parser.isSet("list-apps")) {
        const auto scanned = controller.listApplications();
        if (!scanned.ok) {
            err << scanned.errorCode << ": " << scanned.message << "\n";
            return 2;
        }
        for (const AppInfo& appInfo : scanned.value) {
            out << appInfo.desktopId << "\t" << appInfo.localizedName << "\t" << appInfo.exec << "\n";
        }
        return 0;
    }

    if (parser.isSet("list-windows")) {
        const auto listed = controller.listWindows();
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
        const auto loaded = controller.reloadConfig();
        if (!loaded.ok) {
            err << loaded.errorCode << ": " << loaded.message << "\n";
            return 2;
        }

        const auto triggered = controller.triggerAction(parser.value("trigger"));
        if (!triggered.ok) {
            err << triggered.errorCode << ": " << triggered.message << "\n";
            return 2;
        }
        out << triggered.value << "\n";
        return 0;
    }

    X11Connection connection;
    const auto opened = connection.open();
    if (!opened.ok) {
        err << opened.errorCode << ": " << opened.message << "\n";
        return 2;
    }

    const auto loaded = controller.reloadConfig();
    if (!loaded.ok) {
        err << loaded.errorCode << ": " << loaded.message << "\n";
        return 2;
    }

    X11HotkeyBackend hotkeys(connection);
    QStringList registrationMessages;
    const auto registered = controller.registerHotkeys(hotkeys, &registrationMessages);
    for (const QString& message : registrationMessages) {
        if (message.startsWith("registered ")) {
            out << message << "\n";
        } else {
            err << message << "\n";
        }
    }

    if (!registered.ok) {
        err << registered.message << "\n";
        return 2;
    }

    out << "listening for hotkeys; press Ctrl+C to exit\n";
    out.flush();

    while (true) {
        const QString action = hotkeys.pollTriggeredAction();
        if (!action.isEmpty()) {
            const auto triggered = controller.triggerHotkeyAction(action);
            if (triggered.ok) {
                out << triggered.value << "\n";
            } else {
                err << triggered.errorCode << ": " << triggered.message << "\n";
            }
            out.flush();
        }
        QThread::msleep(20);
    }

    return 0;
}
