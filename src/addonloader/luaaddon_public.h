/*
 * SPDX-FileCopyrightText: 2020~2020 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX5_LUA_ADDONLOADER_LUAADDON_PUBLIC_H_
#define _FCITX5_LUA_ADDONLOADER_LUAADDON_PUBLIC_H_

#include <fcitx-config/rawconfig.h>
#include <fcitx-utils/metastring.h>
#include <fcitx/addoninstance.h>
#include <fcitx/inputcontext.h>

/// Trigger quickphrase, with following format:
/// description_text prefix_text
FCITX_ADDON_DECLARE_FUNCTION(LuaAddon, invokeLuaFunction,
                             fcitx::RawConfig(fcitx::InputContext *ic,
                                              const std::string &text,
                                              const fcitx::RawConfig &config));

#endif // _FCITX5_LUA_ADDONLOADER_LUAADDON_PUBLIC_H_
