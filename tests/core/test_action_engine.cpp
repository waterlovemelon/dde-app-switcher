#include <QtTest/QtTest>
#include "core/ActionEngine.h"

using namespace deepswitch;

class ActionEngineTest : public QObject {
    Q_OBJECT

private slots:
    void noWindowsLaunchesWhenAllowed()
    {
        Binding binding;
        binding.id = "app.firefox";
        binding.desktopId = "firefox.desktop";
        binding.launchIfNotRunning = true;

        AppInfo app;
        app.desktopId = "firefox.desktop";
        app.exec = "firefox";

        const ActionDecision decision = ActionEngine::decide(binding, app, {});
        QCOMPARE(decision.type, ActionType::Launch);
    }

    void oneWindowFocuses()
    {
        Binding binding;
        binding.id = "app.firefox";
        binding.desktopId = "firefox.desktop";

        AppInfo app;
        app.desktopId = "firefox.desktop";

        WindowInfo window;
        window.id = 42;

        const ActionDecision decision = ActionEngine::decide(binding, app, { window });
        QCOMPARE(decision.type, ActionType::Focus);
        QCOMPARE(decision.windowId, static_cast<WindowId>(42));
    }
};

QTEST_MAIN(ActionEngineTest)
#include "test_action_engine.moc"
