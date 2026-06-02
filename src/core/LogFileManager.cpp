#include "core/LogFileManager.h"

#include <QDateTime>
#include <QDir>
#include <QFileInfo>

namespace oopsjump {

QMutex LogFileManager::s_handlerMutex;
LogFileManager* LogFileManager::s_handlerTarget = nullptr;

namespace {
thread_local bool s_insideLogFileHandler = false;
}

LogFileManager::LogFileManager(QString baseStateDir, qint64 maxSizeBytes)
    : m_baseStateDir(std::move(baseStateDir))
    , m_maxSizeBytes(maxSizeBytes > 0 ? maxSizeBytes : DefaultMaxSizeBytes)
{
}

LogFileManager::~LogFileManager()
{
    uninstallMessageHandler();
}

QString LogFileManager::defaultBaseStateDir()
{
    const QString xdgStateHome = qEnvironmentVariable("XDG_STATE_HOME");
    if (!xdgStateHome.isEmpty()) {
        return xdgStateHome;
    }
    return QDir::homePath() + QStringLiteral("/.local/state");
}

bool LogFileManager::shouldUseFileLogging(const QStringList& arguments)
{
    for (const QString& argument : arguments.mid(1)) {
        if (argument == QStringLiteral("--validate-config")
            || argument == QStringLiteral("--list-bindings")
            || argument == QStringLiteral("--list-apps")
            || argument == QStringLiteral("--list-windows")
            || argument == QStringLiteral("--trigger")
            || argument.startsWith(QStringLiteral("--trigger="))
            || argument == QStringLiteral("--help")
            || argument == QStringLiteral("-h")
            || argument == QStringLiteral("--version")
            || argument == QStringLiteral("-v")) {
            return false;
        }
    }
    return true;
}

QString LogFileManager::logFilePath() const
{
    return logDirectoryPath() + QStringLiteral("/oops-jump.log");
}

QString LogFileManager::rotatedLogFilePath() const
{
    return logFilePath() + QStringLiteral(".1");
}

qint64 LogFileManager::maxSizeBytes() const
{
    return m_maxSizeBytes;
}

VoidResult LogFileManager::install()
{
    {
        QMutexLocker locker(&s_handlerMutex);
        if (m_handlerInstalled) {
            return VoidResult::success();
        }
        if (s_handlerTarget != nullptr) {
            return VoidResult::failure(QStringLiteral("log_handler_busy"),
                QStringLiteral("log file handler is already installed"));
        }
    }

    QDir dir;
    if (!dir.mkpath(logDirectoryPath())) {
        return VoidResult::failure(QStringLiteral("log_dir_create_failed"),
            QStringLiteral("failed to create log directory"));
    }

    const QFileInfo existing(logFilePath());
    if (existing.exists() && existing.size() > m_maxSizeBytes) {
        const auto rotated = rotateIfNeeded(0);
        if (!rotated.ok) {
            return rotated;
        }
    }

    const auto opened = openForAppend();
    if (!opened.ok) {
        return opened;
    }

    QMutexLocker locker(&s_handlerMutex);
    if (s_handlerTarget != nullptr) {
        return VoidResult::failure(QStringLiteral("log_handler_busy"),
            QStringLiteral("log file handler is already installed"));
    }
    s_handlerTarget = this;
    m_previousHandler = qInstallMessageHandler(&LogFileManager::messageHandler);
    m_handlerInstalled = true;
    return VoidResult::success();
}

void LogFileManager::uninstallMessageHandler()
{
    QMutexLocker locker(&s_handlerMutex);
    if (m_handlerInstalled && s_handlerTarget == this) {
        qInstallMessageHandler(m_previousHandler);
        s_handlerTarget = nullptr;
    }
    m_handlerInstalled = false;
    m_previousHandler = nullptr;
}

VoidResult LogFileManager::writeLine(QtMsgType type, const QString& message)
{
    QMutexLocker locker(&m_writeMutex);
    if (!m_file.isOpen()) {
        const auto opened = openForAppend();
        if (!opened.ok) {
            return opened;
        }
    }

    const QString line = QStringLiteral("%1 [%2] %3\n")
        .arg(QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs),
             levelName(type),
             message);
    const auto lineBytes = line.toUtf8();

    const auto rotated = rotateIfNeeded(lineBytes.size());
    if (!rotated.ok) {
        return rotated;
    }

    m_stream << line;
    m_stream.flush();
    if (m_stream.status() != QTextStream::Ok) {
        return VoidResult::failure(QStringLiteral("log_write_failed"),
            QStringLiteral("failed to write log file"));
    }

    return VoidResult::success();
}

void LogFileManager::messageHandler(QtMsgType type, const QMessageLogContext& context, const QString& message)
{
    if (s_insideLogFileHandler) {
        return;
    }

    QtMessageHandler previousHandler = nullptr;
    s_insideLogFileHandler = true;
    {
        QMutexLocker locker(&s_handlerMutex);
        if (s_handlerTarget != nullptr) {
            previousHandler = s_handlerTarget->m_previousHandler;
            s_handlerTarget->writeLine(type, message);
        }
    }

    if (previousHandler != nullptr) {
        previousHandler(type, context, message);
    }
    s_insideLogFileHandler = false;
}

QString LogFileManager::levelName(QtMsgType type)
{
    switch (type) {
    case QtDebugMsg:
        return QStringLiteral("debug");
    case QtInfoMsg:
        return QStringLiteral("info");
    case QtWarningMsg:
        return QStringLiteral("warning");
    case QtCriticalMsg:
        return QStringLiteral("critical");
    case QtFatalMsg:
        return QStringLiteral("fatal");
    }
    return QStringLiteral("info");
}

QString LogFileManager::logDirectoryPath() const
{
    return QDir(m_baseStateDir).filePath(QStringLiteral("oops-jump"));
}

VoidResult LogFileManager::openForAppend()
{
    m_file.setFileName(logFilePath());
    if (!m_file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        return VoidResult::failure(QStringLiteral("log_open_failed"),
            QStringLiteral("failed to open log file"));
    }
    m_stream.setDevice(&m_file);
    return VoidResult::success();
}

VoidResult LogFileManager::rotateIfNeeded(qint64 incomingBytes)
{
    const QFileInfo currentInfo(logFilePath());
    const qint64 currentSize = currentInfo.exists() ? currentInfo.size() : 0;
    const bool mustRotateExisting = incomingBytes == 0 && currentSize > m_maxSizeBytes;
    const bool mustRotateForAppend = incomingBytes > 0
        && currentSize > 0
        && currentSize + incomingBytes > m_maxSizeBytes;

    if (!mustRotateExisting && !mustRotateForAppend) {
        return VoidResult::success();
    }

    const bool reopenAfterRotation = m_file.isOpen();
    if (reopenAfterRotation) {
        m_stream.flush();
        m_stream.setDevice(nullptr);
        m_file.close();
    }

    QFile::remove(rotatedLogFilePath());
    if (QFile::exists(logFilePath()) && !QFile::rename(logFilePath(), rotatedLogFilePath())) {
        return VoidResult::failure(QStringLiteral("log_rotate_failed"),
            QStringLiteral("failed to rotate log file"));
    }

    if (reopenAfterRotation) {
        return openForAppend();
    }
    return VoidResult::success();
}

}
