#include "overlay/OverlayWindow.h"

#include <QQmlContext>
#include <QScreen>
#include <QUrl>
#include <utility>

namespace deepswitch {

OverlayWindow::OverlayWindow(QString kind, QString message, QWindow* parent)
    : QQuickView(parent)
{
    setFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::BypassWindowManagerHint);
    setColor(Qt::transparent);
    setResizeMode(QQuickView::SizeRootObjectToView);
    setWidth(360);
    setHeight(84);

    rootContext()->setContextProperty(QStringLiteral("overlayKind"), std::move(kind));
    rootContext()->setContextProperty(QStringLiteral("overlayMessage"), std::move(message));
    setSource(QUrl(QStringLiteral("qrc:/overlay/qml/Overlay.qml")));
}

void OverlayWindow::showHint()
{
    if (QScreen* currentScreen = screen()) {
        const QRect geometry = currentScreen->availableGeometry();
        setPosition(
            geometry.x() + (geometry.width() - width()) / 2,
            geometry.y() + 72);
    }
    show();
    raise();
}

}
