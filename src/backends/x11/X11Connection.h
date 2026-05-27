#pragma once

#include "core/Result.h"
#include <QString>
#include <X11/Xlib.h>

namespace deepswitch {

class X11Connection {
public:
    X11Connection();
    ~X11Connection();

    X11Connection(const X11Connection&) = delete;
    X11Connection& operator=(const X11Connection&) = delete;

    VoidResult open();
    Display* display() const;
    Window rootWindow() const;
    Atom atom(const char* name) const;

private:
    Display* m_display = nullptr;
};

}
