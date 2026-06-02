#include <QtTest/QtTest>
#include "core/AppMatcher.h"

using namespace oopsjump;

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
        QCOMPARE(result.evidence.first().ruleType, QString("startup_wm_class"));
        QCOMPARE(result.evidence.first().value, QString("firefox"));
        QCOMPARE(result.evidence.first().actual, QString("Firefox"));
        QCOMPARE(result.evidence.first().scoreDelta, 100);
        QCOMPARE(result.evidence.first().matched, true);
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
        QCOMPARE(result.evidence.last().ruleType, QString("window_title"));
        QCOMPARE(result.evidence.last().value, QString("Private"));
        QCOMPARE(result.evidence.last().actual, QString("Private Window"));
        QCOMPARE(result.evidence.last().scoreDelta, -1000);
        QCOMPARE(result.evidence.last().effect, QString("exclude"));
        QCOMPARE(result.evidence.last().matched, true);
    }

    void recordsUnmatchedRuleEvidenceForDiagnostics()
    {
        AppInfo app;
        app.desktopId = "browser.desktop";

        WindowInfo window;
        window.wmClass = "Terminal";
        window.title = "Shell";

        MatchRule rule;
        rule.type = "wm_class";
        rule.op = "equals_ignore_case";
        rule.value = "Browser";
        rule.weight = 125;

        const MatchResult result = AppMatcher::match(app, window, { rule });
        QVERIFY(!result.matched);
        QVERIFY(!result.evidence.isEmpty());
        const MatchEvidence evidence = result.evidence.last();
        QCOMPARE(evidence.ruleType, QString("wm_class"));
        QCOMPARE(evidence.value, QString("Browser"));
        QCOMPARE(evidence.actual, QString("Terminal"));
        QCOMPARE(evidence.scoreDelta, 0);
        QCOMPARE(evidence.matched, false);
        QCOMPARE(evidence.effect, QString("include"));
    }
};

QTEST_MAIN(AppMatcherTest)
#include "test_app_matcher.moc"
