#include "settings/SettingsController.h"

#include <utility>

namespace deepswitch {

namespace {

QString operationErrorMessage(const AgentCallResult& result)
{
    if (!result.message.isEmpty()) {
        return result.message;
    }
    return result.errorCode;
}

}

SettingsController::SettingsController(QObject* parent)
    : QObject(parent)
    , m_ownedClient(std::make_unique<AgentDBusClient>())
    , m_client(m_ownedClient.get())
{
}

SettingsController::SettingsController(AgentClientInterface& client, QObject* parent)
    : QObject(parent)
    , m_client(&client)
{
}

SettingsController::~SettingsController() = default;

bool SettingsController::connected() const
{
    return m_connected;
}

QVariantMap SettingsController::status() const
{
    return m_status;
}

QVariantList SettingsController::bindings() const
{
    return m_bindings;
}

QVariantList SettingsController::applications() const
{
    return m_applications;
}

QVariantMap SettingsController::backendStatus() const
{
    return m_backendStatus;
}

QString SettingsController::lastError() const
{
    return m_lastError;
}

void SettingsController::refresh()
{
    if (!ensureAvailable()) {
        clearData();
        return;
    }

    const AgentCallResult statusResult = m_client->getStatus();
    if (!statusResult.ok) {
        setConnected(false);
        clearData();
        setLastError(operationErrorMessage(statusResult));
        return;
    }

    setConnected(true);
    setStatus(statusResult.value.toMap());
    const QVariantList backends = m_status.value("backends").toList();
    setBackendStatus(backends.isEmpty() ? QVariantMap {} : backends.first().toMap());

    const AgentCallResult bindingsResult = m_client->listBindings();
    if (bindingsResult.ok) {
        setBindings(bindingsResult.value.toList());
    } else {
        setBindings({});
        setLastError(operationErrorMessage(bindingsResult));
    }

    const AgentCallResult applicationsResult = m_client->listApplications();
    if (applicationsResult.ok) {
        setApplications(applicationsResult.value.toList());
    } else {
        setApplications({});
        setLastError(operationErrorMessage(applicationsResult));
    }

    if (statusResult.ok && bindingsResult.ok && applicationsResult.ok) {
        setLastError(QString());
    }
}

bool SettingsController::saveBinding(const QVariantMap& binding)
{
    if (!ensureAvailable()) {
        return false;
    }
    return applyOperationResult(m_client->setBinding(binding));
}

bool SettingsController::removeBinding(const QString& bindingId)
{
    if (!ensureAvailable()) {
        return false;
    }
    return applyOperationResult(m_client->removeBinding(bindingId));
}

bool SettingsController::testHotkey(const QString& hotkey, const QString& excludeId)
{
    if (!ensureAvailable()) {
        return false;
    }
    return applyOperationResult(m_client->testHotkey(hotkey, excludeId));
}

bool SettingsController::launchApp(const QString& desktopId)
{
    if (!ensureAvailable()) {
        return false;
    }
    return applyOperationResult(m_client->launchApp(desktopId));
}

bool SettingsController::ensureAvailable()
{
    const bool available = m_client && m_client->isAvailable();
    setConnected(available);
    if (!available) {
        setLastError("DeepSwitch agent is unavailable.");
    }
    return available;
}

void SettingsController::setConnected(bool connected)
{
    if (m_connected == connected) {
        return;
    }
    m_connected = connected;
    emit connectedChanged();
}

void SettingsController::setStatus(const QVariantMap& status)
{
    if (m_status == status) {
        return;
    }
    m_status = status;
    emit statusChanged();
}

void SettingsController::setBindings(const QVariantList& bindings)
{
    if (m_bindings == bindings) {
        return;
    }
    m_bindings = bindings;
    emit bindingsChanged();
}

void SettingsController::setApplications(const QVariantList& applications)
{
    if (m_applications == applications) {
        return;
    }
    m_applications = applications;
    emit applicationsChanged();
}

void SettingsController::setBackendStatus(const QVariantMap& backendStatus)
{
    if (m_backendStatus == backendStatus) {
        return;
    }
    m_backendStatus = backendStatus;
    emit backendStatusChanged();
}

void SettingsController::setLastError(const QString& lastError)
{
    if (m_lastError == lastError) {
        return;
    }
    m_lastError = lastError;
    emit lastErrorChanged();
}

bool SettingsController::applyOperationResult(const AgentCallResult& result)
{
    if (!result.ok) {
        setLastError(operationErrorMessage(result));
        return false;
    }
    setLastError(QString());
    return true;
}

void SettingsController::clearData()
{
    setStatus({});
    setBackendStatus({});
    setBindings({});
    setApplications({});
}

}
