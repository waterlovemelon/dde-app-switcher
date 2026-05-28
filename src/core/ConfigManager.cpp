#include "core/ConfigManager.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QSet>
#include <QLoggingCategory>
#include <QStandardPaths>
#include <utility>

Q_LOGGING_CATEGORY(lcConfig, "deepswitch.config")

namespace deepswitch {

ConfigManager::ConfigManager(QString path)
    : m_path(std::move(path))
{
}

QString ConfigManager::defaultConfigPath()
{
    const QString configHome = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
    return QDir(configHome).filePath("deepswitch/config.json");
}

static MatchRule matchRuleFromJson(const QJsonObject& object)
{
    MatchRule rule;
    rule.type = object.value("type").toString();
    rule.op = object.value("operator").toString();
    rule.value = object.value("value").toString();
    rule.weight = object.value("weight").toInt(0);
    rule.effect = object.value("effect").toString("include");
    return rule;
}

static Binding bindingFromJson(const QJsonObject& object)
{
    Binding binding;
    binding.id = object.value("id").toString();
    binding.enabled = object.value("enabled").toBool(true);
    binding.hotkey = object.value("hotkey").toString();
    binding.selectionKey = object.value("selection_key").toString();
    binding.desktopId = object.value("desktop_id").toString();
    binding.command = object.value("command").toString();
    binding.strategy = multiWindowStrategyFromString(object.value("multi_window_strategy").toString("default"));
    binding.launchIfNotRunning = object.value("launch_if_not_running").toBool(true);
    binding.focusExistingWindow = object.value("focus_existing_window").toBool(true);
    const QJsonArray rules = object.value("match_rules").toArray();
    for (const QJsonValue& val : rules) {
        if (val.isObject()) {
            binding.matchRules.append(matchRuleFromJson(val.toObject()));
        }
    }
    return binding;
}

static QJsonObject matchRuleToJson(const MatchRule& rule)
{
    QJsonObject object;
    object["type"] = rule.type;
    object["operator"] = rule.op;
    object["value"] = rule.value;
    object["weight"] = rule.weight;
    object["effect"] = rule.effect;
    return object;
}

static QJsonObject bindingToJson(const Binding& binding)
{
    QJsonObject object;
    object["id"] = binding.id;
    object["enabled"] = binding.enabled;
    object["hotkey"] = binding.hotkey;
    object["selection_key"] = binding.selectionKey;
    object["desktop_id"] = binding.desktopId;
    object["command"] = binding.command;
    object["multi_window_strategy"] = multiWindowStrategyToString(binding.strategy);
    object["launch_if_not_running"] = binding.launchIfNotRunning;
    object["focus_existing_window"] = binding.focusExistingWindow;
    QJsonArray rules;
    for (const MatchRule& rule : binding.matchRules) {
        rules.append(matchRuleToJson(rule));
    }
    object["match_rules"] = rules;
    return object;
}

Result<Config> ConfigManager::load() const
{
    QFile file(m_path);
    if (!file.exists()) {
        qCInfo(lcConfig) << "No config file found, using defaults:" << m_path;
        return Result<Config>::success(Config::defaults());
    }
    if (!file.open(QIODevice::ReadOnly)) {
        qCCritical(lcConfig) << "Cannot open config file:" << m_path;
        return Result<Config>::failure("config_read_failed", "Cannot open config file.");
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        qCCritical(lcConfig) << "Config parse error:" << parseError.errorString() << "at offset" << parseError.offset;
        return Result<Config>::failure("config_parse_failed", parseError.errorString());
    }

    const QJsonObject root = document.object();
    Config config = Config::defaults();
    config.version = root.value("version").toInt(1);

    const QJsonObject general = root.value("general").toObject();
    config.general.enabled = general.value("enabled").toBool(true);
    config.general.autostart = general.value("autostart").toBool(false);
    config.general.sessionBackend = general.value("session_backend").toString("auto");
    config.general.showOverlay = general.value("show_overlay").toBool(true);
    config.general.logLevel = general.value("log_level").toString("info");

    const QJsonObject hotkey = root.value("hotkey").toObject();
    config.hotkey.mode = hotkeyModeFromString(hotkey.value("mode").toString("direct"));
    config.hotkey.leaderKey = hotkey.value("leader_key").toString("Alt+Space");
    config.hotkey.leaderTimeoutMs = hotkey.value("leader_timeout_ms").toInt(1500);

    const QJsonObject window = root.value("window").toObject();
    config.window.defaultStrategy = multiWindowStrategyFromString(window.value("default_multi_window_strategy").toString("cycle"));
    config.window.cycleTimeoutMs = window.value("cycle_timeout_ms").toInt(1200);
    config.window.launchTimeoutMs = window.value("launch_timeout_ms").toInt(8000);
    config.window.includeAllWorkspaces = window.value("include_all_workspaces").toBool(true);
    config.window.switchWorkspaceWhenNeeded = window.value("switch_workspace_when_needed").toBool(true);

    const QJsonArray bindings = root.value("bindings").toArray();
    for (const QJsonValue& value : bindings) {
        if (value.isObject()) {
            config.bindings.append(bindingFromJson(value.toObject()));
        }
    }

    const auto validation = validate(config);
    if (!validation.ok) {
        qCWarning(lcConfig) << "Config validation failed:" << validation.message;
        return Result<Config>::failure(validation.errorCode, validation.message);
    }

    qCInfo(lcConfig) << "Config loaded:" << config.bindings.size() << "bindings from" << m_path;
    return Result<Config>::success(config);
}

VoidResult ConfigManager::save(const Config& config) const
{
    const auto validation = validate(config);
    if (!validation.ok) {
        return validation;
    }

    QJsonObject root;
    root["version"] = config.version;

    QJsonObject general;
    general["enabled"] = config.general.enabled;
    general["autostart"] = config.general.autostart;
    general["session_backend"] = config.general.sessionBackend;
    general["show_overlay"] = config.general.showOverlay;
    general["log_level"] = config.general.logLevel;
    root["general"] = general;

    QJsonObject hotkey;
    hotkey["mode"] = hotkeyModeToString(config.hotkey.mode);
    hotkey["leader_key"] = config.hotkey.leaderKey;
    hotkey["leader_timeout_ms"] = config.hotkey.leaderTimeoutMs;
    root["hotkey"] = hotkey;

    QJsonObject window;
    window["default_multi_window_strategy"] = multiWindowStrategyToString(config.window.defaultStrategy);
    window["cycle_timeout_ms"] = config.window.cycleTimeoutMs;
    window["launch_timeout_ms"] = config.window.launchTimeoutMs;
    window["include_all_workspaces"] = config.window.includeAllWorkspaces;
    window["switch_workspace_when_needed"] = config.window.switchWorkspaceWhenNeeded;
    root["window"] = window;

    QJsonArray bindings;
    for (const Binding& binding : config.bindings) {
        bindings.append(bindingToJson(binding));
    }
    root["bindings"] = bindings;

    QDir().mkpath(QFileInfo(m_path).absolutePath());
    QSaveFile file(m_path);
    if (!file.open(QIODevice::WriteOnly)) {
        qCCritical(lcConfig) << "Cannot open config for writing:" << m_path;
        return VoidResult::failure("config_write_failed", "Cannot open config file for writing.");
    }
    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    if (!file.commit()) {
        qCCritical(lcConfig) << "Cannot commit config file:" << m_path;
        return VoidResult::failure("config_write_failed", "Cannot commit config file.");
    }
    qCInfo(lcConfig) << "Config saved:" << config.bindings.size() << "bindings to" << m_path;
    return VoidResult::success();
}

VoidResult ConfigManager::validate(const Config& config)
{
    QSet<QString> ids;
    for (const Binding& binding : config.bindings) {
        if (binding.id.trimmed().isEmpty()) {
            qCWarning(lcConfig) << "Validation failed: empty binding id";
            return VoidResult::failure("config_validation_failed", "Binding id is empty.");
        }
        if (ids.contains(binding.id)) {
            qCWarning(lcConfig) << "Validation failed: duplicate binding id:" << binding.id;
            return VoidResult::failure("config_validation_failed", "Duplicate binding id: " + binding.id);
        }
        ids.insert(binding.id);
    }
    return VoidResult::success();
}

}
