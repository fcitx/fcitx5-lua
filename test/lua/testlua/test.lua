--
-- SPDX-FileCopyrightText: 2020 Weng Xuetian <wengxt@gmail.com>
--
-- SPDX-License-Identifier: LGPL-2.1-or-later
--
local fcitx = require("fcitx")

fcitx.log("ABCD");

fcitx.watchEvent(fcitx.EventType.KeyEvent, "key_logger")
fcitx.addConverter("convert")

function key_logger(sym, state, release)
    if state == fcitx.KeyState.Ctrl then
        print(fcitx.currentInputMethod())
    end
    return false
end

function convert(str)
    print("Convert called")
    str = string.gsub(str, "([abc])", string.upper)
    return str
end

function testProgram()
    return fcitx.currentProgram()
end

function testInputMethod()
    return fcitx.currentInputMethod()
end

function testInvoke(config)
    if type(config) == "string" then
        assert(config == "ABC")
        return "DEF"
    end
    print(fcitx.dump(config))
    config["B"] = {
        C = "5",
        D = "6",
        E = {
            F = "7",
            G = "8",
        }
    }
    return config
end

function testUtf16Conversion(str)
    fcitx.log(str);
    return fcitx.UTF8ToUTF16(str)
end

function testUtf8Conversion(str)
    return fcitx.UTF16ToUTF8(str)
end
