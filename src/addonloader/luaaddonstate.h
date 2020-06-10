/*
 * SPDX-FileCopyrightText: 2020~2020 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX5_LUA_ADDONLOADER_LUAADDONSTATE_H_
#define _FCITX5_LUA_ADDONLOADER_LUAADDONSTATE_H_

#include "luahelper.h"
#include "luastate.h"
#include <fcitx/addoninfo.h>
#include <fcitx/addonmanager.h>
#include <fcitx/instance.h>
#include <quickphrase_public.h>
///
// @module fcitx

namespace fcitx {

#define DEFINE_LUA_FUNCTION(FUNCTION_NAME)                                     \
    static int FUNCTION_NAME(lua_State *lua) {                                 \
        auto state = GetLuaAddonState(lua);                                    \
        auto args = LuaCheckArgument(state->state_.get(),                      \
                                     &LuaAddonState::FUNCTION_NAME##Impl);     \
        try {                                                                  \
            auto fn = std::mem_fn(&LuaAddonState::FUNCTION_NAME##Impl);        \
            auto combined_args = std::tuple_cat(std::make_tuple(state), args); \
            return LuaReturn(state->state_.get(),                              \
                             fcitx::callWithTuple(fn, combined_args));         \
        } catch (const std::exception &e) {                                    \
            return state->state_.get()->luaL_error(e.what());                  \
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
    LuaAddonState(Library &luaLibrary, const std::string &name,
                  const std::string &library, AddonManager *manager);

    operator LuaState *() { return state_.get(); }

    RawConfig invokeLuaFunction(InputContext *ic, const std::string &name,
                                const RawConfig &config);

private:
    InputContext *currentInputContext() { return inputContext_.get(); }

    /// Get the version of fcitx.
    // @function version
    // @treturn string The version of fcitx.
    DEFINE_LUA_FUNCTION(version);
    /// Get the last committed string.
    // @function lastCommit
    // @treturn string The last commit string from fcitx.
    DEFINE_LUA_FUNCTION(lastCommit);
    /// a helper function to split the string by delimiter.
    // @function splitString
    // @string str string to be split.
    // @string delim string used as delimiter.
    // @treturn table An array of string split by delimiter, empty string will
    // be skipped.
    DEFINE_LUA_FUNCTION(splitString);
    /// a helper function to send Debug level log to fcitx.
    // @function log
    // @string str log string.
    DEFINE_LUA_FUNCTION(log);
    /// Watch for a event from fcitx.
    // @function watchEvent
    // @fcitx.EventType event Event Type.
    // @string function the function name.
    // @return A unique integer identifier.
    DEFINE_LUA_FUNCTION(watchEvent);
    /// Unwatch a certain event.
    // @function unwatchEvent
    // @int id id of the watcher.
    // @see watchEvent
    DEFINE_LUA_FUNCTION(unwatchEvent);
    /// Return the current input method.
    // @function currentInputMethod
    // @treturn string the unique string of current input method.
    DEFINE_LUA_FUNCTION(currentInputMethod);
    /// Add a string converter for commiting string.
    // @function addConverter
    // @string function the function name.
    // @treturn int A unique integer identifier.
    DEFINE_LUA_FUNCTION(addConverter);
    /// Remove a converter.
    // @function removeConverter
    // @int id id of this converter.
    // @see addConverter
    DEFINE_LUA_FUNCTION(removeConverter);
    /// Add a quick phrase handler.
    // @function addQuickPhraseHandler
    // @string function the function name.
    // @treturn int A unique integer identifier.
    DEFINE_LUA_FUNCTION(addQuickPhraseHandler);
    /// Remove a quickphrase handler.
    // @function removeQuickPhraseHandler
    // @int id id of this handler.
    // @see addQuickPhraseHandler
    DEFINE_LUA_FUNCTION(removeQuickPhraseHandler);
    /// Commit string to current input context.
    // @function commitString
    // @string str string to be commit to input context.
    DEFINE_LUA_FUNCTION(commitString);
    /// Locate all files with given path and suffix.
    // @function standardPathLocate
    // @int type
    // @string path
    // @string suffix
    // @treturn table A table of full file name.
    // @see StandardPath
    DEFINE_LUA_FUNCTION(standardPathLocate);
    /// Helper function to convert UTF16 string to UTF8.
    // @function UTF16ToUTF8
    // @string str UTF16 string.
    // @treturn string UTF8 string or empty string if it fails.
    DEFINE_LUA_FUNCTION(UTF16ToUTF8)
    /// Helper function to convert UTF8 string to UTF16.
    // @function UTF8ToUTF16
    // @string str UTF8 string.
    // @treturn string UTF16 string or empty string if it fails.
    DEFINE_LUA_FUNCTION(UTF8ToUTF16)

    template <typename T>
    std::unique_ptr<HandlerTableEntry<EventHandler>> watchEvent(
        EventType type, int id,
        std::function<int(std::unique_ptr<LuaState> &, T &)> pushArguments =
            nullptr,
        std::function<void(std::unique_ptr<LuaState> &, T &)>
            handleReturnValue = nullptr);

    std::tuple<std::string> versionImpl() { return Instance::version(); }

    std::tuple<std::string> lastCommitImpl() { return lastCommit_; }
    std::tuple<> logImpl(const char *msg);
    std::tuple<int> watchEventImpl(int eventType, const char *function);
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

    bool handleQuickPhrase(InputContext *ic, const std::string &input,
                           const QuickPhraseAddCandidateCallback &callback);
    Instance *instance_;
    std::unique_ptr<LuaState> state_;
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
