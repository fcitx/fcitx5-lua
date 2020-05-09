/*
 * SPDX-FileCopyrightText: 2020~2020 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX5_LUA_ADDONLOADER_LUAADDON_H_
#define _FCITX5_LUA_ADDONLOADER_LUAADDON_H_

#include "luaaddon_public.h"
#include "luaaddonstate.h"
#include "luahelper.h"
#include <fcitx/addoninfo.h>
#include <fcitx/addoninstance.h>
#include <fcitx/addonmanager.h>
#include <fcitx/instance.h>

namespace fcitx {

class AddonManager;

class LuaAddon : public AddonInstance {
public:
    LuaAddon(const AddonInfo &info, AddonManager *manager);

    void reloadConfig() override;

private:
    RawConfig invokeLuaFunction(InputContext *ic, const std::string &name,
                                const RawConfig &config);
    FCITX_ADDON_EXPORT_FUNCTION(LuaAddon, invokeLuaFunction);

    Instance *instance_;
    const std::string name_;
    const std::string library_;

    std::unique_ptr<LuaAddonState> state_;
};

} // namespace fcitx

#endif // _FCITX5_LUA_ADDONLOADER_LUAADDON_H_
