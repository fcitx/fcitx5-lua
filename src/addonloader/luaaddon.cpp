/*
 * SPDX-FileCopyrightText: 2020~2020 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#include "luaaddon.h"
#include "luaaddonstate.h"
#include "luahelper.h"
#include <exception>
#include <fcitx-config/rawconfig.h>
#include <fcitx-utils/library.h>
#include <fcitx/addoninfo.h>
#include <fcitx/inputcontext.h>
#include <memory>
#include <string>
#include <utility>

namespace fcitx {

LuaAddon::LuaAddon(Library *luaLibrary, const AddonInfo &info,
                   AddonManager *manager)
    : instance_(manager->instance()), name_(info.uniqueName()),
      library_(info.library()), state_(std::make_unique<LuaAddonState>(
                                    luaLibrary, name_, library_, manager)),
      luaLibrary_(luaLibrary) {}

void LuaAddon::reloadConfig() {
    try {
        auto newState = std::make_unique<LuaAddonState>(
            luaLibrary_, name_, library_, &instance_->addonManager());
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
