cmake_minimum_required(VERSION 3.10)
project(fcitx5-lua VERSION 5.0.11)

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
find_package(Fcitx5Module REQUIRED COMPONENTS QuickPhrase)

option(USE_DLOPEN "Use dlopen to load lua library." On)
option(ENABLE_TEST "Build Test" On)

include("${FCITX_INSTALL_CMAKECONFIG_DIR}/Fcitx5Utils/Fcitx5CompilerSettings.cmake")

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

function(_RESOLVE_LIBRARY_PATH _FILE_NAME _LIBRARY_PATH)
    if (CMAKE_OBJDUMP)
        execute_process (
            COMMAND ${CMAKE_OBJDUMP} -p "${_LIBRARY_PATH}"
            RESULT_VARIABLE STATUS
            OUTPUT_VARIABLE OBJDUMP_RESULT
            ERROR_QUIET
        )
        string(REGEX REPLACE ".*SONAME +([^ ]+)\n.*$" "\\1" SONAME_OUT "${OBJDUMP_RESULT}")
        if (SONAME_OUT)
            set(_FILE_NAME_OUT "${SONAME_OUT}")
        endif ()
    endif()
    if (NOT _FILE_NAME_OUT)
        get_filename_component(_FILE_NAME_OUT "${_LIBRARY_PATH}" NAME)
    endif()
    set(${_FILE_NAME} ${_FILE_NAME_OUT} PARENT_SCOPE)
endfunction()

if(USE_DLOPEN)
    _RESOLVE_LIBRARY_PATH(LUA_LIBRARY_PATH  ${LUA_LIBRARY})
endif()

configure_file(config.h.in config.h)
include_directories(${CMAKE_CURRENT_BINARY_DIR})
add_definitions(-DFCITX_GETTEXT_DOMAIN=\"fcitx5-lua\")
fcitx5_add_i18n_definition()

add_subdirectory(src)
add_subdirectory(po)

if (ENABLE_TEST)
    find_package(Fcitx5ModuleTestFrontend REQUIRED)
    find_package(Fcitx5ModuleTestIM REQUIRED)
    enable_testing()
    add_subdirectory(test)
endif()

fcitx5_translate_desktop_file(org.fcitx.Fcitx5.Addon.Lua.metainfo.xml.in
    org.fcitx.Fcitx5.Addon.Lua.metainfo.xml XML)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/org.fcitx.Fcitx5.Addon.Lua.metainfo.xml" DESTINATION ${CMAKE_INSTALL_DATADIR}/metainfo)
feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
