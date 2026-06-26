#include "overlay/OverlayWindow.h"
#include "overlay/IconProvider.h"

#include <QGuiApplication>
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

namespace {

QQuickView* createBaseView()
{
    auto* view = new QQuickView;
    view->setFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::WindowDoesNotAcceptFocus);
    view->setColor(Qt::transparent);
    view->setResizeMode(QQuickView::SizeRootObjectToView);
    return view;
}

void setupAppBarView(QQuickView* view, const QVariantList& apps)
{
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

void setupToastView(QQuickView* view, const QString& kind, const QString& message)
{
    view->rootContext()->setContextProperty(QStringLiteral("overlayMode"), QStringLiteral("toast"));
    view->rootContext()->setContextProperty(QStringLiteral("appEntries"), QVariantList());
    view->rootContext()->setContextProperty(QStringLiteral("overlayKind"), kind);
    view->rootContext()->setContextProperty(QStringLiteral("overlayMessage"), message);
    view->setSource(QUrl(QStringLiteral("qrc:/overlay/qml/Overlay.qml")));

    view->setGeometry(0, 0, 360, 84);
}

} // namespace

OverlayWindow::OverlayWindow(QVariantList apps, QObject* parent)
    : QObject(parent)
{
    const auto screens = QGuiApplication::screens();
    for (QScreen* screen : screens) {
        auto* view = createBaseView();
        setupAppBarView(view, apps);
        m_windows.append(view);
    }
}

OverlayWindow::OverlayWindow(QString kind, QString message, QObject* parent)
    : QObject(parent)
{
    const auto screens = QGuiApplication::screens();
    for (QScreen* screen : screens) {
        auto* view = createBaseView();
        setupToastView(view, kind, message);
        m_windows.append(view);
    }
}

void OverlayWindow::showHint()
{
    const auto screens = QGuiApplication::screens();
    const int count = qMin(m_windows.size(), screens.size());

    for (int i = 0; i < count; ++i) {
        QQuickWindow* window = m_windows[i];
        QScreen* screen = screens[i];

        const QRect geometry = screen->availableGeometry();
        const int x = geometry.x() + (geometry.width() - window->width()) / 2;
        const int y = geometry.y() + (geometry.height() - window->height()) / 2;
        window->setPosition(x, y);
        window->show();
        window->raise();
    }

    // If somehow we have more windows than screens, show extras on primary screen
    for (int i = count; i < m_windows.size(); ++i) {
        m_windows[i]->show();
        m_windows[i]->raise();
    }
}

}
