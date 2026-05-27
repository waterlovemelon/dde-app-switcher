#pragma once

#include "core/AppInfo.h"
#include "core/Result.h"

namespace deepswitch {

class Launcher {
public:
    static VoidResult launch(const AppInfo& app);
};

}
