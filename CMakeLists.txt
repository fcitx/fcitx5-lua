cmake_minimum_required(VERSION 3.10)
project(fcitx5-lua VERSION 5.0.1)

find_package(ECM REQUIRED 1.0.0)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})

include(FeatureSummary)
include(GNUInstallDirs)
include(ECMSetupVersion)
include(ECMUninstallTarget)

find_package(Lua 5.3 REQUIRED)
find_package(Fcitx5Core REQUIRED)
find_package(Gettext REQUIRED)
find_package(Pthread REQUIRED)
find_package(Fcitx5Module REQUIRED COMPONENTS TestFrontend TestIM QuickPhrase)

option(USE_DLOPEN "Use dlopen to load lua library." On)

include("${FCITX_INSTALL_CMAKECONFIG_DIR}/Fcitx5Utils/Fcitx5CompilerSettings.cmake")
include(Helper)

if (NOT TARGET Lua::_LuaLibrary)
    if (USE_DLOPEN)
        add_library(Lua::_LuaLibrary IMPORTED INTERFACE)
        set_target_properties(Lua::_LuaLibrary PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES ${LUA_INCLUDE_DIR})
    else()
        add_library(Lua::_LuaLibrary UNKNOWN IMPORTED ${LUA_LIBRARY})
        set_target_properties(Lua::_LuaLibrary PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES ${LUA_INCLUDE_DIR}
            IMPORTED_LOCATION ${LUA_LIBRARY})
    endif()
endif()

configure_file(config.h.in config.h)
include_directories(${CMAKE_CURRENT_BINARY_DIR})
add_definitions(-DFCITX_GETTEXT_DOMAIN=\"fcitx5-lua\")
fcitx5_add_i18n_definition()

enable_testing()
add_subdirectory(src)
add_subdirectory(test)
add_subdirectory(po)

fcitx5_translate_desktop_file(org.fcitx.Fcitx5.Addon.Lua.metainfo.xml.in
    org.fcitx.Fcitx5.Addon.Lua.metainfo.xml XML)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/org.fcitx.Fcitx5.Addon.Lua.metainfo.xml" DESTINATION ${CMAKE_INSTALL_DATADIR}/metainfo)
feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
