cmake_minimum_required(VERSION 3.20)

project(deepswitch VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(GNUInstallDirs)

# UOS packaging paths (compliant with UOS App Store Packaging Specification)
set(UOS_APP_ID "cn.org.oops.deepswitch")
set(UOS_APP_DIR "/opt/apps/${UOS_APP_ID}")

find_package(Qt6 REQUIRED COMPONENTS Core Test Gui DBus)
find_package(Qt6 COMPONENTS Qml Quick QuickControls2 QUIET)
find_package(X11 REQUIRED)

enable_testing()

# --- Translation build (ts → qm) ---
find_package(Qt6 COMPONENTS LinguistTools QUIET)

set(TRANSLATION_TS_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/translations/deepswitch_zh_CN.ts
)
set(TRANSLATION_QM_DIR ${CMAKE_CURRENT_BINARY_DIR}/translations)
file(MAKE_DIRECTORY ${TRANSLATION_QM_DIR})

if(Qt6LinguistTools_FOUND)
    qt_add_translations(deepswitch-core
        TS_FILES ${TRANSLATION_TS_FILES}
        QM_FILES_OUTPUT_VARIABLE TRANSLATION_QM_FILES
    )
else()
    # Fallback: use lrelease (from qt6-l10n-tools or linglong) to compile .ts → .qm
    set(TRANSLATION_QM_FILES "")
    foreach(ts_file ${TRANSLATION_TS_FILES})
        get_filename_component(ts_name ${ts_file} NAME_WE)
        set(qm_file ${TRANSLATION_QM_DIR}/${ts_name}.qm)
        add_custom_command(
            OUTPUT ${qm_file}
            COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/translations/compile_translations.sh
                ${ts_file} ${qm_file}
            DEPENDS ${ts_file} ${CMAKE_CURRENT_SOURCE_DIR}/translations/compile_translations.sh
            COMMENT "Compiling translation: ${ts_name}.ts → ${ts_name}.qm"
        )
        list(APPEND TRANSLATION_QM_FILES ${qm_file})
    endforeach()

    # Generate translations resource file
    set(TRANSLATION_QRC ${TRANSLATION_QM_DIR}/translations.qrc)
    file(WRITE ${TRANSLATION_QRC} "<RCC><qresource prefix=\"/i18n\">\n")
    foreach(qm_file ${TRANSLATION_QM_FILES})
        get_filename_component(qm_name ${qm_file} NAME)
        file(APPEND ${TRANSLATION_QRC} "    <file>${qm_name}</file>\n")
    endforeach()
    file(APPEND ${TRANSLATION_QRC} "</qresource></RCC>\n")

    set(TRANSLATION_QRC ${TRANSLATION_QM_DIR}/translations.qrc)
endif()

add_library(deepswitch_core STATIC
    src/core/ActionEngine.cpp
    src/core/AppMatcher.cpp
    src/core/AppRegistry.cpp
    src/core/AutostartManager.cpp
    src/core/Config.cpp
    src/core/ConfigManager.cpp
    src/core/DesktopEntry.cpp
    src/core/Hotkey.cpp
    src/core/Launcher.cpp
    src/core/LogFileManager.cpp
)

target_include_directories(deepswitch_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(deepswitch_core PUBLIC
    Qt6::Core
)

add_library(deepswitch_x11 STATIC
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
    Xi
)

add_library(deepswitch_ipc STATIC
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

add_library(deepswitch_settings STATIC
    src/settings/SettingsController.cpp
)

target_include_directories(deepswitch_settings PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(deepswitch_settings PUBLIC
    deepswitch_ipc
    Qt6::Core
)

add_library(deepswitch_agent STATIC
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

install(TARGETS deepswitch-agent
    RUNTIME DESTINATION ${UOS_APP_DIR}/files/bin
)

install(FILES
    deepin-manico-like-design.md
    docs/m0-x11-findings.md
    DESTINATION ${UOS_APP_DIR}/files/doc
)

install(DIRECTORY docs/superpowers/plans
    DESTINATION ${UOS_APP_DIR}/files/doc/superpowers
)

if (Qt6Qml_FOUND AND Qt6Quick_FOUND AND Qt6QuickControls2_FOUND)
    add_executable(deepswitch-overlay
        src/overlay/main.cpp
        src/overlay/IconProvider.cpp
        src/overlay/IconProvider.h
        src/overlay/OverlayWindow.cpp
        src/overlay/OverlayWindow.h
        src/overlay/qml/qml.qrc
        ${TRANSLATION_QM_FILES}
        ${TRANSLATION_QRC}
    )

    target_include_directories(deepswitch-overlay PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
    )

    target_link_libraries(deepswitch-overlay PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Qml
        Qt6::Quick
    )

    install(TARGETS deepswitch-overlay
        RUNTIME DESTINATION ${UOS_APP_DIR}/files/bin
    )

    add_executable(deepswitch-settings
        src/settings/main.cpp
        src/overlay/IconProvider.cpp
        src/overlay/IconProvider.h
        src/settings/qml/qml.qrc
        ${TRANSLATION_QM_FILES}
        ${TRANSLATION_QRC}
    )

    target_link_libraries(deepswitch-settings PRIVATE
        deepswitch_settings
        Qt6::Core
        Qt6::Gui
        Qt6::Qml
        Qt6::Quick
        Qt6::QuickControls2
        Qt6::DBus
    )

    install(TARGETS deepswitch-settings
        RUNTIME DESTINATION ${UOS_APP_DIR}/files/bin
    )

    install(FILES packaging/linux/cn.org.oops.deepswitch.desktop
        DESTINATION ${UOS_APP_DIR}/entries/applications
    )

    install(FILES packaging/linux/icons/hicolor/scalable/apps/cn.org.oops.deepswitch.svg
        DESTINATION ${UOS_APP_DIR}/entries/icons/hicolor/scalable/apps
    )

    install(FILES packaging/linux/info
        DESTINATION ${UOS_APP_DIR}
    )
else()
    message(STATUS "Qt6 QML/Quick development packages not found; skipping deepswitch-overlay, deepswitch-settings GUI target and launcher entry install")
endif()

add_subdirectory(tests)
