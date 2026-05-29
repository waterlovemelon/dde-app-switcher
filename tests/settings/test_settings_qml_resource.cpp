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

    void settingsPageQmlIsEmbedded()
    {
        const QResource settingsPageQml(":/qml/SettingsPage.qml");
        QVERIFY(settingsPageQml.isValid());
        QVERIFY(settingsPageQml.size() > 0);
    }

    void aboutPageQmlIsEmbedded()
    {
        const QResource aboutPageQml(":/qml/AboutPage.qml");
        QVERIFY(aboutPageQml.isValid());
        QVERIFY(aboutPageQml.size() > 0);
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

    void mainQmlUsesTabBarLayout()
    {
        QFile mainQml(QFINDTESTDATA("../../src/settings/qml/Main.qml"));
        QVERIFY(mainQml.open(QIODevice::ReadOnly | QIODevice::Text));

        const QString qml = QString::fromUtf8(mainQml.readAll());
        QVERIFY(qml.contains("BindingsPage"));
        QVERIFY(qml.contains("SettingsPage"));
        QVERIFY(qml.contains("AboutPage"));
        QVERIFY(qml.contains("StackLayout"));
    }

    void settingsPageContainsAutostartToggle()
    {
        QFile settingsPageQml(QFINDTESTDATA("../../src/settings/qml/SettingsPage.qml"));
        QVERIFY(settingsPageQml.open(QIODevice::ReadOnly | QIODevice::Text));

        const QString qml = QString::fromUtf8(settingsPageQml.readAll());
        QVERIFY(qml.contains("controller.autostartEnabled"));
        QVERIFY(qml.contains("controller.setAutostartEnabled"));
        QVERIFY(qml.contains("controller.showOverlay"));
        QVERIFY(qml.contains("controller.defaultWindowStrategy"));
        QVERIFY(qml.contains("controller.switchWorkspaceWhenNeeded"));
        QVERIFY(qml.contains("controller.includeAllWorkspaces"));
    }

    void aboutPageContainsDebugInfo()
    {
        QFile aboutPageQml(QFINDTESTDATA("../../src/settings/qml/AboutPage.qml"));
        QVERIFY(aboutPageQml.open(QIODevice::ReadOnly | QIODevice::Text));

        const QString qml = QString::fromUtf8(aboutPageQml.readAll());
        QVERIFY(qml.contains("DeepSwitch"));
        QVERIFY(qml.contains("capabilities"));
        QVERIFY(qml.contains("global_hotkey"));
        QVERIFY(qml.contains("window_list"));
        QVERIFY(qml.contains("activate_window"));
        QVERIFY(qml.contains("launch_app"));
    }
};

QTEST_MAIN(SettingsQmlResourceTest)

#include "test_settings_qml_resource.moc"
