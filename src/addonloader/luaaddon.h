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
    RawConfig invokeLuaFunction(const std::string &name,
                                const RawConfig &config);
    FCITX_ADDON_EXPORT_FUNCTION(LuaAddon, invokeLuaFunction);

    Instance *instance_;
    const std::string name_;
    const std::string library_;

    std::unique_ptr<LuaAddonState> state_;
};

} // namespace fcitx

#endif // _FCITX5_LUA_ADDONLOADER_LUAADDON_H_
