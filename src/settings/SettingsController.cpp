#include "settings/SettingsController.h"

#include "core/ConfigManager.h"

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

bool configuredAutostartEnabled(const QString& configPath, const AutostartManager& autostartManager)
{
    const auto loaded = ConfigManager(configPath).load();
    if (loaded.ok) {
        return loaded.value.general.autostart;
    }
    return autostartManager.isEnabled();
}

}

SettingsController::SettingsController(QObject* parent)
    : QObject(parent)
    , m_ownedClient(std::make_unique<AgentDBusClient>())
    , m_client(m_ownedClient.get())
    , m_configPath(ConfigManager::defaultConfigPath())
    , m_autostartEnabled(configuredAutostartEnabled(m_configPath, m_autostartManager))
{
}

SettingsController::SettingsController(AgentClientInterface& client, QObject* parent)
    : SettingsController(client, ConfigManager::defaultConfigPath(), parent)
{
}

SettingsController::SettingsController(AgentClientInterface& client, QString configPath, QObject* parent)
    : QObject(parent)
    , m_client(&client)
    , m_configPath(std::move(configPath))
    , m_autostartEnabled(configuredAutostartEnabled(m_configPath, m_autostartManager))
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

QVariantList SettingsController::windowDiagnostics() const
{
    return m_windowDiagnostics;
}

QVariantMap SettingsController::backendStatus() const
{
    return m_backendStatus;
}

QString SettingsController::lastError() const
{
    return m_lastError;
}

QString SettingsController::lastErrorCode() const
{
    return m_lastErrorCode;
}

bool SettingsController::autostartEnabled() const
{
    return m_autostartEnabled;
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
        setLastErrorResult(statusResult);
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
        setLastErrorResult(bindingsResult);
    }

    const AgentCallResult applicationsResult = m_client->listApplications();
    if (applicationsResult.ok) {
        setApplications(applicationsResult.value.toList());
    } else {
        setApplications({});
        setLastErrorResult(applicationsResult);
    }

    if (statusResult.ok && bindingsResult.ok && applicationsResult.ok) {
        setLastError(QString());
        setLastErrorCode(QString());
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

bool SettingsController::refreshWindowDiagnostics(const QString& appId)
{
    if (!ensureAvailable()) {
        setWindowDiagnostics({});
        return false;
    }

    const AgentCallResult result = m_client->listWindows(appId);
    if (!result.ok) {
        setWindowDiagnostics({});
        setLastErrorResult(result);
        return false;
    }

    setWindowDiagnostics(result.value.toList());
    setLastError(QString());
    setLastErrorCode(QString());
    return true;
}

bool SettingsController::setAutostartEnabled(bool enabled)
{
    ConfigManager configManager(m_configPath);
    auto loaded = configManager.load();
    if (!loaded.ok) {
        setLastErrorResult(AgentCallResult::failure(loaded.errorCode, loaded.message));
        return false;
    }

    Config config = loaded.value;
    const bool previousAutostart = config.general.autostart;

    const auto result = m_autostartManager.setEnabled(enabled);
    if (!result.ok) {
        setLastError(result.message);
        setLastErrorCode(result.errorCode);
        updateAutostartEnabled(previousAutostart);
        return false;
    }

    config.general.autostart = enabled;
    const auto saved = configManager.save(config);
    if (!saved.ok) {
        m_autostartManager.setEnabled(previousAutostart);
        updateAutostartEnabled(previousAutostart);
        setLastErrorResult(AgentCallResult::failure(saved.errorCode, saved.message));
        return false;
    }

    updateAutostartEnabled(enabled);
    setLastError(QString());
    setLastErrorCode(QString());
    return true;
}

bool SettingsController::ensureAvailable()
{
    const bool available = m_client && m_client->isAvailable();
    setConnected(available);
    if (!available) {
        setLastError("DeepSwitch agent is unavailable.");
        setLastErrorCode("agent_unavailable");
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

void SettingsController::setWindowDiagnostics(const QVariantList& windows)
{
    if (m_windowDiagnostics == windows) {
        return;
    }
    m_windowDiagnostics = windows;
    emit windowDiagnosticsChanged();
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

void SettingsController::setLastErrorCode(const QString& lastErrorCode)
{
    if (m_lastErrorCode == lastErrorCode) {
        return;
    }
    m_lastErrorCode = lastErrorCode;
    emit lastErrorCodeChanged();
}

void SettingsController::setLastErrorResult(const AgentCallResult& result)
{
    setLastError(operationErrorMessage(result));
    setLastErrorCode(result.errorCode);
}

bool SettingsController::applyOperationResult(const AgentCallResult& result)
{
    if (!result.ok) {
        setLastErrorResult(result);
        return false;
    }
    setLastError(QString());
    setLastErrorCode(QString());
    return true;
}

void SettingsController::clearData()
{
    setStatus({});
    setBackendStatus({});
    setBindings({});
    setApplications({});
    setWindowDiagnostics({});
}

void SettingsController::updateAutostartEnabled(bool enabled)
{
    if (m_autostartEnabled == enabled) {
        return;
    }
    m_autostartEnabled = enabled;
    emit autostartEnabledChanged();
}

}
