#include <QtTest/QtTest>

#include "core/ConfigManager.h"
#include "settings/SettingsController.h"

#include <QDir>
#include <QFile>
#include <QTemporaryDir>

using namespace oopsjump;

class FakeAgentClient : public AgentClientInterface {
    Q_OBJECT

public:
    bool available = true;
    AgentCallResult statusResult = AgentCallResult::success(QVariantMap {
        { "running", true },
        { "enabled", true },
        { "active_backend", "x11" },
        { "session_type", "x11" },
        { "hotkey_backend", QVariantMap {
              { "name", "x11" },
              { "available", true },
              { "running", true },
              { "message", "ready" },
          } },
        { "window_backend", QVariantMap {
              { "name", "x11" },
              { "available", true },
              { "running", true },
              { "message", "ready" },
          } },
        { "capabilities", QVariantMap {
              { "global_hotkey", true },
              { "window_list", true },
              { "activate_window", true },
              { "launch_app", true },
          } },
        { "binding_statuses", QVariantList {
              QVariantMap {
                  { "id", "terminal" },
                  { "hotkey", "Alt+Return" },
                  { "desktop_id", "org.deepin.Terminal.desktop" },
                  { "status", "registered" },
                  { "message", "registered" },
              },
          } },
        { "warnings", QStringList {} },
        { "backends", QVariantList { QVariantMap {
                          { "name", "x11" },
                          { "available", true },
                          { "running", true },
                          { "message", "ready" },
                      } } },
    });
    AgentCallResult bindingsResult = AgentCallResult::success(QVariantList {
        QVariantMap {
            { "id", "terminal" },
            { "hotkey", "Alt+Return" },
            { "desktop_id", "org.deepin.Terminal.desktop" },
        },
    });
    AgentCallResult applicationsResult = AgentCallResult::success(QVariantList {
        QVariantMap {
            { "desktop_id", "org.deepin.Terminal.desktop" },
            { "name", "Terminal" },
        },
    });
    AgentCallResult windowsResult = AgentCallResult::success(QVariantList {
        QVariantMap {
            { "id", QVariant::fromValue<qulonglong>(42) },
            { "title", "Terminal" },
            { "wm_class", "deepin-terminal" },
            { "match_score", 120 },
            { "match_evidence", QVariantList {
                  QVariantMap {
                      { "rule_type", "wm_class" },
                      { "value", "deepin-terminal" },
                      { "actual", "deepin-terminal" },
                      { "score_delta", 120 },
                      { "matched", true },
                      { "effect", "include" },
                  },
              } },
        },
    });
    AgentCallResult operationResult = AgentCallResult::failure("not_implemented", "SetBinding is not implemented yet.");
    QVariantMap lastSavedBinding;
    QString lastRemovedId;
    QString lastHotkey;
    QString lastExcludeId;
    QString lastLaunchedDesktopId;
    QString lastWindowFilter;

    bool isAvailable() const override { return available; }
    AgentCallResult getStatus() override { return statusResult; }
    AgentCallResult listBindings() override { return bindingsResult; }
    AgentCallResult listApplications() override { return applicationsResult; }
    AgentCallResult listWindows(const QString& filter) override
    {
        lastWindowFilter = filter;
        return windowsResult;
    }
    AgentCallResult setBinding(const QVariantMap& binding) override
    {
        lastSavedBinding = binding;
        return operationResult;
    }
    AgentCallResult removeBinding(const QString& bindingId) override
    {
        lastRemovedId = bindingId;
        return operationResult;
    }
    AgentCallResult testHotkey(const QString& hotkey, const QString& excludeId) override
    {
        lastHotkey = hotkey;
        lastExcludeId = excludeId;
        return operationResult;
    }
    AgentCallResult launchApp(const QString& desktopId) override
    {
        lastLaunchedDesktopId = desktopId;
        return operationResult;
    }
};

class SettingsControllerTest : public QObject {
    Q_OBJECT

private slots:
    void autostartToggleWorksWithoutAgentConnection()
    {
        QTemporaryDir configHome;
        QVERIFY(configHome.isValid());
        const QByteArray previousConfigHome = qgetenv("XDG_CONFIG_HOME");
        const bool hadConfigHome = qEnvironmentVariableIsSet("XDG_CONFIG_HOME");
        qputenv("XDG_CONFIG_HOME", configHome.path().toUtf8());

        FakeAgentClient client;
        client.available = false;
        const QString configPath = configHome.path() + "/oops-jump/config.json";
        SettingsController controller(client, configPath);
        QSignalSpy autostartChanged(&controller, &SettingsController::autostartEnabledChanged);

        QCOMPARE(controller.autostartEnabled(), false);
        QCOMPARE(controller.setAutostartEnabled(true), true);
        QCOMPARE(controller.autostartEnabled(), true);
        QVERIFY(QFile::exists(configHome.path() + "/autostart/oops-jump-agent.desktop"));
        const auto enabledConfig = ConfigManager(configPath).load();
        QVERIFY(enabledConfig.ok);
        QCOMPARE(enabledConfig.value.general.autostart, true);
        QCOMPARE(autostartChanged.count(), 1);

        QCOMPARE(controller.setAutostartEnabled(false), true);
        QCOMPARE(controller.autostartEnabled(), false);
        QVERIFY(!QFile::exists(configHome.path() + "/autostart/oops-jump-agent.desktop"));
        const auto disabledConfig = ConfigManager(configPath).load();
        QVERIFY(disabledConfig.ok);
        QCOMPARE(disabledConfig.value.general.autostart, false);
        QCOMPARE(autostartChanged.count(), 2);
        QCOMPARE(controller.connected(), false);

        if (hadConfigHome) {
            qputenv("XDG_CONFIG_HOME", previousConfigHome);
        } else {
            qunsetenv("XDG_CONFIG_HOME");
        }
    }

