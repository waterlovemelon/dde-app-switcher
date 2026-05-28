#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QString>
#include <QTest>

#include "core/DesktopEntry.h"

using namespace deepswitch;

class LinuxDesktopEntryPackagingTest : public QObject {
    Q_OBJECT

private slots:
    void desktopEntryIsVisibleAndLaunchesSettings()
    {
        const QString desktopPath = QStringLiteral("packaging/linux/org.deepin.DeepSwitch.desktop");

        const auto result = DesktopEntry::fromFile(desktopPath);
        QVERIFY2(result.ok, qPrintable(result.message));

        const AppInfo app = result.value;
        QCOMPARE(app.desktopId, QStringLiteral("org.deepin.DeepSwitch.desktop"));
        QCOMPARE(app.name, QStringLiteral("DeepSwitch"));
        QCOMPARE(app.exec, QStringLiteral("deepswitch-settings"));
        QCOMPARE(app.icon, QStringLiteral("deepswitch"));
        QVERIFY(app.categories.contains(QStringLiteral("Settings")));
        QVERIFY(app.categories.contains(QStringLiteral("Utility")));
        QVERIFY(!app.noDisplay);

        QFile desktopFile(desktopPath);
        QVERIFY(desktopFile.open(QIODevice::ReadOnly | QIODevice::Text));
        const QString desktopText = QString::fromUtf8(desktopFile.readAll());
        QVERIFY(desktopText.contains(QStringLiteral("StartupNotify=true")));
        QVERIFY(!desktopText.contains(QRegularExpression(QStringLiteral("^NoDisplay=true$"), QRegularExpression::MultilineOption)));
    }

    void scalableHicolorIconExists()
    {
        const QFileInfo iconInfo(QStringLiteral("packaging/linux/icons/hicolor/scalable/apps/deepswitch.svg"));
        QVERIFY(iconInfo.exists());
        QVERIFY(iconInfo.isFile());
        QVERIFY(iconInfo.size() > 0);
    }
};

QTEST_MAIN(LinuxDesktopEntryPackagingTest)

#include "test_linux_desktop_entry.moc"
