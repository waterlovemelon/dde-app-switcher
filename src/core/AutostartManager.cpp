#include "core/AutostartManager.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QStandardPaths>

namespace oopsjump {

namespace {

constexpr auto kAutostartDesktopEntry =
    "[Desktop Entry]\n"
    "Type=Application\n"
    "Name=Oops Jump Agent\n"
    "Exec=/opt/apps/cn.org.oops.oops-jump/files/bin/oops-jump-agent\n"
    "X-GNOME-Autostart-enabled=true\n"
    "NoDisplay=true\n";

}

QString AutostartManager::configHome() const
{
    const QString xdgConfigHome = qEnvironmentVariable("XDG_CONFIG_HOME").trimmed();
    if (!xdgConfigHome.isEmpty()) {
        return xdgConfigHome;
    }
    return QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
}

QString AutostartManager::autostartFilePath() const
{
    return QDir(configHome()).filePath("autostart/oops-jump-agent.desktop");
}

bool AutostartManager::isEnabled() const
{
    return QFileInfo::exists(autostartFilePath());
}

VoidResult AutostartManager::setEnabled(bool enabled) const
{
    return enabled ? enable() : disable();
}

VoidResult AutostartManager::enable() const
{
    const QString path = autostartFilePath();
    const QFileInfo fileInfo(path);
    if (!QDir().mkpath(fileInfo.absolutePath())) {
        return VoidResult::failure("autostart_write_failed", "Cannot create user autostart directory.");
    }

    if (fileInfo.exists()) {
        QFile existing(path);
        if (!existing.open(QIODevice::ReadOnly | QIODevice::Text)) {
            return VoidResult::failure("autostart_write_failed", "Cannot inspect existing user autostart file.");
        }
        const QByteArray content = existing.readAll();
        if (content != QByteArray(kAutostartDesktopEntry)) {
            return VoidResult::failure("autostart_conflict", "User autostart file already exists with custom content.");
        }
        return VoidResult::success();
    }

    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return VoidResult::failure("autostart_write_failed", "Cannot open user autostart file for writing.");
    }
    file.write(kAutostartDesktopEntry);
    if (!file.commit()) {
        return VoidResult::failure("autostart_write_failed", "Cannot commit user autostart file.");
    }

    // Remove old "deepswitch-agent" autostart entry if present.
    const QString oldPath = QDir(configHome()).filePath("autostart/deepswitch-agent.desktop");
    if (QFileInfo::exists(oldPath)) {
        QFile::remove(oldPath);
    }

    return VoidResult::success();
}

VoidResult AutostartManager::disable() const
{
    const QString path = autostartFilePath();
    if (!QFileInfo::exists(path)) {
        return VoidResult::success();
    }

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return VoidResult::failure("autostart_remove_failed", "Cannot inspect user autostart file.");
    }
    const QByteArray content = file.readAll();
    file.close();
    if (content != QByteArray(kAutostartDesktopEntry)) {
        return VoidResult::success();
    }

    if (!file.remove()) {
        return VoidResult::failure("autostart_remove_failed", "Cannot remove user autostart file.");
    }
    return VoidResult::success();
}

}
