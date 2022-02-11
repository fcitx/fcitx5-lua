/*
 * SPDX-FileCopyrightText: 2020~2020 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#include "luastate.h"
#include "config.h"
#include "luahelper.h"
#include <stdexcept>

#ifdef USE_DLOPEN
#define GET_LUA_API(FUNCTION)                                                  \
    (reinterpret_cast<decltype(&::FUNCTION)>(luaLibrary_->resolve(#FUNCTION)))
#else
#define GET_LUA_API(FUNCTION) (&::FUNCTION)
#endif
#define FILL_LUA_API(FUNCTION) FUNCTION##_ = GET_LUA_API(FUNCTION)

namespace fcitx {
LuaState::LuaState(Library *library)
    : luaLibrary_(library), state_(nullptr, _fcitx_lua_close) {
    // Resolve all required lua function first.
#define FOREACH_LUA_FUNCTION(NAME)                                             \
    FILL_LUA_API(NAME);                                                        \
    if (!NAME##_) {                                                            \
        throw std::runtime_error("Failed to resolve lua function");            \
    }
#include "luafunc.h"
#undef FOREACH_LUA_FUNCTION
    state_.reset(_fcitx_luaL_newstate());
}
} // namespace fcitx