    void autostartToggleInitializesFromConfig()
    {
        QTemporaryDir configHome;
        QVERIFY(configHome.isValid());
        const QByteArray previousConfigHome = qgetenv("XDG_CONFIG_HOME");
        const bool hadConfigHome = qEnvironmentVariableIsSet("XDG_CONFIG_HOME");
        qputenv("XDG_CONFIG_HOME", configHome.path().toUtf8());

        const QString configPath = configHome.path() + "/oops-jump/config.json";
        Config config = Config::defaults();
        config.general.autostart = true;
        QVERIFY(ConfigManager(configPath).save(config).ok);

        FakeAgentClient client;
        SettingsController controller(client, configPath);

        QCOMPARE(controller.autostartEnabled(), true);

        if (hadConfigHome) {
            qputenv("XDG_CONFIG_HOME", previousConfigHome);
        } else {
            qunsetenv("XDG_CONFIG_HOME");
        }
    }

    void autostartToggleFailureDoesNotPersistConfig()
    {
        QTemporaryDir configHome;
        QVERIFY(configHome.isValid());
        const QByteArray previousConfigHome = qgetenv("XDG_CONFIG_HOME");
        const bool hadConfigHome = qEnvironmentVariableIsSet("XDG_CONFIG_HOME");
        qputenv("XDG_CONFIG_HOME", configHome.path().toUtf8());

        QVERIFY(QDir().mkpath(configHome.path() + "/autostart"));
        QFile customAutostart(configHome.path() + "/autostart/oops-jump-agent.desktop");
        QVERIFY(customAutostart.open(QIODevice::WriteOnly | QIODevice::Text));
        customAutostart.write("[Desktop Entry]\n"
                              "Type=Application\n"
                              "Name=Custom Oops Jump Agent\n"
                              "Exec=oops-jump-agent --custom\n");
        customAutostart.close();

        const QString configPath = configHome.path() + "/oops-jump/config.json";
        Config config = Config::defaults();
        config.general.autostart = false;
        QVERIFY(ConfigManager(configPath).save(config).ok);

        FakeAgentClient client;
        SettingsController controller(client, configPath);

        QCOMPARE(controller.setAutostartEnabled(true), false);
        QCOMPARE(controller.autostartEnabled(), false);
        QCOMPARE(controller.lastErrorCode(), QString("autostart_conflict"));
        const auto loaded = ConfigManager(configPath).load();
        QVERIFY(loaded.ok);
        QCOMPARE(loaded.value.general.autostart, false);

        if (hadConfigHome) {
            qputenv("XDG_CONFIG_HOME", previousConfigHome);
        } else {
            qunsetenv("XDG_CONFIG_HOME");
        }
    }

    void refreshPublishesStatusBindingsAndApplications()
    {
        FakeAgentClient client;
        SettingsController controller(client);

        QSignalSpy connectedChanged(&controller, &SettingsController::connectedChanged);
        QSignalSpy bindingsChanged(&controller, &SettingsController::bindingsChanged);
        QSignalSpy applicationsChanged(&controller, &SettingsController::applicationsChanged);

        controller.refresh();

        QCOMPARE(controller.connected(), true);
        QCOMPARE(controller.status().value("active_backend").toString(), QString("x11"));
        QCOMPARE(controller.backendStatus().value("name").toString(), QString("x11"));
        QCOMPARE(controller.status().value("session_type").toString(), QString("x11"));
        QCOMPARE(controller.status().value("capabilities").toMap().value("global_hotkey").toBool(), true);
        QCOMPARE(controller.status().value("binding_statuses").toList().first().toMap().value("status").toString(), QString("registered"));
        QCOMPARE(controller.bindings().size(), 1);
        QCOMPARE(controller.bindings().first().toMap().value("id").toString(), QString("terminal"));
        QCOMPARE(controller.applications().size(), 1);
        QCOMPARE(controller.applications().first().toMap().value("name").toString(), QString("Terminal"));
        QCOMPARE(controller.lastError(), QString());
        QCOMPARE(controller.lastErrorCode(), QString());
        QCOMPARE(connectedChanged.count(), 1);
        QCOMPARE(bindingsChanged.count(), 1);
        QCOMPARE(applicationsChanged.count(), 1);
    }

