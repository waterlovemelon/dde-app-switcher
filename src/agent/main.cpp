#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusError>
#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QTextStream>
#include <QTimer>

#include "agent/AgentController.h"
#include "backends/x11/X11Connection.h"
#include "backends/x11/X11HotkeyBackend.h"
#include "core/AppInfo.h"
#include "core/Config.h"
#include "core/ConfigManager.h"
#include "core/WindowInfo.h"
#include "ipc/AgentDBusContract.h"
#include "ipc/AgentDBusService.h"

using namespace deepswitch;

namespace {

QString overlayKindForResult(const Result<QString>& triggered)
{
    if (!triggered.ok) {
        return QStringLiteral("failed");
    }

    const QString message = triggered.value.toLower();
    if (message.startsWith(QStringLiteral("launched"))) {
        return QStringLiteral("launched");
    }
    if (message.startsWith(QStringLiteral("cycled"))) {
        return QStringLiteral("cycled");
    }
    if (message.startsWith(QStringLiteral("focused"))) {
        return QStringLiteral("focused");
    }
    return QStringLiteral("focused");
}

bool launchOverlayHint(const Result<QString>& triggered)
{
    const QString executable = QCoreApplication::applicationDirPath() + QDir::separator() + QStringLiteral("deepswitch-overlay");
    const QFileInfo overlayInfo(executable);
    if (!overlayInfo.exists() || !overlayInfo.isExecutable()) {
        return false;
    }

    const QString message = triggered.ok ? triggered.value : triggered.message;
    return QProcess::startDetached(executable, {
        QStringLiteral("--kind"),
        overlayKindForResult(triggered),
        QStringLiteral("--message"),
        message,
    });
}

}

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
        ? ConfigManager::defaultConfigPath()
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

    AgentDBusService dbusService(controller);
    QDBusConnection sessionBus = QDBusConnection::sessionBus();
    if (!sessionBus.registerService(AgentDBusContract::ServiceName)) {
        err << "fatal: failed to register D-Bus service "
            << AgentDBusContract::ServiceName << ": "
            << sessionBus.lastError().message() << "\n";
        return 2;
    }

    if (!sessionBus.registerObject(AgentDBusContract::ObjectPath,
            &dbusService,
            QDBusConnection::ExportAllSlots | QDBusConnection::ExportAllSignals)) {
        err << "fatal: failed to register D-Bus object "
            << AgentDBusContract::ObjectPath << ": "
            << sessionBus.lastError().message() << "\n";
        sessionBus.unregisterService(AgentDBusContract::ServiceName);
        return 2;
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
    const auto autostartSynced = controller.syncAutostart();
    if (!autostartSynced.ok) {
        err << autostartSynced.errorCode << ": " << autostartSynced.message << "\n";
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

    QTimer pollTimer;
    QObject::connect(&pollTimer, &QTimer::timeout, [&]() {
        const QString action = hotkeys.pollTriggeredAction();
        if (!action.isEmpty()) {
            const auto triggered = controller.triggerHotkeyAction(action);
            if (controller.showOverlay()) {
                launchOverlayHint(triggered);
            }
            if (triggered.ok) {
                out << triggered.value << "\n";
                emit dbusService.HotkeyTriggered(action, {
                    { "ok", true },
                    { "message", triggered.value },
                });
            } else {
                err << triggered.errorCode << ": " << triggered.message << "\n";
                emit dbusService.HotkeyTriggered(action, {
                    { "ok", false },
                    { "error_code", triggered.errorCode },
                    { "message", triggered.message },
                });
                emit dbusService.ErrorOccurred(triggered.errorCode, triggered.message);
            }
            out.flush();
        }
    });
    pollTimer.start(20);

    return app.exec();
}
