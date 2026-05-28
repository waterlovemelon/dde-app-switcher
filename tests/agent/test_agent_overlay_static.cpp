#include <QFile>
#include <QString>
#include <QTest>

class AgentOverlayStaticTest : public QObject {
    Q_OBJECT

private slots:
    void hotkeyPathUsesDetachedOverlayLaunch()
    {
        QFile mainCpp(QFINDTESTDATA("../../src/agent/main.cpp"));
        QVERIFY(mainCpp.open(QIODevice::ReadOnly | QIODevice::Text));

        const QString source = QString::fromUtf8(mainCpp.readAll());
        QVERIFY(source.contains("QProcess::startDetached"));
        QVERIFY(source.contains("showOverlay"));
        QVERIFY(source.contains("triggered.ok"));
        QVERIFY(source.contains("launchOverlayHint"));
        QVERIFY(source.contains("failed"));
    }
};

QTEST_MAIN(AgentOverlayStaticTest)

#include "test_agent_overlay_static.moc"
