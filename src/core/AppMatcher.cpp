#include "core/AppMatcher.h"

namespace oopsjump {

static bool equalsIgnoreCase(const QString& left, const QString& right)
{
    return left.compare(right, Qt::CaseInsensitive) == 0;
}

static bool containsIgnoreCase(const QString& haystack, const QString& needle)
{
    return haystack.contains(needle, Qt::CaseInsensitive);
}

static QString actualValueForRule(const QString& ruleType, const WindowInfo& window)
{
    if (ruleType == "wm_class") {
        return window.wmClass;
    }
    if (ruleType == "window_title") {
        return window.title;
    }
    if (ruleType == "process_name") {
        return window.appId;
    }
    return window.title;
}

static MatchEvidence evidence(
    const QString& ruleType,
    const QString& value,
    const QString& actual,
    int scoreDelta,
    bool matched,
    const QString& effect = QString("include"))
{
    MatchEvidence item;
    item.source = ruleType;
    item.expected = value;
    item.actual = actual;
    item.score = scoreDelta;
    item.ruleType = ruleType;
    item.value = value;
    item.scoreDelta = scoreDelta;
    item.matched = matched;
    item.effect = effect;
    return item;
}

bool AppMatcher::ruleMatches(const MatchRule& rule, const WindowInfo& window)
{
    if (rule.type != "wm_class" && rule.type != "window_title" && rule.type != "process_name") {
        return false;
    }
    const QString actual = actualValueForRule(rule.type, window);

    if (rule.op == "equals_ignore_case") {
        return equalsIgnoreCase(actual, rule.value);
    }
    if (rule.op == "contains_ignore_case") {
        return containsIgnoreCase(actual, rule.value);
    }
    if (rule.op == "equals") {
        return actual == rule.value;
    }
    if (rule.op == "contains") {
        return actual.contains(rule.value);
    }
    return false;
}

MatchResult AppMatcher::match(const AppInfo& app, const WindowInfo& window, const QList<MatchRule>& rules)
{
    MatchResult result;

    if (!app.startupWmClass.isEmpty() && equalsIgnoreCase(app.startupWmClass, window.wmClass)) {
        result.totalScore += 100;
        result.evidence.append(evidence("startup_wm_class", app.startupWmClass, window.wmClass, 100, true));
    }

    const QString desktopBase = app.desktopId.section('.', 0, -2);
    if (!desktopBase.isEmpty() && equalsIgnoreCase(desktopBase, window.wmClass)) {
        result.totalScore += 80;
        result.evidence.append(evidence("desktop_id", desktopBase, window.wmClass, 80, true));
    }

    const QString execBase = app.exec.section('/', -1).section(' ', 0, 0);
    if (!execBase.isEmpty() && equalsIgnoreCase(execBase, window.appId)) {
        result.totalScore += 70;
        result.evidence.append(evidence("exec", execBase, window.appId, 70, true));
    }

    for (const MatchRule& rule : rules) {
        const QString actualField = actualValueForRule(rule.type, window);
        const bool matched = ruleMatches(rule, window);
        if (!matched) {
            result.evidence.append(evidence(rule.type, rule.value, actualField, 0, false, rule.effect));
            continue;
        }
        if (rule.effect == "exclude") {
            result.matched = false;
            result.totalScore = 0;
            result.evidence.append(evidence(rule.type, rule.value, actualField, -1000, true, "exclude"));
            return result;
        }
        const int score = rule.weight == 0 ? 120 : rule.weight;
        result.totalScore += score;
        result.evidence.append(evidence(rule.type, rule.value, actualField, score, true, rule.effect));
    }

    result.matched = result.totalScore >= 80;
    return result;
}

}
