/*
 * SPDX-FileCopyrightText: 2020~2020 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#include "luaaddonstate.h"
#include "base.lua.h"
#include "luahelper.h"
#include "luastate.h"
#include "quickphrase_public.h"
#include <cstddef>
#include <cstdint>
#include <fcitx-config/rawconfig.h>
#include <fcitx-utils/handlertable.h>
#include <fcitx-utils/library.h>
#include <fcitx-utils/standardpaths.h>
#include <fcitx-utils/stringutils.h>
#include <fcitx-utils/trackableobject.h>
#include <fcitx-utils/utf8.h>
#include <fcitx/addonmanager.h>
#include <fcitx/event.h>
#include <fcitx/inputcontext.h>
#include <fcitx/instance.h>
#include <fcntl.h>
#include <filesystem>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace fcitx {

namespace {

void LuaPError(int err, const char *s) {
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

void LuaPrintError(LuaState *lua) {
    if (lua_gettop(lua) > 0) {
        FCITX_LUA_ERROR() << lua_tostring(lua, -1);
    }
}

void rawConfigToLua(LuaState *state, const RawConfig &config) {
    if (!config.hasSubItems()) {
        lua_pushlstring(state, config.value().data(), config.value().size());
        return;
    }

    lua_newtable(state);
    if (!config.value().empty()) {
        lua_pushstring(state, "");
        lua_pushlstring(state, config.value().data(), config.value().size());
        lua_rawset(state, -3);
    }
    if (config.hasSubItems()) {
        auto options = config.subItems();
        for (auto &option : options) {
            auto subConfig = config.get(option);
            lua_pushstring(state, option.data());
            rawConfigToLua(state, *subConfig);
            lua_rawset(state, -3);
        }
    }
}

void luaToRawConfig(LuaState *state, RawConfig &config) {
    int type = lua_type(state, -1);
    if (type == LUA_TSTRING) {
        if (const auto *str = lua_tostring(state, -1)) {
            auto l = lua_rawlen(state, -1);
            config.setValue(std::string(str, l));
        }
        return;
    }

    if (type == LUA_TTABLE) {
        /* table is in the stack at index 't' */
        lua_pushnil(state); /* first key */
        while (lua_next(state, -2) != 0) {
            if (lua_type(state, -2) == LUA_TSTRING) {
                if (const auto *str = lua_tostring(state, -2)) {
                    if (str[0]) {
                        luaToRawConfig(state, config[str]);
                    } else if (lua_type(state, -1) == LUA_TSTRING) {
                        luaToRawConfig(state, config);
                    }
                }
            }
            lua_pop(state, 1);
        }
    }
}

} // namespace

