#pragma once

#include <QQuickView>
#include <QString>

namespace deepswitch {

class OverlayWindow : public QQuickView {
    Q_OBJECT

public:
    explicit OverlayWindow(QString kind, QString message, QWindow* parent = nullptr);

    void showHint();
};

}
