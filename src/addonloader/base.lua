--
-- SPDX-FileCopyrightText: 2020 Weng Xuetian <wengxt@gmail.com>
--
-- SPDX-License-Identifier: LGPL-2.1-or-later
--
--- Fcitx module
-- @module fcitx
local fcitx = require("fcitx.core")

--- Call a global function by its name.
-- @param function_name name of the function
-- @param ... the arguments forwarded to the function.
-- @return nil if function is not found, or the return value of the function.
function fcitx.call_by_name(function_name, ...)
    if type(_G[function_name]) ~= 'function' then
        return nil
    end
    return _G[function_name](...)
end

--- The lua version of fcitx::KeyState. It represent the value of modifier keys.
-- @table KeyState
local KeyState = {
    None = 0,
    Shift = 1 << 0,
    CapsLock = 1 << 1,
    Ctrl = 1 << 2,
    Alt = 1 << 3,
    NumLock = 1 << 4,
    Mod3 = 1 << 5,
    Super = 1 << 6,
    Mod5 = 1 << 7,
    MousePressed = 1 << 8,
    HandledMask = 1 << 24,
    IgnoredMask = 1 << 25,
    Super2 = 1 << 26,
    Hyper = 1 << 27,
    Meta = 1 << 28,
    UsedMask = 0x5c001fff,
}

KeyState.Mod1 = KeyState.Alt
KeyState.Alt_Shift = KeyState.Alt | KeyState.Shift
KeyState.Ctrl_Shift = KeyState.Ctrl | KeyState.Shift
KeyState.Ctrl_Alt = KeyState.Ctrl | KeyState.Alt
KeyState.Ctrl_Alt_Shift = KeyState.Ctrl | KeyState.Alt | KeyState.Shift
KeyState.Mod2 = KeyState.NumLock
KeyState.Mod4 = KeyState.Super
KeyState.SimpleMask = KeyState.Ctrl_Alt_Shift | KeyState.Super | KeyState.Super2 | KeyState.Hyper | KeyState.Meta

fcitx.KeyState = KeyState

--- The lua version of fcitx::StandardPath::Type. It represent the value of different
-- type of directory.
-- @table StandardPath
fcitx.StandardPath = {
    Config = 0,
    PkgConfig = 1,
    Data = 2,
    Cache = 3,
    Runtime = 4,
    Addon = 5,
    PkgData = 6
}

--- The lua version of fcitx::QuickPhraseAction. It represent the different value
-- that can be returned from quickphrase handler.
-- @table QuickPhraseAction
fcitx.QuickPhraseAction = {
    Break = -1,

    Commit = 0,
    TypeToBuffer = 1,
    DigitSelection = 2,
    AlphaSelection = 3,
    NoneSelection = 4,
    DoNothing = 5,
    AutoCommit = 6,
}

local function dump(o)
   if type(o) == 'table' then
      local s = '{ '
      for k,v in pairs(o) do
         if type(k) ~= 'number' then k = '"'..k..'"' end
         s = s .. '['..k..'] = ' .. dump(v) .. ','
      end
      return s .. '} '
   else
      return tostring(o)
   end
end

fcitx.dump = dump

--- The lua version of fcitx::EventType. It represent the value of different
-- type of events.
-- @table EventType
local EventType = {
    ContextCreated = 0x0001000 | 0x1,
    ContextDestroyed = 0x0001000 | 0x2,
    FocusOut = 0x0001000 | 0x3,
    FocusIn = 0x0001000 | 0x4,
    KeyEvent = 0x0001000 | 0x5,
    SurroundingTextUpdated = 0x0001000 | 0x7,
    CursorRectChanged = 0x0001000 | 0x9,
    SwitchInputMethod = 0x0001000 | 0xA,
    InputMethodActivated = 0x0001000 | 0xB,
    InputMethodDeactivated = 0x0001000 | 0xC,

    CommitString = 0x0002000 | 0x2,
    UpdatePreedit = 0x0002000 | 0x4,
}

fcitx.EventType = EventType

return fcitx
