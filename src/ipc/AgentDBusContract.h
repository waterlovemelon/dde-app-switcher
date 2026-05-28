#pragma once

namespace deepswitch {

struct AgentDBusContract {
    static constexpr const char* ServiceName = "org.deepin.DeepSwitch";
    static constexpr const char* InterfaceName = "org.deepin.DeepSwitch.Agent";
    static constexpr const char* ObjectPath = "/org/deepin/DeepSwitch";
};

}
