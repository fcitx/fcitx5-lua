cmake_minimum_required(VERSION 3.10)
project(fcitx5-lua VERSION 0.0.1)

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

include("${FCITX_INSTALL_CMAKECONFIG_DIR}/Fcitx5Utils/Fcitx5CompilerSettings.cmake")
include(Helper)

if (NOT TARGET Lua::_LuaLibrary)
add_library(Lua::_LuaLibrary IMPORTED INTERFACE)
set_target_properties(Lua::_LuaLibrary PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES ${LUA_INCLUDE_DIR})

endif()

configure_file(config.h.in config.h)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_subdirectory(src)
add_subdirectory(test)
add_subdirectory(po)

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
