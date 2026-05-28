#include <QtTest/QtTest>

#include "settings/SettingsController.h"

using namespace deepswitch;

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
    AgentCallResult operationResult = AgentCallResult::failure("not_implemented", "SetBinding is not implemented yet.");
    QVariantMap lastSavedBinding;
    QString lastRemovedId;
    QString lastHotkey;
    QString lastExcludeId;
    QString lastLaunchedDesktopId;

    bool isAvailable() const override { return available; }
    AgentCallResult getStatus() override { return statusResult; }
    AgentCallResult listBindings() override { return bindingsResult; }
    AgentCallResult listApplications() override { return applicationsResult; }
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
