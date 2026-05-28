#pragma once

#include "core/AutostartManager.h"
#include "ipc/AgentDBusClient.h"

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVariantMap>

#include <memory>

namespace deepswitch {

class SettingsController : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool connected READ connected NOTIFY connectedChanged)
    Q_PROPERTY(QVariantMap status READ status NOTIFY statusChanged)
    Q_PROPERTY(QVariantList bindings READ bindings NOTIFY bindingsChanged)
    Q_PROPERTY(QVariantList applications READ applications NOTIFY applicationsChanged)
    Q_PROPERTY(QVariantMap backendStatus READ backendStatus NOTIFY backendStatusChanged)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)
    Q_PROPERTY(QString lastErrorCode READ lastErrorCode NOTIFY lastErrorCodeChanged)
    Q_PROPERTY(bool autostartEnabled READ autostartEnabled NOTIFY autostartEnabledChanged)

public:
    explicit SettingsController(QObject* parent = nullptr);
    explicit SettingsController(AgentClientInterface& client, QObject* parent = nullptr);
    SettingsController(AgentClientInterface& client, QString configPath, QObject* parent = nullptr);
    ~SettingsController() override;

    bool connected() const;
    QVariantMap status() const;
    QVariantList bindings() const;
    QVariantList applications() const;
    QVariantMap backendStatus() const;
    QString lastError() const;
    QString lastErrorCode() const;
    bool autostartEnabled() const;

    Q_INVOKABLE void refresh();
    Q_INVOKABLE bool saveBinding(const QVariantMap& binding);
    Q_INVOKABLE bool removeBinding(const QString& bindingId);
    Q_INVOKABLE bool testHotkey(const QString& hotkey, const QString& excludeId = QString());
    Q_INVOKABLE bool launchApp(const QString& desktopId);
    Q_INVOKABLE bool setAutostartEnabled(bool enabled);

signals:
    void connectedChanged();
    void statusChanged();
    void bindingsChanged();
    void applicationsChanged();
    void backendStatusChanged();
    void lastErrorChanged();
    void lastErrorCodeChanged();
    void autostartEnabledChanged();

private:
    bool ensureAvailable();
    void setConnected(bool connected);
    void setStatus(const QVariantMap& status);
    void setBindings(const QVariantList& bindings);
    void setApplications(const QVariantList& applications);
    void setBackendStatus(const QVariantMap& backendStatus);
    void setLastError(const QString& lastError);
    void setLastErrorCode(const QString& lastErrorCode);
    void setLastErrorResult(const AgentCallResult& result);
    bool applyOperationResult(const AgentCallResult& result);
    void clearData();
    void updateAutostartEnabled(bool enabled);

    std::unique_ptr<AgentClientInterface> m_ownedClient;
    AgentClientInterface* m_client = nullptr;
    AutostartManager m_autostartManager;
    QString m_configPath;
    bool m_connected = false;
    bool m_autostartEnabled = false;
    QVariantMap m_status;
    QVariantList m_bindings;
    QVariantList m_applications;
    QVariantMap m_backendStatus;
    QString m_lastError;
    QString m_lastErrorCode;
};

}
