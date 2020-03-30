add_custom_target(copy)
add_custom_command(TARGET copy COMMAND ${CMAKE_COMMAND} -E copy ${PROJECT_BINARY_DIR}/src/addonloader/luaaddonloader.conf ${CMAKE_CURRENT_BINARY_DIR}
    DEPENDS ${PROJECT_BINARY_DIR}/src/addonloader/luaaddonloader.conf)
add_custom_command(TARGET copy COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/testlua.conf ${CMAKE_CURRENT_BINARY_DIR}
    DEPENDS testlua.conf)
