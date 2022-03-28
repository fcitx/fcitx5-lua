add_library(luaaddonloader MODULE luastate.cpp luaaddonstate.cpp luaaddonloader.cpp luaaddon.cpp luahelper.cpp)
target_link_libraries(luaaddonloader Lua::_LuaLibrary Fcitx5::Core Fcitx5::Module::QuickPhrase)
target_include_directories(luaaddonloader PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
install(TARGETS luaaddonloader DESTINATION "${CMAKE_INSTALL_LIBDIR}/fcitx5")

file(READ "base.lua" BASE_LUA_CONTENT)
configure_file(base.lua.h.in ${CMAKE_CURRENT_BINARY_DIR}/base.lua.h @ONLY)

configure_file(luaaddonloader.conf.in.in luaaddonloader.conf.in)
fcitx5_translate_desktop_file("${CMAKE_CURRENT_BINARY_DIR}/luaaddonloader.conf.in" luaaddonloader.conf)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/luaaddonloader.conf" DESTINATION "${FCITX_INSTALL_PKGDATADIR}/addon"
        COMPONENT config)

fcitx5_export_module(LuaAddonLoader TARGET luaaddonloader BUILD_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}" HEADERS luaaddon_public.h INSTALL)
