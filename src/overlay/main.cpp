#include "overlay/OverlayWindow.h"

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QGuiApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>

using namespace deepswitch;

namespace {

QVariantList parseAppsJson(const QString& json)
{
    QVariantList result;
    const QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8());
    if (!doc.isArray()) {
        return result;
    }

    const QJsonArray arr = doc.array();
    for (const QJsonValue& val : arr) {
        if (!val.isObject()) {
            continue;
        }
        const QJsonObject obj = val.toObject();
        QVariantMap entry;
        entry[QStringLiteral("icon")] = obj.value(QStringLiteral("icon")).toString();
        entry[QStringLiteral("name")] = obj.value(QStringLiteral("name")).toString();
        entry[QStringLiteral("hotkey")] = obj.value(QStringLiteral("hotkey")).toString();
        entry[QStringLiteral("running")] = obj.value(QStringLiteral("running")).toBool(false);
        entry[QStringLiteral("active")] = obj.value(QStringLiteral("active")).toBool(false);
        result.append(entry);
    }
    return result;
}

}

int main(int argc, char* argv[])
{
    QGuiApplication app(argc, argv);
    QGuiApplication::setApplicationName(QStringLiteral("deepswitch-overlay"));
    QGuiApplication::setApplicationVersion(QStringLiteral("0.2.0"));

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("DeepSwitch overlay"));
    parser.addHelpOption();
    // App bar mode
    parser.addOption({ QStringLiteral("apps"), QStringLiteral("JSON array of app entries for app bar mode."), QStringLiteral("json") });
    // Legacy toast mode
    parser.addOption({ QStringLiteral("kind"), QStringLiteral("Action result kind (legacy toast mode)."), QStringLiteral("kind") });
    parser.addOption({ QStringLiteral("message"), QStringLiteral("Action result message (legacy toast mode)."), QStringLiteral("message") });
    parser.process(app);

    const QString appsJson = parser.value(QStringLiteral("apps"));

    if (!appsJson.isEmpty()) {
        // App bar mode
        const QVariantList apps = parseAppsJson(appsJson);
        if (apps.isEmpty()) {
            return 1;
        }

        OverlayWindow window(apps);
        window.showHint();

        // Stay alive until killed by agent (Super key release)
        return app.exec();
    }

    // Legacy toast mode
    const QString kind = parser.value(QStringLiteral("kind")).isEmpty()
        ? QStringLiteral("focused")
        : parser.value(QStringLiteral("kind"));
    const QString message = parser.value(QStringLiteral("message"));

    OverlayWindow window(kind, message);
    window.showHint();

    QTimer::singleShot(850, &app, &QCoreApplication::quit);
    return app.exec();
}
