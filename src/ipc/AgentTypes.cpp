#include "ipc/AgentTypes.h"

namespace deepswitch {
namespace {

QVariantMap matchRuleToVariantMap(const MatchRule& rule)
{
    return {
        { "type", rule.type },
        { "op", rule.op },
        { "value", rule.value },
        { "weight", rule.weight },
        { "effect", rule.effect },
    };
}

MatchRule matchRuleFromVariantMap(const QVariantMap& map)
{
    MatchRule rule;
    rule.type = map.value("type").toString();
    rule.op = map.value("op").toString();
    rule.value = map.value("value").toString();
    rule.weight = map.value("weight").toInt();
    rule.effect = map.value("effect", QString("include")).toString();
    return rule;
}

QVariantList matchRulesToVariantList(const QList<MatchRule>& rules)
{
    QVariantList list;
    list.reserve(rules.size());
    for (const MatchRule& rule : rules) {
        list.append(matchRuleToVariantMap(rule));
    }
    return list;
}

QList<MatchRule> matchRulesFromVariantList(const QVariantList& rules)
{
    QList<MatchRule> list;
    list.reserve(rules.size());
    for (const QVariant& rule : rules) {
        list.append(matchRuleFromVariantMap(rule.toMap()));
    }
    return list;
}

QVariantMap matchEvidenceToVariantMap(const MatchEvidence& evidence)
{
    return {
        { "source", evidence.source },
        { "expected", evidence.expected },
        { "actual", evidence.actual },
        { "score", evidence.score },
        { "rule_type", evidence.ruleType },
        { "value", evidence.value },
        { "score_delta", evidence.scoreDelta },
        { "matched", evidence.matched },
        { "effect", evidence.effect },
    };
}

MatchEvidence matchEvidenceFromVariantMap(const QVariantMap& map)
{
    MatchEvidence evidence;
    evidence.source = map.value("source", map.value("rule_type")).toString();
    evidence.expected = map.value("expected", map.value("value")).toString();
    evidence.actual = map.value("actual").toString();
    evidence.score = map.value("score", map.value("score_delta")).toInt();
    evidence.ruleType = map.value("rule_type", evidence.source).toString();
    evidence.value = map.value("value", evidence.expected).toString();
    evidence.scoreDelta = map.value("score_delta", evidence.score).toInt();
    evidence.matched = map.value("matched").toBool();
    evidence.effect = map.value("effect", QString("include")).toString();
    return evidence;
}

QVariantList matchEvidenceToVariantList(const QList<MatchEvidence>& evidence)
{
    QVariantList list;
    list.reserve(evidence.size());
    for (const MatchEvidence& item : evidence) {
        list.append(matchEvidenceToVariantMap(item));
    }
    return list;
}

QList<MatchEvidence> matchEvidenceFromVariantList(const QVariantList& evidence)
{
    QList<MatchEvidence> list;
    list.reserve(evidence.size());
    for (const QVariant& item : evidence) {
        list.append(matchEvidenceFromVariantMap(item.toMap()));
    }
    return list;
}

QVariantList stringListToVariantList(const QStringList& values)
{
    QVariantList list;
    list.reserve(values.size());
    for (const QString& value : values) {
        list.append(value);
    }
    return list;
}

QStringList stringListFromVariantList(const QVariantList& values)
{
    QStringList list;
    list.reserve(values.size());
    for (const QVariant& value : values) {
        list.append(value.toString());
    }
    return list;
}

}

BindingDto BindingDto::fromCore(const Binding& binding)
{
    BindingDto dto;
    dto.id = binding.id;
    dto.enabled = binding.enabled;
    dto.hotkey = binding.hotkey;
    dto.selectionKey = binding.selectionKey;
    dto.desktopId = binding.desktopId;
    dto.command = binding.command;
    dto.multiWindowStrategy = multiWindowStrategyToString(binding.strategy);
    dto.launchIfNotRunning = binding.launchIfNotRunning;
    dto.focusExistingWindow = binding.focusExistingWindow;
    dto.matchRules = matchRulesToVariantList(binding.matchRules);
    return dto;
}

Binding BindingDto::toCore() const
{
    Binding binding;
    binding.id = id;
    binding.enabled = enabled;
    binding.hotkey = hotkey;
    binding.selectionKey = selectionKey;
    binding.desktopId = desktopId;
    binding.command = command;
    binding.strategy = multiWindowStrategyFromString(multiWindowStrategy);
    binding.launchIfNotRunning = launchIfNotRunning;
    binding.focusExistingWindow = focusExistingWindow;
    binding.matchRules = matchRulesFromVariantList(matchRules);
    return binding;
}

BindingDto BindingDto::fromVariantMap(const QVariantMap& map)
{
    BindingDto dto;
    dto.id = map.value("id").toString();
    dto.enabled = map.value("enabled", true).toBool();
    dto.hotkey = map.value("hotkey").toString();
    dto.selectionKey = map.value("selection_key").toString();
    dto.desktopId = map.value("desktop_id").toString();
    dto.command = map.value("command").toString();
    dto.multiWindowStrategy = map.value("multi_window_strategy", QString("default")).toString();
    dto.launchIfNotRunning = map.value("launch_if_not_running", true).toBool();
    dto.focusExistingWindow = map.value("focus_existing_window", true).toBool();
    dto.matchRules = map.value("match_rules").toList();
    return dto;
}

QVariantMap BindingDto::toVariantMap() const
{
    return {
        { "id", id },
        { "enabled", enabled },
        { "hotkey", hotkey },
        { "selection_key", selectionKey },
        { "desktop_id", desktopId },
        { "command", command },
        { "multi_window_strategy", multiWindowStrategy },
        { "launch_if_not_running", launchIfNotRunning },
        { "focus_existing_window", focusExistingWindow },
        { "match_rules", matchRules },
    };
}

QVariantList BindingDto::toVariantList(const QList<Binding>& bindings)
{
    QVariantList list;
    list.reserve(bindings.size());
    for (const Binding& binding : bindings) {
        list.append(BindingDto::fromCore(binding).toVariantMap());
    }
    return list;
}

QList<Binding> BindingDto::toCoreList(const QVariantList& bindings)
{
    QList<Binding> list;
    list.reserve(bindings.size());
    for (const QVariant& binding : bindings) {
        list.append(BindingDto::fromVariantMap(binding.toMap()).toCore());
    }
    return list;
}

AppInfoDto AppInfoDto::fromCore(const AppInfo& app)
{
    AppInfoDto dto;
    dto.desktopId = app.desktopId;
    dto.name = app.name;
    dto.localizedName = app.localizedName;
    dto.exec = app.exec;
    dto.icon = app.icon;
    dto.startupWmClass = app.startupWmClass;
    dto.categories = app.categories;
    dto.desktopFilePath = app.desktopFilePath;
    dto.terminal = app.terminal;
    dto.noDisplay = app.noDisplay;
    dto.hidden = app.hidden;
    return dto;
}

AppInfo AppInfoDto::toCore() const
{
    AppInfo app;
    app.desktopId = desktopId;
    app.name = name;
    app.localizedName = localizedName;
    app.exec = exec;
    app.icon = icon;
    app.startupWmClass = startupWmClass;
    app.categories = categories;
    app.desktopFilePath = desktopFilePath;
    app.terminal = terminal;
    app.noDisplay = noDisplay;
    app.hidden = hidden;
    return app;
}

AppInfoDto AppInfoDto::fromVariantMap(const QVariantMap& map)
{
    AppInfoDto dto;
    dto.desktopId = map.value("desktop_id").toString();
    dto.name = map.value("name").toString();
    dto.localizedName = map.value("localized_name").toString();
    dto.exec = map.value("exec").toString();
    dto.icon = map.value("icon").toString();
    dto.startupWmClass = map.value("startup_wm_class").toString();
    dto.categories = stringListFromVariantList(map.value("categories").toList());
    dto.desktopFilePath = map.value("desktop_file_path").toString();
    dto.terminal = map.value("terminal").toBool();
    dto.noDisplay = map.value("no_display").toBool();
    dto.hidden = map.value("hidden").toBool();
    return dto;
}

QVariantMap AppInfoDto::toVariantMap() const
{
    return {
        { "desktop_id", desktopId },
        { "name", name },
        { "localized_name", localizedName },
        { "exec", exec },
        { "icon", icon },
        { "startup_wm_class", startupWmClass },
        { "categories", stringListToVariantList(categories) },
        { "desktop_file_path", desktopFilePath },
        { "terminal", terminal },
        { "no_display", noDisplay },
        { "hidden", hidden },
    };
}

QVariantList AppInfoDto::toVariantList(const QList<AppInfo>& apps)
{
    QVariantList list;
    list.reserve(apps.size());
    for (const AppInfo& app : apps) {
        list.append(AppInfoDto::fromCore(app).toVariantMap());
    }
    return list;
}

QList<AppInfo> AppInfoDto::toCoreList(const QVariantList& apps)
{
    QList<AppInfo> list;
    list.reserve(apps.size());
    for (const QVariant& app : apps) {
        list.append(AppInfoDto::fromVariantMap(app.toMap()).toCore());
    }
    return list;
}

WindowInfoDto WindowInfoDto::fromCore(const WindowInfo& window)
{
    WindowInfoDto dto;
    dto.id = window.id;
    dto.title = window.title;
    dto.wmClass = window.wmClass;
    dto.instanceName = window.instanceName;
    dto.pid = window.pid;
    dto.desktop = window.desktop;
    dto.minimized = window.minimized;
    dto.active = window.active;
    dto.lastActiveOrder = window.lastActiveOrder;
    dto.windowType = window.windowType;
    dto.appId = window.appId;
    dto.skipTaskbar = window.skipTaskbar;
    dto.matchScore = window.matchScore;
    dto.matchEvidence = matchEvidenceToVariantList(window.matchEvidence);
    return dto;
}

WindowInfo WindowInfoDto::toCore() const
{
    WindowInfo window;
    window.id = id;
    window.title = title;
    window.wmClass = wmClass;
    window.instanceName = instanceName;
    window.pid = pid;
    window.desktop = desktop;
    window.minimized = minimized;
    window.active = active;
    window.lastActiveOrder = lastActiveOrder;
    window.windowType = windowType;
    window.appId = appId;
    window.skipTaskbar = skipTaskbar;
    window.matchScore = matchScore;
    window.matchEvidence = matchEvidenceFromVariantList(matchEvidence);
    return window;
}

WindowInfoDto WindowInfoDto::fromVariantMap(const QVariantMap& map)
{
    WindowInfoDto dto;
    dto.id = map.value("id").toULongLong();
    dto.title = map.value("title").toString();
    dto.wmClass = map.value("wm_class").toString();
    dto.instanceName = map.value("instance_name").toString();
    dto.pid = map.value("pid").toInt();
    dto.desktop = map.value("desktop", -1).toInt();
    dto.minimized = map.value("minimized").toBool();
    dto.active = map.value("active").toBool();
    dto.lastActiveOrder = map.value("last_active_order").toInt();
    dto.windowType = map.value("window_type").toString();
    dto.appId = map.value("app_id").toString();
    dto.skipTaskbar = map.value("skip_taskbar").toBool();
    dto.matchScore = map.value("match_score").toInt();
    dto.matchEvidence = map.value("match_evidence").toList();
    return dto;
}

QVariantMap WindowInfoDto::toVariantMap() const
{
    return {
        { "id", QVariant::fromValue<qulonglong>(id) },
        { "title", title },
        { "wm_class", wmClass },
        { "instance_name", instanceName },
        { "pid", pid },
        { "desktop", desktop },
        { "minimized", minimized },
        { "active", active },
        { "last_active_order", lastActiveOrder },
        { "window_type", windowType },
        { "app_id", appId },
        { "skip_taskbar", skipTaskbar },
        { "match_score", matchScore },
        { "match_evidence", matchEvidence },
    };
}

QVariantList WindowInfoDto::toVariantList(const QList<WindowInfo>& windows)
{
    QVariantList list;
    list.reserve(windows.size());
    for (const WindowInfo& window : windows) {
        list.append(WindowInfoDto::fromCore(window).toVariantMap());
    }
    return list;
}

QList<WindowInfo> WindowInfoDto::toCoreList(const QVariantList& windows)
{
    QList<WindowInfo> list;
    list.reserve(windows.size());
    for (const QVariant& window : windows) {
        list.append(WindowInfoDto::fromVariantMap(window.toMap()).toCore());
    }
    return list;
}

BackendStatusDto BackendStatusDto::fromVariantMap(const QVariantMap& map)
{
    BackendStatusDto dto;
    dto.name = map.value("name").toString();
    dto.available = map.value("available").toBool();
    dto.running = map.value("running").toBool();
    dto.message = map.value("message").toString();
    return dto;
}

QVariantMap BackendStatusDto::toVariantMap() const
{
    return {
        { "name", name },
        { "available", available },
        { "running", running },
        { "message", message },
    };
}

AgentStatusDto AgentStatusDto::fromVariantMap(const QVariantMap& map)
{
    AgentStatusDto dto;
    dto.running = map.value("running").toBool();
    dto.enabled = map.value("enabled").toBool();
    dto.activeBackend = map.value("active_backend").toString();
    const QVariantList backendMaps = map.value("backends").toList();
    dto.backends.reserve(backendMaps.size());
    for (const QVariant& backend : backendMaps) {
        dto.backends.append(BackendStatusDto::fromVariantMap(backend.toMap()));
    }
    return dto;
}

QVariantMap AgentStatusDto::toVariantMap() const
{
    QVariantList backendMaps;
    backendMaps.reserve(backends.size());
    for (const BackendStatusDto& backend : backends) {
        backendMaps.append(backend.toVariantMap());
    }

    return {
        { "running", running },
        { "enabled", enabled },
        { "active_backend", activeBackend },
        { "backends", backendMaps },
    };
}

ConfigIssueDto ConfigIssueDto::fromVariantMap(const QVariantMap& map)
{
    ConfigIssueDto dto;
    dto.code = map.value("code").toString();
    dto.message = map.value("message").toString();
    dto.path = map.value("path").toString();
    dto.severity = map.value("severity").toString();
    return dto;
}

QVariantMap ConfigIssueDto::toVariantMap() const
{
    return {
        { "code", code },
        { "message", message },
        { "path", path },
        { "severity", severity },
    };
}

QVariantList ConfigIssueDto::toVariantList(const QList<ConfigIssueDto>& issues)
{
    QVariantList list;
    list.reserve(issues.size());
    for (const ConfigIssueDto& issue : issues) {
        list.append(issue.toVariantMap());
    }
    return list;
}

QList<ConfigIssueDto> ConfigIssueDto::fromVariantList(const QVariantList& issues)
{
    QList<ConfigIssueDto> list;
    list.reserve(issues.size());
    for (const QVariant& issue : issues) {
        list.append(ConfigIssueDto::fromVariantMap(issue.toMap()));
    }
    return list;
}

}
