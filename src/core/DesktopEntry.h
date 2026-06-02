#pragma once

#include "core/AppInfo.h"
#include "core/Result.h"
#include <QString>

namespace oopsjump {

class DesktopEntry {
public:
    static Result<AppInfo> fromFile(const QString& path);
    static QString cleanExec(const QString& exec);
};

}
