#include "backends/x11/X11HotkeyBackend.h"

#include <X11/keysym.h>

namespace deepswitch {
namespace {

int g_lastGrabError = 0;

int grabErrorHandler(Display*, XErrorEvent* event)
{
    g_lastGrabError = event->error_code;
    return 0;
}

}

X11HotkeyBackend::X11HotkeyBackend(X11Connection& connection)
    : m_connection(connection)
{
}

unsigned int X11HotkeyBackend::modifierMask(const QStringList& modifiers) const
{
    unsigned int mask = 0;
    if (modifiers.contains("Ctrl")) {
        mask |= ControlMask;
    }
    if (modifiers.contains("Alt")) {
        mask |= Mod1Mask;
    }
    if (modifiers.contains("Shift")) {
        mask |= ShiftMask;
    }
    if (modifiers.contains("Meta")) {
        mask |= Mod4Mask;
    }
    return mask;
}

KeySym X11HotkeyBackend::keySym(const QString& key) const
{
    if (key == "Enter") {
        return XK_Return;
    }
    if (key == "Esc") {
        return XK_Escape;
    }
    if (key == "Space") {
        return XK_space;
    }
    if (key.size() == 1) {
        return XStringToKeysym(key.toLower().toLocal8Bit().constData());
    }
    return XStringToKeysym(key.toLocal8Bit().constData());
}

KeyCode X11HotkeyBackend::keyCode(const Hotkey& hotkey) const
{
    const KeySym sym = keySym(hotkey.key);
    if (sym == NoSymbol) {
        return 0;
    }
    return XKeysymToKeycode(m_connection.display(), sym);
}

QList<unsigned int> X11HotkeyBackend::lockVariants(unsigned int base) const
{
    constexpr unsigned int numLock = Mod2Mask;
    constexpr unsigned int capsLock = LockMask;
    constexpr unsigned int scrollLock = Mod5Mask;
    return {
        base,
        base | numLock,
        base | capsLock,
        base | scrollLock,
        base | numLock | capsLock,
        base | numLock | scrollLock,
        base | capsLock | scrollLock,
        base | numLock | capsLock | scrollLock
    };
}

VoidResult X11HotkeyBackend::registerHotkey(const Hotkey& hotkey, const QString& actionId)
{
    const KeyCode keycode = keyCode(hotkey);
    if (keycode == 0) {
        return VoidResult::failure("hotkey_invalid", "Cannot map hotkey key to X11 keycode.");
    }

    const unsigned int baseMask = modifierMask(hotkey.modifiers);
    g_lastGrabError = 0;
    const auto previousHandler = XSetErrorHandler(grabErrorHandler);
    for (const unsigned int mask : lockVariants(baseMask)) {
        XGrabKey(
            m_connection.display(),
            keycode,
            mask,
            m_connection.rootWindow(),
            True,
            GrabModeAsync,
            GrabModeAsync);
    }
    XSync(m_connection.display(), False);
    XSetErrorHandler(previousHandler);

    if (g_lastGrabError != 0) {
        unregisterHotkey(hotkey);
        if (g_lastGrabError == BadAccess) {
            return VoidResult::failure("hotkey_conflict", "Hotkey is already grabbed by another client.");
        }
        return VoidResult::failure("hotkey_backend_unavailable", "X11 failed to grab the hotkey.");
    }

    m_keyToAction.insert(QString("%1:%2").arg(static_cast<int>(keycode)).arg(baseMask), actionId);
    return VoidResult::success();
}

void X11HotkeyBackend::unregisterHotkey(const Hotkey& hotkey)
{
    const KeyCode keycode = keyCode(hotkey);
    if (keycode == 0) {
        return;
    }

    const unsigned int baseMask = modifierMask(hotkey.modifiers);
    for (const unsigned int mask : lockVariants(baseMask)) {
        XUngrabKey(m_connection.display(), keycode, mask, m_connection.rootWindow());
    }
    XFlush(m_connection.display());
    m_keyToAction.remove(QString("%1:%2").arg(static_cast<int>(keycode)).arg(baseMask));
}

void X11HotkeyBackend::unregisterAll()
{
    XUngrabKey(m_connection.display(), AnyKey, AnyModifier, m_connection.rootWindow());
    XFlush(m_connection.display());
    m_keyToAction.clear();
}

QString X11HotkeyBackend::pollTriggeredAction()
{
    while (XPending(m_connection.display()) > 0) {
        XEvent event;
        XNextEvent(m_connection.display(), &event);
        if (event.type != KeyPress) {
            continue;
        }

        const unsigned int cleanState = event.xkey.state & ~(Mod2Mask | LockMask | Mod5Mask);
        const QString key = QString("%1:%2").arg(event.xkey.keycode).arg(cleanState);
        if (m_keyToAction.contains(key)) {
            return m_keyToAction.value(key);
        }
    }
    return {};
}

}
