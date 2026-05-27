#include <QtTest/QtTest>
#include "core/AppMatcher.h"

using namespace deepswitch;

class AppMatcherTest : public QObject {
    Q_OBJECT

private slots:
    void startupWmClassMatchesWindowClass()
    {
        AppInfo app;
        app.desktopId = "firefox.desktop";
        app.exec = "firefox";
        app.startupWmClass = "firefox";

        WindowInfo window;
        window.wmClass = "Firefox";
        window.title = "GitHub - Mozilla Firefox";

        const MatchResult result = AppMatcher::match(app, window, {});
        QVERIFY(result.matched);
        QVERIFY(result.totalScore >= 80);
    }

    void excludeRuleRejectsWindow()
    {
        AppInfo app;
        app.desktopId = "firefox.desktop";
        app.startupWmClass = "firefox";

        WindowInfo window;
        window.wmClass = "firefox";
        window.title = "Private Window";

        MatchRule rule;
        rule.type = "window_title";
        rule.op = "contains_ignore_case";
        rule.value = "Private";
        rule.effect = "exclude";

        const MatchResult result = AppMatcher::match(app, window, { rule });
        QVERIFY(!result.matched);
    }
};

QTEST_MAIN(AppMatcherTest)
#include "test_app_matcher.moc"
