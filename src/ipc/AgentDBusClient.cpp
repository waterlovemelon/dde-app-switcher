#include "ipc/AgentDBusClient.h"

#include "ipc/AgentDBusContract.h"

#include <QDBusArgument>
#include <QDBusInterface>
#include <QDBusMessage>

#include <utility>

namespace oopsjump {
namespace {

// D-Bus arrays/maps arrive as QDBusArgument, not QVariantList/QVariantMap.
// These helpers unwrap them so the rest of the code can use plain QVariant types.

QVariant unwrapArgument(const QVariant& variant);

QVariantMap variantToMap(const QVariant& variant)
{
    if (variant.canConvert<QDBusArgument>()) {
        const QDBusArgument arg = variant.value<QDBusArgument>();
        QVariantMap map;
        arg >> map;
        // Recursively unwrap nested values.
        for (auto it = map.begin(); it != map.end(); ++it) {
            it.value() = unwrapArgument(it.value());
        }
        return map;
    }
    return variant.toMap();
}

QVariantList variantToList(const QVariant& variant)
{
    if (variant.canConvert<QDBusArgument>()) {
        const QDBusArgument arg = variant.value<QDBusArgument>();
        QVariantList list;
        arg >> list;
        // Recursively unwrap each element.
        for (int i = 0; i < list.size(); ++i) {
            list[i] = unwrapArgument(list[i]);
        }
        return list;
    }
    return variant.toList();
}

QVariant unwrapArgument(const QVariant& variant)
{
    if (!variant.canConvert<QDBusArgument>()) {
        return variant;
    }
    const QDBusArgument arg = variant.value<QDBusArgument>();
    // Peek at the D-Bus type to decide how to unwrap.
    if (arg.currentType() == QDBusArgument::MapType ||
        arg.currentType() == QDBusArgument::StructureType) {
        return QVariant(variantToMap(variant));
    }
    if (arg.currentType() == QDBusArgument::ArrayType) {
        return QVariant(variantToList(variant));
    }
    return variant;
}

QVariantList argumentToList(const QVariant& variant)
{
    return variantToList(variant);
}

QVariantMap argumentToMap(const QVariant& variant)
{
    return variantToMap(variant);
}

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
        return AgentCallResult::success(argumentToMap(reply.arguments().first()));
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
        return AgentCallResult::success(argumentToList(reply.arguments().first()));
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
    return AgentCallResult::success(argumentToList(envelope.value("items")));
}

AgentCallResult AgentDBusClient::listWindows(const QString& filter)
{
    AgentCallResult result = callMapMethod("ListWindows", { filter });
    if (!result.ok) {
        return result;
    }

    const QVariantMap envelope = result.value.toMap();
    if (!envelope.value("ok").toBool()) {
        return AgentCallResult::failure(
            envelope.value("error_code").toString(),
            envelope.value("message").toString());
    }
    return AgentCallResult::success(argumentToList(envelope.value("items")));
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
    return AgentCallResult::failure("agent_unavailable", "Oops Jump agent is unavailable.");
}

}
