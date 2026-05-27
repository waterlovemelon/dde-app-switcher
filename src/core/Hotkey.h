#pragma once

#include "core/Result.h"
#include <QString>
#include <QStringList>

namespace deepswitch {

struct Hotkey {
    QString sequence;
    QString key;
    QStringList modifiers;

    static Result<Hotkey> parse(const QString& input);
};

}
