#include "settings/SettingsController.h"
#include "ipc/AgentDBusContract.h"

#include <QCoreApplication>
#include <QDBusServiceWatcher>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QProcess>
#include <QQuickStyle>
#include <QStandardPaths>
#include <QTimer>
#include <QUrl>

using namespace deepswitch;

namespace {

bool tryStartAgent()
{
    const QString agentPath = QStandardPaths::findExecutable(QStringLiteral("deepswitch-agent"));
    if (agentPath.isEmpty()) {
        return false;
    }
    return QProcess::startDetached(agentPath, {});
}

} // namespace

int main(int argc, char* argv[])
{
    QGuiApplication app(argc, argv);
    QGuiApplication::setApplicationName(QStringLiteral("DeepSwitch Settings"));
    QGuiApplication::setDesktopFileName(QStringLiteral("org.deepin.DeepSwitch"));
    QGuiApplication::setOrganizationName(QStringLiteral("DeepSwitch"));

    QQuickStyle::setStyle(QStringLiteral("Fusion"));

    SettingsController settingsController;

    // Initial refresh — works if agent is already running.
    settingsController.refresh();

    // If agent is not running, try to start it.
    if (!settingsController.connected()) {
        tryStartAgent();
    }

    // Auto-refresh when the agent D-Bus service appears.
    QDBusServiceWatcher watcher(
        AgentDBusContract::ServiceName,
        QDBusConnection::sessionBus(),
        QDBusServiceWatcher::WatchForRegistration | QDBusServiceWatcher::WatchForUnregistration);
    QObject::connect(&watcher, &QDBusServiceWatcher::serviceRegistered,
                     &settingsController, [&settingsController]() {
                         settingsController.refresh();
                     });
    QObject::connect(&watcher, &QDBusServiceWatcher::serviceUnregistered,
                     &settingsController, [&settingsController]() {
                         settingsController.refresh();
                     });

    // Periodic retry if agent is not yet available (every 3 seconds).
    if (!settingsController.connected()) {
        QTimer retryTimer;
        QObject::connect(&retryTimer, &QTimer::timeout, &settingsController, [&settingsController, &retryTimer] {
            settingsController.refresh();
            if (settingsController.connected()) {
                retryTimer.stop();
            }
        });
        retryTimer.start(3000);
    }

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("settingsController"), &settingsController);

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        [] {
            QCoreApplication::exit(-1);
        },
        Qt::QueuedConnection);

    engine.load(QUrl(QStringLiteral("qrc:/qml/Main.qml")));
    return app.exec();
}
