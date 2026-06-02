#include <QtTest/QtTest>

#include "core/LogFileManager.h"

#include <QDir>
#include <QFile>
#include <QThread>
#include <QTemporaryDir>

#include <atomic>

using namespace oopsjump;

namespace {

std::atomic<int> g_handlerCalls { 0 };
std::atomic<int> g_reentrantHandlerCalls { 0 };

void countingMessageHandler(QtMsgType, const QMessageLogContext&, const QString&)
{
    ++g_handlerCalls;
}

void reentrantMessageHandler(QtMsgType, const QMessageLogContext&, const QString&)
{
    ++g_reentrantHandlerCalls;
    qWarning("nested message from previous handler");
}

class ScopedMessageHandler {
public:
    explicit ScopedMessageHandler(QtMessageHandler handler)
        : m_previous(qInstallMessageHandler(handler))
    {
    }

    ~ScopedMessageHandler()
    {
        qInstallMessageHandler(m_previous);
    }

private:
    QtMessageHandler m_previous = nullptr;
};

}

class ScopedStateHome {
public:
    explicit ScopedStateHome(const QString& path)
        : m_hadValue(qEnvironmentVariableIsSet("XDG_STATE_HOME"))
        , m_previous(qEnvironmentVariable("XDG_STATE_HOME"))
    {
        qputenv("XDG_STATE_HOME", path.toUtf8());
    }

    ~ScopedStateHome()
    {
        if (m_hadValue) {
            qputenv("XDG_STATE_HOME", m_previous.toUtf8());
        } else {
            qunsetenv("XDG_STATE_HOME");
        }
    }

private:
    bool m_hadValue = false;
    QString m_previous;
};

class LogFileManagerTest : public QObject {
    Q_OBJECT

private slots:
    void defaultPathUsesXdgStateHome()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        ScopedStateHome stateHome(dir.path());

        LogFileManager manager;

