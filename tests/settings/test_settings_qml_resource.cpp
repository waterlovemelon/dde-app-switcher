#include <QResource>
#include <QTest>

class SettingsQmlResourceTest : public QObject {
    Q_OBJECT

private slots:
    void mainQmlIsEmbedded()
    {
        const QResource mainQml(":/qml/Main.qml");
        QVERIFY(mainQml.isValid());
        QVERIFY(mainQml.size() > 0);
    }
};

QTEST_MAIN(SettingsQmlResourceTest)

#include "test_settings_qml_resource.moc"
