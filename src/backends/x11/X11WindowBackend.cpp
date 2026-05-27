#include "backends/x11/X11WindowBackend.h"

#include <QByteArray>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <cstring>

namespace deepswitch {

X11WindowBackend::X11WindowBackend(X11Connection& connection)
    : m_connection(connection)
{
}

QString X11WindowBackend::windowStringProperty(Window window, Atom atom) const
{
    Atom actualType = None;
    int actualFormat = 0;
    unsigned long itemCount = 0;
    unsigned long bytesAfter = 0;
    unsigned char* data = nullptr;

    const int status = XGetWindowProperty(
        m_connection.display(),
        window,
        atom,
        0,
        4096,
        False,
        AnyPropertyType,
        &actualType,
        &actualFormat,
        &itemCount,
        &bytesAfter,
        &data);

    if (status != Success || !data) {
        return {};
    }

    QByteArray bytes(reinterpret_cast<const char*>(data), static_cast<int>(itemCount));
    XFree(data);
    return QString::fromUtf8(bytes).trimmed();
}

QStringList X11WindowBackend::windowClass(Window window) const
{
    XClassHint hint;
    if (!XGetClassHint(m_connection.display(), window, &hint)) {
        return {};
    }

    QStringList result;
    if (hint.res_name) {
        result << QString::fromLocal8Bit(hint.res_name);
        XFree(hint.res_name);
    }
    if (hint.res_class) {
        result << QString::fromLocal8Bit(hint.res_class);
        XFree(hint.res_class);
    }
    return result;
}

int X11WindowBackend::windowPid(Window window) const
{
    Atom actualType = None;
    int actualFormat = 0;
    unsigned long itemCount = 0;
    unsigned long bytesAfter = 0;
    unsigned char* data = nullptr;

    const int status = XGetWindowProperty(
        m_connection.display(),
        window,
        m_connection.atom("_NET_WM_PID"),
        0,
        1,
        False,
        XA_CARDINAL,
        &actualType,
        &actualFormat,
        &itemCount,
        &bytesAfter,
        &data);

    if (status != Success || !data || itemCount == 0 || actualFormat != 32) {
        if (data) {
            XFree(data);
        }
        return 0;
    }

    const long rawPid = *reinterpret_cast<long*>(data);
    XFree(data);
    return static_cast<int>(rawPid & 0x7FFFFFFF);
}

bool X11WindowBackend::isSkippable(Window window) const
{
    const QString type = windowStringProperty(window, m_connection.atom("_NET_WM_WINDOW_TYPE"));
    return type.contains("_NET_WM_WINDOW_TYPE_DOCK")
        || type.contains("_NET_WM_WINDOW_TYPE_DESKTOP")
        || type.contains("_NET_WM_WINDOW_TYPE_NOTIFICATION");
}

Result<QList<WindowInfo>> X11WindowBackend::listWindows() const
{
    Atom actualType = None;
    int actualFormat = 0;
    unsigned long itemCount = 0;
    unsigned long bytesAfter = 0;
    unsigned char* data = nullptr;

    const Atom clientList = m_connection.atom("_NET_CLIENT_LIST_STACKING");
    const int status = XGetWindowProperty(
        m_connection.display(),
        m_connection.rootWindow(),
        clientList,
        0,
        4096,
        False,
        XA_WINDOW,
        &actualType,
        &actualFormat,
        &itemCount,
        &bytesAfter,
        &data);

    if (status != Success || !data) {
        return Result<QList<WindowInfo>>::failure("window_backend_unavailable", "Cannot read X11 client list.");
    }

    QList<WindowInfo> windows;
    const Window* rawWindows = reinterpret_cast<const Window*>(data);
    for (unsigned long i = 0; i < itemCount; ++i) {
        const Window window = rawWindows[i];
        if (isSkippable(window)) {
            continue;
        }

        WindowInfo info;
        info.id = static_cast<WindowId>(window);
        info.title = windowStringProperty(window, m_connection.atom("_NET_WM_NAME"));
        const QStringList cls = windowClass(window);
        if (!cls.isEmpty()) {
            info.instanceName = cls.value(0);
            info.wmClass = cls.value(1, cls.value(0));
        }
        info.pid = windowPid(window);
        info.lastActiveOrder = static_cast<int>(i);
        windows.append(info);
    }

    XFree(data);
    return Result<QList<WindowInfo>>::success(windows);
}

VoidResult X11WindowBackend::activateWindow(WindowId id) const
{
    XEvent event;
    memset(&event, 0, sizeof(event));
    event.xclient.type = ClientMessage;
    event.xclient.display = m_connection.display();
    event.xclient.window = static_cast<Window>(id);
    event.xclient.message_type = m_connection.atom("_NET_ACTIVE_WINDOW");
    event.xclient.format = 32;
    event.xclient.data.l[0] = 2;
    event.xclient.data.l[1] = CurrentTime;
    event.xclient.data.l[2] = 0;

    const int status = XSendEvent(
        m_connection.display(),
        m_connection.rootWindow(),
        False,
        SubstructureRedirectMask | SubstructureNotifyMask,
        &event);
    XFlush(m_connection.display());

    if (status == 0) {
        return VoidResult::failure("window_activate_failed", "XSendEvent failed.");
    }
    return VoidResult::success();
}

}
