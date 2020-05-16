/*
 * SPDX-FileCopyrightText: 2020~2020 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#include "luaaddon.h"

namespace fcitx {

LuaAddon::LuaAddon(Library &luaLibrary, const AddonInfo &info,
                   AddonManager *manager)
    : instance_(manager->instance()), name_(info.uniqueName()),
      library_(info.library()), state_(std::make_unique<LuaAddonState>(
                                    luaLibrary, name_, library_, manager)),
      luaLibrary_(&luaLibrary) {}

void LuaAddon::reloadConfig() {
    try {
        auto newState = std::make_unique<LuaAddonState>(
            *luaLibrary_, name_, library_, &instance_->addonManager());
        state_ = std::move(newState);
    } catch (const std::exception &e) {
        FCITX_LUA_ERROR() << e.what();
    }
}

RawConfig LuaAddon::invokeLuaFunction(InputContext *ic, const std::string &name,
                                      const RawConfig &config) {
    return state_->invokeLuaFunction(ic, name, config);
}

} // namespace fcitx
