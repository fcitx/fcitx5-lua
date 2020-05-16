/*
 * SPDX-FileCopyrightText: 2020~2020 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#include "luaaddonstate.h"
#include "baselua.h"
#include <fcitx-utils/standardpath.h>
#include <fcitx-utils/utf8.h>
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

static void LuaPrintError(LuaState *lua) {
    if (lua->lua_gettop() > 0) {
        FCITX_LUA_ERROR() << lua->lua_tolstring(-1, nullptr);
    }
}

LuaAddonState::LuaAddonState(Library &luaLibrary, const std::string &name,
                             const std::string &library, AddonManager *manager)
    : instance_(manager->instance()),
      state_(std::make_unique<LuaState>(luaLibrary)) {
    if (!state_) {
        throw std::runtime_error("Failed to create lua state.");
    }

    auto path = StandardPath::global().locate(
        StandardPath::Type::PkgData,
        stringutils::joinPath("lua", name, library));
    if (path.empty()) {
        throw std::runtime_error("Couldn't find lua source.");
    }
    LuaAddonState **ppmodule = reinterpret_cast<LuaAddonState **>(
        state_->lua_newuserdata(sizeof(LuaAddonState *)));
    *ppmodule = this;
    state_->lua_setglobal(kLuaModuleName);
    state_->luaL_openlibs();
    auto open_fcitx_core = [](lua_State *state) {
        static const luaL_Reg fcitxlib[] = {
            {"version", &LuaAddonState::version},
            {"lastCommit", &LuaAddonState::lastCommit},
            {"log", &LuaAddonState::log},
            {"splitString", &LuaAddonState::splitString},
            {"watchEvent", &LuaAddonState::watchEvent},
            {"unwatchEvent", &LuaAddonState::unwatchEvent},
            {"currentInputMethod", &LuaAddonState::currentInputMethod},
            {"addConverter", &LuaAddonState::addConverter},
            {"removeConverter", &LuaAddonState::removeConverter},
            {"addQuickPhraseHandler", &LuaAddonState::addQuickPhraseHandler},
            {"removeQuickPhraseHandler",
             &LuaAddonState::removeQuickPhraseHandler},
            {"standardPathLocate", &LuaAddonState::standardPathLocate},
            {"UTF16ToUTF8", &LuaAddonState::UTF16ToUTF8},
            {"UTF8ToUTF16", &LuaAddonState::UTF8ToUTF16},
        };
        auto addon = GetLuaAddonState(state);
        addon->state_->luaL_checkversion_(LUA_VERSION_NUM, LUAL_NUMSIZES);
        addon->state_->lua_createtable(0, FCITX_ARRAY_SIZE(fcitxlib));
        addon->state_->luaL_setfuncs(fcitxlib, 0);
        return 1;
    };
    auto open_fcitx = [](lua_State *state) {
        auto s = GetLuaAddonState(state)->state_.get();
        if (int rv = (s->luaL_loadstring(baselua),
                      s->lua_pcallk(0, LUA_MULTRET, 0, 0, nullptr));
            rv != LUA_OK) {
            LuaPError(rv, "luaL_loadbuffer() failed");
            LuaPrintError(GetLuaAddonState(state)->state_.get());
            return 0;
        }
        return 1;
    };
    state_->luaL_requiref("fcitx.core", open_fcitx_core, false);
    state_->luaL_requiref("fcitx", open_fcitx, false);
    if (int rv = state_->luaL_loadfilex(path.data(), nullptr); rv != 0) {
        LuaPError(rv, "luaL_loadfilex() failed");
        LuaPrintError(*this);
        throw std::runtime_error("Failed to load lua source.");
    }

    if (int rv = state_->lua_pcallk(0, 0, 0, 0, nullptr); rv != 0) {
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
            state_->lua_getglobal(iter->second.function().data());
            state_->lua_pushinteger(keyEvent.key().sym());
            state_->lua_pushinteger(keyEvent.key().states());
            state_->lua_pushboolean(keyEvent.isRelease());
            int rv = state_->lua_pcallk(3, 1, 0, 0, nullptr);
            if (rv != 0) {
                LuaPError(rv, "lua_pcall() failed");
                LuaPrintError(*this);
            } else if (state_->lua_gettop() >= 1) {
                auto b = state_->lua_toboolean(-1);
                if (b) {
                    keyEvent.filterAndAccept();
                }
            }
            state_->pop(state_->lua_gettop());
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
                    state_->lua_getglobal(iter->second.function().data());
                    state_->lua_pushstring(orig.data());
                    int rv = state_->lua_pcallk(1, 1, 0, 0, nullptr);
                    if (rv != 0) {
                        LuaPError(rv, "lua_pcall() failed");
                        LuaPrintError(*this);
                    } else if (state_->lua_gettop() >= 1) {
                        auto s = state_->lua_tolstring(-1, nullptr);
                        if (s) {
                            orig = s;
                        }
                    }
                    state_->pop(state_->lua_gettop());
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
    InputContext *ic, const std::string &input,
    const QuickPhraseAddCandidateCallback &callback) {
    ScopedICSetter setter(inputContext_, ic->watch());
    bool flag = true;
    for (auto iter = quickphraseHandler_.begin(),
              end = quickphraseHandler_.end();
         iter != end; ++iter) {
        state_->lua_getglobal(iter->second.data());
        state_->lua_pushstring(input.data());
        int rv = state_->lua_pcallk(1, 1, 0, 0, nullptr);
        if (rv != 0) {
            LuaPError(rv, "lua_pcall() failed");
            LuaPrintError(*this);
        } else if (state_->lua_gettop() >= 1) {
            do {
                int type = state_->lua_type(-1);
                if (type != LUA_TTABLE) {
                    break;
                }
                auto len = state_->luaL_len(-1);
                if (len < 1) {
                    break;
                }
                for (int i = 1; i <= len; ++i) {
                    state_->lua_pushinteger(i);
                    /* stack, table, integer */
                    state_->lua_gettable(-2);
                    std::string result, display;
                    int action;
                    if (state_->lua_type(-1) == LUA_TTABLE) {
                        state_->lua_pushinteger(1);
                        state_->lua_gettable(-2);
                        const char *str = state_->lua_tolstring(-1, nullptr);
                        result = str;
                        state_->pop(1);
                        if (!str) {
                            continue;
                        }
                        state_->lua_pushinteger(2);
                        state_->lua_gettable(-2);
                        str = state_->lua_tolstring(-1, nullptr);
                        display = str;
                        state_->pop(1);
                        if (!str) {
                            continue;
                        }
                        state_->lua_pushinteger(3);
                        state_->lua_gettable(-2);
                        action = state_->lua_tointegerx(-1, nullptr);
                        state_->pop(1);
                        // -1 is lua's custom value for break.
                        if (action == -1) {
                            flag = false;
                        } else {
                            callback(result, display,
                                     static_cast<QuickPhraseAction>(action));
                        }
                    }
                    /* stack, table */
                    state_->pop(1);
                }
            } while (0);
        }
        if (!flag) {
            return false;
        }
        state_->pop(state_->lua_gettop());
    }
    return true;
}

