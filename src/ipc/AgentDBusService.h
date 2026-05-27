#pragma once

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVariantMap>

namespace deepswitch {

class AgentController;

class AgentDBusService : public QObject {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.deepin.DeepSwitch.Agent")

public:
    static constexpr const char* ServiceName = "org.deepin.DeepSwitch";
    static constexpr const char* InterfaceName = "org.deepin.DeepSwitch.Agent";
    static constexpr const char* ObjectPath = "/org/deepin/DeepSwitch";

    explicit AgentDBusService(AgentController& controller, QObject* parent = nullptr);

public slots:
    QVariantMap GetStatus() const;
    QVariantMap ReloadConfig();
    QVariantMap Pause();
    QVariantMap Resume();
    QVariantList ListBindings() const;
    QVariantMap SetBinding(const QVariantMap& binding);
    QVariantMap RemoveBinding(const QString& bindingId);
    QVariantMap TestHotkey(const QString& hotkey);
    QVariantMap ListApplications();
    QVariantMap ListWindows(const QString& filter = QString());
    QVariantMap ActivateWindow(qulonglong windowId);
    QVariantMap LaunchApp(const QString& desktopId);

signals:
    void StatusChanged(const QVariantMap& status);
    void HotkeyTriggered(const QString& actionId, const QVariantMap& result);
    void BindingChanged(const QString& bindingId, const QVariantMap& binding);
    void BackendChanged(const QVariantMap& backend);
    void WindowListChanged(const QVariantList& windows);
    void ErrorOccurred(const QString& errorCode, const QString& message);

private:
    QVariantMap unsupported(const QString& operation) const;
    void emitError(const QVariantMap& result);

    AgentController& m_controller;
};

}
