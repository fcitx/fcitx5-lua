/*
 * SPDX-FileCopyrightText: 2020~2020 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#include "luahelper.h"
#include "luaaddonstate.h"

namespace fcitx {

FCITX_DEFINE_LOG_CATEGORY(lua_log, "lua");

decltype(&::lua_getglobal) _fcitx_lua_getglobal;
decltype(&::lua_touserdata) _fcitx_lua_touserdata;
decltype(&::lua_settop) _fcitx_lua_settop;
decltype(&::lua_close) _fcitx_lua_close;
decltype(&::luaL_newstate) _fcitx_luaL_newstate;

LuaAddonState *GetLuaAddonState(lua_State *lua) {
    _fcitx_lua_getglobal(lua, kLuaModuleName);
    LuaAddonState **module =
        reinterpret_cast<LuaAddonState **>(_fcitx_lua_touserdata(lua, -1));
    _fcitx_lua_settop(lua, -2);
    return *module;
}

} // namespace fcitx
