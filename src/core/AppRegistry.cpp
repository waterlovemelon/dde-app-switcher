#include "core/AppRegistry.h"
#include "core/DesktopEntry.h"

#include <QDir>
#include <QSet>

namespace oopsjump {

void AppRegistry::setApplicationDirs(QStringList dirs)
{
    m_dirs = std::move(dirs);
}

Result<int> AppRegistry::scan()
{
    m_apps.clear();
    QSet<QString> seenDesktopIds;

    const QStringList dirs = m_dirs.isEmpty()
        ? QStringList{ QDir::homePath() + "/.local/share/applications", "/usr/local/share/applications", "/usr/share/applications" }
        : m_dirs;

    for (const QString& dirPath : dirs) {
        QDir dir(dirPath);
        const QStringList files = dir.entryList({ "*.desktop" }, QDir::Files, QDir::Name);
        for (const QString& fileName : files) {
            if (seenDesktopIds.contains(fileName)) {
                continue;
            }
            const auto parsed = DesktopEntry::fromFile(dir.absoluteFilePath(fileName));
            if (!parsed.ok) {
                continue;
            }
            const AppInfo app = parsed.value;
            if (app.hidden || app.noDisplay) {
                continue;
            }
            seenDesktopIds.insert(app.desktopId);
            m_apps.append(app);
        }
    }

    std::sort(m_apps.begin(), m_apps.end(), [](const AppInfo& left, const AppInfo& right) {
        return left.localizedName.localeAwareCompare(right.localizedName) < 0;
    });

    return Result<int>::success(m_apps.size());
}

QList<AppInfo> AppRegistry::listApplications() const
{
    return m_apps;
}

std::optional<AppInfo> AppRegistry::findByDesktopId(const QString& desktopId) const
{
    for (const AppInfo& app : m_apps) {
        if (app.desktopId == desktopId) {
            return app;
        }
    }
    return std::nullopt;
}

}
