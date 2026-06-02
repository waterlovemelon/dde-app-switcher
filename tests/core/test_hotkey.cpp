#include <QtTest/QtTest>
#include "core/Hotkey.h"

using namespace oopsjump;

class HotkeyTest : public QObject {
    Q_OBJECT

private slots:
    void normalizesAliases()
    {
        const auto parsed = Hotkey::parse("control + win + return");
        QVERIFY(parsed.ok);
        QCOMPARE(parsed.value.sequence, QString("Ctrl+Meta+Enter"));
        QCOMPARE(parsed.value.key, QString("Enter"));
        QVERIFY(parsed.value.modifiers.contains("Ctrl"));
        QVERIFY(parsed.value.modifiers.contains("Meta"));
    }

    void rejectsModifierOnlyHotkey()
    {
        const auto parsed = Hotkey::parse("Alt");
        QVERIFY(!parsed.ok);
        QCOMPARE(parsed.errorCode, QString("hotkey_invalid"));
    }

    void detectsDuplicateModifier()
    {
        const auto parsed = Hotkey::parse("Alt+Alt+1");
        QVERIFY(!parsed.ok);
        QCOMPARE(parsed.errorCode, QString("hotkey_invalid"));
    }

    void normalizesCaseAndSpacing()
    {
        const auto parsed = Hotkey::parse(" alt + shift + q ");
        QVERIFY(parsed.ok);
        QCOMPARE(parsed.value.sequence, QString("Alt+Shift+Q"));
    }

    void normalizesCommonNonTextKeys_data()
    {
        QTest::addColumn<QString>("input");
        QTest::addColumn<QString>("expected");

        QTest::addRow("space") << "Ctrl+space" << "Ctrl+Space";
        QTest::addRow("tab") << "Alt+tab" << "Alt+Tab";
        QTest::addRow("backspace") << "Shift+backspace" << "Shift+BackSpace";
        QTest::addRow("delete") << "Meta+delete" << "Meta+Delete";
        QTest::addRow("left") << "Ctrl+left" << "Ctrl+Left";
        QTest::addRow("right") << "Ctrl+right" << "Ctrl+Right";
        QTest::addRow("up") << "Ctrl+up" << "Ctrl+Up";
        QTest::addRow("down") << "Ctrl+down" << "Ctrl+Down";
        QTest::addRow("home") << "Alt+home" << "Alt+Home";
        QTest::addRow("end") << "Alt+end" << "Alt+End";
        QTest::addRow("page-up") << "Meta+pageup" << "Meta+Page_Up";
        QTest::addRow("page-down") << "Meta+pagedown" << "Meta+Page_Down";
        QTest::addRow("insert") << "Ctrl+insert" << "Ctrl+Insert";
    }

    void normalizesCommonNonTextKeys()
    {
        QFETCH(QString, input);
        QFETCH(QString, expected);

        const auto parsed = Hotkey::parse(input);

        QVERIFY(parsed.ok);
        QCOMPARE(parsed.value.sequence, expected);
    }
};

QTEST_MAIN(HotkeyTest)
#include "test_hotkey.moc"
