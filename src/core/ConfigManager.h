#pragma once

#include "core/Config.h"
#include "core/Result.h"
#include <QString>

namespace deepswitch {

class ConfigManager {
public:
    explicit ConfigManager(QString path);

    static QString defaultConfigPath();

    Result<Config> load() const;
    VoidResult save(const Config& config) const;
    static VoidResult validate(const Config& config);

private:
    QString m_path;
};

}
