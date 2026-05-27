#pragma once

#include "core/AppInfo.h"
#include "core/Config.h"
#include "core/WindowInfo.h"

#include <QList>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>

namespace deepswitch {

struct BindingDto {
    QString id;
    bool enabled = true;
    QString hotkey;
    QString selectionKey;
    QString desktopId;
    QString command;
    QString multiWindowStrategy = "default";
    bool launchIfNotRunning = true;
    bool focusExistingWindow = true;
    QVariantList matchRules;

    static BindingDto fromCore(const Binding& binding);
    Binding toCore() const;

    static BindingDto fromVariantMap(const QVariantMap& map);
    QVariantMap toVariantMap() const;

    static QVariantList toVariantList(const QList<Binding>& bindings);
    static QList<Binding> toCoreList(const QVariantList& bindings);
};

struct AppInfoDto {
    QString desktopId;
    QString name;
    QString localizedName;
    QString exec;
    QString icon;
    QString startupWmClass;
    QStringList categories;
    QString desktopFilePath;
    bool terminal = false;
    bool noDisplay = false;
    bool hidden = false;

    static AppInfoDto fromCore(const AppInfo& app);
    AppInfo toCore() const;

    static AppInfoDto fromVariantMap(const QVariantMap& map);
    QVariantMap toVariantMap() const;

    static QVariantList toVariantList(const QList<AppInfo>& apps);
    static QList<AppInfo> toCoreList(const QVariantList& apps);
};

struct WindowInfoDto {
    WindowId id = 0;
    QString title;
    QString wmClass;
    QString instanceName;
    int pid = 0;
    int desktop = -1;
    bool minimized = false;
    bool active = false;
    int lastActiveOrder = 0;
    QString windowType;
    QString appId;
    bool skipTaskbar = false;

    static WindowInfoDto fromCore(const WindowInfo& window);
    WindowInfo toCore() const;

    static WindowInfoDto fromVariantMap(const QVariantMap& map);
    QVariantMap toVariantMap() const;

    static QVariantList toVariantList(const QList<WindowInfo>& windows);
    static QList<WindowInfo> toCoreList(const QVariantList& windows);
};

struct BackendStatusDto {
    QString name;
    bool available = false;
    bool running = false;
    QString message;

    static BackendStatusDto fromVariantMap(const QVariantMap& map);
    QVariantMap toVariantMap() const;
};

struct AgentStatusDto {
    bool running = false;
    bool enabled = false;
    QString activeBackend;
    QList<BackendStatusDto> backends;

    static AgentStatusDto fromVariantMap(const QVariantMap& map);
    QVariantMap toVariantMap() const;
};

struct ConfigIssueDto {
    QString code;
    QString message;
    QString path;
    QString severity;

    static ConfigIssueDto fromVariantMap(const QVariantMap& map);
    QVariantMap toVariantMap() const;

    static QVariantList toVariantList(const QList<ConfigIssueDto>& issues);
    static QList<ConfigIssueDto> fromVariantList(const QVariantList& issues);
};

}
