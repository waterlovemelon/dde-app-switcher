#include <QtTest/QtTest>

#include "core/AutostartManager.h"

#include <QDir>
#include <QFile>
#include <QProcessEnvironment>
#include <QTemporaryDir>

using namespace deepswitch;

class ScopedConfigHome {
public:
    explicit ScopedConfigHome(const QString& path)
        : m_hadValue(qEnvironmentVariableIsSet("XDG_CONFIG_HOME"))
        , m_previous(qEnvironmentVariable("XDG_CONFIG_HOME"))
    {
        qputenv("XDG_CONFIG_HOME", path.toUtf8());
    }

    ~ScopedConfigHome()
    {
        if (m_hadValue) {
            qputenv("XDG_CONFIG_HOME", m_previous.toUtf8());
        } else {
            qunsetenv("XDG_CONFIG_HOME");
        }
    }

private:
    bool m_hadValue = false;
    QString m_previous;
};

class AutostartManagerTest : public QObject {
    Q_OBJECT

private slots:
    void enableWritesExpectedDesktopEntry()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        ScopedConfigHome configHome(dir.path());

        AutostartManager manager;
        const auto enabled = manager.enable();
        QVERIFY2(enabled.ok, qPrintable(enabled.message));

        QCOMPARE(manager.autostartFilePath(), dir.path() + "/autostart/deepswitch-agent.desktop");
        QVERIFY(manager.isEnabled());

        QFile file(manager.autostartFilePath());
        QVERIFY(file.open(QIODevice::ReadOnly | QIODevice::Text));
        QCOMPARE(QString::fromUtf8(file.readAll()),
                 QString("[Desktop Entry]\n"
                         "Type=Application\n"
                         "Name=DeepSwitch Agent\n"
                         "Exec=deepswitch-agent\n"
                         "X-GNOME-Autostart-enabled=true\n"
                         "NoDisplay=true\n"));
    }

    void disableRemovesOnlyGeneratedFile()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        ScopedConfigHome configHome(dir.path());

        AutostartManager manager;
        QVERIFY(manager.enable().ok);

        const QString otherFilePath = dir.path() + "/autostart/other.desktop";
        QFile otherFile(otherFilePath);
        QVERIFY(otherFile.open(QIODevice::WriteOnly | QIODevice::Text));
        otherFile.write("[Desktop Entry]\nName=Other\n");
        otherFile.close();

        const auto disabled = manager.disable();
        QVERIFY2(disabled.ok, qPrintable(disabled.message));

        QVERIFY(!QFile::exists(manager.autostartFilePath()));
        QVERIFY(QFile::exists(otherFilePath));
        QVERIFY(!manager.isEnabled());
    }

    void disablePreservesCustomizedAutostartFile()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        ScopedConfigHome configHome(dir.path());

        AutostartManager manager;
        QVERIFY(QDir().mkpath(dir.path() + "/autostart"));
        QFile file(manager.autostartFilePath());
        QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
        file.write("[Desktop Entry]\n"
                   "Type=Application\n"
                   "Name=Custom DeepSwitch Agent\n"
                   "Exec=deepswitch-agent --custom\n");
        file.close();

        const auto disabled = manager.disable();
        QVERIFY2(disabled.ok, qPrintable(disabled.message));

        QVERIFY(QFile::exists(manager.autostartFilePath()));
        QVERIFY(manager.isEnabled());
    }

    void enablePreservesCustomizedAutostartFile()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        ScopedConfigHome configHome(dir.path());

        AutostartManager manager;
        QVERIFY(QDir().mkpath(dir.path() + "/autostart"));
        QFile file(manager.autostartFilePath());
        QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
        file.write("[Desktop Entry]\n"
                   "Type=Application\n"
                   "Name=Custom DeepSwitch Agent\n"
                   "Exec=deepswitch-agent --custom\n");
        file.close();

        const auto enabled = manager.enable();
        QVERIFY(!enabled.ok);
        QCOMPARE(enabled.errorCode, QString("autostart_conflict"));

        QFile preserved(manager.autostartFilePath());
        QVERIFY(preserved.open(QIODevice::ReadOnly | QIODevice::Text));
        QVERIFY(QString::fromUtf8(preserved.readAll()).contains("--custom"));
    }

    void enableTreatsExistingGeneratedFileAsSuccess()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        ScopedConfigHome configHome(dir.path());

        AutostartManager manager;
        QVERIFY(manager.enable().ok);
        QVERIFY(manager.enable().ok);
        QVERIFY(manager.isEnabled());
    }

    void disableTreatsMissingFileAsSuccess()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        ScopedConfigHome configHome(dir.path());

        AutostartManager manager;
        const auto disabled = manager.disable();

        QVERIFY2(disabled.ok, qPrintable(disabled.message));
        QVERIFY(!manager.isEnabled());
    }

    void xdgConfigHomeKeepsWritesIsolated()
    {
        QTemporaryDir firstDir;
        QTemporaryDir secondDir;
        QVERIFY(firstDir.isValid());
        QVERIFY(secondDir.isValid());

        {
            ScopedConfigHome configHome(firstDir.path());
            AutostartManager manager;
            QVERIFY(manager.setEnabled(true).ok);
            QCOMPARE(manager.autostartFilePath(), firstDir.path() + "/autostart/deepswitch-agent.desktop");
        }

        {
            ScopedConfigHome configHome(secondDir.path());
            AutostartManager manager;
            QVERIFY(!manager.isEnabled());
            QCOMPARE(manager.autostartFilePath(), secondDir.path() + "/autostart/deepswitch-agent.desktop");
            QVERIFY(!QFile::exists(manager.autostartFilePath()));
        }

        QVERIFY(QFile::exists(firstDir.path() + "/autostart/deepswitch-agent.desktop"));
    }
};

QTEST_MAIN(AutostartManagerTest)
#include "test_autostart_manager.moc"
