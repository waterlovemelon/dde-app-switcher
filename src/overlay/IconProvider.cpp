#include "overlay/IconProvider.h"

#include <QPainter>

namespace deepswitch {

IconProvider::IconProvider()
    : QQuickImageProvider(QQuickImageProvider::Pixmap)
{
}

QPixmap IconProvider::requestPixmap(const QString& id, QSize* size, const QSize& requestedSize)
{
    const int width = requestedSize.width() > 0 ? requestedSize.width() : 48;
    const int height = requestedSize.height() > 0 ? requestedSize.height() : 48;

    QIcon icon = QIcon::fromTheme(id);
    if (icon.isNull()) {
        icon = QIcon::fromTheme(QStringLiteral("application-x-executable"));
    }

    QPixmap pixmap = icon.pixmap(QSize(width, height));

    if (size) {
        *size = pixmap.size();
    }

    return pixmap;
}

}
