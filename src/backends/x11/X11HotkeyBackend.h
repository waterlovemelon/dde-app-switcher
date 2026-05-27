#pragma once

#include "core/Hotkey.h"
#include "core/Result.h"
#include "backends/x11/X11Connection.h"
#include <QHash>

namespace deepswitch {

class X11HotkeyBackend {
public:
    explicit X11HotkeyBackend(X11Connection& connection);

    VoidResult registerHotkey(const Hotkey& hotkey, const QString& actionId);
    void unregisterAll();
    QString pollTriggeredAction();

private:
    unsigned int modifierMask(const QStringList& modifiers) const;
    KeySym keySym(const QString& key) const;
    QList<unsigned int> lockVariants(unsigned int base) const;

    X11Connection& m_connection;
    QHash<QString, QString> m_keyToAction;
};

}
