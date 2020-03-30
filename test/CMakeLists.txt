add_subdirectory(addon)

configure_file(testdir.h.in ${CMAKE_CURRENT_BINARY_DIR}/testdir.h @ONLY)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_executable(testlua testlua.cpp)
target_link_libraries(testlua Fcitx5::Core Fcitx5::Module::LuaAddonLoader Fcitx5::Module::TestFrontend
Fcitx5::Module::TestIM Pthread::Pthread)
add_dependencies(testlua luaaddonloader copy luaaddonloader.conf.in-fmt)
add_test(NAME testlua COMMAND testlua)
