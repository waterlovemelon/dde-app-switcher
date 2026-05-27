#include <QtTest/QtTest>
#include "core/Hotkey.h"

using namespace deepswitch;

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
};

QTEST_MAIN(HotkeyTest)
#include "test_hotkey.moc"
