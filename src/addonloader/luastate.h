/*
 * SPDX-FileCopyrightText: 2020~2020 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX5_LUA_ADDONLOADER_LUASTATE_H_
#define _FCITX5_LUA_ADDONLOADER_LUASTATE_H_

#include <fcitx-utils/library.h>
#include <lua.hpp>
#include <memory>

namespace fcitx {

struct LuaState {
public:
    LuaState(Library &library);

#define DEFINE_LUA_API_FUNCTION(NAME)                                          \
    template <typename... Args>                                                \
    auto NAME(Args &&... args) {                                               \
        return NAME##_(state_.get(), std::forward<Args>(args)...);             \
    }

#define FOREACH_LUA_FUNCTION DEFINE_LUA_API_FUNCTION
#include "luafunc.h"
#undef FOREACH_LUA_FUNCTION

    void pop(int n) { this->lua_settop(-n - 1); }

private:
    Library *luaLibrary_;

#define DECLARE_LUA_FUNCTION_PTR(NAME) decltype(&::NAME) NAME##_ = nullptr;
#define FOREACH_LUA_FUNCTION DECLARE_LUA_FUNCTION_PTR
#include "luafunc.h"
#undef FOREACH_LUA_FUNCTION
    std::unique_ptr<lua_State, decltype(&lua_close)> state_;
};

} // namespace fcitx

#endif // _FCITX5_LUA_ADDONLOADER_LUASTATE_H_
