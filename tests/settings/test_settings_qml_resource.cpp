#include <QFile>
#include <QResource>
#include <QTest>
#include <QString>

class SettingsQmlResourceTest : public QObject {
    Q_OBJECT

private slots:
    void mainQmlIsEmbedded()
    {
        const QResource mainQml(":/qml/Main.qml");
        QVERIFY(mainQml.isValid());
        QVERIFY(mainQml.size() > 0);
    }

    void bindingsPageQmlIsEmbedded()
    {
        const QResource bindingsPageQml(":/qml/BindingsPage.qml");
        QVERIFY(bindingsPageQml.isValid());
        QVERIFY(bindingsPageQml.size() > 0);
    }

    void applicationPickerQmlIsEmbedded()
    {
        const QResource applicationPickerQml(":/qml/ApplicationPicker.qml");
        QVERIFY(applicationPickerQml.isValid());
        QVERIFY(applicationPickerQml.size() > 0);
    }

    void hotkeyRecorderQmlIsEmbedded()
    {
        const QResource hotkeyRecorderQml(":/qml/HotkeyRecorder.qml");
        QVERIFY(hotkeyRecorderQml.isValid());
        QVERIFY(hotkeyRecorderQml.size() > 0);
    }

    void hotkeyRecorderMapsModifiedTextKeysWithoutEventText()
    {
        QFile hotkeyRecorderQml(QFINDTESTDATA("../../src/settings/qml/HotkeyRecorder.qml"));
        QVERIFY(hotkeyRecorderQml.open(QIODevice::ReadOnly | QIODevice::Text));

        const QString qml = QString::fromUtf8(hotkeyRecorderQml.readAll());
        QVERIFY(qml.contains("event.key >= Qt.Key_A"));
        QVERIFY(qml.contains("event.key <= Qt.Key_Z"));
        QVERIFY(qml.contains("event.key >= Qt.Key_0"));
        QVERIFY(qml.contains("event.key <= Qt.Key_9"));
        QVERIFY(qml.contains("Qt.Key_Minus"));
        QVERIFY(qml.contains("event.text && event.text.length === 1"));
    }
};

QTEST_MAIN(SettingsQmlResourceTest)

#include "test_settings_qml_resource.moc"
