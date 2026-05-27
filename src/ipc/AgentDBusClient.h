#pragma once

#include <QDBusConnection>
#include <QObject>
#include <QString>
#include <QVariant>
#include <QVariantList>
#include <QVariantMap>

#include <memory>

namespace deepswitch {

struct AgentCallResult {
    bool ok = false;
    QVariant value;
    QString errorCode;
    QString message;

    static AgentCallResult success(const QVariant& value = {});
    static AgentCallResult failure(QString errorCode, QString message);
};

class AgentClientInterface : public QObject {
    Q_OBJECT

public:
    explicit AgentClientInterface(QObject* parent = nullptr);
    ~AgentClientInterface() override;

    virtual bool isAvailable() const = 0;
    virtual AgentCallResult getStatus() = 0;
    virtual AgentCallResult listBindings() = 0;
    virtual AgentCallResult listApplications() = 0;
    virtual AgentCallResult setBinding(const QVariantMap& binding) = 0;
    virtual AgentCallResult removeBinding(const QString& bindingId) = 0;
    virtual AgentCallResult testHotkey(const QString& hotkey, const QString& excludeId) = 0;
    virtual AgentCallResult launchApp(const QString& desktopId) = 0;
};

class AgentDBusTransport {
public:
    virtual ~AgentDBusTransport();

    virtual bool isAvailable() const = 0;
    virtual AgentCallResult callMapMethod(const QString& method, const QList<QVariant>& arguments) = 0;
    virtual AgentCallResult callListMethod(const QString& method, const QList<QVariant>& arguments) = 0;
};

class AgentDBusClient : public AgentClientInterface {
    Q_OBJECT

public:
    explicit AgentDBusClient(QObject* parent = nullptr);
    explicit AgentDBusClient(QDBusConnection connection, QObject* parent = nullptr);
    explicit AgentDBusClient(AgentDBusTransport& transport, QObject* parent = nullptr);
    ~AgentDBusClient() override;

    bool isAvailable() const override;
    AgentCallResult getStatus() override;
    AgentCallResult listBindings() override;
    AgentCallResult listApplications() override;
    AgentCallResult setBinding(const QVariantMap& binding) override;
    AgentCallResult removeBinding(const QString& bindingId) override;
    AgentCallResult testHotkey(const QString& hotkey, const QString& excludeId) override;
    AgentCallResult launchApp(const QString& desktopId) override;

private:
    AgentCallResult callMapMethod(const QString& method, const QList<QVariant>& arguments = {}) const;
    AgentCallResult callListMethod(const QString& method, const QList<QVariant>& arguments = {}) const;
    AgentCallResult operationResult(const AgentCallResult& result) const;
    AgentCallResult unavailableResult() const;

    std::unique_ptr<AgentDBusTransport> m_ownedTransport;
    AgentDBusTransport* m_transport = nullptr;
};

}
