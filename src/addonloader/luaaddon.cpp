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
fcitx::LuaAddon::invokeLuaFunction(const std::string &name,
                                   const fcitx::RawConfig &config) {
    return state_->invokeLuaFunction(name, config);
}

} // namespace fcitx
