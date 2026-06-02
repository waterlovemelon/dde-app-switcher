#pragma once

#include "core/AppInfo.h"
#include "core/Result.h"

namespace oopsjump {

class Launcher {
public:
    static VoidResult launch(const AppInfo& app);
};

}
