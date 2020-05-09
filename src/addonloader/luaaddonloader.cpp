/*
 * SPDX-FileCopyrightText: 2020~2020 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#include "luaaddonloader.h"
#include "luaaddon.h"
#include <fcitx/addonmanager.h>

namespace fcitx {

AddonInstance *LuaAddonLoader::load(const AddonInfo &info,
                                    AddonManager *manager) {
    if (info.category() == AddonCategory::Module) {
        try {
            auto addon = std::make_unique<LuaAddon>(info, manager);
            return addon.release();
        } catch (const std::exception &e) {
            FCITX_LUA_ERROR() << "Loading lua addon " << info.uniqueName()
                              << " failed: " << e.what();
            return nullptr;
        }
    }
    return nullptr;
}

LuaAddonLoaderAddon::LuaAddonLoaderAddon(AddonManager *manager)
    : manager_(manager) {
    manager->registerLoader(std::make_unique<LuaAddonLoader>());
}

LuaAddonLoaderAddon::~LuaAddonLoaderAddon() {
    manager_->unregisterLoader("Lua");
}

AddonInstance *LuaAddonLoaderFactory::create(AddonManager *manager) {
    return new LuaAddonLoaderAddon(manager);
}

} // namespace fcitx

FCITX_ADDON_FACTORY(fcitx::LuaAddonLoaderFactory);
