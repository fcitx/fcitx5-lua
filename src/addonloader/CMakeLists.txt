add_library(luaaddonloader MODULE luaaddonstate.cpp luaaddonloader.cpp luaaddon.cpp luahelper.cpp ${CMAKE_CURRENT_BINARY_DIR}/baselua.h)
target_link_libraries(luaaddonloader Lua::_LuaLibrary Fcitx5::Core Fcitx5::Module::QuickPhrase)
target_include_directories(luaaddonloader PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
set_target_properties(luaaddonloader PROPERTIES PREFIX "")
install(TARGETS luaaddonloader DESTINATION "${CMAKE_INSTALL_LIBDIR}/fcitx5")

file2cstring(baselua base.lua)

fcitx5_translate_desktop_file(luaaddonloader.conf.in luaaddonloader.conf)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/luaaddonloader.conf" DESTINATION "${FCITX_INSTALL_PKGDATADIR}/addon")

fcitx5_export_module(LuaAddonLoader TARGET luaaddonloader BUILD_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}" HEADERS luaaddon_public.h INSTALL)
