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
