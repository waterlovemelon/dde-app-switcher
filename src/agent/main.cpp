#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusError>
#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QTextStream>
#include <QTimer>

#include "agent/AgentController.h"
#include "backends/x11/X11Connection.h"
#include "backends/x11/X11HotkeyBackend.h"
#include "core/AppInfo.h"
#include "core/Config.h"
#include "core/ConfigManager.h"
#include "core/LogFileManager.h"
#include "core/WindowInfo.h"
#include "ipc/AgentDBusContract.h"
#include "ipc/AgentDBusService.h"

#include <optional>

using namespace oopsjump;

namespace {

bool launchOverlayBar(AgentController& controller, const QString& activeActionId)
{
    const QString executable = QCoreApplication::applicationDirPath() + QDir::separator() + QStringLiteral("oops-jump-overlay");
    const QFileInfo overlayInfo(executable);
    if (!overlayInfo.exists() || !overlayInfo.isExecutable()) {
        return false;
    }

    QVariantList apps = controller.resolveOverlayApps();
    for (auto& entry : apps) {
        QVariantMap map = entry.toMap();
        if (map.value(QStringLiteral("id")).toString() == activeActionId) {
            map[QStringLiteral("active")] = true;
            entry = map;
        }
    }

    QJsonArray arr;
    for (const auto& entry : apps) {
        const QVariantMap map = entry.toMap();
        QJsonObject obj;
        obj[QStringLiteral("icon")] = map.value(QStringLiteral("icon")).toString();
        obj[QStringLiteral("name")] = map.value(QStringLiteral("name")).toString();
        obj[QStringLiteral("hotkey")] = map.value(QStringLiteral("hotkey")).toString();
        obj[QStringLiteral("running")] = map.value(QStringLiteral("running")).toBool();
        obj[QStringLiteral("active")] = map.value(QStringLiteral("active")).toBool();
        arr.append(obj);
    }

    const QString json = QString::fromUtf8(QJsonDocument(arr).toJson(QJsonDocument::Compact));

    return QProcess::startDetached(executable, {
        QStringLiteral("--apps"),
        json,
    });
}

}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("oops-jump-agent");
    QCoreApplication::setApplicationVersion("0.1.0");

    QCommandLineParser parser;
    parser.setApplicationDescription("Oops Jump command-line agent");
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

    std::optional<LogFileManager> logFileManager;
    if (LogFileManager::shouldUseFileLogging(QCoreApplication::arguments())) {
        logFileManager.emplace();
        const auto loggingInstalled = logFileManager->install();
        if (!loggingInstalled.ok) {
            err << loggingInstalled.errorCode << ": " << loggingInstalled.message << "\n";
            logFileManager.reset();
        }
    }

    auto writeOut = [&](const QString& message) {
        out << message << "\n";
        if (logFileManager.has_value()) {
            logFileManager->writeLine(QtInfoMsg, message);
        }
    };

    auto writeErr = [&](const QString& message) {
        err << message << "\n";
        if (logFileManager.has_value()) {
            logFileManager->writeLine(QtCriticalMsg, message);
        }
    };

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
        writeErr(QStringLiteral("fatal: failed to register D-Bus service %1: %2")
            .arg(AgentDBusContract::ServiceName, sessionBus.lastError().message()));
        return 2;
    }

    if (!sessionBus.registerObject(AgentDBusContract::ObjectPath,
            &dbusService,
            QDBusConnection::ExportAllSlots | QDBusConnection::ExportAllSignals)) {
        writeErr(QStringLiteral("fatal: failed to register D-Bus object %1: %2")
            .arg(AgentDBusContract::ObjectPath, sessionBus.lastError().message()));
        sessionBus.unregisterService(AgentDBusContract::ServiceName);
        return 2;
    }

    X11Connection connection;
    const auto opened = connection.open();
    if (!opened.ok) {
        writeErr(opened.errorCode + QStringLiteral(": ") + opened.message);
        return 2;
    }

    const auto loaded = controller.reloadConfig();
    if (!loaded.ok) {
        writeErr(loaded.errorCode + QStringLiteral(": ") + loaded.message);
        return 2;
    }
    const auto autostartSynced = controller.syncAutostart();
    if (!autostartSynced.ok) {
        writeErr(autostartSynced.errorCode + QStringLiteral(": ") + autostartSynced.message);
        return 2;
    }

    X11HotkeyBackend hotkeys(connection);
    QStringList registrationMessages;
    const auto registered = controller.registerHotkeys(hotkeys, &registrationMessages);
    for (const QString& message : registrationMessages) {
        if (message.startsWith("registered ")) {
            writeOut(message);
        } else {
            writeErr(message);
        }
    }

    if (!registered.ok) {
        writeErr(registered.message);
        return 2;
    }

    // Register Super key for overlay show/hide
    const auto superRegistered = hotkeys.registerSuperKey();
    if (superRegistered.ok) {
        writeOut(QStringLiteral("registered Super_L for overlay"));
    } else {
        writeErr(superRegistered.errorCode + QStringLiteral(": ") + superRegistered.message);
    }

    writeOut(QStringLiteral("listening for hotkeys; press Ctrl+C to exit"));
    out.flush();

    // Track overlay process for kill-on-release
    QProcess* overlayProcess = nullptr;

    QTimer pollTimer;
    QObject::connect(&pollTimer, &QTimer::timeout, [&]() {
        SuperKeyEvent superEvent = SuperKeyEvent::NoEvent;
        const QString action = hotkeys.pollAllEvents(superEvent);

        // Handle Super key press/release for overlay
        if (superEvent == SuperKeyEvent::Pressed && controller.showOverlay()) {
            if (overlayProcess) {
                overlayProcess->kill();
                overlayProcess->deleteLater();
                overlayProcess = nullptr;
            }
            overlayProcess = new QProcess;
            const QString executable = QCoreApplication::applicationDirPath()
                + QDir::separator() + QStringLiteral("oops-jump-overlay");
            QVariantList apps = controller.resolveOverlayApps();
            QJsonArray arr;
            for (const auto& entry : apps) {
                const QVariantMap map = entry.toMap();
                QJsonObject obj;
                obj[QStringLiteral("icon")] = map.value(QStringLiteral("icon")).toString();
                obj[QStringLiteral("name")] = map.value(QStringLiteral("name")).toString();
                obj[QStringLiteral("hotkey")] = map.value(QStringLiteral("hotkey")).toString();
                obj[QStringLiteral("running")] = map.value(QStringLiteral("running")).toBool();
                obj[QStringLiteral("active")] = map.value(QStringLiteral("active")).toBool();
                arr.append(obj);
            }
            const QString json = QString::fromUtf8(QJsonDocument(arr).toJson(QJsonDocument::Compact));
            overlayProcess->start(executable, { QStringLiteral("--apps"), json });
        } else if (superEvent == SuperKeyEvent::Released) {
            if (overlayProcess) {
                overlayProcess->kill();
                overlayProcess->deleteLater();
                overlayProcess = nullptr;
            }
        }

        // Handle regular hotkey actions
        if (!action.isEmpty()) {
            const auto triggered = controller.triggerHotkeyAction(action);
            if (triggered.ok) {
                writeOut(triggered.value);
                emit dbusService.HotkeyTriggered(action, {
                    { "ok", true },
                    { "message", triggered.value },
                });
            } else {
                writeErr(triggered.errorCode + QStringLiteral(": ") + triggered.message);
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
