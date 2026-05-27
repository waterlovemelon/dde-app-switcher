#pragma once

#include <QString>

namespace deepswitch {

using WindowId = quint64;

struct WindowInfo {
    WindowId id = 0;
    QString title;
    QString wmClass;
    QString instanceName;
    int pid = 0;
    int desktop = -1;
    bool minimized = false;
    bool active = false;
    int lastActiveOrder = 0;
    QString windowType;
    QString appId;
    bool skipTaskbar = false;
};

}
