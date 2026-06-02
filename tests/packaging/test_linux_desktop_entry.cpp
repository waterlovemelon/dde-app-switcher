#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QString>
#include <QTest>

#include "core/DesktopEntry.h"

using namespace oopsjump;

class LinuxDesktopEntryPackagingTest : public QObject {
    Q_OBJECT

private slots:
    void desktopEntryIsVisibleAndLaunchesSettings()
    {
        const QString desktopPath = QStringLiteral("packaging/linux/cn.org.oops.oops-jump.desktop");

        const auto result = DesktopEntry::fromFile(desktopPath);
        QVERIFY2(result.ok, qPrintable(result.message));

        const AppInfo app = result.value;
        QCOMPARE(app.desktopId, QStringLiteral("cn.org.oops.oops-jump.desktop"));
        QCOMPARE(app.name, QStringLiteral("Oops Jump"));
        QVERIFY(app.exec.contains(QStringLiteral("oops-jump-settings")));
        QCOMPARE(app.icon, QStringLiteral("/opt/apps/cn.org.oops.oops-jump/entries/icons/hicolor/256x256/apps/cn.org.oops.oops-jump.png"));
        QVERIFY(app.categories.contains(QStringLiteral("Settings")));
        QVERIFY(!app.noDisplay);

        QFile desktopFile(desktopPath);
        QVERIFY(desktopFile.open(QIODevice::ReadOnly | QIODevice::Text));
        const QString desktopText = QString::fromUtf8(desktopFile.readAll());
        QVERIFY(desktopText.contains(QStringLiteral("StartupNotify=true")));
        QVERIFY(!desktopText.contains(QRegularExpression(QStringLiteral("^NoDisplay=true$"), QRegularExpression::MultilineOption)));
    }

    void hicolorIconSizesExist()
    {
        const QList<int> expectedSizes = {16, 24, 32, 48, 64, 96, 128, 256, 512};

        for (const int size : expectedSizes) {
            const QString iconPath = QStringLiteral("packaging/linux/icons/hicolor/%1x%1/apps/cn.org.oops.oops-jump.png").arg(size);
            const QFileInfo iconInfo(iconPath);
            QVERIFY2(iconInfo.exists(), qPrintable(iconPath + QStringLiteral(" is missing")));
            QVERIFY2(iconInfo.isFile(), qPrintable(iconPath + QStringLiteral(" is not a regular file")));
            QVERIFY2(iconInfo.size() > 0, qPrintable(iconPath + QStringLiteral(" is empty")));
        }
    }
};

QTEST_MAIN(LinuxDesktopEntryPackagingTest)

#include "test_linux_desktop_entry.moc"
