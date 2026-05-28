#include <QtTest/QtTest>

#include "agent/AgentController.h"
#include "core/ConfigManager.h"
#include "ipc/AgentDBusContract.h"
#include "ipc/AgentDBusService.h"

#include <QDBusConnection>
#include <QDBusError>
#include <QDBusInterface>
#include <QDBusReply>
#include <QMetaClassInfo>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QUuid>

using namespace deepswitch;

class AgentDBusServiceTest : public QObject {
    Q_OBJECT

private slots:
    void exportsAgentInterfaceName()
    {
        const QMetaObject metaObject = AgentDBusService::staticMetaObject;
        bool foundInterface = false;
        for (int i = 0; i < metaObject.classInfoCount(); ++i) {
            const QMetaClassInfo info = metaObject.classInfo(i);
            if (QString::fromLatin1(info.name()) == "D-Bus Interface") {
                foundInterface = true;
                QCOMPARE(QString::fromLatin1(info.value()), QString(AgentDBusContract::InterfaceName));
            }
        }
        QVERIFY(foundInterface);
        QCOMPARE(QString(AgentDBusService::InterfaceName), QString(AgentDBusContract::InterfaceName));
        QCOMPARE(QString(AgentDBusContract::InterfaceName), QString("org.deepin.DeepSwitch.Agent"));
    }

    void exposesStatusAndControllerBackedLists()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString configPath = dir.path() + "/config.json";

        Config config = Config::defaults();
        Binding binding;
        binding.id = "terminal";
        binding.hotkey = "Alt+Return";
        binding.desktopId = "terminal.desktop";
        config.bindings.append(binding);
        QVERIFY2(ConfigManager(configPath).save(config).ok, "failed to save test config");

        QFile desktopFile(dir.path() + "/terminal.desktop");
        QVERIFY(desktopFile.open(QIODevice::WriteOnly | QIODevice::Text));
        desktopFile.write("[Desktop Entry]\n"
                          "Type=Application\n"
                          "Name=Terminal\n"
                          "Exec=terminal\n");
        desktopFile.close();

        AgentController controller(configPath, AgentController::BackendMode::Disabled);
        controller.setApplicationDirs({ dir.path() });
        AgentDBusService service(controller);

        QSignalSpy statusChanged(&service, &AgentDBusService::StatusChanged);
        const QVariantMap reloadResult = service.ReloadConfig();
        QVERIFY2(reloadResult.value("ok").toBool(), qPrintable(reloadResult.value("message").toString()));
        QCOMPARE(statusChanged.count(), 1);

        const QVariantMap status = service.GetStatus();
        QCOMPARE(status.value("running").toBool(), true);
        QCOMPARE(status.value("enabled").toBool(), true);
        QCOMPARE(status.value("active_backend").toString(), QString("disabled"));
        QVERIFY(status.contains("session_type"));
        QCOMPARE(status.value("hotkey_backend").toMap().value("name").toString(), QString("disabled"));
        QCOMPARE(status.value("window_backend").toMap().value("name").toString(), QString("disabled"));
        QCOMPARE(status.value("capabilities").toMap().value("launch_app").toBool(), true);
        QCOMPARE(status.value("capabilities").toMap().value("global_hotkey").toBool(), false);
        const QVariantList bindingStatuses = status.value("binding_statuses").toList();
        QCOMPARE(bindingStatuses.size(), 1);
        QCOMPARE(bindingStatuses.first().toMap().value("id").toString(), QString("terminal"));
        QCOMPARE(bindingStatuses.first().toMap().value("status").toString(), QString("not_registered"));
        QCOMPARE(status.value("warnings").toStringList().size(), 0);

        const QVariantList bindings = service.ListBindings();
        QCOMPARE(bindings.size(), 1);
        QCOMPARE(bindings.first().toMap().value("id").toString(), QString("terminal"));

