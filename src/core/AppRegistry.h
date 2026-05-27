#pragma once

#include "core/AppInfo.h"
#include "core/Result.h"
#include <QList>
#include <QStringList>
#include <optional>

namespace deepswitch {

class AppRegistry {
public:
    void setApplicationDirs(QStringList dirs);
    Result<int> scan();
    QList<AppInfo> listApplications() const;
    std::optional<AppInfo> findByDesktopId(const QString& desktopId) const;

private:
    QStringList m_dirs;
    QList<AppInfo> m_apps;
};

}
