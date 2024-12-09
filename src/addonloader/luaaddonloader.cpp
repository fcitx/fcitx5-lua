/*
 * SPDX-FileCopyrightText: 2020~2020 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#include "luaaddonloader.h"
#include "config.h"
#include "luaaddon.h"
#include <fcitx/addonmanager.h>
#include <stdexcept>

namespace fcitx {

LuaAddonLoader::LuaAddonLoader() {
#ifdef USE_DLOPEN
    luaLibrary_ = std::make_unique<Library>(LUA_LIBRARY_PATH);
    luaLibrary_->load(
        {LibraryLoadHint::NewNameSpace, LibraryLoadHint::DefaultHint});
    if (!luaLibrary_->loaded()) {
        FCITX_LUA_ERROR() << "Failed to load lua library: "
                          << luaLibrary_->error();
    }
    _fcitx_lua_getglobal = reinterpret_cast<decltype(_fcitx_lua_getglobal)>(
        luaLibrary_->resolve("lua_getglobal"));
    _fcitx_lua_touserdata = reinterpret_cast<decltype(_fcitx_lua_touserdata)>(
        luaLibrary_->resolve("lua_touserdata"));
    _fcitx_lua_settop = reinterpret_cast<decltype(_fcitx_lua_settop)>(
        luaLibrary_->resolve("lua_settop"));
    _fcitx_lua_close = reinterpret_cast<decltype(_fcitx_lua_close)>(
        luaLibrary_->resolve("lua_close"));
    _fcitx_luaL_newstate = reinterpret_cast<decltype(_fcitx_luaL_newstate)>(
        luaLibrary_->resolve("luaL_newstate"));
#else
    _fcitx_lua_getglobal = &::lua_getglobal;
    _fcitx_lua_touserdata = &::lua_touserdata;
    _fcitx_lua_settop = &::lua_settop;
    _fcitx_lua_close = &::lua_close;
    _fcitx_luaL_newstate = &::luaL_newstate;
#endif

    if (!_fcitx_lua_getglobal || !_fcitx_lua_touserdata || !_fcitx_lua_settop ||
        !_fcitx_lua_close || !_fcitx_luaL_newstate) {
        throw std::runtime_error("Failed to resolve lua functions.");
    }

    // Create test state to ensure the function can be resolved.
    LuaState testState(luaLibrary_.get());
}

AddonInstance *LuaAddonLoader::load(const AddonInfo &info,
                                    AddonManager *manager) {
#ifdef USE_DLOPEN
    if (!luaLibrary_->loaded()) {
        return nullptr;
    }
#endif
    if (info.category() == AddonCategory::Module) {
        try {
            auto addon =
                std::make_unique<LuaAddon>(luaLibrary_.get(), info, manager);
            return addon.release();
        } catch (const std::exception &e) {
            FCITX_LUA_ERROR() << "Loading lua addon " << info.uniqueName()
                              << " failed: " << e.what();
            return nullptr;
        }
    }
    return nullptr;
}

LuaAddonLoaderAddon::LuaAddonLoaderAddon(AddonManager *manager)
    : manager_(manager) {
    manager->registerLoader(std::make_unique<LuaAddonLoader>());
}

LuaAddonLoaderAddon::~LuaAddonLoaderAddon() {
    manager_->unregisterLoader("Lua");
}

AddonInstance *LuaAddonLoaderFactory::create(AddonManager *manager) {
    return new LuaAddonLoaderAddon(manager);
}

} // namespace fcitx

FCITX_ADDON_FACTORY_V2(luaaddonloader, fcitx::LuaAddonLoaderFactory)
