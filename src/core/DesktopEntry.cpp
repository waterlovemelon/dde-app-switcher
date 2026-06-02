#include "core/DesktopEntry.h"

#include <QFile>
#include <QFileInfo>
#include <QLocale>
#include <QMap>
#include <QTextStream>

namespace oopsjump {

static bool parseBool(const QString& value)
{
    return value.trimmed().compare("true", Qt::CaseInsensitive) == 0;
}

QString DesktopEntry::cleanExec(const QString& exec)
{
    QString cleaned = exec;
    const QStringList fieldCodes = { "%f", "%F", "%u", "%U", "%i", "%c", "%k" };
    for (const QString& code : fieldCodes) {
        cleaned.replace(code, "");
    }
    return cleaned.simplified();
}

Result<AppInfo> DesktopEntry::fromFile(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return Result<AppInfo>::failure("desktop_file_invalid", "Cannot open desktop file.");
    }

    AppInfo app;
    app.desktopFilePath = QFileInfo(path).absoluteFilePath();
    app.desktopId = QFileInfo(path).fileName();

    bool inDesktopEntry = false;
    bool hasType = false;
    QMap<QString, QString> localizedNames; // e.g. "zh_CN" -> "终端"
    QTextStream stream(&file);
    while (!stream.atEnd()) {
        const QString line = stream.readLine().trimmed();
        if (line.isEmpty() || line.startsWith('#')) {
            continue;
        }
        if (line == "[Desktop Entry]") {
            inDesktopEntry = true;
            continue;
        }
        if (line.startsWith('[') && line.endsWith(']')) {
            inDesktopEntry = false;
            continue;
        }
        if (!inDesktopEntry) {
            continue;
        }

        const int equals = line.indexOf('=');
        if (equals <= 0) {
            continue;
        }
        const QString key = line.left(equals);
        const QString value = line.mid(equals + 1);

        if (key == "Type") {
            hasType = true;
            if (value != "Application") {
                return Result<AppInfo>::failure("desktop_file_invalid", "Desktop entry is not an application.");
            }
        }
        if (key == "Name") {
            app.name = value;
        } else if (key.startsWith("Name[") && key.endsWith(']')) {
            localizedNames.insert(key.mid(5, key.length() - 6), value);
        } else if (key == "Exec") {
            app.exec = cleanExec(value);
        } else if (key == "Icon") {
            app.icon = value;
        } else if (key == "StartupWMClass") {
            app.startupWmClass = value;
        } else if (key == "Categories") {
            app.categories = value.split(';', Qt::SkipEmptyParts);
        } else if (key == "Terminal") {
            app.terminal = parseBool(value);
        } else if (key == "NoDisplay") {
            app.noDisplay = parseBool(value);
        } else if (key == "Hidden") {
            app.hidden = parseBool(value);
        }
    }

    if (!hasType) {
        return Result<AppInfo>::failure("desktop_file_invalid", "Desktop entry is missing Type field.");
    }
    if (app.name.isEmpty() || app.exec.isEmpty()) {
        return Result<AppInfo>::failure("desktop_file_invalid", "Desktop entry is missing Name or Exec.");
    }
    // Pick localized name based on system locale, fallback to English (Name)
    if (!localizedNames.isEmpty()) {
        const QString sysLocale = QLocale::system().name(); // e.g. "zh_CN"
        if (localizedNames.contains(sysLocale)) {
            app.localizedName = localizedNames.value(sysLocale);
        } else {
            // Try language-only match (e.g. "zh" for "zh_CN")
            QString lang = sysLocale.left(sysLocale.indexOf('_'));
            if (lang.isEmpty()) lang = sysLocale;
            auto it = localizedNames.find(lang);
            if (it == localizedNames.end()) {
                for (auto i = localizedNames.begin(); i != localizedNames.end(); ++i) {
                    if (i.key().startsWith(lang)) {
                        it = i;
                        break;
                    }
                }
            }
            if (it != localizedNames.end()) {
                app.localizedName = it.value();
            }
        }
    }
    if (app.localizedName.isEmpty()) {
        app.localizedName = app.name;
    }

    return Result<AppInfo>::success(app);
}

}
