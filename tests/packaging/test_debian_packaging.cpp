#include <QDir>
#include <QFile>
#include <QRegularExpression>
#include <QStringList>
#include <QTest>
#include <QtGlobal>

class DebianPackagingTest : public QObject {
    Q_OBJECT

private slots:
    void debianMetadataFilesExist()
    {
        const QStringList requiredFiles = {
            QStringLiteral("packaging/debian/control"),
            QStringLiteral("packaging/debian/rules"),
            QStringLiteral("packaging/debian/changelog"),
            QStringLiteral("packaging/debian/copyright"),
            QStringLiteral("packaging/debian/cn.org.oops.oops-jump.install"),
        };

        for (const QString& path : requiredFiles) {
            QFileInfo info(path);
            QVERIFY2(info.exists(), qPrintable(path + QStringLiteral(" is missing")));
            QVERIFY2(info.isFile(), qPrintable(path + QStringLiteral(" is not a regular file")));
            QVERIFY2(info.size() > 0, qPrintable(path + QStringLiteral(" is empty")));
        }
    }

    void rootDebianLinkTargetsPackagingMetadata()
    {
        QFileInfo info(QStringLiteral("debian"));
        QVERIFY(info.exists());
        QVERIFY(info.isSymLink());
        QCOMPARE(info.symLinkTarget(), QDir::current().absoluteFilePath(QStringLiteral("packaging/debian")));
    }

    void controlDeclaresRuntimeAndBuildDependencies()
    {
        const QString controlText = readText(QStringLiteral("packaging/debian/control"));

        QVERIFY(controlText.contains(QStringLiteral("Package: cn.org.oops.oops-jump")));
        QVERIFY(controlText.contains(QStringLiteral("qt6-base-dev")));
        QVERIFY(controlText.contains(QStringLiteral("qt6-declarative-dev")));
        QVERIFY(controlText.contains(QStringLiteral("libx11-dev")));

        const QStringList runtimeDependencies = {
            QStringLiteral("${shlibs:Depends}"),
            QStringLiteral("libqt6core6"),
            QStringLiteral("libqt6gui6"),
            QStringLiteral("libqt6qml6"),
            QStringLiteral("libqt6quick6"),
            QStringLiteral("libqt6quickcontrols2-6"),
            QStringLiteral("libqt6dbus6"),
            QStringLiteral("libx11-6"),
        };

        for (const QString& dependency : runtimeDependencies) {
            QVERIFY2(controlText.contains(dependency),
                     qPrintable(QStringLiteral("missing runtime dependency: ") + dependency));
        }

        QVERIFY(controlText.contains(QStringLiteral("user-session")));
        QVERIFY(!controlText.contains(QStringLiteral("systemd")));
    }

    void installManifestPackagesSessionBinariesAndAssets()
    {
        const QString installText = readText(QStringLiteral("packaging/debian/cn.org.oops.oops-jump.install"));

        QVERIFY(installText.contains(QStringLiteral("opt/apps/cn.org.oops.oops-jump/files/")));
        QVERIFY(installText.contains(QStringLiteral("opt/apps/cn.org.oops.oops-jump/entries/")));
        QVERIFY(installText.contains(QStringLiteral("opt/apps/cn.org.oops.oops-jump/info")));
        QVERIFY(!installText.contains(QStringLiteral("systemd")));
    }

    void cmakeInstallKeepsSettingsLauncherBehindQuickGate()
    {
        const QString cmakeText = readText(QStringLiteral("CMakeLists.txt"));

        const qsizetype gateStart = cmakeText.indexOf(QStringLiteral("if (Qt6Qml_FOUND AND Qt6Quick_FOUND AND Qt6QuickControls2_FOUND)"));
        QVERIFY(gateStart >= 0);

        const qsizetype elseStart = cmakeText.indexOf(QStringLiteral("else()"), gateStart);
        QVERIFY(elseStart > gateStart);

        const QString gatedInstallBlock = cmakeText.mid(gateStart, elseStart - gateStart);
        QVERIFY(gatedInstallBlock.contains(QStringLiteral("oops-jump-settings")));
        QVERIFY(gatedInstallBlock.contains(QStringLiteral("cn.org.oops.oops-jump.desktop")));
        QVERIFY(gatedInstallBlock.contains(QStringLiteral("packaging/linux/icons/hicolor/")));
        QVERIFY(gatedInstallBlock.contains(QStringLiteral("*.png")));
    }

private:
    static QString readText(const QString& path)
    {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qWarning("%s", qPrintable(QStringLiteral("failed to open ") + path));
            return {};
        }
        return QString::fromUtf8(file.readAll());
    }
};

QTEST_MAIN(DebianPackagingTest)

#include "test_debian_packaging.moc"
