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
#include "luaaddonstate.h"
#include "baselua.h"
#include <fcitx-utils/standardpath.h>
#include <fcitx/addonmanager.h>
#include <fcntl.h>

namespace fcitx {

static void LuaPError(int err, const char *s) {
    switch (err) {
    case LUA_ERRSYNTAX:
        FCITX_LUA_ERROR() << "syntax error during pre-compilation " << s;
        break;
    case LUA_ERRMEM:
        FCITX_LUA_ERROR() << "memory allocation error " << s;
        break;
    case LUA_ERRFILE:
        FCITX_LUA_ERROR() << "cannot open/read the file " << s;
        break;
    case LUA_ERRRUN:
        FCITX_LUA_ERROR() << "a runtime error " << s;
        break;
    case LUA_ERRERR:
        FCITX_LUA_ERROR() << "error while running the error handler function "
                          << s;
        break;
    case LUA_OK:
        FCITX_LUA_ERROR() << "ok: " << s;
        break;
    default:
        FCITX_LUA_ERROR() << "unknown error: " << err << " " << s;
        break;
    }
}

static void LuaPrintError(lua_State *lua) {
    if (lua_gettop(lua) > 0) {
        FCITX_LUA_ERROR() << lua_tostring(lua, -1);
    }
}

LuaAddonState::LuaAddonState(const std::string &name,
                             const std::string &library, AddonManager *manager)
    : instance_(manager->instance()), state_(luaL_newstate(), lua_close) {
    if (!state_) {
        throw std::runtime_error("Failed to create lua state.");
    }

    auto path = StandardPath::global().locate(
        StandardPath::Type::PkgData,
        stringutils::joinPath("lua", name, library));
    if (path.empty()) {
        throw std::runtime_error("Couldn't find lua source.");
    }
    luaL_openlibs(*this);
    auto open_fcitx_core = [](lua_State *state) {
        static const luaL_Reg fcitxlib[] = {
            {"log", &LuaAddonState::log},
            {"watchEvent", &LuaAddonState::watchEvent},
            {"unwatchEvent", &LuaAddonState::unwatchEvent},
            {"currentInputMethod", &LuaAddonState::currentInputMethod},
            {"addConverter", &LuaAddonState::addConverter},
            {"removeConverter", &LuaAddonState::removeConverter},
            {"addQuickPhraseHandler", &LuaAddonState::addQuickPhraseHandler},
            {"removeQuickPhraseHandler",
             &LuaAddonState::removeQuickPhraseHandler},
            {"standardPathLocate", &LuaAddonState::standardPathLocate},
        };
        luaL_newlib(state, fcitxlib);
        return 1;
    };
    auto open_fcitx = [](lua_State *state) {
        if (int rv = luaL_dostring(state, baselua); rv != LUA_OK) {
            LuaPError(rv, "luaL_loadbuffer() failed");
            LuaPrintError(state);
            return 0;
        }
        return 1;
    };
    luaL_requiref(*this, "fcitx.core", open_fcitx_core, false);
    luaL_requiref(*this, "fcitx", open_fcitx, false);
    LuaAddonState **ppmodule = reinterpret_cast<LuaAddonState **>(
        lua_newuserdata(*this, sizeof(LuaAddonState *)));
    *ppmodule = this;
    lua_setglobal(*this, kLuaModuleName);
    if (int rv = luaL_loadfile(*this, path.data()); rv != 0) {
        LuaPError(rv, "luaL_loadfile() failed");
        LuaPrintError(*this);
        throw std::runtime_error("Failed to load lua source.");
    }

    if (int rv = lua_pcall(*this, 0, 0, 0); rv != 0) {
        LuaPError(rv, "lua_pcall() failed");
        LuaPrintError(*this);
        throw std::runtime_error("Failed to run lua source.");
    }
}

std::tuple<> LuaAddonState::logImpl(const char *msg) {
    FCITX_LUA_DEBUG() << msg;
    return {};
}

std::tuple<int> LuaAddonState::watchEventImpl(const char *event,
                                              const char *function) {
    std::string_view eventView(event);
    bool validEvent = true;
    EventType type;
    if (eventView == "KeyEvent") {
        type = EventType::InputContextKeyEvent;
    } else {
        validEvent = false;
    }
    if (!validEvent) {
        throw std::runtime_error("Invalid eventype");
    }
    int newId = ++currentId_;
    std::unique_ptr<HandlerTableEntry<EventHandler>> handler;
    handler = instance_->watchEvent(
        type, EventWatcherPhase::PreInputMethod, [this, newId](Event &event) {
            auto iter = eventHandler_.find(newId);
            if (iter == eventHandler_.end()) {
                return;
            }
            auto &keyEvent = static_cast<KeyEvent &>(event);
            ScopedICSetter setter(inputContext_,
                                  keyEvent.inputContext()->watch());
            lua_getglobal(*this, iter->second.function().data());
            lua_pushinteger(*this, keyEvent.key().sym());
            lua_pushinteger(*this, keyEvent.key().states());
            lua_pushboolean(*this, keyEvent.isRelease());
            int rv = lua_pcall(*this, 3, 1, 0);
            if (rv != 0) {
                LuaPError(rv, "lua_pcall() failed");
                LuaPrintError(*this);
            } else if (lua_gettop(*this) == 1) {
                auto b = lua_toboolean(*this, -1);
                if (b) {
                    keyEvent.filterAndAccept();
                }
            }
            lua_pop(*this, lua_gettop(*this));
        });
    eventHandler_.emplace(std::piecewise_construct,
                          std::forward_as_tuple(newId),
                          std::forward_as_tuple(function, std::move(handler)));
    return {newId};
}

std::tuple<> LuaAddonState::unwatchEventImpl(int id) {
    eventHandler_.erase(id);
    return {};
}

std::tuple<std::string> LuaAddonState::currentInputMethodImpl() {
    auto ic = inputContext_.get();
    if (ic) {
        return {instance_->inputMethod(ic)};
    }
    return {""};
}

std::tuple<int> LuaAddonState::addConverterImpl(const char *function) {
    int newId = ++currentId_;
    converter_.emplace(
        std::piecewise_construct, std::forward_as_tuple(newId),
        std::forward_as_tuple(
            function,
            instance_->connect<Instance::CommitFilter>(
                [this, newId](InputContext *inputContext, std::string &orig) {
                    auto iter = converter_.find(newId);
                    if (iter == converter_.end()) {
                        return;
                    }

                    ScopedICSetter setter(inputContext_, inputContext->watch());
                    lua_getglobal(*this, iter->second.function().data());
                    lua_pushstring(*this, orig.data());
                    int rv = lua_pcall(*this, 1, 1, 0);
                    if (rv != 0) {
                        LuaPError(rv, "lua_pcall() failed");
                        LuaPrintError(*this);
                    } else if (lua_gettop(*this) == 1) {
                        auto s = lua_tostring(*this, -1);
                        if (s) {
                            orig = s;
                        }
                    }
                    lua_pop(*this, lua_gettop(*this));
                })));
    return {newId};
}

std::tuple<> LuaAddonState::removeConverterImpl(int id) {
    converter_.erase(id);
    return {};
}

std::tuple<> LuaAddonState::commitStringImpl(const char *str) {
    if (auto ic = inputContext_.get()) {
        ic->commitString(str);
    }
    return {};
}

bool LuaAddonState::handleQuickPhrase(
    const std::string &input, const QuickPhraseAddCandidateCallback &callback) {
    bool flag = true;
    for (auto iter = quickphraseHandler_.begin(),
              end = quickphraseHandler_.end();
         iter != end; ++iter) {
        lua_getglobal(*this, iter->second.data());
        lua_pushstring(*this, input.data());
        int rv = lua_pcall(*this, 1, 1, 0);
        if (rv != 0) {
            LuaPError(rv, "lua_pcall() failed");
            LuaPrintError(*this);
        } else if (lua_gettop(*this) >= 1) {
            do {
                int type = lua_type(*this, -1);
                if (type != LUA_TTABLE) {
                    break;
                }
                auto len = luaL_len(*this, -1);
                if (len < 1) {
                    break;
                }
                for (int i = 1; i <= len; ++i) {
                    lua_pushinteger(*this, i);
                    /* stack, table, integer */
                    lua_gettable(*this, -2);
                    std::string result, display;
                    int action;
                    if (lua_type(*this, -1) == LUA_TTABLE) {
                        lua_pushinteger(*this, 1);
                        lua_gettable(*this, -2);
                        const char *str = lua_tostring(*this, -1);
                        result = str;
                        lua_pop(*this, 1);
                        if (!str) {
                            continue;
                        }
                        lua_pushinteger(*this, 2);
                        lua_gettable(*this, -2);
                        str = lua_tostring(*this, -1);
                        display = str;
                        lua_pop(*this, 1);
                        if (!str) {
                            continue;
                        }
                        lua_pushinteger(*this, 3);
                        lua_gettable(*this, -2);
                        action = lua_tointeger(*this, -1);
                        lua_pop(*this, 1);
                        // -1 is lua's custom value for break.
                        if (action == -1) {
                            flag = false;
                        } else {
                            callback(result, display,
                                     static_cast<QuickPhraseAction>(action));
                        }
                    }
                    /* stack, table */
                    lua_pop(*this, 1);
                }
            } while (0);
        }
        if (!flag) {
            return false;
        }
        lua_pop(*this, lua_gettop(*this));
    }
    return true;
}

std::tuple<int> LuaAddonState::addQuickPhraseHandlerImpl(const char *function) {
    int newId = ++currentId_;
    quickphraseHandler_.emplace(newId, function);
    if (!quickphraseCallback_) {
        quickphraseCallback_ = quickphrase()->call<IQuickPhrase::addProvider>(
            [this](const std::string &input,
                   QuickPhraseAddCandidateCallback callback) {
                return handleQuickPhrase(input, callback);
            });
    }
    return {newId};
}

std::tuple<> LuaAddonState::removeQuickPhraseHandlerImpl(int id) {
    quickphraseHandler_.erase(id);
    if (quickphraseHandler_.empty()) {
        quickphraseCallback_.reset();
    }
    return {};
}

std::tuple<std::vector<std::string>>
LuaAddonState::standardPathLocateImpl(int type, const char *path,
                                      const char *suffix) {
    std::vector<std::string> result;
    auto files = StandardPath::global().multiOpen(
        static_cast<StandardPath::Type>(type), path, O_RDONLY,
        filter::Suffix(suffix));
    for (const auto &file : files) {
        result.push_back(file.second.path());
    }
    return {std::move(result)};
}

} // namespace fcitx
