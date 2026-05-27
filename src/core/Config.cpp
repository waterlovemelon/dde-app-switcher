#include "core/Config.h"

namespace deepswitch {

Config Config::defaults()
{
    return {};
}

QString multiWindowStrategyToString(MultiWindowStrategy strategy)
{
    switch (strategy) {
    case MultiWindowStrategy::Recent:
        return "recent";
    case MultiWindowStrategy::Cycle:
        return "cycle";
    case MultiWindowStrategy::Picker:
        return "picker";
    case MultiWindowStrategy::Default:
        return "default";
    }
    return "default";
}

MultiWindowStrategy multiWindowStrategyFromString(const QString& value)
{
    const QString normalized = value.trimmed().toLower();
    if (normalized == "recent") {
        return MultiWindowStrategy::Recent;
    }
    if (normalized == "cycle") {
        return MultiWindowStrategy::Cycle;
    }
    if (normalized == "picker") {
        return MultiWindowStrategy::Picker;
    }
    return MultiWindowStrategy::Default;
}

QString hotkeyModeToString(HotkeyMode mode)
{
    switch (mode) {
    case HotkeyMode::Direct:
        return "direct";
    case HotkeyMode::Leader:
        return "leader";
    }
    return "direct";
}

HotkeyMode hotkeyModeFromString(const QString& value)
{
    const QString normalized = value.trimmed().toLower();
    if (normalized == "leader") {
        return HotkeyMode::Leader;
    }
    return HotkeyMode::Direct;
}

}
