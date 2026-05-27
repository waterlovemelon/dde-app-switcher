#include "backends/x11/X11Connection.h"

namespace deepswitch {

X11Connection::X11Connection() = default;

X11Connection::~X11Connection()
{
    if (m_display) {
        XCloseDisplay(m_display);
    }
}

VoidResult X11Connection::open()
{
    if (m_display) {
        return VoidResult::success();
    }
    m_display = XOpenDisplay(nullptr);
    if (!m_display) {
        return VoidResult::failure("x11_display_unavailable", "Cannot open X11 display.");
    }
    return VoidResult::success();
}

Display* X11Connection::display() const
{
    return m_display;
}

Window X11Connection::rootWindow() const
{
    if (!m_display) {
        return None;
    }
    return DefaultRootWindow(m_display);
}

Atom X11Connection::atom(const char* name) const
{
    if (!m_display) {
        return None;
    }
    return XInternAtom(m_display, name, False);
}

}
