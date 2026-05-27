#include <QtTest/QtTest>
#include "core/Config.h"
#include "ipc/AgentTypes.h"

using namespace deepswitch;

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
};

QTEST_MAIN(AgentTypesTest)
#include "test_agent_types.moc"
