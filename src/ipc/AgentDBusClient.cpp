#include "ipc/AgentDBusClient.h"

#include "ipc/AgentDBusContract.h"

#include <QDBusInterface>
#include <QDBusMessage>

#include <utility>

namespace deepswitch {
namespace {

class QDBusAgentTransport : public AgentDBusTransport {
public:
    explicit QDBusAgentTransport(QDBusConnection connection)
        : m_connection(std::move(connection))
    {
    }

    bool isAvailable() const override
    {
        if (!m_connection.isConnected()) {
            return false;
        }

        QDBusInterface iface(
            AgentDBusContract::ServiceName,
            AgentDBusContract::ObjectPath,
            AgentDBusContract::InterfaceName,
            m_connection);
        return iface.isValid();
    }

    AgentCallResult callMapMethod(const QString& method, const QList<QVariant>& arguments) override
    {
        QDBusInterface iface(
            AgentDBusContract::ServiceName,
            AgentDBusContract::ObjectPath,
            AgentDBusContract::InterfaceName,
            m_connection);
        const QDBusMessage reply = iface.callWithArgumentList(QDBus::Block, method, arguments);
        if (reply.type() == QDBusMessage::ErrorMessage) {
            return AgentCallResult::failure(reply.errorName(), reply.errorMessage());
        }
        if (reply.arguments().isEmpty()) {
            return AgentCallResult::failure("invalid_reply", method + " returned no value.");
        }
        return AgentCallResult::success(reply.arguments().first().toMap());
    }

    AgentCallResult callListMethod(const QString& method, const QList<QVariant>& arguments) override
    {
        QDBusInterface iface(
            AgentDBusContract::ServiceName,
            AgentDBusContract::ObjectPath,
            AgentDBusContract::InterfaceName,
            m_connection);
        const QDBusMessage reply = iface.callWithArgumentList(QDBus::Block, method, arguments);
        if (reply.type() == QDBusMessage::ErrorMessage) {
            return AgentCallResult::failure(reply.errorName(), reply.errorMessage());
        }
        if (reply.arguments().isEmpty()) {
            return AgentCallResult::failure("invalid_reply", method + " returned no value.");
        }
        return AgentCallResult::success(reply.arguments().first().toList());
    }

private:
    QDBusConnection m_connection;
};

}

AgentCallResult AgentCallResult::success(const QVariant& value)
{
    AgentCallResult result;
    result.ok = true;
    result.value = value;
    return result;
}

AgentCallResult AgentCallResult::failure(QString errorCode, QString message)
{
    AgentCallResult result;
    result.ok = false;
    result.errorCode = std::move(errorCode);
    result.message = std::move(message);
    return result;
}

AgentClientInterface::AgentClientInterface(QObject* parent)
    : QObject(parent)
{
}

AgentClientInterface::~AgentClientInterface() = default;

AgentDBusTransport::~AgentDBusTransport() = default;

AgentDBusClient::AgentDBusClient(QObject* parent)
    : AgentDBusClient(QDBusConnection::sessionBus(), parent)
{
}

AgentDBusClient::AgentDBusClient(QDBusConnection connection, QObject* parent)
    : AgentClientInterface(parent)
    , m_ownedTransport(std::make_unique<QDBusAgentTransport>(std::move(connection)))
    , m_transport(m_ownedTransport.get())
{
}

AgentDBusClient::AgentDBusClient(AgentDBusTransport& transport, QObject* parent)
    : AgentClientInterface(parent)
    , m_transport(&transport)
{
}

AgentDBusClient::~AgentDBusClient() = default;

bool AgentDBusClient::isAvailable() const
{
    return m_transport && m_transport->isAvailable();
}

AgentCallResult AgentDBusClient::getStatus()
{
    return callMapMethod("GetStatus");
}

AgentCallResult AgentDBusClient::listBindings()
{
    return callListMethod("ListBindings");
}

AgentCallResult AgentDBusClient::listApplications()
{
    AgentCallResult result = callMapMethod("ListApplications");
    if (!result.ok) {
        return result;
    }

    const QVariantMap envelope = result.value.toMap();
    if (!envelope.value("ok").toBool()) {
        return AgentCallResult::failure(
            envelope.value("error_code").toString(),
            envelope.value("message").toString());
    }
    return AgentCallResult::success(envelope.value("items").toList());
}

AgentCallResult AgentDBusClient::setBinding(const QVariantMap& binding)
{
    return operationResult(callMapMethod("SetBinding", { binding }));
}

AgentCallResult AgentDBusClient::removeBinding(const QString& bindingId)
{
    return operationResult(callMapMethod("RemoveBinding", { bindingId }));
}

AgentCallResult AgentDBusClient::testHotkey(const QString& hotkey, const QString& excludeId)
{
    return operationResult(callMapMethod("TestHotkey", { hotkey, excludeId }));
}

AgentCallResult AgentDBusClient::launchApp(const QString& desktopId)
{
    return operationResult(callMapMethod("LaunchApp", { desktopId }));
}

AgentCallResult AgentDBusClient::callMapMethod(const QString& method, const QList<QVariant>& arguments) const
{
    if (!isAvailable()) {
        return unavailableResult();
    }
    return m_transport->callMapMethod(method, arguments);
}

AgentCallResult AgentDBusClient::callListMethod(const QString& method, const QList<QVariant>& arguments) const
{
    if (!isAvailable()) {
        return unavailableResult();
    }
    return m_transport->callListMethod(method, arguments);
}

AgentCallResult AgentDBusClient::operationResult(const AgentCallResult& result) const
{
    if (!result.ok) {
        return result;
    }
    const QVariantMap map = result.value.toMap();
    if (map.contains("ok") && !map.value("ok").toBool()) {
        return AgentCallResult::failure(
            map.value("error_code").toString(),
            map.value("message").toString());
    }
    return result;
}

AgentCallResult AgentDBusClient::unavailableResult() const
{
    return AgentCallResult::failure("agent_unavailable", "DeepSwitch agent is unavailable.");
}

}
