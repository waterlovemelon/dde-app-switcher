#include "core/Launcher.h"

#include <QProcess>

namespace oopsjump {

VoidResult Launcher::launch(const AppInfo& app)
{
    if (app.exec.trimmed().isEmpty()) {
        return VoidResult::failure("launch_failed", "Application Exec is empty.");
    }

    const QString program = app.exec.section(' ', 0, 0);
    const QString argsString = app.exec.section(' ', 1);
    const QStringList args = argsString.isEmpty()
        ? QStringList {}
        : QProcess::splitCommand(argsString);

    if (!QProcess::startDetached(program, args)) {
        return VoidResult::failure("launch_failed", "Failed to start process.");
    }
    return VoidResult::success();
}

}
