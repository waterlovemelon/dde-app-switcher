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

    void multipleWindowsCyclesToNextWindowAfterActive()
    {
        Binding binding;
        binding.id = "app.firefox";
        binding.desktopId = "firefox.desktop";

        AppInfo app;
        app.desktopId = "firefox.desktop";

        WindowInfo active;
        active.id = 10;
        active.active = true;
        active.lastActiveOrder = 0;

        WindowInfo next;
        next.id = 20;
        next.lastActiveOrder = 1;

        WindowInfo older;
        older.id = 30;
        older.lastActiveOrder = 2;

        const ActionDecision decision = ActionEngine::decide(binding, app, { older, active, next });
        QCOMPARE(decision.type, ActionType::Cycle);
        QCOMPARE(decision.windowId, static_cast<WindowId>(20));
    }

    void multipleWindowsUseLastActiveOrderWhenNoActiveMatch()
    {
        Binding binding;
        binding.id = "app.firefox";
        binding.desktopId = "firefox.desktop";

        AppInfo app;
        app.desktopId = "firefox.desktop";

        WindowInfo older;
        older.id = 30;
        older.lastActiveOrder = 4;

        WindowInfo recent;
        recent.id = 20;
        recent.lastActiveOrder = 1;

        const ActionDecision decision = ActionEngine::decide(binding, app, { older, recent });
        QCOMPARE(decision.type, ActionType::Cycle);
        QCOMPARE(decision.windowId, static_cast<WindowId>(20));
    }
};

QTEST_MAIN(ActionEngineTest)
#include "test_action_engine.moc"
