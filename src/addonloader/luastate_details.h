/*
 * SPDX-FileCopyrightText: 2020~2020 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX5_LUA_ADDONLOADER_LUASTATE_DETAILS_H_
#define _FCITX5_LUA_ADDONLOADER_LUASTATE_DETAILS_H_

#include <tuple>
#include <utility>

namespace fcitx {

template <typename Ret, typename... Args>
auto LuaGetFunctionArgType(Ret (*p)(Args...)) -> std::tuple<Args...>;

// This is for handling the NULL argument problem.
// Args&& will make NULL to be integer, which prevent it from passing to lua
// library function. So we just do the manual cast if orig argument is pointer
// and our argument is integer here.
template <typename OrigType>
struct LuaForwardOrCast {
    template <typename Arg>
    static auto cast(Arg &&arg) {
        return std::forward<Arg>(arg);
    }
};

template <typename OrigType>
struct LuaForwardOrCast<OrigType *> {
    template <typename Arg,
              std::enable_if_t<
                  !std::is_integral_v<std::remove_reference_t<Arg>>, int> = 0>
    static auto cast(Arg &&arg) {
        return std::forward<Arg>(arg);
    }
    template <typename Arg,
              std::enable_if_t<std::is_integral_v<std::remove_reference_t<Arg>>,
                               int> = 0>
    static auto cast(Arg &&arg) {
        return reinterpret_cast<OrigType *>(arg);
    }
};

#define DEFINE_LUA_API_FUNCTION(NAME)                                          \
    template <typename OrigArgs, typename Args, std::size_t... I>              \
    auto _##NAME##_impl(std::index_sequence<I...>, Args args) {                \
        return NAME##_(                                                        \
            LuaForwardOrCast<std::tuple_element_t<I, OrigArgs>>::cast(         \
                std::get<I>(args))...);                                        \
    }                                                                          \
    template <typename... Args>                                                \
    auto NAME(Args &&...args) {                                                \
        using ArgType = decltype(LuaGetFunctionArgType(NAME##_));              \
        return _##NAME##_impl<ArgType>(                                        \
            std::index_sequence_for<int, Args...>{},                           \
            std::forward_as_tuple(state_.get(), args...));                     \
    }

#define DECLARE_LUA_FUNCTION_PTR(NAME) decltype(&::NAME) NAME##_ = nullptr;

#define DEFINE_BRIDGE_LUA_API_FUNCTION(NAME)                                   \
    template <typename StatePtr, typename... Args>                             \
    auto NAME(const StatePtr &state, Args &&...args) {                         \
        return state->NAME(std::forward<Args>(args)...);                       \
    }

} // namespace fcitx

#endif // _FCITX5_LUA_ADDONLOADER_LUASTATE_DETAILS_H_
