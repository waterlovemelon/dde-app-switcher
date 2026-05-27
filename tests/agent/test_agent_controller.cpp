#include <QtTest/QtTest>

#include "agent/AgentController.h"
#include "core/ConfigManager.h"

#include <QTemporaryDir>

using namespace deepswitch;

class AgentControllerTest : public QObject {
    Q_OBJECT

private slots:
    void reloadConfigMakesBindingsAvailable()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString configPath = dir.path() + "/config.json";

        Config config = Config::defaults();
        Binding binding;
        binding.id = "terminal";
        binding.hotkey = "Alt+Return";
        binding.desktopId = "org.example.Terminal.desktop";
        config.bindings.append(binding);

        ConfigManager manager(configPath);
        const auto saved = manager.save(config);
        QVERIFY2(saved.ok, qPrintable(saved.message));

        AgentController controller(configPath, AgentController::BackendMode::Disabled);
        const auto reloaded = controller.reloadConfig();
        QVERIFY2(reloaded.ok, qPrintable(reloaded.message));

        const QList<Binding> bindings = controller.listBindings();
        QCOMPARE(bindings.size(), 1);
        QCOMPARE(bindings.first().id, QString("terminal"));
        QCOMPARE(bindings.first().hotkey, QString("Alt+Return"));
        QCOMPARE(controller.status().state, AgentControllerState::Running);
    }

    void pauseAndResumeUpdateStatus()
    {
        AgentController controller("/path/that/uses/defaults.json", AgentController::BackendMode::Disabled);
        QVERIFY(controller.reloadConfig().ok);

        controller.pause();
        QCOMPARE(controller.status().state, AgentControllerState::Paused);

        controller.resume();
        QCOMPARE(controller.status().state, AgentControllerState::Running);
    }

    void listApplicationsUsesConfiguredApplicationDirs()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        QFile desktopFile(dir.path() + "/example.desktop");
        QVERIFY(desktopFile.open(QIODevice::WriteOnly | QIODevice::Text));
        desktopFile.write("[Desktop Entry]\n"
                          "Type=Application\n"
                          "Name=Example App\n"
                          "Exec=example --flag\n");
        desktopFile.close();

        AgentController controller("/path/that/uses/defaults.json", AgentController::BackendMode::Disabled);
        controller.setApplicationDirs({ dir.path() });

        const auto listed = controller.listApplications();
        QVERIFY2(listed.ok, qPrintable(listed.message));
        QCOMPARE(listed.value.size(), 1);
        QCOMPARE(listed.value.first().desktopId, QString("example.desktop"));
        QCOMPARE(listed.value.first().localizedName, QString("Example App"));
    }

    void strictTriggerFailsWithoutBackendAndDoesNotLaunch()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString configPath = dir.path() + "/config.json";

        Config config = Config::defaults();
        Binding binding;
        binding.id = "example";
        binding.desktopId = "example.desktop";
        config.bindings.append(binding);
        QVERIFY(ConfigManager(configPath).save(config).ok);

        QFile desktopFile(dir.path() + "/example.desktop");
        QVERIFY(desktopFile.open(QIODevice::WriteOnly | QIODevice::Text));
        desktopFile.write("[Desktop Entry]\n"
                          "Type=Application\n"
                          "Name=Example App\n"
                          "Exec=example\n");
        desktopFile.close();

        bool launched = false;
        AgentController controller(configPath, AgentController::BackendMode::Disabled, [&launched](const AppInfo&) {
            launched = true;
            return VoidResult::success();
        });
        controller.setApplicationDirs({ dir.path() });
        QVERIFY(controller.reloadConfig().ok);

        const auto triggered = controller.triggerAction("example");
        QVERIFY(!triggered.ok);
        QCOMPARE(triggered.errorCode, QString("backend_unavailable"));
        QVERIFY(!launched);
        QCOMPARE(controller.status().state, AgentControllerState::Degraded);
    }

    void hotkeyTriggerLaunchesWhenWindowBackendIsUnavailable()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString configPath = dir.path() + "/config.json";

        Config config = Config::defaults();
        Binding binding;
        binding.id = "example";
        binding.desktopId = "example.desktop";
        config.bindings.append(binding);
        QVERIFY(ConfigManager(configPath).save(config).ok);

        QFile desktopFile(dir.path() + "/example.desktop");
        QVERIFY(desktopFile.open(QIODevice::WriteOnly | QIODevice::Text));
        desktopFile.write("[Desktop Entry]\n"
                          "Type=Application\n"
                          "Name=Example App\n"
                          "Exec=example\n");
        desktopFile.close();

        bool launched = false;
        AgentController controller(configPath, AgentController::BackendMode::Disabled, [&launched](const AppInfo& appInfo) {
            launched = appInfo.desktopId == "example.desktop";
            return VoidResult::success();
        });
        controller.setApplicationDirs({ dir.path() });
        QVERIFY(controller.reloadConfig().ok);

        const auto triggered = controller.triggerHotkeyAction("example");
        QVERIFY2(triggered.ok, qPrintable(triggered.message));
        QVERIFY(launched);
        QCOMPARE(triggered.value, QString("launched example.desktop"));
        QCOMPARE(controller.status().state, AgentControllerState::Running);
    }

    void triggerActionReportsBindingNotFound()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString configPath = dir.path() + "/config.json";
        QVERIFY(ConfigManager(configPath).save(Config::defaults()).ok);

        AgentController controller(configPath, AgentController::BackendMode::Disabled);
        QVERIFY(controller.reloadConfig().ok);

        const auto triggered = controller.triggerAction("missing");
        QVERIFY(!triggered.ok);
        QCOMPARE(triggered.errorCode, QString("app_not_found"));
        QCOMPARE(triggered.message, QString("binding not found"));
    }
};

QTEST_MAIN(AgentControllerTest)
#include "test_agent_controller.moc"
