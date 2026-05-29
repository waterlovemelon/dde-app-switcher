#include "overlay/OverlayWindow.h"

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QGuiApplication>
#include <QLocale>
#include <QTimer>
#include <QTranslator>

using namespace deepswitch;

int main(int argc, char* argv[])
{
    QGuiApplication app(argc, argv);
    QGuiApplication::setApplicationName(QStringLiteral("deepswitch-overlay"));
    QGuiApplication::setApplicationVersion(QStringLiteral("0.1.0"));

    QTranslator translator;
    if (translator.load(QLocale::system(), "deepswitch", "_", ":/i18n")) {
        QGuiApplication::installTranslator(&translator);
    }

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("DeepSwitch action overlay"));
    parser.addHelpOption();
    parser.addOption({ QStringLiteral("kind"), QStringLiteral("Action result kind."), QStringLiteral("kind") });
    parser.addOption({ QStringLiteral("message"), QStringLiteral("Action result message."), QStringLiteral("message") });
    parser.process(app);

    const QString kind = parser.value(QStringLiteral("kind")).isEmpty()
        ? QStringLiteral("focused")
        : parser.value(QStringLiteral("kind"));
    const QString message = parser.value(QStringLiteral("message"));

    OverlayWindow window(kind, message);
    window.showHint();

    QTimer::singleShot(850, &app, &QCoreApplication::quit);
    return app.exec();
}
