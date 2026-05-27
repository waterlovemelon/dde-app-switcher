function(add_deepswitch_test name source)
    add_executable(${name} ${source})
    target_link_libraries(${name} PRIVATE deepswitch_core Qt6::Test Qt6::Core)
    add_test(NAME ${name} COMMAND ${name})
    set_tests_properties(${name} PROPERTIES WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
endfunction()

add_deepswitch_test(test_desktop_entry core/test_desktop_entry.cpp)

add_deepswitch_test(test_app_matcher core/test_app_matcher.cpp)

add_deepswitch_test(test_hotkey core/test_hotkey.cpp)

add_deepswitch_test(test_config core/test_config.cpp)

add_deepswitch_test(test_action_engine core/test_action_engine.cpp)

add_deepswitch_test(test_agent_types ipc/test_agent_types.cpp)
target_link_libraries(test_agent_types PRIVATE deepswitch_ipc)

add_deepswitch_test(test_agent_dbus_service ipc/test_agent_dbus_service.cpp)
target_link_libraries(test_agent_dbus_service PRIVATE deepswitch_agent deepswitch_ipc Qt6::DBus)

add_deepswitch_test(test_agent_controller agent/test_agent_controller.cpp)
target_link_libraries(test_agent_controller PRIVATE deepswitch_agent)