        const QVariantMap apps = service.ListApplications();
        QCOMPARE(apps.value("ok").toBool(), true);
        QCOMPARE(apps.value("error_code").toString(), QString());
        QCOMPARE(apps.value("message").toString(), QString());
        const QVariantList appItems = apps.value("items").toList();
        QCOMPARE(appItems.size(), 1);
        QCOMPARE(appItems.first().toMap().value("desktop_id").toString(), QString("terminal.desktop"));
    }

    void returnsStructuredErrorsForUnsupportedOrUnavailableOperations()
    {
        AgentController controller("/path/that/uses/defaults.json", AgentController::BackendMode::Disabled);
        AgentDBusService service(controller);

        const QVariantMap setBinding = service.SetBinding({ { "id", "terminal" } });
        QCOMPARE(setBinding.value("ok").toBool(), false);
        QCOMPARE(setBinding.value("error_code").toString(), QString("not_implemented"));

        const QVariantMap removeBinding = service.RemoveBinding("terminal");
        QCOMPARE(removeBinding.value("ok").toBool(), false);
        QCOMPARE(removeBinding.value("error_code").toString(), QString("not_implemented"));

        const QVariantMap activateWindow = service.ActivateWindow(123);
        QCOMPARE(activateWindow.value("ok").toBool(), false);
        QCOMPARE(activateWindow.value("error_code").toString(), QString("not_implemented"));

        const QVariantMap launchApp = service.LaunchApp("terminal.desktop");
        QCOMPARE(launchApp.value("ok").toBool(), false);
        QCOMPARE(launchApp.value("error_code").toString(), QString("not_implemented"));

        const QVariantMap windows = service.ListWindows(QString());
        QCOMPARE(windows.value("ok").toBool(), false);
        QCOMPARE(windows.value("error_code").toString(), QString("backend_unavailable"));
        QCOMPARE(windows.value("items").toList().size(), 0);
    }

    void testHotkeyReturnsControllerResultEnvelope()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString configPath = dir.path() + "/config.json";

        Config config = Config::defaults();
        Binding binding;
        binding.id = "terminal";
        binding.hotkey = "Alt+Return";
        config.bindings.append(binding);
        QVERIFY(ConfigManager(configPath).save(config).ok);

        AgentController controller(configPath, AgentController::BackendMode::Disabled);
        QVERIFY(controller.reloadConfig().ok);
        AgentDBusService service(controller);

        const QVariantMap invalid = service.TestHotkey("Alt", QString());
        QCOMPARE(invalid.value("ok").toBool(), false);
        QCOMPARE(invalid.value("error_code").toString(), QString("hotkey_invalid"));

        const QVariantMap conflict = service.TestHotkey("alt+enter", "browser");
        QCOMPARE(conflict.value("ok").toBool(), false);
        QCOMPARE(conflict.value("error_code").toString(), QString("hotkey_conflict"));

        const QVariantMap unchangedExcluded = service.TestHotkey("Alt+Enter", "terminal");
        QCOMPARE(unchangedExcluded.value("ok").toBool(), true);
    }

    void testHotkeyReturnsSuccessEnvelopeWhenBackendTestPasses()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString configPath = dir.path() + "/config.json";
        QVERIFY(ConfigManager(configPath).save(Config::defaults()).ok);

        AgentController controller(
            configPath,
            AgentController::BackendMode::Disabled,
            {},
            [](const Hotkey&) {
                return VoidResult::success();
            });
        QVERIFY(controller.reloadConfig().ok);
        AgentDBusService service(controller);

        const QVariantMap available = service.TestHotkey("Meta+Space", QString());
        QCOMPARE(available.value("ok").toBool(), true);
        QCOMPARE(available.value("message").toString(), QString("hotkey available"));
    }

    void registersOnSessionBusWhenAvailable()
    {
        AgentController controller("/path/that/uses/defaults.json", AgentController::BackendMode::Disabled);
        AgentDBusService service(controller);

        QDBusConnection bus = QDBusConnection::sessionBus();
        if (!bus.isConnected()) {
            QSKIP(qPrintable("session bus unavailable: " + bus.lastError().message()));
        }

        const QString suffix = "t" + QUuid::createUuid().toString(QUuid::WithoutBraces).replace('-', '_');
        const QString serviceName = "org.deepin.DeepSwitch.Test." + suffix;
        const QString objectPath = "/org/deepin/DeepSwitch/Test/" + suffix;
        if (!bus.registerService(serviceName)) {
            QSKIP(qPrintable("cannot register temporary service: " + bus.lastError().message()));
        }

        const bool objectRegistered = bus.registerObject(
            objectPath,
            &service,
            QDBusConnection::ExportAllSlots | QDBusConnection::ExportAllSignals);
        if (!objectRegistered) {
            bus.unregisterService(serviceName);
            QSKIP(qPrintable("cannot register temporary object: " + bus.lastError().message()));
        }

        QDBusInterface iface(serviceName, objectPath, AgentDBusContract::InterfaceName, bus);
        QVERIFY2(iface.isValid(), qPrintable(iface.lastError().message()));

        QDBusReply<QVariantMap> reply = iface.call("GetStatus");
        QVERIFY2(reply.isValid(), qPrintable(reply.error().message()));
        QCOMPARE(reply.value().value("active_backend").toString(), QString("disabled"));

        bus.unregisterObject(objectPath);
        bus.unregisterService(serviceName);
    }
};

QTEST_MAIN(AgentDBusServiceTest)
#include "test_agent_dbus_service.moc"
