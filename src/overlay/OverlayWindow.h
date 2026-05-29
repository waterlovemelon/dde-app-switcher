#pragma once

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QQuickWindow>

namespace deepswitch {

class OverlayWindow : public QObject {
    Q_OBJECT

public:
    explicit OverlayWindow(QVariantList apps, QObject* parent = nullptr);
    explicit OverlayWindow(QString kind, QString message, QObject* parent = nullptr);

    void showHint();

private:
    QQuickWindow* m_window = nullptr;
};

}
