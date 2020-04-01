add_custom_target(copy DEPENDS luaaddonloader.conf.in-fmt imeapi.conf.in-fmt testlua.conf)
add_custom_command(TARGET copy COMMAND ${CMAKE_COMMAND} -E copy ${PROJECT_BINARY_DIR}/src/addonloader/luaaddonloader.conf ${CMAKE_CURRENT_BINARY_DIR}/luaaddonloader.conf)
add_custom_command(TARGET copy COMMAND ${CMAKE_COMMAND} -E copy ${PROJECT_BINARY_DIR}/src/imeapi/imeapi.conf ${CMAKE_CURRENT_BINARY_DIR}/imeapi.conf)
add_custom_command(TARGET copy COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/testlua.conf ${CMAKE_CURRENT_BINARY_DIR})
