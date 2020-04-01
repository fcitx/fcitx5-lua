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
#include "luaaddon_public.h"
#include "testdir.h"
#include "testfrontend_public.h"
#include "testim_public.h"
#include <fcitx-utils/eventdispatcher.h>
#include <fcitx-utils/log.h>
#include <fcitx-utils/standardpath.h>
#include <fcitx/addonmanager.h>
#include <fcitx/inputmethodmanager.h>
#include <fcitx/instance.h>
#include <iostream>
#include <thread>

using namespace fcitx;

void scheduleEvent(EventDispatcher *dispatcher, Instance *instance) {
    dispatcher->schedule([instance]() {
        auto luaaddonloader = instance->addonManager().addon("luaaddonloader");
        FCITX_ASSERT(luaaddonloader);
        auto luaaddon = instance->addonManager().addon("testlua");
        FCITX_ASSERT(luaaddon);
        auto imeapi = instance->addonManager().addon("imeapi");
        FCITX_ASSERT(imeapi);
    });
    dispatcher->schedule([dispatcher, instance]() {
        instance->inputMethodManager().currentGroup();
        auto testfrontend = instance->addonManager().addon("testfrontend");
        auto testim = instance->addonManager().addon("testim");
        testim->call<ITestIM::setHandler>(
            [](const InputMethodEntry &, KeyEvent &keyEvent) {
                if (keyEvent.key().states() != KeyState::None ||
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
        testfrontend->call<ITestFrontend::keyEvent>(uuid, Key("a"), false);
        testfrontend->call<ITestFrontend::keyEvent>(uuid, Key("b"), false);
        testfrontend->call<ITestFrontend::keyEvent>(uuid, Key("c"), false);
        testfrontend->call<ITestFrontend::keyEvent>(uuid, Key("d"), false);
        testfrontend->call<ITestFrontend::keyEvent>(uuid, Key("Control+space"),
                                                    false);
        testfrontend->call<ITestFrontend::destroyInputContext>(uuid);

        auto luaaddon = instance->addonManager().addon("testlua");
        FCITX_ASSERT(luaaddon);
        RawConfig config;
        config["A"].setValue("5");
        config["A"]["Q"].setValue("4");
        FCITX_INFO() << config;
        auto ret = luaaddon->call<ILuaAddon::invokeLuaFunction>(
            nullptr, "testInvoke", config);
        FCITX_INFO() << ret;
        assert(ret["B"]["E"]["F"].value() == "7");
        RawConfig strConfig;
        strConfig.setValue("ABC");
        ret = luaaddon->call<ILuaAddon::invokeLuaFunction>(
            nullptr, "testInvoke", strConfig);
        assert(ret.value() == "DEF");
        FCITX_INFO() << ret;

        dispatcher->detach();
        instance->exit();
    });
}

void runInstance() {}

int main() {
    setenv("SKIP_FCITX_PATH", "1", 1);
    // Path to library
    setenv("FCITX_ADDON_DIRS",
           stringutils::concat(TESTING_BINARY_DIR "/src/addonloader:",
                               StandardPath::fcitxPath("addondir"))
               .data(),
           1);
    setenv("FCITX_DATA_HOME", "/Invalid/Path", 1);
    setenv("FCITX_CONFIG_HOME", "/Invalid/Path", 1);
    setenv("FCITX_DATA_DIRS",
           stringutils::concat(TESTING_SOURCE_DIR "/test:" TESTING_BINARY_DIR
                                                  "/test:",
                               StandardPath::fcitxPath("pkgdatadir", "testing"))
               .data(),
           1);
    fcitx::Log::setLogRule("default=5,lua=5");
    Instance instance(0, nullptr);
    instance.addonManager().registerDefaultLoader(nullptr);
    EventDispatcher dispatcher;
    dispatcher.attach(&instance.eventLoop());
    std::thread thread(scheduleEvent, &dispatcher, &instance);
    instance.exec();
    thread.join();

    return 0;
}
