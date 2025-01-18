/*
 * SPDX-FileCopyrightText: 2020~2020 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX5_LUA_ADDONLOADER_LUASTATE_H_
#define _FCITX5_LUA_ADDONLOADER_LUASTATE_H_

#include "luastate_details.h"
#include <fcitx-utils/library.h>
#include <functional>
#include <lua.hpp> // IWYU pragma: export
#include <memory>

namespace fcitx {

struct LuaState {
public:
    LuaState(Library *library);

#define FOREACH_LUA_FUNCTION DEFINE_LUA_API_FUNCTION
#include "luafunc.h"
#undef FOREACH_LUA_FUNCTION

    template <typename... Args>
    auto luaL_error(Args &&...args) {
        return luaL_error_(state_.get(), std::forward<Args>(args)...);
    }

private:
    Library *luaLibrary_;

#define FOREACH_LUA_FUNCTION DECLARE_LUA_FUNCTION_PTR
#include "luafunc.h"
#undef FOREACH_LUA_FUNCTION
    decltype(&::luaL_error) luaL_error_ = nullptr;
    std::unique_ptr<lua_State, std::function<void(lua_State *)>> state_;
};

#define FOREACH_LUA_FUNCTION DEFINE_BRIDGE_LUA_API_FUNCTION
#include "luafunc.h"
#undef FOREACH_LUA_FUNCTION

// luaL_error is vaarg function, which won't work with the type cast and we
// don't need to handle that for it either. So just manually define the
// redirection here.
template <typename StatePtr, typename... Args>
auto luaL_error(const StatePtr &state, Args &&...args) {
    return state->luaL_error(std::forward<Args>(args)...);
}

} // namespace fcitx

#endif // _FCITX5_LUA_ADDONLOADER_LUASTATE_H_
