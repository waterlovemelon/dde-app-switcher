#pragma once

#include "core/Result.h"

#include <QString>

namespace deepswitch {

class AutostartManager {
public:
    QString autostartFilePath() const;
    bool isEnabled() const;
    VoidResult setEnabled(bool enabled) const;
    VoidResult enable() const;
    VoidResult disable() const;

private:
    QString configHome() const;
};

}
