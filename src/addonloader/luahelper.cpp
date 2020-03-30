//
// Copyright (C) 2020~2020 by CSSlayer
// wengxt@gmail.com
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2, or (at your option)
// any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
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
