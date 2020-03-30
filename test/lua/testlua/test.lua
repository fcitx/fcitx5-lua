local fcitx = require("fcitx")

fcitx.log("ABCD");

fcitx.watchEvent("KeyEvent", "key_logger")
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