LuaAddonState::LuaAddonState(Library *luaLibrary, const std::string &name,
                             const std::string &library, AddonManager *manager)
    : instance_(manager->instance()),
      state_(std::make_unique<LuaState>(luaLibrary)) {
    if (!state_) {
        throw std::runtime_error("Failed to create lua state.");
    }

    auto path = StandardPaths::global().locate(
        StandardPathsType::PkgData,
        stringutils::joinPath("lua", name, library));
    if (path.empty()) {
        throw std::runtime_error("Couldn't find lua source.");
    }
    LuaAddonState **ppmodule = reinterpret_cast<LuaAddonState **>(
        lua_newuserdata(state_, sizeof(LuaAddonState *)));
    *ppmodule = this;
    lua_setglobal(state_, kLuaModuleName);
    luaL_openlibs(state_);
    auto open_fcitx_core = [](lua_State *state) {
        static const luaL_Reg fcitxlib[] = {
            {"version", &LuaAddonState::version},
            {"lastCommit", &LuaAddonState::lastCommit},
            {"splitString", &LuaAddonState::splitString},
            {"log", &LuaAddonState::log},
            {"watchEvent", &LuaAddonState::watchEvent},
            {"unwatchEvent", &LuaAddonState::unwatchEvent},
            {"currentInputMethod", &LuaAddonState::currentInputMethod},
            {"setCurrentInputMethod", &LuaAddonState::setCurrentInputMethod},
            {"currentProgram", &LuaAddonState::currentProgram},
            {"addConverter", &LuaAddonState::addConverter},
            {"removeConverter", &LuaAddonState::removeConverter},
            {"addQuickPhraseHandler", &LuaAddonState::addQuickPhraseHandler},
            {"removeQuickPhraseHandler",
             &LuaAddonState::removeQuickPhraseHandler},
            {"commitString", &LuaAddonState::commitString},
            {"forwardKey", &LuaAddonState::forwardKey},
            {"standardPathLocate", &LuaAddonState::standardPathLocate},
            {"UTF16ToUTF8", &LuaAddonState::UTF16ToUTF8},
            {"UTF8ToUTF16", &LuaAddonState::UTF8ToUTF16},
        };
        auto *addon = GetLuaAddonState(state);
        luaL_newlib(addon->state_, fcitxlib);
        return 1;
    };
    auto open_fcitx = [](lua_State *state) {
        auto *s = GetLuaAddonState(state)->state_.get();
        if (int rv = luaL_loadstring(s, baseLua) ||
                     lua_pcallk(s, 0, LUA_MULTRET, 0, 0, nullptr);
            rv != LUA_OK) {
            LuaPError(rv, "luaL_loadbuffer() failed");
            LuaPrintError(GetLuaAddonState(state)->state_.get());
            return 0;
        }
        return 1;
    };
    luaL_requiref(state_, "fcitx.core", open_fcitx_core, false);
    luaL_requiref(state_, "fcitx", open_fcitx, false);
    if (int rv = luaL_loadfile(state_, path.string().c_str()); rv != 0) {
        LuaPError(rv, "luaL_loadfilex() failed");
        LuaPrintError(*this);
        throw std::runtime_error("Failed to load lua source.");
    }

    if (int rv = lua_pcall(state_, 0, 0, 0); rv != 0) {
        LuaPError(rv, "lua_pcall() failed");
        LuaPrintError(*this);
        throw std::runtime_error("Failed to run lua source.");
    }

    commitHandler_ = instance_->watchEvent(
        EventType::InputContextCommitString, EventWatcherPhase::PreInputMethod,
        [this](Event &event) {
            auto &commitEvent = static_cast<CommitStringEvent &>(event);
            lastCommit_ = commitEvent.text();
        });
}

std::tuple<> LuaAddonState::logImpl(const char *msg) {
    FCITX_LUA_DEBUG() << msg;
    return {};
}

template <typename T>
std::unique_ptr<HandlerTableEntry<EventHandler>> LuaAddonState::watchEvent(
    EventType type, int id,
    std::function<int(std::unique_ptr<LuaState> &, T &)> pushArguments,
    std::function<void(std::unique_ptr<LuaState> &, T &)> handleReturnValue) {
    return instance_->watchEvent(
        type, EventWatcherPhase::PreInputMethod,
        [this, id, pushArguments, handleReturnValue](Event &event_) {
            auto iter = eventHandler_.find(id);
            int argc = 0;
            if (iter == eventHandler_.end()) {
                return;
            }
            auto &event = static_cast<T &>(event_);
            ScopedICSetter setter(inputContext_, event.inputContext()->watch());
            lua_getglobal(state_, iter->second.function().data());
            if (pushArguments) {
                argc = pushArguments(state_, event);
            }
            int rv = lua_pcall(state_, argc, 1, 0);
            if (rv != 0) {
                LuaPError(rv, "lua_pcall() failed");
                LuaPrintError(*this);
            } else if (lua_gettop(state_) >= 1) {
                if (handleReturnValue) {
                    handleReturnValue(state_, event);
                }
            }
            lua_pop(state_, lua_gettop(state_));
        });
}

