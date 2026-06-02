function(add_oopsjump_test name)
    add_executable(${name} ${ARGN})
    target_link_libraries(${name} PRIVATE oopsjump_core Qt6::Test Qt6::Core)
    add_test(NAME ${name} COMMAND ${name})
    set_tests_properties(${name} PROPERTIES WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
endfunction()

add_oopsjump_test(test_desktop_entry core/test_desktop_entry.cpp)

add_oopsjump_test(test_app_matcher core/test_app_matcher.cpp)

add_oopsjump_test(test_hotkey core/test_hotkey.cpp)

add_oopsjump_test(test_config core/test_config.cpp)

add_oopsjump_test(test_log_file_manager core/test_log_file_manager.cpp)

add_oopsjump_test(test_autostart_manager core/test_autostart_manager.cpp)

add_oopsjump_test(test_action_engine core/test_action_engine.cpp)

add_oopsjump_test(test_agent_types ipc/test_agent_types.cpp)
target_link_libraries(test_agent_types PRIVATE oopsjump_ipc)

add_oopsjump_test(test_agent_dbus_service ipc/test_agent_dbus_service.cpp)
target_link_libraries(test_agent_dbus_service PRIVATE oopsjump_agent oopsjump_ipc Qt6::DBus)

add_oopsjump_test(test_agent_dbus_client ipc/test_agent_dbus_client.cpp)
target_link_libraries(test_agent_dbus_client PRIVATE oopsjump_ipc Qt6::DBus)

add_oopsjump_test(test_agent_controller agent/test_agent_controller.cpp)
target_link_libraries(test_agent_controller PRIVATE oopsjump_agent)

add_oopsjump_test(test_agent_overlay_static agent/test_agent_overlay_static.cpp)

add_oopsjump_test(test_settings_controller settings/test_settings_controller.cpp)
target_link_libraries(test_settings_controller PRIVATE oopsjump_settings)

add_oopsjump_test(test_settings_qml_resource
    settings/test_settings_qml_resource.cpp
    ${CMAKE_SOURCE_DIR}/src/settings/qml/qml.qrc
)

add_oopsjump_test(test_overlay_qml_resource
    overlay/test_overlay_qml_resource.cpp
    ${CMAKE_SOURCE_DIR}/src/overlay/qml/qml.qrc
)

add_oopsjump_test(test_linux_desktop_entry packaging/test_linux_desktop_entry.cpp)

add_oopsjump_test(test_debian_packaging packaging/test_debian_packaging.cpp)