    void unavailableAgentKeepsSafeEmptyState()
    {
        FakeAgentClient client;
        client.available = false;
        SettingsController controller(client);

        controller.refresh();

        QCOMPARE(controller.connected(), false);
        QCOMPARE(controller.bindings().size(), 0);
        QCOMPARE(controller.applications().size(), 0);
        QVERIFY(controller.lastError().contains("unavailable"));
        QCOMPARE(controller.lastErrorCode(), QString("agent_unavailable"));
        QCOMPARE(controller.saveBinding({ { "id", "terminal" } }), false);
        QCOMPARE(controller.removeBinding("terminal"), false);
        QCOMPARE(controller.testHotkey("Alt+Return", "terminal"), false);
        QCOMPARE(controller.launchApp("org.deepin.Terminal.desktop"), false);
        QVERIFY(client.lastSavedBinding.isEmpty());
        QCOMPARE(client.lastRemovedId, QString());
        QCOMPARE(client.lastHotkey, QString());
        QCOMPARE(client.lastLaunchedDesktopId, QString());
    }

    void refreshWindowDiagnosticsPublishesListWindowsResult()
    {
        FakeAgentClient client;
        SettingsController controller(client);
        QSignalSpy diagnosticsChanged(&controller, &SettingsController::windowDiagnosticsChanged);

        QCOMPARE(controller.refreshWindowDiagnostics("org.deepin.Terminal.desktop"), true);

        QCOMPARE(client.lastWindowFilter, QString("org.deepin.Terminal.desktop"));
        QCOMPARE(controller.windowDiagnostics().size(), 1);
        const QVariantMap window = controller.windowDiagnostics().first().toMap();
        QCOMPARE(window.value("match_score").toInt(), 120);
        const QVariantMap evidence = window.value("match_evidence").toList().first().toMap();
        QCOMPARE(evidence.value("rule_type").toString(), QString("wm_class"));
        QCOMPARE(evidence.value("score_delta").toInt(), 120);
        QCOMPARE(evidence.value("matched").toBool(), true);
        QCOMPARE(diagnosticsChanged.count(), 1);
    }

    void operationsForwardArgumentsAndSurfaceAgentErrors()
    {
        FakeAgentClient client;
        SettingsController controller(client);

        QCOMPARE(controller.saveBinding({ { "id", "terminal" } }), false);
        QCOMPARE(client.lastSavedBinding.value("id").toString(), QString("terminal"));
        QCOMPARE(controller.lastError(), QString("SetBinding is not implemented yet."));
        QCOMPARE(controller.lastErrorCode(), QString("not_implemented"));

        QCOMPARE(controller.removeBinding("terminal"), false);
        QCOMPARE(client.lastRemovedId, QString("terminal"));

        QCOMPARE(controller.testHotkey("Alt+Return", "terminal"), false);
        QCOMPARE(client.lastHotkey, QString("Alt+Return"));
        QCOMPARE(client.lastExcludeId, QString("terminal"));

        QCOMPARE(controller.launchApp("org.deepin.Terminal.desktop"), false);
        QCOMPARE(client.lastLaunchedDesktopId, QString("org.deepin.Terminal.desktop"));
    }

    void testHotkeyPreservesStructuredErrorCodeAndClearsOnSuccess()
    {
        FakeAgentClient client;
        SettingsController controller(client);

        client.operationResult = AgentCallResult::failure("hotkey_conflict", "Hotkey conflicts with binding 'terminal'.");
        QCOMPARE(controller.testHotkey("Alt+Return", "browser"), false);
        QCOMPARE(controller.lastErrorCode(), QString("hotkey_conflict"));
        QCOMPARE(controller.lastError(), QString("Hotkey conflicts with binding 'terminal'."));

        client.operationResult = AgentCallResult::failure("hotkey_backend_unavailable", "Backend-level test unavailable.");
        QCOMPARE(controller.testHotkey("Ctrl+Alt+Enter", "terminal"), false);
        QCOMPARE(controller.lastErrorCode(), QString("hotkey_backend_unavailable"));
        QCOMPARE(controller.lastError(), QString("Backend-level test unavailable."));

        client.operationResult = AgentCallResult::success(QVariantMap {});
        QCOMPARE(controller.testHotkey("Meta+B", "browser"), true);
        QCOMPARE(controller.lastErrorCode(), QString());
        QCOMPARE(controller.lastError(), QString());
    }
};

QTEST_MAIN(SettingsControllerTest)
#include "test_settings_controller.moc"