std::tuple<int> LuaAddonState::watchEventImpl(int eventType,
                                              const char *function) {
    int newId = currentId_ + 1;
    std::unique_ptr<HandlerTableEntry<EventHandler>> handler = nullptr;

    switch (static_cast<EventType>(eventType)) {
    case EventType::InputContextCreated:
    case EventType::InputContextDestroyed:
    case EventType::InputContextFocusIn:
    case EventType::InputContextFocusOut:
    case EventType::InputContextSurroundingTextUpdated:
    case EventType::InputContextCursorRectChanged:
    case EventType::InputContextUpdatePreedit:
        handler = watchEvent<InputContextEvent>(
            static_cast<EventType>(eventType), newId);
        break;
    case EventType::InputContextKeyEvent:
        handler = watchEvent<KeyEvent>(
            EventType::InputContextKeyEvent, newId,
            [](std::unique_ptr<LuaState> &state, KeyEvent &event) -> int {
                lua_pushinteger(state, event.key().sym());
                lua_pushinteger(state, event.key().states());
                lua_pushboolean(state, event.isRelease());
                return 3;
            },
            [](std::unique_ptr<LuaState> &state, KeyEvent &event) {
                auto b = lua_toboolean(state, -1);
                if (b) {
                    event.filterAndAccept();
                }
            });
        break;
    case EventType::InputContextCommitString:
        handler = watchEvent<CommitStringEvent>(
            EventType::InputContextCommitString, newId,
            [](std::unique_ptr<LuaState> &state,
               CommitStringEvent &event) -> int {
                lua_pushstring(state, event.text().c_str());
                return 1;
            });
        break;
    case EventType::InputContextInputMethodActivated:
    case EventType::InputContextInputMethodDeactivated:
        handler = watchEvent<InputMethodNotificationEvent>(
            static_cast<EventType>(eventType), newId,
            [](std::unique_ptr<LuaState> &state,
               InputMethodNotificationEvent &event) -> int {
                lua_pushstring(state, event.name().c_str());
                return 1;
            });
        break;
    case EventType::InputContextSwitchInputMethod:
        handler = watchEvent<InputContextSwitchInputMethodEvent>(
            static_cast<EventType>(eventType), newId,
            [](std::unique_ptr<LuaState> &state,
               InputContextSwitchInputMethodEvent &event) -> int {
                lua_pushstring(state, event.oldInputMethod().c_str());
                return 1;
            });
        break;
    default:
        throw std::runtime_error("Invalid eventype");
    }
    currentId_++;
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
    auto *ic = inputContext_.get();
    if (ic) {
        return {instance_->inputMethod(ic)};
    }
    return {""};
}

std::tuple<std::string> LuaAddonState::currentProgramImpl() {
    auto *ic = inputContext_.get();
    if (ic) {
        return {ic->program()};
    }
    return {""};
}

std::tuple<> LuaAddonState::setCurrentInputMethodImpl(const char *str,
                                                      bool local) {
    auto *ic = inputContext_.get();
    if (ic) {
        instance_->setCurrentInputMethod(ic, str, local);
    }
    return {};
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
                    lua_getglobal(state_, iter->second.function().data());
                    lua_pushstring(state_, orig.data());
                    if (int rv = lua_pcall(state_, 1, 1, 0); rv != 0) {
                        LuaPError(rv, "lua_pcall() failed");
                        LuaPrintError(*this);
                    } else if (lua_gettop(state_) >= 1) {
                        const auto *s = lua_tostring(state_, -1);
                        if (s) {
                            orig = s;
                        }
                    }
                    lua_pop(state_, lua_gettop(state_));
                })));
    return {newId};
}

std::tuple<> LuaAddonState::removeConverterImpl(int id) {
    converter_.erase(id);
    return {};
}

std::tuple<> LuaAddonState::commitStringImpl(const char *str) {
    if (auto *ic = inputContext_.get()) {
        ic->commitString(str);
    }
    return {};
}

std::tuple<> LuaAddonState::forwardKeyImpl(int keySym, int keyStates, bool release) {
    if (auto *ic = inputContext_.get()) {
        Key key(static_cast<KeySym>(keySym), static_cast<KeyStates>(keyStates));
        ic->forwardKey(key, release);
    }
    return {};
}


bool LuaAddonState::handleQuickPhrase(
    InputContext *ic, const std::string &input,
    const QuickPhraseAddCandidateCallback &callback) {
    ScopedICSetter setter(inputContext_, ic->watch());
    bool flag = true;
    for (auto &handler : quickphraseHandler_) {
        lua_getglobal(state_, handler.second.data());
        lua_pushstring(state_, input.data());
        int rv = lua_pcall(state_, 1, 1, 0);
        if (rv != 0) {
            LuaPError(rv, "lua_pcall() failed");
            LuaPrintError(*this);
        } else if (lua_gettop(state_) >= 1) {
            do {
                int type = lua_type(state_, -1);
                if (type != LUA_TTABLE) {
                    break;
                }
                auto len = luaL_len(state_, -1);
                if (len < 1) {
                    break;
                }
                for (int i = 1; i <= len; ++i) {
                    lua_pushinteger(state_, i);
                    /* stack, table, integer */
                    lua_gettable(state_, -2);
                    std::string result;
                    std::string display;
                    int action;
                    if (lua_type(state_, -1) == LUA_TTABLE) {
                        lua_pushinteger(state_, 1);
                        lua_gettable(state_, -2);
                        const char *str = lua_tostring(state_, -1);
                        result = str;
                        lua_pop(state_, 1);
                        if (!str) {
                            continue;
                        }
                        lua_pushinteger(state_, 2);
                        lua_gettable(state_, -2);
                        str = lua_tostring(state_, -1);
                        display = str;
                        lua_pop(state_, 1);
                        if (!str) {
                            continue;
                        }
                        lua_pushinteger(state_, 3);
                        lua_gettable(state_, -2);
                        action = lua_tointeger(state_, -1);
                        lua_pop(state_, 1);
                        // -1 is lua's custom value for break.
                        if (action == -1) {
                            flag = false;
                        } else {
                            callback(result, display,
                                     static_cast<QuickPhraseAction>(action));
                        }
                    }
                    /* stack, table */
                    lua_pop(state_, 1);
                }
            } while (0);
        }
        if (!flag) {
            return false;
        }
        lua_pop(state_, lua_gettop(state_));
    }
    return true;
}

