cmake_minimum_required(VERSION 3.20)

project(deepswitch VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Qt6 REQUIRED COMPONENTS Core Test Gui DBus)
find_package(X11 REQUIRED)

enable_testing()

add_library(deepswitch_core
    src/core/ActionEngine.cpp
    src/core/AppMatcher.cpp
    src/core/AppRegistry.cpp
    src/core/Config.cpp
    src/core/ConfigManager.cpp
    src/core/DesktopEntry.cpp
    src/core/Hotkey.cpp
    src/core/Launcher.cpp
)

target_include_directories(deepswitch_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(deepswitch_core PUBLIC
    Qt6::Core
)

add_library(deepswitch_x11
    src/backends/x11/X11Connection.cpp
    src/backends/x11/X11HotkeyBackend.cpp
    src/backends/x11/X11WindowBackend.cpp
)

target_include_directories(deepswitch_x11 PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(deepswitch_x11 PUBLIC
    deepswitch_core
    Qt6::Core
    Qt6::Gui
    X11::X11
)

add_library(deepswitch_ipc
    src/ipc/AgentDBusClient.cpp
    src/ipc/AgentTypes.cpp
)

target_include_directories(deepswitch_ipc PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(deepswitch_ipc PUBLIC
    deepswitch_core
    Qt6::Core
    Qt6::DBus
)

add_library(deepswitch_settings
    src/settings/SettingsController.cpp
)

target_include_directories(deepswitch_settings PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(deepswitch_settings PUBLIC
    deepswitch_ipc
    Qt6::Core
)

add_library(deepswitch_agent
    src/agent/AgentController.cpp
    src/ipc/AgentDBusService.cpp
)

target_include_directories(deepswitch_agent PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(deepswitch_agent PUBLIC
    deepswitch_core
    deepswitch_ipc
    deepswitch_x11
    Qt6::Core
)

add_executable(deepswitch-agent
    src/agent/main.cpp
)

target_link_libraries(deepswitch-agent PRIVATE
    deepswitch_agent
    deepswitch_core
    deepswitch_ipc
    deepswitch_x11
    Qt6::Core
    Qt6::DBus
)

add_subdirectory(tests)
