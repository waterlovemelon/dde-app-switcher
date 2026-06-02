#include "settings/SettingsController.h"
#include "overlay/IconProvider.h"
#include "core/ConfigManager.h"
#include "ipc/AgentDBusContract.h"

#include <QCoreApplication>
#include <QDBusServiceWatcher>
#include <QDir>
#include <QFileInfo>
#include <QIcon>
#include <QLocale>
#include <QTranslator>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QProcess>
#include <QQuickStyle>
#include <QStandardPaths>
#include <QTimer>
#include <QUrl>

using namespace oopsjump;

namespace {

constexpr auto DesktopFileName = "cn.org.oops.oops-jump";
constexpr auto InstalledIconPath = "/opt/apps/cn.org.oops.oops-jump/entries/icons/hicolor/256x256/apps/cn.org.oops.oops-jump.png";

bool tryStartAgent()
{
    // First try same directory as this executable (installed layout).
    const QString siblingPath = QCoreApplication::applicationDirPath()
        + QDir::separator() + QStringLiteral("oops-jump-agent");
    if (QFileInfo::exists(siblingPath) && QFileInfo(siblingPath).isExecutable()) {
        return QProcess::startDetached(siblingPath, {});
    }
    // Fallback: search PATH.
    const QString agentPath = QStandardPaths::findExecutable(QStringLiteral("oops-jump-agent"));
    if (agentPath.isEmpty()) {
        return false;
    }
    return QProcess::startDetached(agentPath, {});
}

} // namespace

int main(int argc, char* argv[])
{
    QGuiApplication app(argc, argv);
    QGuiApplication::setApplicationName(QStringLiteral("Oops Jump Settings"));
    QGuiApplication::setDesktopFileName(QString::fromLatin1(DesktopFileName));
    QGuiApplication::setOrganizationName(QStringLiteral("Oops Jump"));
    QGuiApplication::setWindowIcon(QIcon(QString::fromLatin1(InstalledIconPath)));

    // Load language preference from config
    const auto configResult = ConfigManager(ConfigManager::defaultConfigPath()).load();
    const QString lang = configResult.ok ? configResult.value.general.language : "system";

    const QLocale locale = (lang == "system") ? QLocale::system() : QLocale(lang);
    QTranslator translator;
    if (translator.load(locale, "oops-jump", "_", ":/i18n")) {
        QGuiApplication::installTranslator(&translator);
    }

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
    engine.addImageProvider(QStringLiteral("theme"), new oopsjump::IconProvider);
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
