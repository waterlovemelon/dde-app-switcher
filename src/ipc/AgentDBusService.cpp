#include "ipc/AgentDBusService.h"

#include "agent/AgentController.h"
#include "ipc/AgentTypes.h"

namespace deepswitch {
namespace {

QString stateToString(AgentControllerState state)
{
    switch (state) {
    case AgentControllerState::Starting:
        return "starting";
    case AgentControllerState::Running:
        return "running";
    case AgentControllerState::Paused:
        return "paused";
    case AgentControllerState::Degraded:
        return "degraded";
    case AgentControllerState::Error:
        return "error";
    }
    return "unknown";
}

QVariantMap successResult(const QString& message = QString())
{
    QVariantMap result {
        { "ok", true },
        { "error_code", QString() },
        { "message", message },
    };
    return result;
}

QVariantMap listResult(const QVariantList& items)
{
    QVariantMap result = successResult();
    result.insert("items", items);
    return result;
}

QVariantMap failureResult(const QString& errorCode, const QString& message)
{
    return {
        { "ok", false },
        { "error_code", errorCode },
        { "message", message },
    };
}

QVariantMap listFailureResult(const QString& errorCode, const QString& message)
{
    QVariantMap result = failureResult(errorCode, message);
    result.insert("items", QVariantList {});
    return result;
}

BackendStatusDto backendFromStatus(const AgentControllerStatus& status)
{
    BackendStatusDto backend;
    backend.name = status.activeBackend;
    backend.available = status.activeBackend != "disabled";
    backend.running = status.state == AgentControllerState::Running || status.state == AgentControllerState::Degraded;
    backend.message = status.message;
    return backend;
}

}

AgentDBusService::AgentDBusService(AgentController& controller, QObject* parent)
    : QObject(parent)
    , m_controller(controller)
{
}

QVariantMap AgentDBusService::GetStatus() const
{
    const AgentControllerStatus status = m_controller.status();
    AgentStatusDto dto;
    dto.running = status.state == AgentControllerState::Running || status.state == AgentControllerState::Degraded;
    dto.enabled = status.enabled;
    dto.activeBackend = status.activeBackend;
    dto.backends.append(backendFromStatus(status));

    QVariantMap map = dto.toVariantMap();
    map.insert("state", stateToString(status.state));
    map.insert("message", status.message);
    map.insert("session_type", status.sessionType);
    map.insert("hotkey_backend", status.hotkeyBackend);
    map.insert("window_backend", status.windowBackend);
    map.insert("capabilities", status.capabilities);
    map.insert("binding_statuses", status.bindingStatuses);
    map.insert("warnings", status.warnings);
    return map;
}

QVariantMap AgentDBusService::ReloadConfig()
{
    const auto reloaded = m_controller.reloadConfig();
    if (!reloaded.ok) {
        const QVariantMap result = failureResult(reloaded.errorCode, reloaded.message);
        emitError(result);
        emit StatusChanged(GetStatus());
        return result;
    }

    emit StatusChanged(GetStatus());
    emit BindingChanged(QString(), {});
    return successResult("config reloaded");
}

QVariantMap AgentDBusService::Pause()
{
    m_controller.pause();
    emit StatusChanged(GetStatus());
    return successResult("paused");
}

QVariantMap AgentDBusService::Resume()
{
    m_controller.resume();
    emit StatusChanged(GetStatus());
    return successResult("resumed");
}

QVariantList AgentDBusService::ListBindings() const
{
    return BindingDto::toVariantList(m_controller.listBindings());
}

QVariantMap AgentDBusService::SetBinding(const QVariantMap& binding)
{
    Q_UNUSED(binding)
    const QVariantMap result = unsupported("SetBinding");
    emitError(result);
    return result;
}

QVariantMap AgentDBusService::RemoveBinding(const QString& bindingId)
{
    Q_UNUSED(bindingId)
    const QVariantMap result = unsupported("RemoveBinding");
    emitError(result);
    return result;
}

QVariantMap AgentDBusService::TestHotkey(const QString& hotkey, const QString& excludeId)
{
    const auto tested = m_controller.testHotkey(hotkey, excludeId);
    if (!tested.ok) {
        const QVariantMap result = failureResult(tested.errorCode, tested.message);
        emitError(result);
        return result;
    }
    return successResult("hotkey available");
}

QVariantMap AgentDBusService::ListApplications()
{
    const auto listed = m_controller.listApplications();
    if (!listed.ok) {
        const QVariantMap result = listFailureResult(listed.errorCode, listed.message);
        emitError(result);
        return result;
    }
    return listResult(AppInfoDto::toVariantList(listed.value));
}

QVariantMap AgentDBusService::ListWindows(const QString& filter)
{
    const auto listed = m_controller.listWindows(filter);
    if (!listed.ok) {
        const QVariantMap result = listFailureResult(listed.errorCode, listed.message);
        emitError(result);
        return result;
    }

    const QVariantList windows = WindowInfoDto::toVariantList(listed.value);
    emit WindowListChanged(windows);
    return listResult(windows);
}

QVariantMap AgentDBusService::ActivateWindow(qulonglong windowId)
{
    Q_UNUSED(windowId)
    const QVariantMap result = unsupported("ActivateWindow");
    emitError(result);
    return result;
}

QVariantMap AgentDBusService::LaunchApp(const QString& desktopId)
{
    Q_UNUSED(desktopId)
    const QVariantMap result = unsupported("LaunchApp");
    emitError(result);
    return result;
}

QVariantMap AgentDBusService::unsupported(const QString& operation) const
{
    return failureResult("not_implemented", operation + " is not implemented yet.");
}

void AgentDBusService::emitError(const QVariantMap& result)
{
    emit ErrorOccurred(result.value("error_code").toString(), result.value("message").toString());
}

}
