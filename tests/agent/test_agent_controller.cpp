#include <QtTest/QtTest>

#include "agent/AgentController.h"
#include "core/ConfigManager.h"

#include <QTemporaryDir>

using namespace deepswitch;

class AgentControllerTest : public QObject {
    Q_OBJECT

private slots:
    void reloadConfigDoesNotSyncAutostartFromGeneralConfig()
    {
        QTemporaryDir dir;
        QTemporaryDir configHome;
        QVERIFY(dir.isValid());
        QVERIFY(configHome.isValid());
        const QString configPath = dir.path() + "/config.json";
        const QByteArray previousConfigHome = qgetenv("XDG_CONFIG_HOME");
        const bool hadConfigHome = qEnvironmentVariableIsSet("XDG_CONFIG_HOME");
        qputenv("XDG_CONFIG_HOME", configHome.path().toUtf8());

        Config config = Config::defaults();
        config.general.autostart = true;
        QVERIFY(ConfigManager(configPath).save(config).ok);

        AgentController controller(configPath, AgentController::BackendMode::Disabled);
        QVERIFY2(controller.reloadConfig().ok, qPrintable(controller.status().message));
        QVERIFY(!QFile::exists(configHome.path() + "/autostart/deepswitch-agent.desktop"));

        if (hadConfigHome) {
            qputenv("XDG_CONFIG_HOME", previousConfigHome);
        } else {
            qunsetenv("XDG_CONFIG_HOME");
        }
    }

    void syncAutostartAppliesGeneralConfig()
    {
        QTemporaryDir dir;
        QTemporaryDir configHome;
        QVERIFY(dir.isValid());
        QVERIFY(configHome.isValid());
        const QString configPath = dir.path() + "/config.json";
        const QByteArray previousConfigHome = qgetenv("XDG_CONFIG_HOME");
        const bool hadConfigHome = qEnvironmentVariableIsSet("XDG_CONFIG_HOME");
        qputenv("XDG_CONFIG_HOME", configHome.path().toUtf8());

        Config config = Config::defaults();
        config.general.autostart = true;
        QVERIFY(ConfigManager(configPath).save(config).ok);

        AgentController controller(configPath, AgentController::BackendMode::Disabled);
        QVERIFY2(controller.reloadConfig().ok, qPrintable(controller.status().message));
        QVERIFY2(controller.syncAutostart().ok, qPrintable(controller.status().message));
        QVERIFY(QFile::exists(configHome.path() + "/autostart/deepswitch-agent.desktop"));

        config.general.autostart = false;
        QVERIFY(ConfigManager(configPath).save(config).ok);
        QVERIFY2(controller.reloadConfig().ok, qPrintable(controller.status().message));
        QVERIFY2(controller.syncAutostart().ok, qPrintable(controller.status().message));
        QVERIFY(!QFile::exists(configHome.path() + "/autostart/deepswitch-agent.desktop"));

        if (hadConfigHome) {
            qputenv("XDG_CONFIG_HOME", previousConfigHome);
        } else {
            qunsetenv("XDG_CONFIG_HOME");
        }
    }

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

    void testHotkeyRejectsInvalidHotkeys()
    {
        AgentController controller("/path/that/uses/defaults.json", AgentController::BackendMode::Disabled);
        QVERIFY(controller.reloadConfig().ok);

        const auto tested = controller.testHotkey("Alt");

        QVERIFY(!tested.ok);
        QCOMPARE(tested.errorCode, QString("hotkey_invalid"));
    }

    void testHotkeyDetectsDuplicateConfigBindingExceptExcludedAction()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString configPath = dir.path() + "/config.json";

        Config config = Config::defaults();
        Binding terminal;
        terminal.id = "terminal";
        terminal.hotkey = "Ctrl+Alt+Enter";
        config.bindings.append(terminal);
        Binding browser;
        browser.id = "browser";
        browser.hotkey = "Meta+B";
        config.bindings.append(browser);
        QVERIFY(ConfigManager(configPath).save(config).ok);

        AgentController controller(configPath, AgentController::BackendMode::Disabled);
        QVERIFY(controller.reloadConfig().ok);

        const auto conflicting = controller.testHotkey("alt + control + return", "browser");
        QVERIFY(!conflicting.ok);
        QCOMPARE(conflicting.errorCode, QString("hotkey_conflict"));
        QVERIFY(conflicting.message.contains("terminal"));

