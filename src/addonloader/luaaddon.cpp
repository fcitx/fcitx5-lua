/*
 * SPDX-FileCopyrightText: 2020~2020 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#include "luaaddon.h"
#include <lua.hpp>

namespace fcitx {

LuaAddon::LuaAddon(const AddonInfo &info, AddonManager *manager)
    : instance_(manager->instance()), name_(info.uniqueName()),
      library_(info.library()),
      state_(std::make_unique<LuaAddonState>(name_, library_, manager)) {}

void fcitx::LuaAddon::reloadConfig() {
    try {
        auto newState = std::make_unique<LuaAddonState>(
            name_, library_, &instance_->addonManager());
        state_ = std::move(newState);
    } catch (const std::exception &e) {
        FCITX_LUA_ERROR() << e.what();
    }
}

fcitx::RawConfig
fcitx::LuaAddon::invokeLuaFunction(InputContext *ic, const std::string &name,
                                   const fcitx::RawConfig &config) {
    return state_->invokeLuaFunction(ic, name, config);
}

} // namespace fcitx
