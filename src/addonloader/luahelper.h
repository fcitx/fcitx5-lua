/*
 * SPDX-FileCopyrightText: 2020~2020 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX5_LUA_ADDONLOADER_LUAHELPER_H_
#define _FCITX5_LUA_ADDONLOADER_LUAHELPER_H_

#include <cstdint>
#include <fcitx-utils/log.h>
#include <fcitx-utils/macros.h>
#include <lua.hpp>
#include <tuple>
#include <utility>

namespace fcitx {

class LuaAddonState;

template <typename Arg>
struct LuaArgTypeTraits;

template <>
struct LuaArgTypeTraits<int> {
    static int check(lua_State *lua, int arg) {
        return luaL_checkinteger(lua, arg);
    }
    static void ret(lua_State *lua, int v) { lua_pushinteger(lua, v); }
};
template <>
struct LuaArgTypeTraits<const char *> {
    static const char *check(lua_State *lua, int arg) {
        return luaL_checkstring(lua, arg);
    }
    static void ret(lua_State *lua, const char *s) { lua_pushstring(lua, s); }
};
template <>
struct LuaArgTypeTraits<std::string> {
    static void ret(lua_State *lua, const std::string &s) {
        lua_pushstring(lua, s.data());
    }
};
template <>
struct LuaArgTypeTraits<std::vector<std::string>> {
    static void ret(lua_State *lua, const std::vector<std::string> &s) {
        lua_createtable(lua, s.size(), 0);
        for (size_t i = 0; i < s.size(); i++) {
            lua_pushstring(lua, s[i].data());
            lua_rawseti(lua, -2, i + 1); /* In lua indices start at 1 */
        }
    }
};

template <typename TraitsTuple, std::size_t... I>
auto LuaCheckArgumentImpl(lua_State *lua, std::index_sequence<I...>) {
    FCITX_UNUSED(lua);
    return std::make_tuple(
        std::tuple_element_t<I, TraitsTuple>::check(lua, I + 1)...);
}

template <typename Ret, typename... Args, typename T>
std::tuple<Args...> LuaCheckArgument(lua_State *lua, Ret (T::*)(Args...)) {
    if (auto argnum = lua_gettop(lua); argnum != sizeof...(Args)) {
        luaL_error(lua, "Wrong argument number %d, expecting %d", argnum,
                   sizeof...(Args));
    }

    using tupleType = std::tuple<LuaArgTypeTraits<Args>...>;

    return LuaCheckArgumentImpl<tupleType>(lua,
                                           std::index_sequence_for<Args...>{});
}

template <typename Tuple, std::size_t... I>
void LuaReturnImpl(lua_State *lua, const Tuple &tuple,
                   std::index_sequence<I...>) {
    FCITX_UNUSED(lua);
    (LuaArgTypeTraits<std::tuple_element_t<I, Tuple>>::ret(lua,
                                                           std::get<I>(tuple)),
     ...);
}

template <typename... Args>
int LuaReturn(lua_State *s, const std::tuple<Args...> &args) {
    LuaReturnImpl(s, args, std::index_sequence_for<Args...>{});
    return sizeof...(Args);
}

constexpr char kLuaModuleName[] = "__fcitx_luaaddon";

LuaAddonState *GetLuaAddonState(lua_State *lua);

FCITX_DECLARE_LOG_CATEGORY(lua_log);
#define FCITX_LUA_INFO() FCITX_LOGC(::fcitx::lua_log, Info)
#define FCITX_LUA_WARN() FCITX_LOGC(::fcitx::lua_log, Warn)
#define FCITX_LUA_ERROR() FCITX_LOGC(::fcitx::lua_log, Error)
#define FCITX_LUA_DEBUG() FCITX_LOGC(::fcitx::lua_log, Debug)

} // namespace fcitx

#endif // _FCITX5_LUA_ADDONLOADER_LUAHELPER_H_
