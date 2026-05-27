#pragma once

#include <QString>
#include <QStringList>

namespace deepswitch {

struct AppInfo {
    QString desktopId;
    QString name;
    QString localizedName;
    QString exec;
    QString icon;
    QString startupWmClass;
    QStringList categories;
    QString desktopFilePath;
    bool terminal = false;
    bool noDisplay = false;
    bool hidden = false;
};

}
