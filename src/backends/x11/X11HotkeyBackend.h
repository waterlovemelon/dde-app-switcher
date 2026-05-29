#pragma once

#include "core/Hotkey.h"
#include "core/Result.h"
#include "backends/x11/X11Connection.h"
#include <QHash>

namespace deepswitch {

enum class SuperKeyEvent {
    NoEvent,
    Pressed,
    Released
};

class X11HotkeyBackend {
public:
    explicit X11HotkeyBackend(X11Connection& connection);

    VoidResult registerHotkey(const Hotkey& hotkey, const QString& actionId);
    void unregisterHotkey(const Hotkey& hotkey);
    void unregisterAll();
    QString pollTriggeredAction();
    VoidResult registerSuperKey();
    SuperKeyEvent pollSuperKey();

    // Unified event poll: handles both Super key and regular hotkeys.
    // Returns triggered hotkey action (empty if none). Sets superEvent.
    QString pollAllEvents(SuperKeyEvent& superEvent);

private:
    unsigned int modifierMask(const QStringList& modifiers) const;
    KeySym keySym(const QString& key) const;
    KeyCode keyCode(const Hotkey& hotkey) const;
    QList<unsigned int> lockVariants(unsigned int base) const;

    X11Connection& m_connection;
    QHash<QString, QString> m_keyToAction;
};

}