        QCOMPARE(manager.logFilePath(), dir.path() + "/oops-jump/oops-jump.log");
        QCOMPARE(manager.rotatedLogFilePath(), dir.path() + "/oops-jump/oops-jump.log.1");
    }

    void installCreatesStateDirectoryAndLogFile()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        LogFileManager manager(dir.path(), 1024);
        const auto installed = manager.install();
        QVERIFY2(installed.ok, qPrintable(installed.message));

        QVERIFY(QDir(dir.path() + "/oops-jump").exists());
        QVERIFY(QFile::exists(manager.logFilePath()));
    }

    void rotatesExistingLogOverMaxSize()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        const QString logDir = dir.path() + "/oops-jump";
        QVERIFY(QDir().mkpath(logDir));
        QFile existing(logDir + "/oops-jump.log");
        QVERIFY(existing.open(QIODevice::WriteOnly));
        existing.write(QByteArray(20, 'x'));
        existing.close();

        LogFileManager manager(dir.path(), 10);
        const auto installed = manager.install();
        QVERIFY2(installed.ok, qPrintable(installed.message));

        QFile rotated(manager.rotatedLogFilePath());
        QVERIFY(rotated.open(QIODevice::ReadOnly));
        QCOMPARE(rotated.readAll(), QByteArray(20, 'x'));

        QFile current(manager.logFilePath());
        QVERIFY(current.open(QIODevice::ReadOnly));
        QCOMPARE(current.readAll(), QByteArray());
    }

    void rotatesBeforeAppendingPastMaxSize()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        LogFileManager manager(dir.path(), 32);
        QVERIFY(manager.install().ok);
        QVERIFY(manager.writeLine(QtInfoMsg, "first-line").ok);
        QVERIFY(manager.writeLine(QtInfoMsg, QString(40, QLatin1Char('z'))).ok);

        QFile rotated(manager.rotatedLogFilePath());
        QVERIFY(rotated.open(QIODevice::ReadOnly | QIODevice::Text));
        QVERIFY(QString::fromUtf8(rotated.readAll()).contains("first-line"));

        QFile current(manager.logFilePath());
        QVERIFY(current.open(QIODevice::ReadOnly | QIODevice::Text));
        const QString currentText = QString::fromUtf8(current.readAll());
        QVERIFY(currentText.contains(QString(40, QLatin1Char('z'))));
        QVERIFY(!currentText.contains("first-line"));
    }

    void defaultMaxSizeIsTwoMiB()
    {
        LogFileManager manager(QStringLiteral("/tmp/unused-state-dir"));

        QCOMPARE(manager.maxSizeBytes(), 2 * 1024 * 1024);
    }

    void oneShotCommandPolicyDoesNotNeedSensitiveArgumentContents()
    {
        QStringList triggerArgs {
            "oops-jump-agent",
            "--trigger",
            "secret-token-should-not-be-logged",
        };
        QStringList configArgs {
            "oops-jump-agent",
            "--config",
            "/sensitive/path/config.json",
        };
        QStringList inlineConfigArgs {
            "oops-jump-agent",
            "--config=/sensitive/path/config.json",
        };

        QVERIFY(!LogFileManager::shouldUseFileLogging(triggerArgs));
        QVERIFY(LogFileManager::shouldUseFileLogging(configArgs));
        QVERIFY(LogFileManager::shouldUseFileLogging(inlineConfigArgs));
        QVERIFY(LogFileManager::shouldUseFileLogging({ "oops-jump-agent" }));
    }

    void installChainsAndRestoresPreviousMessageHandler()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        g_handlerCalls = 0;
        ScopedMessageHandler scopedHandler(&countingMessageHandler);

        LogFileManager manager(dir.path(), 1024);
        QVERIFY(manager.install().ok);
        qWarning("message while log manager is installed");
        manager.uninstallMessageHandler();
        qWarning("message after log manager is uninstalled");

        QCOMPARE(g_handlerCalls.load(), 2);

        QFile current(manager.logFilePath());
        QVERIFY(current.open(QIODevice::ReadOnly | QIODevice::Text));
        const QString currentText = QString::fromUtf8(current.readAll());
        QVERIFY(currentText.contains("message while log manager is installed"));
        QVERIFY(!currentText.contains("message after log manager is uninstalled"));
    }

    void writeLineSerializesConcurrentWriters()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        LogFileManager manager(dir.path(), 64 * 1024);
        QVERIFY(manager.install().ok);

        QList<QThread*> threads;
        for (int worker = 0; worker < 4; ++worker) {
            threads.append(QThread::create([&manager, worker]() {
                for (int line = 0; line < 25; ++line) {
                    manager.writeLine(QtInfoMsg,
                        QStringLiteral("worker-%1-line-%2").arg(worker).arg(line));
                }
            }));
        }

        for (QThread* thread : threads) {
            thread->start();
        }
        for (QThread* thread : threads) {
            thread->wait();
            delete thread;
        }

        QFile current(manager.logFilePath());
        QVERIFY(current.open(QIODevice::ReadOnly | QIODevice::Text));
        const QString currentText = QString::fromUtf8(current.readAll());
        for (int worker = 0; worker < 4; ++worker) {
            for (int line = 0; line < 25; ++line) {
                QVERIFY(currentText.contains(
                    QStringLiteral("worker-%1-line-%2").arg(worker).arg(line)));
            }
        }
    }

    void reentrantPreviousHandlerDoesNotDeadlock()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        g_reentrantHandlerCalls = 0;
        ScopedMessageHandler scopedHandler(&reentrantMessageHandler);

        LogFileManager manager(dir.path(), 1024);
        QVERIFY(manager.install().ok);
        qWarning("outer message");
        manager.uninstallMessageHandler();

        QCOMPARE(g_reentrantHandlerCalls.load(), 1);

        QFile current(manager.logFilePath());
        QVERIFY(current.open(QIODevice::ReadOnly | QIODevice::Text));
        const QString currentText = QString::fromUtf8(current.readAll());
        QVERIFY(currentText.contains("outer message"));
    }

    void installIsIdempotentAndRejectsSecondActiveManager()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        LogFileManager first(dir.path() + "/first", 1024);
        QVERIFY(first.install().ok);
        QVERIFY(first.install().ok);

        LogFileManager second(dir.path() + "/second", 1024);
        const auto installedSecond = second.install();
        QVERIFY(!installedSecond.ok);
        QCOMPARE(installedSecond.errorCode, QString("log_handler_busy"));

        first.uninstallMessageHandler();
        QVERIFY(second.install().ok);
    }
};

QTEST_MAIN(LogFileManagerTest)
#include "test_log_file_manager.moc"
