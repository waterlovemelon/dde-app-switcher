#pragma once

#include "core/Result.h"

#include <QFile>
#include <QMessageLogContext>
#include <QMutex>
#include <QString>
#include <QTextStream>
#include <QtGlobal>

namespace oopsjump {

class LogFileManager {
public:
    static constexpr qint64 DefaultMaxSizeBytes = 2 * 1024 * 1024;

    explicit LogFileManager(QString baseStateDir = defaultBaseStateDir(),
        qint64 maxSizeBytes = DefaultMaxSizeBytes);
    ~LogFileManager();

    Q_DISABLE_COPY(LogFileManager)

    static QString defaultBaseStateDir();
    static bool shouldUseFileLogging(const QStringList& arguments);

    QString logFilePath() const;
    QString rotatedLogFilePath() const;
    qint64 maxSizeBytes() const;

    VoidResult install();
    void uninstallMessageHandler();
    VoidResult writeLine(QtMsgType type, const QString& message);

private:
    static void messageHandler(QtMsgType type, const QMessageLogContext& context, const QString& message);
    static QString levelName(QtMsgType type);

    QString logDirectoryPath() const;
    VoidResult openForAppend();
    VoidResult rotateIfNeeded(qint64 incomingBytes);

    QString m_baseStateDir;
    qint64 m_maxSizeBytes = DefaultMaxSizeBytes;
    QFile m_file;
    QTextStream m_stream;
    bool m_handlerInstalled = false;
    QtMessageHandler m_previousHandler = nullptr;
    QMutex m_writeMutex;

    static QMutex s_handlerMutex;
    static LogFileManager* s_handlerTarget;
};

}
