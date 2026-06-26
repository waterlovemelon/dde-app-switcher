#pragma once

#include <QObject>
#include <QList>
#include <QString>
#include <QVariantList>
#include <QQuickWindow>

namespace oopsjump {

class OverlayWindow : public QObject {
    Q_OBJECT

public:
    explicit OverlayWindow(QVariantList apps, QObject* parent = nullptr);
    explicit OverlayWindow(QString kind, QString message, QObject* parent = nullptr);

    void showHint();

private:
    QList<QQuickWindow*> m_windows;
};

}
