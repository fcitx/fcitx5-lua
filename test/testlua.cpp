/*
 * SPDX-FileCopyrightText: 2020~2020 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#include "luaaddon_public.h"
#include "testdir.h"
#include "testfrontend_public.h"
#include "testim_public.h"
#include <fcitx-utils/eventdispatcher.h>
#include <fcitx-utils/log.h>
#include <fcitx-utils/standardpath.h>
#include <fcitx-utils/testing.h>
#include <fcitx/addonmanager.h>
#include <fcitx/inputmethodmanager.h>
#include <fcitx/instance.h>
#include <iostream>
#include <thread>

using namespace fcitx;

void scheduleEvent(EventDispatcher *dispatcher, Instance *instance) {
    dispatcher->schedule([instance]() {
        auto luaaddonloader =
            instance->addonManager().addon("luaaddonloader", true);
        FCITX_ASSERT(luaaddonloader);
        auto luaaddon = instance->addonManager().addon("testlua");
        FCITX_ASSERT(luaaddon);
        auto imeapi = instance->addonManager().addon("imeapi");
        FCITX_ASSERT(imeapi);
    });
    dispatcher->schedule([dispatcher, instance]() {
        // Setup the input method group with two input method
        auto groupName = instance->inputMethodManager().currentGroup();
        InputMethodGroup group(groupName);
        group.inputMethodList().push_back(InputMethodGroupItem("keyboard-us"));
        group.inputMethodList().push_back(InputMethodGroupItem("testim"));
        group.setDefaultInputMethod("testim");
        instance->inputMethodManager().setGroup(group);

        auto testfrontend = instance->addonManager().addon("testfrontend");
        auto testim = instance->addonManager().addon("testim");
        auto luaaddon = instance->addonManager().addon("testlua");
        testim->call<ITestIM::setHandler>(
            [](const InputMethodEntry &, KeyEvent &keyEvent) {
                if (keyEvent.key().states() != KeyState::NoState ||
                    keyEvent.isRelease()) {
                    return;
                }
                auto s = Key::keySymToUTF8(keyEvent.key().sym());
                if (!s.empty()) {
                    keyEvent.inputContext()->commitString(s);
                    keyEvent.filterAndAccept();
                }
            });
        auto uuid =
            testfrontend->call<ITestFrontend::createInputContext>("testapp");
        auto ic = instance->inputContextManager().findByUUID(uuid);
        FCITX_ASSERT(ic);

        // Test with the converter in test.lua
        testfrontend->call<ITestFrontend::keyEvent>(uuid, Key("a"), false);
        testfrontend->call<ITestFrontend::keyEvent>(uuid, Key("b"), false);
        testfrontend->call<ITestFrontend::keyEvent>(uuid, Key("c"), false);
        testfrontend->call<ITestFrontend::keyEvent>(uuid, Key("d"), false);

        // Test lua currentInputMethod
        auto ret = luaaddon->call<ILuaAddon::invokeLuaFunction>(
            ic, "testInputMethod", RawConfig{});
        FCITX_INFO() << ret;
        assert(ret.value() == "keyboard-us");

        testfrontend->call<ITestFrontend::keyEvent>(uuid, Key("Control+space"),
                                                    false);

        // Test lua currentProgram
        ret = luaaddon->call<ILuaAddon::invokeLuaFunction>(ic, "testProgram",
                                                           RawConfig{});
        FCITX_INFO() << ret;
        assert(ret.value() == "testapp");

        // Test lua currentInputMethod after ctrl space
        ret = luaaddon->call<ILuaAddon::invokeLuaFunction>(
            ic, "testInputMethod", RawConfig{});
        FCITX_INFO() << ret;
        assert(ret.value() == "testim");

        // Test with complex value with invokeLuaFunction
        FCITX_ASSERT(luaaddon);
        RawConfig config;
        config["A"].setValue("5");
        config["A"]["Q"].setValue("4");
        FCITX_INFO() << config;
        ret = luaaddon->call<ILuaAddon::invokeLuaFunction>(ic, "testInvoke",
                                                           config);
        FCITX_INFO() << ret;
        assert(ret["B"]["E"]["F"].value() == "7");
        RawConfig strConfig;
        strConfig.setValue("ABC");
        ret = luaaddon->call<ILuaAddon::invokeLuaFunction>(ic, "testInvoke",
                                                           strConfig);
        assert(ret.value() == "DEF");
        FCITX_INFO() << ret;

        dispatcher->detach();
        instance->exit();
    });
}

void runInstance() {}

int main() {
    setupTestingEnvironment(
        TESTING_BINARY_DIR,
        {"src/addonloader", StandardPath::fcitxPath("addondir")},
        {"test", TESTING_SOURCE_DIR "/test",
         StandardPath::fcitxPath("pkgdatadir", "testing")});

    fcitx::Log::setLogRule("default=5,lua=5");
    char arg0[] = "testlua";
    char arg1[] = "--disable=all";
    char arg2[] = "--enable=testim,testfrontend,luaaddonloader,imeapi,testlua";
    char *argv[] = {arg0, arg1, arg2};
    Instance instance(FCITX_ARRAY_SIZE(argv), argv);
    setenv("FCITX_DATA_HOME", "/Invalid/Path", 1);
    setenv("FCITX_CONFIG_HOME", "/Invalid/Path", 1);
    instance.addonManager().registerDefaultLoader(nullptr);
    EventDispatcher dispatcher;
    dispatcher.attach(&instance.eventLoop());
    std::thread thread(scheduleEvent, &dispatcher, &instance);
    instance.exec();
    thread.join();

    return 0;
}
