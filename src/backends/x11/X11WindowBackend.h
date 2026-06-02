#pragma once

#include "core/Result.h"
#include "core/WindowInfo.h"
#include "backends/x11/X11Connection.h"
#include <QList>

namespace oopsjump {

class X11WindowBackend {
public:
    explicit X11WindowBackend(X11Connection& connection);

    Result<QList<WindowInfo>> listWindows() const;
    VoidResult activateWindow(WindowId id) const;

private:
    QString windowStringProperty(Window window, Atom atom) const;
    QStringList windowClass(Window window) const;
    int windowPid(Window window) const;
    bool isSkippable(Window window) const;
    Window activeWindow() const;

    X11Connection& m_connection;
};

}
