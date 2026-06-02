#include "overlay/OverlayWindow.h"
#include "overlay/IconProvider.h"

#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickView>
#include <QScreen>
#include <QUrl>
#include <utility>

namespace oopsjump {

static constexpr int kSlotWidth = 108;
static constexpr int kPaddingH = 40;
static constexpr int kBarHeight = 120;

OverlayWindow::OverlayWindow(QVariantList apps, QObject* parent)
    : QObject(parent)
{
    auto* view = new QQuickView;
    m_window = view;

    view->setFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::WindowDoesNotAcceptFocus);
    view->setColor(Qt::transparent);
    view->setResizeMode(QQuickView::SizeRootObjectToView);

    const int count = apps.size();
    const int barWidth = kPaddingH + count * kSlotWidth;
    const int w = barWidth > kPaddingH ? barWidth : 200;

    view->engine()->addImageProvider(QStringLiteral("theme"), new IconProvider);
    view->rootContext()->setContextProperty(QStringLiteral("overlayMode"), QStringLiteral("appbar"));
    view->rootContext()->setContextProperty(QStringLiteral("appEntries"), apps);
    view->rootContext()->setContextProperty(QStringLiteral("overlayKind"), QString());
    view->rootContext()->setContextProperty(QStringLiteral("overlayMessage"), QString());
    view->setSource(QUrl(QStringLiteral("qrc:/overlay/qml/Overlay.qml")));

    view->setGeometry(0, 0, w, kBarHeight);
}

OverlayWindow::OverlayWindow(QString kind, QString message, QObject* parent)
    : QObject(parent)
{
    auto* view = new QQuickView;
    m_window = view;

    view->setFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::WindowDoesNotAcceptFocus);
    view->setColor(Qt::transparent);
    view->setResizeMode(QQuickView::SizeRootObjectToView);

    view->rootContext()->setContextProperty(QStringLiteral("overlayMode"), QStringLiteral("toast"));
    view->rootContext()->setContextProperty(QStringLiteral("appEntries"), QVariantList());
    view->rootContext()->setContextProperty(QStringLiteral("overlayKind"), std::move(kind));
    view->rootContext()->setContextProperty(QStringLiteral("overlayMessage"), std::move(message));
    view->setSource(QUrl(QStringLiteral("qrc:/overlay/qml/Overlay.qml")));

    view->setGeometry(0, 0, 360, 84);
}

void OverlayWindow::showHint()
{
    if (!m_window) return;

    if (QScreen* currentScreen = m_window->screen()) {
        const QRect geometry = currentScreen->availableGeometry();
        const int x = geometry.x() + (geometry.width() - m_window->width()) / 2;
        const int y = geometry.y() + (geometry.height() - m_window->height()) / 2;
        m_window->setPosition(x, y);
    }
    m_window->show();
    m_window->raise();
}

}
