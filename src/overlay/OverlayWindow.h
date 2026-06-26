#pragma once

#include <QObject>
#include <QList>
#include <QString>
#include <QVariantList>
#include <QQuickWindow>

class QQuickView;

namespace oopsjump {

class OverlayWindow : public QObject {
    Q_OBJECT

public:
    explicit OverlayWindow(QVariantList apps, QObject* parent = nullptr);
    explicit OverlayWindow(QString kind, QString message, QObject* parent = nullptr);

    void showHint();
    void hide();

signals:
    void appClicked(int index);

private:
    void connectViewSignals(QQuickView* view);
    QList<QQuickView*> m_windows;
};

}
