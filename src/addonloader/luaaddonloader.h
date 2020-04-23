//
// Copyright (C) 2020~2020 by CSSlayer
// wengxt@gmail.com
//
// This library is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; see the file COPYING. If not,
// see <http://www.gnu.org/licenses/>.
//
#ifndef _FCITX5_LUA_ADDONLOADER_LUAADDONLOADER_H_
#define _FCITX5_LUA_ADDONLOADER_LUAADDONLOADER_H_

#include <fcitx/addonfactory.h>
#include <fcitx/addoninstance.h>
#include <fcitx/addonloader.h>

namespace fcitx {

class LuaAddonLoader : public AddonLoader {
public:
    std::string type() const override { return "Lua"; }
    AddonInstance *load(const AddonInfo &info, AddonManager *manager) override;
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
