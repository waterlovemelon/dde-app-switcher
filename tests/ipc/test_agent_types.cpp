#include <QtTest/QtTest>
#include "core/Config.h"
#include "ipc/AgentTypes.h"

using namespace oopsjump;

class AgentTypesTest : public QObject {
    Q_OBJECT

private slots:
    void bindingRoundTripsThroughVariantMap()
    {
        Binding binding;
        binding.id = "terminal";
        binding.enabled = false;
        binding.hotkey = "Alt+Return";
        binding.selectionKey = "t";
        binding.desktopId = "org.deepin.Terminal.desktop";
        binding.command = "deepin-terminal";
        binding.strategy = MultiWindowStrategy::Picker;
        binding.launchIfNotRunning = false;
        binding.focusExistingWindow = true;
        MatchRule rule;
        rule.type = "wm_class";
        rule.op = "equals";
        rule.value = "deepin-terminal";
        rule.weight = 100;
        rule.effect = "include";
        binding.matchRules.append(rule);

        const QVariantMap map = BindingDto::fromCore(binding).toVariantMap();

        const QStringList expectedKeys = {
            "command",
            "desktop_id",
            "enabled",
            "focus_existing_window",
            "hotkey",
            "id",
            "launch_if_not_running",
            "match_rules",
            "multi_window_strategy",
            "selection_key",
        };
        QStringList actualKeys = map.keys();
        actualKeys.sort();
        QCOMPARE(actualKeys, expectedKeys);

        QCOMPARE(map.value("id").toString(), QString("terminal"));
        QCOMPARE(map.value("enabled").toBool(), false);
        QCOMPARE(map.value("hotkey").toString(), QString("Alt+Return"));
        QCOMPARE(map.value("selection_key").toString(), QString("t"));
        QCOMPARE(map.value("desktop_id").toString(), QString("org.deepin.Terminal.desktop"));
        QCOMPARE(map.value("command").toString(), QString("deepin-terminal"));
        QCOMPARE(map.value("multi_window_strategy").toString(), QString("picker"));
        QCOMPARE(map.value("launch_if_not_running").toBool(), false);
        QCOMPARE(map.value("focus_existing_window").toBool(), true);
        QCOMPARE(map.value("match_rules").toList().size(), 1);

        const BindingDto dto = BindingDto::fromVariantMap(map);
        const Binding roundTripped = dto.toCore();

        QCOMPARE(roundTripped.id, binding.id);
        QCOMPARE(roundTripped.enabled, binding.enabled);
        QCOMPARE(roundTripped.hotkey, binding.hotkey);
        QCOMPARE(roundTripped.selectionKey, binding.selectionKey);
        QCOMPARE(roundTripped.desktopId, binding.desktopId);
        QCOMPARE(roundTripped.command, binding.command);
        QCOMPARE(roundTripped.strategy, binding.strategy);
        QCOMPARE(roundTripped.launchIfNotRunning, binding.launchIfNotRunning);
        QCOMPARE(roundTripped.focusExistingWindow, binding.focusExistingWindow);
        QCOMPARE(roundTripped.matchRules.size(), 1);
        QCOMPARE(roundTripped.matchRules.first().type, rule.type);
        QCOMPARE(roundTripped.matchRules.first().op, rule.op);
        QCOMPARE(roundTripped.matchRules.first().value, rule.value);
        QCOMPARE(roundTripped.matchRules.first().weight, rule.weight);
        QCOMPARE(roundTripped.matchRules.first().effect, rule.effect);
    }

    void windowInfoRoundTripsMatchDiagnostics()
    {
        WindowInfo window;
        window.id = 42;
        window.title = "Terminal";
        window.wmClass = "deepin-terminal";
        window.active = true;
        window.lastActiveOrder = 0;
        window.matchScore = 120;

        MatchEvidence evidence;
        evidence.ruleType = "wm_class";
        evidence.value = "deepin-terminal";
        evidence.actual = "deepin-terminal";
        evidence.scoreDelta = 120;
        evidence.matched = true;
        evidence.effect = "include";
        window.matchEvidence.append(evidence);

        const QVariantMap map = WindowInfoDto::fromCore(window).toVariantMap();

        QCOMPARE(map.value("match_score").toInt(), 120);
        const QVariantList evidenceList = map.value("match_evidence").toList();
        QCOMPARE(evidenceList.size(), 1);
        const QVariantMap evidenceMap = evidenceList.first().toMap();
        QCOMPARE(evidenceMap.value("rule_type").toString(), QString("wm_class"));
        QCOMPARE(evidenceMap.value("value").toString(), QString("deepin-terminal"));
        QCOMPARE(evidenceMap.value("actual").toString(), QString("deepin-terminal"));
        QCOMPARE(evidenceMap.value("score_delta").toInt(), 120);
        QCOMPARE(evidenceMap.value("matched").toBool(), true);
        QCOMPARE(evidenceMap.value("effect").toString(), QString("include"));

        const WindowInfo roundTripped = WindowInfoDto::fromVariantMap(map).toCore();
        QCOMPARE(roundTripped.matchScore, 120);
        QCOMPARE(roundTripped.matchEvidence.size(), 1);
        QCOMPARE(roundTripped.matchEvidence.first().ruleType, QString("wm_class"));
        QCOMPARE(roundTripped.matchEvidence.first().scoreDelta, 120);
        QCOMPARE(roundTripped.matchEvidence.first().matched, true);
    }
};

QTEST_MAIN(AgentTypesTest)
#include "test_agent_types.moc"
