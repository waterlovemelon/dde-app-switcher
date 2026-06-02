#include "overlay/IconProvider.h"

#include <QFileInfo>
#include <QPainter>

namespace oopsjump {

IconProvider::IconProvider()
    : QQuickImageProvider(QQuickImageProvider::Pixmap)
{
}

QPixmap IconProvider::requestPixmap(const QString& id, QSize* size, const QSize& requestedSize)
{
    const int width = requestedSize.width() > 0 ? requestedSize.width() : 48;
    const int height = requestedSize.height() > 0 ? requestedSize.height() : 48;

    QIcon icon;

    // If id is an absolute file path, load directly from file.
    if (QFileInfo::exists(id)) {
        icon = QIcon(id);
    }

    // Otherwise (or if file-based icon was null), try theme lookup.
    if (icon.isNull()) {
        icon = QIcon::fromTheme(id);
    }

    // Final fallback.
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
