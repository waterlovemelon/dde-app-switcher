#pragma once

namespace oopsjump {

struct AgentDBusContract {
    static constexpr const char* ServiceName = "org.oops.Jump";
    static constexpr const char* InterfaceName = "org.oops.Jump.Agent";
    static constexpr const char* ObjectPath = "/org/oops/Jump";
};

}
