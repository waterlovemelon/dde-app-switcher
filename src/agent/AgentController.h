#pragma once

#include "core/AppInfo.h"
#include "core/Config.h"
#include "core/Hotkey.h"
#include "core/Result.h"
#include "core/WindowInfo.h"

#include <QList>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>
#include <functional>
#include <optional>

namespace deepswitch {

class X11HotkeyBackend;

enum class AgentControllerState {
    Starting,
    Running,
    Paused,
    Degraded,
    Error
};

struct AgentControllerStatus {
    AgentControllerState state = AgentControllerState::Starting;
    bool enabled = false;
    QString activeBackend;
    QString message;
    QString sessionType = "unknown";
    QVariantMap hotkeyBackend;
    QVariantMap windowBackend;
    QVariantMap capabilities;
    QVariantList bindingStatuses;
    QStringList warnings;
};

class AgentController {
public:
    using LauncherFn = std::function<VoidResult(const AppInfo&)>;
    using HotkeyTestFn = std::function<VoidResult(const Hotkey&)>;

    enum class BackendMode {
        X11,
        Disabled
    };

    explicit AgentController(
        QString configPath,
        BackendMode backendMode = BackendMode::X11,
        LauncherFn launcher = {},
        HotkeyTestFn hotkeyTester = {});

    VoidResult reloadConfig();
    void pause();
    void resume();
    Result<QString> triggerAction(const QString& actionId);
    Result<QString> triggerHotkeyAction(const QString& actionId);
    QList<Binding> listBindings() const;
    Result<QList<AppInfo>> listApplications();
    Result<QList<WindowInfo>> listWindows(const QString& filter = QString()) const;
    VoidResult testHotkey(const QString& hotkey, const QString& excludeActionId = QString()) const;
    AgentControllerStatus status() const;

    void setApplicationDirs(QStringList dirs);
    Result<int> registerHotkeys(X11HotkeyBackend& hotkeys, QStringList* messages = nullptr);

private:
    enum class TriggerMode {
        Strict,
        TolerateWindowListFailure
    };

    Result<QString> triggerAction(const QString& actionId, TriggerMode mode);
    std::optional<Binding> findBinding(const QString& actionId) const;
    Result<AppInfo> findApplication(const Binding& binding);
    void updateState(AgentControllerState state, QString message = QString());

    QString m_configPath;
    BackendMode m_backendMode = BackendMode::X11;
    Config m_config;
    QStringList m_applicationDirs;
    QList<AppInfo> m_applications;
    AgentControllerStatus m_status;
    LauncherFn m_launcher;
    HotkeyTestFn m_hotkeyTester;
    QSet<QString> m_registeredHotkeyActionIds;
};

}
