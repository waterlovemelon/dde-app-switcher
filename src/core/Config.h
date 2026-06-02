#pragma once

#include <QList>
#include <QString>

namespace oopsjump {

enum class HotkeyMode {
    Direct,
    Leader
};

enum class MultiWindowStrategy {
    Default,
    Recent,
    Cycle,
    Picker
};

struct GeneralConfig {
    bool enabled = true;
    bool autostart = false;
    QString sessionBackend = "auto";
    bool showOverlay = true;
    QString logLevel = "info";
    QString language = "system"; // "system", "en", "zh_CN"
};

struct HotkeyConfig {
    HotkeyMode mode = HotkeyMode::Direct;
    QString leaderKey = "Alt+Space";
    int leaderTimeoutMs = 1500;
};

struct WindowConfig {
    MultiWindowStrategy defaultStrategy = MultiWindowStrategy::Cycle;
    int cycleTimeoutMs = 1200;
    int launchTimeoutMs = 8000;
    bool includeAllWorkspaces = true;
    bool switchWorkspaceWhenNeeded = true;
};

struct MatchRule {
    QString type;
    QString op;
    QString value;
    int weight = 0;
    QString effect = "include";
};

struct Binding {
    QString id;
    bool enabled = true;
    QString hotkey;
    QString selectionKey;
    QString desktopId;
    QString command;
    MultiWindowStrategy strategy = MultiWindowStrategy::Default;
    bool launchIfNotRunning = true;
    bool focusExistingWindow = true;
    QList<MatchRule> matchRules;
};

struct Config {
    int version = 1;
    GeneralConfig general;
    HotkeyConfig hotkey;
    WindowConfig window;
    QList<Binding> bindings;

    static Config defaults();
};

QString multiWindowStrategyToString(MultiWindowStrategy strategy);
MultiWindowStrategy multiWindowStrategyFromString(const QString& value);
QString hotkeyModeToString(HotkeyMode mode);
HotkeyMode hotkeyModeFromString(const QString& value);

}