std::tuple<int> LuaAddonState::addQuickPhraseHandlerImpl(const char *function) {
    int newId = ++currentId_;
    quickphraseHandler_.emplace(newId, function);
    if (!quickphraseCallback_ && quickphrase()) {
        quickphraseCallback_ = quickphrase()->call<IQuickPhrase::addProvider>(
            [this](InputContext *ic, const std::string &input,
                   const QuickPhraseAddCandidateCallback &callback) {
                return handleQuickPhrase(ic, input, callback);
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
    auto files = StandardPaths::global().locate(
        static_cast<StandardPathsType>(type), path,
        [suffix](const std::filesystem::path &file) {
            return file.string().ends_with(suffix);
        });
    result.reserve(files.size());
    for (const auto &file : files) {
        result.push_back(file.second.string());
    }
    return {std::move(result)};
}

std::tuple<std::string> LuaAddonState::UTF16ToUTF8Impl(const char *str) {
    const auto *data = reinterpret_cast<const uint16_t *>(str);
    std::string result;
    size_t i = 0;
    while (data[i]) {
        uint32_t ucs4 = 0;
        if (data[i] < 0xD800 || data[i] > 0xDFFF) {
            ucs4 = data[i];
            i += 1;
        } else if (0xD800 <= data[i] && data[i] <= 0xDBFF) {
            if (!data[i + 1]) {
                return {};
            }
            if (0xDC00 <= data[i + 1] && data[i + 1] <= 0xDFFF) {
                /* We have a valid surrogate pair.  */
                ucs4 = (((data[i] & 0x3FF) << 10) | (data[i + 1] & 0x3FF)) +
                       (1 << 16);
                i += 2;
            }
        } else if (0xDC00 <= data[i] && data[i] <= 0xDFFF) {
            return {};
        }
        result.append(utf8::UCS4ToUTF8(ucs4));
    }
    return result;
}

std::tuple<std::string> LuaAddonState::UTF8ToUTF16Impl(const char *str) {
    std::string s(str);
    if (!utf8::validate(s)) {
        return {};
    }
    std::vector<uint16_t> result;
    for (const auto ucs4 : utf8::MakeUTF8CharRange(s)) {
        if (ucs4 < 0x10000) {
            result.push_back(static_cast<uint16_t>(ucs4));
        } else if (ucs4 < 0x110000) {
            result.push_back(0xD800 | (((ucs4 - 0x10000) >> 10) & 0x3ff));
            result.push_back(0xDC00 | (ucs4 & 0x3ff));
        } else {
            return {};
        }
    }
    result.push_back(0);
    return std::string(reinterpret_cast<char *>(result.data()),
                       result.size() * sizeof(uint16_t));
}

RawConfig LuaAddonState::invokeLuaFunction(InputContext *ic,
                                           const std::string &name,
                                           const RawConfig &config) {
    TrackableObjectReference<InputContext> icRef;
    if (ic) {
        icRef = ic->watch();
    }
    ScopedICSetter setter(inputContext_, icRef);
    lua_getglobal(state_, name.data());
    rawConfigToLua(state_.get(), config);
    int rv = lua_pcall(state_, 1, 1, 0);
    RawConfig ret;
    if (rv != 0) {
        LuaPError(rv, "lua_pcall() failed");
        LuaPrintError(state_.get());
    } else if (lua_gettop(state_) >= 1) {
        luaToRawConfig(state_.get(), ret);
    }

    lua_pop(state_, lua_gettop(state_));
    return ret;
}

} // namespace fcitx
