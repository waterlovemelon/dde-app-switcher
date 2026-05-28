#include "backends/x11/X11WindowBackend.h"

#include <QByteArray>
#include <QHash>
#include <QSet>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <cstring>

namespace deepswitch {
namespace {

QList<WindowId> s_recentActiveWindows;

void updateActiveHistory(WindowId activeWindowId, const QSet<WindowId>& visibleWindowIds)
{
    for (auto it = s_recentActiveWindows.begin(); it != s_recentActiveWindows.end();) {
        if (!visibleWindowIds.contains(*it)) {
            it = s_recentActiveWindows.erase(it);
        } else {
            ++it;
        }
    }

    if (activeWindowId == 0) {
        return;
    }

    s_recentActiveWindows.removeAll(activeWindowId);
    s_recentActiveWindows.prepend(activeWindowId);
    while (s_recentActiveWindows.size() > 64) {
        s_recentActiveWindows.removeLast();
    }
}

void applyActivationOrder(QList<WindowInfo>& windows, WindowId activeWindowId)
{
    QHash<WindowId, int> historyOrder;
    for (int i = 0; i < s_recentActiveWindows.size(); ++i) {
        historyOrder.insert(s_recentActiveWindows.at(i), i);
    }

    const int fallbackBase = historyOrder.size();
    for (int i = 0; i < windows.size(); ++i) {
        WindowInfo& window = windows[i];
        window.active = activeWindowId != 0 && window.id == activeWindowId;
        if (historyOrder.contains(window.id)) {
            window.lastActiveOrder = historyOrder.value(window.id);
        } else {
            window.lastActiveOrder = fallbackBase + (windows.size() - i);
        }
    }
}

}

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

Window X11WindowBackend::activeWindow() const
{
    Atom actualType = None;
    int actualFormat = 0;
    unsigned long itemCount = 0;
    unsigned long bytesAfter = 0;
    unsigned char* data = nullptr;

    const int status = XGetWindowProperty(
        m_connection.display(),
        m_connection.rootWindow(),
        m_connection.atom("_NET_ACTIVE_WINDOW"),
        0,
        1,
        False,
        XA_WINDOW,
        &actualType,
        &actualFormat,
        &itemCount,
        &bytesAfter,
        &data);

    if (status != Success || !data || itemCount == 0 || actualFormat != 32) {
        if (data) {
            XFree(data);
        }
        return None;
    }

    const Window window = *reinterpret_cast<Window*>(data);
    XFree(data);
    return window;
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

    const Window currentActiveWindow = activeWindow();

    QList<WindowInfo> windows;
    QSet<WindowId> visibleWindowIds;
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
        windows.append(info);
        visibleWindowIds.insert(info.id);
    }

    XFree(data);
    const WindowId activeWindowId = currentActiveWindow == None ? 0 : static_cast<WindowId>(currentActiveWindow);
    updateActiveHistory(activeWindowId, visibleWindowIds);
    applyActivationOrder(windows, activeWindowId);
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
