/*
 * SPDX-FileCopyrightText: 2020~2020 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#include "luahelper.h"

namespace fcitx {

FCITX_DEFINE_LOG_CATEGORY(lua_log, "lua");

LuaAddonState *GetLuaAddonState(lua_State *lua) {
    lua_getglobal(lua, kLuaModuleName);
    LuaAddonState **module =
        reinterpret_cast<LuaAddonState **>(lua_touserdata(lua, -1));
    lua_pop(lua, 1);
    return *module;
}

} // namespace fcitx
