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
#ifndef _FCITX5_LUA_ADDONLOADER_LUAADDONSTATE_H_
#define _FCITX5_LUA_ADDONLOADER_LUAADDONSTATE_H_

#include "luahelper.h"
#include <fcitx/addoninfo.h>
#include <fcitx/addonmanager.h>
#include <fcitx/instance.h>
#include <lua.hpp>
#include <quickphrase_public.h>

namespace fcitx {

#define DEFINE_LUA_FUNCTION(FUNCTION_NAME)                                     \
    static int FUNCTION_NAME(lua_State *lua) {                                 \
        auto args =                                                            \
            LuaCheckArgument(lua, &LuaAddonState::FUNCTION_NAME##Impl);        \
        try {                                                                  \
            auto fn = std::mem_fn(&LuaAddonState::FUNCTION_NAME##Impl);        \
            auto combined_args =                                               \
                std::tuple_cat(std::make_tuple(GetLuaAddonState(lua)), args);  \
            return LuaReturn(lua, fcitx::callWithTuple(fn, combined_args));    \
        } catch (const std::exception &e) {                                    \
            return luaL_error(lua, e.what());                                  \
        }                                                                      \
    }

template <typename T>
class ScopedSetter {
public:
    ScopedSetter(T &t, const T &n) : old_(t), orig_(&t) { t = n; }

    ~ScopedSetter() { *orig_ = old_; }

private:
    T old_;
    T *orig_;
};

using ScopedICSetter = ScopedSetter<TrackableObjectReference<InputContext>>;

class EventWatcher {
public:
    EventWatcher(std::string functionName,
                 std::unique_ptr<HandlerTableEntry<EventHandler>> handler)
        : functionName_(std::move(functionName)), handler_(std::move(handler)) {
    }
    FCITX_INLINE_DEFINE_DEFAULT_DTOR_AND_MOVE(EventWatcher);

    const auto &function() const { return functionName_; }

private:
    std::string functionName_;
    std::unique_ptr<HandlerTableEntry<EventHandler>> handler_;
};

class Converter {
public:
    Converter(std::string functionName, ScopedConnection connection)
        : functionName_(std::move(functionName)),
          connection_(std::move(connection)) {}
    FCITX_INLINE_DEFINE_DEFAULT_DTOR_AND_MOVE(Converter);

    const auto &function() const { return functionName_; }

private:
    std::string functionName_;
    ScopedConnection connection_;
};

class LuaAddonState {
public:
    LuaAddonState(const std::string &name, const std::string &library,
                  AddonManager *manager);

    operator lua_State *() { return state_.get(); }

    RawConfig invokeLuaFunction(InputContext *ic, const std::string &name,
                                const RawConfig &config);

private:
    InputContext *currentInputContext() { return inputContext_.get(); }

    DEFINE_LUA_FUNCTION(version);
    DEFINE_LUA_FUNCTION(lastCommit);
    DEFINE_LUA_FUNCTION(splitString);
    DEFINE_LUA_FUNCTION(log);
    DEFINE_LUA_FUNCTION(watchEvent);
    DEFINE_LUA_FUNCTION(unwatchEvent);
    DEFINE_LUA_FUNCTION(currentInputMethod);
    DEFINE_LUA_FUNCTION(addConverter);
    DEFINE_LUA_FUNCTION(removeConverter);
    DEFINE_LUA_FUNCTION(addQuickPhraseHandler);
    DEFINE_LUA_FUNCTION(removeQuickPhraseHandler);
    DEFINE_LUA_FUNCTION(commitString);
    DEFINE_LUA_FUNCTION(standardPathLocate);
    DEFINE_LUA_FUNCTION(UTF16ToUTF8)
    DEFINE_LUA_FUNCTION(UTF8ToUTF16)

    std::tuple<std::string> versionImpl() { return Instance::version(); }

    std::tuple<std::string> lastCommitImpl() { return lastCommit_; }
    std::tuple<> logImpl(const char *msg);
    std::tuple<int> watchEventImpl(const char *event, const char *function);
    std::tuple<> unwatchEventImpl(int id);
    std::tuple<std::string> currentInputMethodImpl();

    std::tuple<int> addConverterImpl(const char *function);
    std::tuple<> removeConverterImpl(int id);

    std::tuple<int> addQuickPhraseHandlerImpl(const char *function);
    std::tuple<> removeQuickPhraseHandlerImpl(int id);

    std::tuple<std::vector<std::string>> splitStringImpl(const char *str,
                                                         const char *delim) {
        return stringutils::split(str, delim);
    }

    std::tuple<std::string> UTF8ToUTF16Impl(const char *str);
    std::tuple<std::string> UTF16ToUTF8Impl(const char *str);

    std::tuple<std::vector<std::string>>
    standardPathLocateImpl(int type, const char *name, const char *suffix);

    std::tuple<> commitStringImpl(const char *str);
    FCITX_ADDON_DEPENDENCY_LOADER(quickphrase, instance_->addonManager());

    bool handleQuickPhrase(const std::string &input,
                           const QuickPhraseAddCandidateCallback &callback);
    Instance *instance_;
    std::unique_ptr<lua_State, decltype(&lua_close)> state_;
    TrackableObjectReference<InputContext> inputContext_;

    std::unordered_map<int, EventWatcher> eventHandler_;
    std::unordered_map<int, Converter> converter_;
    std::map<int, std::string> quickphraseHandler_;

    std::unique_ptr<HandlerTableEntry<QuickPhraseProviderCallback>>
        quickphraseCallback_;
    std::unique_ptr<HandlerTableEntry<EventHandler>> commitHandler_;

    int currentId_ = 0;
    std::string lastCommit_;
};

} // namespace fcitx

#endif // _FCITX5_LUA_ADDONLOADER_LUAADDONSTATE_H_
