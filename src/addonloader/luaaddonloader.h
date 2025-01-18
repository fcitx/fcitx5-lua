/*
 * SPDX-FileCopyrightText: 2020~2020 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX5_LUA_ADDONLOADER_LUAADDONLOADER_H_
#define _FCITX5_LUA_ADDONLOADER_LUAADDONLOADER_H_

#include <fcitx-utils/library.h>
#include <fcitx/addonfactory.h>
#include <fcitx/addoninfo.h>
#include <fcitx/addoninstance.h>
#include <fcitx/addonloader.h>
#include <memory>
#include <string>

namespace fcitx {

class LuaAddonLoader : public AddonLoader {
public:
    LuaAddonLoader();
    std::string type() const override { return "Lua"; }
    AddonInstance *load(const AddonInfo &info, AddonManager *manager) override;

private:
    std::unique_ptr<Library> luaLibrary_;
};

class LuaAddonLoaderAddon : public AddonInstance {
public:
    LuaAddonLoaderAddon(AddonManager *manager);
    ~LuaAddonLoaderAddon();

private:
    AddonManager *manager_;
};

class LuaAddonLoaderFactory : public AddonFactory {
    AddonInstance *create(AddonManager *manager) override;
};

} // namespace fcitx
#endif // _FCITX5_LUA_ADDONLOADER_LUAADDONLOADER_H_
