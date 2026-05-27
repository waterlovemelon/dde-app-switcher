#include "core/AppMatcher.h"

namespace deepswitch {

static bool equalsIgnoreCase(const QString& left, const QString& right)
{
    return left.compare(right, Qt::CaseInsensitive) == 0;
}

static bool containsIgnoreCase(const QString& haystack, const QString& needle)
{
    return haystack.contains(needle, Qt::CaseInsensitive);
}

bool AppMatcher::ruleMatches(const MatchRule& rule, const WindowInfo& window)
{
    QString actual;
    if (rule.type == "wm_class") {
        actual = window.wmClass;
    } else if (rule.type == "window_title") {
        actual = window.title;
    } else if (rule.type == "process_name") {
        actual = window.appId;
    } else {
        return false;
    }

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
        result.evidence.append({ "startup_wm_class", app.startupWmClass, window.wmClass, 100 });
    }

    const QString desktopBase = app.desktopId;
    if (!desktopBase.isEmpty() && containsIgnoreCase(desktopBase, window.wmClass)) {
        result.totalScore += 80;
        result.evidence.append({ "desktop_id", desktopBase, window.wmClass, 80 });
    }

    const QString execBase = app.exec.section('/', -1).section(' ', 0, 0);
    if (!execBase.isEmpty() && equalsIgnoreCase(execBase, window.appId)) {
        result.totalScore += 70;
        result.evidence.append({ "exec", execBase, window.appId, 70 });
    }

    for (const MatchRule& rule : rules) {
        if (!ruleMatches(rule, window)) {
            continue;
        }
        QString actualField;
        if (rule.type == "wm_class") {
            actualField = window.wmClass;
        } else if (rule.type == "window_title") {
            actualField = window.title;
        } else if (rule.type == "process_name") {
            actualField = window.appId;
        } else {
            actualField = window.title;
        }
        if (rule.effect == "exclude") {
            result.matched = false;
            result.totalScore = 0;
            result.evidence.append({ "exclude_rule", rule.value, actualField, -1000 });
            return result;
        }
        const int score = rule.weight == 0 ? 120 : rule.weight;
        result.totalScore += score;
        result.evidence.append({ "user_rule", rule.value, actualField, score });
    }

    result.matched = result.totalScore >= 80;
    return result;
}

}
