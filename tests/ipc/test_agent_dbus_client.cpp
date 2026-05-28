#include <QtTest/QtTest>

#include "ipc/AgentDBusClient.h"
#include "ipc/AgentDBusContract.h"

using namespace deepswitch;

class FakeTransport : public AgentDBusTransport {
public:
    bool available = true;
    AgentCallResult nextMapResult = AgentCallResult::success(QVariantMap {});
    AgentCallResult nextListResult = AgentCallResult::success(QVariantList {});
    QString lastMethod;
    QList<QVariant> lastArguments;

    bool isAvailable() const override { return available; }

    AgentCallResult callMapMethod(const QString& method, const QList<QVariant>& arguments) override
    {
        lastMethod = method;
        lastArguments = arguments;
        return nextMapResult;
    }

    AgentCallResult callListMethod(const QString& method, const QList<QVariant>& arguments) override
    {
        lastMethod = method;
        lastArguments = arguments;
        return nextListResult;
    }
};

class AgentDBusClientTest : public QObject {
    Q_OBJECT

private slots:
    void usesAgentFreeContractConstants()
    {
        QCOMPARE(QString(AgentDBusContract::ServiceName), QString("org.deepin.DeepSwitch"));
        QCOMPARE(QString(AgentDBusContract::ObjectPath), QString("/org/deepin/DeepSwitch"));
        QCOMPARE(QString(AgentDBusContract::InterfaceName), QString("org.deepin.DeepSwitch.Agent"));
    }

    void unavailableAgentReturnsAgentUnavailable()
    {
        FakeTransport transport;
        transport.available = false;
        AgentDBusClient client(transport);

        const AgentCallResult result = client.getStatus();

        QCOMPARE(result.ok, false);
        QCOMPARE(result.errorCode, QString("agent_unavailable"));
        QCOMPARE(transport.lastMethod, QString());
    }

    void listApplicationsParsesSuccessfulEnvelopeItems()
    {
        FakeTransport transport;
        transport.nextMapResult = AgentCallResult::success(QVariantMap {
            { "ok", true },
            { "items", QVariantList { QVariantMap {
                           { "desktop_id", "terminal.desktop" },
                           { "name", "Terminal" },
                       } } },
        });
        AgentDBusClient client(transport);

        const AgentCallResult result = client.listApplications();

        QCOMPARE(result.ok, true);
        QCOMPARE(result.value.toList().size(), 1);
        QCOMPARE(result.value.toList().first().toMap().value("desktop_id").toString(), QString("terminal.desktop"));
        QCOMPARE(transport.lastMethod, QString("ListApplications"));
    }

    void listApplicationsSurfacesEnvelopeError()
    {
        FakeTransport transport;
        transport.nextMapResult = AgentCallResult::success(QVariantMap {
            { "ok", false },
            { "error_code", "backend_unavailable" },
            { "message", "backend is unavailable" },
        });
        AgentDBusClient client(transport);

        const AgentCallResult result = client.listApplications();

        QCOMPARE(result.ok, false);
        QCOMPARE(result.errorCode, QString("backend_unavailable"));
        QCOMPARE(result.message, QString("backend is unavailable"));
    }

    void listWindowsParsesDiagnosticsEnvelopeAndForwardsFilter()
    {
        FakeTransport transport;
        transport.nextMapResult = AgentCallResult::success(QVariantMap {
            { "ok", true },
            { "items", QVariantList { QVariantMap {
                           { "id", QVariant::fromValue<qulonglong>(42) },
                           { "title", "Terminal" },
                           { "match_score", 120 },
                           { "match_evidence", QVariantList {
                                 QVariantMap {
                                     { "rule_type", "wm_class" },
                                     { "value", "deepin-terminal" },
                                     { "actual", "deepin-terminal" },
                                     { "score_delta", 120 },
                                     { "matched", true },
                                 },
                             } },
                       } } },
        });
        AgentDBusClient client(transport);

        const AgentCallResult result = client.listWindows("terminal.desktop");

        QCOMPARE(result.ok, true);
        QCOMPARE(result.value.toList().size(), 1);
        const QVariantMap window = result.value.toList().first().toMap();
        QCOMPARE(window.value("match_score").toInt(), 120);
        QCOMPARE(window.value("match_evidence").toList().first().toMap().value("rule_type").toString(), QString("wm_class"));
        QCOMPARE(transport.lastMethod, QString("ListWindows"));
        QCOMPARE(transport.lastArguments.size(), 1);
        QCOMPARE(transport.lastArguments.first().toString(), QString("terminal.desktop"));
    }

    void operationErrorsSurfaceNotImplemented()
    {
        FakeTransport transport;
        transport.nextMapResult = AgentCallResult::success(QVariantMap {
            { "ok", false },
            { "error_code", "not_implemented" },
            { "message", "SetBinding is not implemented yet." },
        });
        AgentDBusClient client(transport);

        const AgentCallResult setBinding = client.setBinding({ { "id", "terminal" } });

        QCOMPARE(setBinding.ok, false);
        QCOMPARE(setBinding.errorCode, QString("not_implemented"));
        QCOMPARE(setBinding.message, QString("SetBinding is not implemented yet."));

        transport.nextMapResult = AgentCallResult::success(QVariantMap {
            { "ok", false },
            { "error_code", "not_implemented" },
            { "message", "TestHotkey is not implemented yet." },
        });
        const AgentCallResult testHotkey = client.testHotkey("Alt+Return", "terminal");
        QCOMPARE(testHotkey.ok, false);
        QCOMPARE(testHotkey.errorCode, QString("not_implemented"));
        QCOMPARE(testHotkey.message, QString("TestHotkey is not implemented yet."));
    }

    void testHotkeyForwardsHotkeyAndExcludeId()
    {
        FakeTransport transport;
        transport.nextMapResult = AgentCallResult::failure("not_implemented", "TestHotkey is not implemented yet.");
        AgentDBusClient client(transport);

        client.testHotkey("Alt+Return", "terminal");

        QCOMPARE(transport.lastMethod, QString("TestHotkey"));
        QCOMPARE(transport.lastArguments.size(), 2);
        QCOMPARE(transport.lastArguments.at(0).toString(), QString("Alt+Return"));
        QCOMPARE(transport.lastArguments.at(1).toString(), QString("terminal"));
    }
};

QTEST_MAIN(AgentDBusClientTest)
#include "test_agent_dbus_client.moc"