std::tuple<int> LuaAddonState::addQuickPhraseHandlerImpl(const char *function) {
    int newId = ++currentId_;
    quickphraseHandler_.emplace(newId, function);
    if (!quickphraseCallback_ && quickphrase()) {
        quickphraseCallback_ = quickphrase()->call<IQuickPhrase::addProvider>(
            [this](InputContext *ic, const std::string &input,
                   QuickPhraseAddCandidateCallback callback) {
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
    auto files = StandardPath::global().multiOpen(
        static_cast<StandardPath::Type>(type), path, O_RDONLY,
        filter::Suffix(suffix));
    for (const auto &file : files) {
        result.push_back(file.second.path());
    }
    return {std::move(result)};
}

std::tuple<std::string> LuaAddonState::UTF16ToUTF8Impl(const char *str) {
    auto len = strlen(str);
    if (len % 2 != 0) {
        return {};
    }
    len /= 2;
    std::string result;
    auto data = reinterpret_cast<const uint16_t *>(str);
    size_t i = 0;
    while (i < len) {
        uint32_t ucs4 = 0;
        if (data[i] < 0xD800 || data[i] > 0xDFFF) {
            ucs4 = data[i];
            i += 1;
        } else if (0xD800 <= data[i] && data[i] <= 0xDBFF) {
            if (i + 1 >= len) {
                return {};
            }
            if (0xDC00 <= data[i + 1] && data[i + 1] <= 0xDFFF) {
                /* We have a valid surrogate pair.  */
                ucs4 = (((data[i] & 0x3FF) << 10) | (data[i + 1] & 0x3FF)) +
                       (1 << 16);
                i += 2;
            }
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
            result.push_back(0xD800 | (((ucs4 >> 16) & 0x1f) - 1) |
                             (ucs4 >> 10));
            result.push_back(0xDC00 | (ucs4 & 0x3ff));
        } else {
            return {};
        }
    }
    result.push_back(0);
    return std::string(reinterpret_cast<char *>(result.data()));
}

void rawConfigToLua(LuaState *state, const RawConfig &config) {
    if (!config.hasSubItems()) {
        state->lua_pushstring(config.value().data());
        return;
    }

    state->lua_createtable(0, 0);
    if (!config.value().empty()) {
        state->lua_pushstring("");
        state->lua_pushstring(config.value().data());
        state->lua_rawset(-3);
    }
    if (config.hasSubItems()) {
        auto options = config.subItems();
        for (auto &option : options) {
            auto subConfig = config.get(option);
            state->lua_pushstring(option.data());
            rawConfigToLua(state, *subConfig);
            state->lua_rawset(-3);
        }
    }
}

void luaToRawConfig(LuaState *state, RawConfig &config) {
    int type = state->lua_type(-1);
    if (type == LUA_TSTRING) {
        if (auto str = state->lua_tolstring(-1, nullptr)) {
            config.setValue(str);
        }
        return;
    }

    if (type == LUA_TTABLE) {
        /* table is in the stack at index 't' */
        state->lua_pushnil(); /* first key */
        while (state->lua_next(-2) != 0) {
            if (state->lua_type(-2) == LUA_TSTRING) {
                if (auto str = state->lua_tolstring(-2, nullptr)) {
                    if (str[0]) {
                        luaToRawConfig(state, config[str]);
                    } else if (state->lua_type(-1) == LUA_TSTRING) {
                        luaToRawConfig(state, config);
                    }
                }
            }
            state->pop(1);
        }
    }
}

RawConfig LuaAddonState::invokeLuaFunction(InputContext *ic,
                                           const std::string &name,
                                           const RawConfig &config) {
    ScopedICSetter setter(inputContext_, ic->watch());
    state_->lua_getglobal(name.data());
    rawConfigToLua(state_.get(), config);
    int rv = state_->lua_pcallk(1, 1, 0, 0, nullptr);
    RawConfig ret;
    if (rv != 0) {
        LuaPError(rv, "lua_pcall() failed");
        LuaPrintError(state_.get());
    } else if (state_->lua_gettop() >= 1) {
        luaToRawConfig(state_.get(), ret);
    }

    state_->pop(state_->lua_gettop());
    return ret;
}

} // namespace fcitx
