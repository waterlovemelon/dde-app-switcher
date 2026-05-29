#pragma once

#include <QQuickImageProvider>
#include <QIcon>

namespace deepswitch {

class IconProvider : public QQuickImageProvider {
public:
    IconProvider();

    QPixmap requestPixmap(const QString& id, QSize* size, const QSize& requestedSize) override;
};

}