        const auto excluded = controller.testHotkey("Alt+Ctrl+Return", "terminal");
        QVERIFY2(excluded.ok, qPrintable(excluded.message));
    }

    void testHotkeyUsesBackendTesterAfterConfigChecks()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString configPath = dir.path() + "/config.json";

        Config config = Config::defaults();
        Binding terminal;
        terminal.id = "terminal";
        terminal.hotkey = "Alt+Return";
        config.bindings.append(terminal);
        QVERIFY(ConfigManager(configPath).save(config).ok);

        QString testedSequence;
        AgentController controller(
            configPath,
            AgentController::BackendMode::Disabled,
            {},
            [&testedSequence](const Hotkey& hotkey) {
                testedSequence = hotkey.sequence;
                return VoidResult::success();
            });
        QVERIFY(controller.reloadConfig().ok);

        const auto available = controller.testHotkey("Meta+Space", QString());
        QVERIFY2(available.ok, qPrintable(available.message));
        QCOMPARE(testedSequence, QString("Meta+Space"));

        const auto conflict = controller.testHotkey("Alt+Enter", QString());
        QVERIFY(!conflict.ok);
        QCOMPARE(conflict.errorCode, QString("hotkey_conflict"));
        QCOMPARE(testedSequence, QString("Meta+Space"));
    }

    void testHotkeyDoesNotBackendTestUnchangedExcludedBinding()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString configPath = dir.path() + "/config.json";

        Config config = Config::defaults();
        Binding terminal;
        terminal.id = "terminal";
        terminal.hotkey = "Alt+Return";
        config.bindings.append(terminal);
        QVERIFY(ConfigManager(configPath).save(config).ok);

        bool backendTesterCalled = false;
        AgentController controller(
            configPath,
            AgentController::BackendMode::Disabled,
            {},
            [&backendTesterCalled](const Hotkey&) {
                backendTesterCalled = true;
                return VoidResult::failure("hotkey_conflict", "self-conflict");
            });
        QVERIFY(controller.reloadConfig().ok);

        const auto tested = controller.testHotkey("Alt+Enter", "terminal");
        QVERIFY2(tested.ok, qPrintable(tested.message));
        QVERIFY(!backendTesterCalled);
    }

    void statusReportsBackendCapabilitiesAndBindingDiagnostics()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString configPath = dir.path() + "/config.json";

        Config config = Config::defaults();

        Binding registered;
        registered.id = "terminal";
        registered.hotkey = "Alt+Return";
        registered.desktopId = "terminal.desktop";
        config.bindings.append(registered);

        Binding disabled;
        disabled.id = "disabled";
        disabled.enabled = false;
        disabled.hotkey = "Meta+D";
        disabled.desktopId = "terminal.desktop";
        config.bindings.append(disabled);

        Binding invalid;
        invalid.id = "invalid";
        invalid.hotkey = "Alt";
        invalid.desktopId = "terminal.desktop";
        config.bindings.append(invalid);

        Binding conflict;
        conflict.id = "conflict";
        conflict.hotkey = "Alt+Enter";
        conflict.desktopId = "terminal.desktop";
        config.bindings.append(conflict);

        Binding missingApp;
        missingApp.id = "missing-app";
        missingApp.hotkey = "Meta+M";
        missingApp.desktopId = "missing.desktop";
        config.bindings.append(missingApp);

        QVERIFY(ConfigManager(configPath).save(config).ok);

        QFile desktopFile(dir.path() + "/terminal.desktop");
        QVERIFY(desktopFile.open(QIODevice::WriteOnly | QIODevice::Text));
        desktopFile.write("[Desktop Entry]\n"
                          "Type=Application\n"
                          "Name=Terminal\n"
                          "Exec=terminal\n");
        desktopFile.close();

        AgentController controller(configPath, AgentController::BackendMode::Disabled);
        controller.setApplicationDirs({ dir.path() });
        QVERIFY(controller.reloadConfig().ok);

        const AgentControllerStatus status = controller.status();
        QCOMPARE(status.hotkeyBackend.value("name").toString(), QString("disabled"));
        QCOMPARE(status.windowBackend.value("name").toString(), QString("disabled"));
        QCOMPARE(status.capabilities.value("global_hotkey").toBool(), false);
        QCOMPARE(status.capabilities.value("window_list").toBool(), false);
        QCOMPARE(status.capabilities.value("activate_window").toBool(), false);
        QCOMPARE(status.capabilities.value("launch_app").toBool(), true);

        QMap<QString, QString> bindingStatuses;
        for (const QVariant& item : status.bindingStatuses) {
            const QVariantMap bindingStatus = item.toMap();
            bindingStatuses.insert(
                bindingStatus.value("id").toString(),
                bindingStatus.value("status").toString());
        }

        QCOMPARE(bindingStatuses.value("terminal"), QString("not_registered"));
        QCOMPARE(bindingStatuses.value("disabled"), QString("disabled"));
        QCOMPARE(bindingStatuses.value("invalid"), QString("invalid"));
        QCOMPARE(bindingStatuses.value("conflict"), QString("conflict"));
        QCOMPARE(bindingStatuses.value("missing-app"), QString("app_not_found"));
        QVERIFY(status.warnings.contains("hotkey_invalid: invalid - Hotkey must contain a main key."));
        QVERIFY(status.warnings.contains("hotkey_conflict: conflict conflicts with terminal"));
        QVERIFY(status.warnings.contains("app_not_found: missing-app - missing.desktop"));
    }

    void statusDoesNotReportRegisteredBeforeHotkeyRegistration()
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
        QVERIFY(ConfigManager(configPath).save(config).ok);

        QFile desktopFile(dir.path() + "/terminal.desktop");
        QVERIFY(desktopFile.open(QIODevice::WriteOnly | QIODevice::Text));
        desktopFile.write("[Desktop Entry]\n"
                          "Type=Application\n"
                          "Name=Terminal\n"
                          "Exec=terminal\n");
        desktopFile.close();

        AgentController controller(configPath, AgentController::BackendMode::X11);
        controller.setApplicationDirs({ dir.path() });
        QVERIFY(controller.reloadConfig().ok);

        const AgentControllerStatus status = controller.status();
        QCOMPARE(status.hotkeyBackend.value("running").toBool(), false);
        QCOMPARE(status.windowBackend.value("running").toBool(), true);
        QCOMPARE(status.bindingStatuses.size(), 1);
        QCOMPARE(status.bindingStatuses.first().toMap().value("status").toString(), QString("not_registered"));
    }
};

QTEST_MAIN(AgentControllerTest)
#include "test_agent_controller.moc"
