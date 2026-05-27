#include <QtTest/QtTest>
#include "core/DesktopEntry.h"
#include "core/AppRegistry.h"

using namespace deepswitch;

class DesktopEntryTest : public QObject {
    Q_OBJECT

private slots:
    void parsesLocalizedNameAndStartupClass()
    {
        const auto result = DesktopEntry::fromFile("tests/fixtures/applications/firefox.desktop");
        QVERIFY(result.ok);
        QCOMPARE(result.value.name, QString("Firefox"));
        QCOMPARE(result.value.localizedName, QString("火狐浏览器"));
        QCOMPARE(result.value.startupWmClass, QString("firefox"));
        QCOMPARE(result.value.exec, QString("firefox"));
    }

    void removesDesktopFieldCodes()
    {
        QCOMPARE(DesktopEntry::cleanExec("/usr/bin/code --unity-launch %F"), QString("/usr/bin/code --unity-launch"));
        QCOMPARE(DesktopEntry::cleanExec("firefox %u"), QString("firefox"));
    }

    void registryFiltersHiddenApplications()
    {
        AppRegistry registry;
        registry.setApplicationDirs({ "tests/fixtures/applications" });
        const auto scan = registry.scan();
        QVERIFY(scan.ok);
        const QList<AppInfo> apps = registry.listApplications();
        QCOMPARE(apps.size(), 2);
        QVERIFY(registry.findByDesktopId("firefox.desktop").has_value());
        QVERIFY(!registry.findByDesktopId("hidden.desktop").has_value());
    }
};

QTEST_MAIN(DesktopEntryTest)
#include "test_desktop_entry.moc"
