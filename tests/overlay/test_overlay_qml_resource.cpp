#include <QResource>
#include <QTest>

class OverlayQmlResourceTest : public QObject {
    Q_OBJECT

private slots:
    void overlayQmlIsEmbedded()
    {
        const QResource overlayQml(":/overlay/qml/Overlay.qml");
        QVERIFY(overlayQml.isValid());
        QVERIFY(overlayQml.size() > 0);
    }
};

QTEST_MAIN(OverlayQmlResourceTest)

#include "test_overlay_qml_resource.moc"
