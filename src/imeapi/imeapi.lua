--
-- SPDX-FileCopyrightText: 2020 Weng Xuetian <wengxt@gmail.com>
--
-- SPDX-License-Identifier: GPL-2.0-or-later
--
--- Google Pinyin-like api module.
-- @module ime

local fcitx = require("fcitx")

-- ime need to be global.
ime = {}

local state = {}
local commands = {}
local triggers = {}

local function starts_with(str, start)
    return str:sub(1, #start) == start
end

local function ends_with(str, ending)
    return ending == "" or str:sub(-#ending) == ending
end

local function checkInputStringMatches(input, input_trigger_strings)
    if input_trigger_strings == nil or type(input) ~= "string" then
        return false
    end
    for _, str in ipairs(input_trigger_strings) do
        if starts_with(str, "*") then
            return ends_with(input, str:sub(-#str+1))
        elseif ends_with(str, "*") then
            return starts_with(input, str:sub(1, #str-1))
        else
            return input == str
        end
    end
    return false
end

local function callImeApiCallback(fcitx_result, func, input, leading)
    -- Append ime api callback result to fcitx result.
    fcitx.log("quickphrase call " .. func)
    local result = fcitx.call_by_name(func, input)
    if type(result) == 'table' then
        for _, item in ipairs(result) do
            if type(item) == 'table' then
                local suggest = item.suggest
                local help = item.help
                local display
                if help ~= nil and #help > 0 then
                    display = suggest .. " [" .. help .. "]"
                else
                    display = suggest
                end
                table.insert(fcitx_result, {suggest, display, fcitx.QuickPhraseAction.TypeToBuffer})
            else
                table.insert(fcitx_result, {tostring(item), tostring(item), fcitx.QuickPhraseAction.Commit})
            end
        end
    elseif result ~= nil then
        local str = tostring(result)
        if str ~= nil then
            table.insert(fcitx_result, {str, str, fcitx.QuickPhraseAction.Commit})
        end
    end
    -- Add functional dummy candidate.
    if leading == "alpha" then
        table.insert(fcitx_result, {"", "", fcitx.QuickPhraseAction.AlphaSelection})
    elseif leading == "digit" then
        table.insert(fcitx_result, {"", "", fcitx.QuickPhraseAction.DigitSelection})
    elseif leading == "none" then
        table.insert(fcitx_result, {"", "", fcitx.QuickPhraseAction.NoneSelection})
    end
    return fcitx_result
end

function TableConcat(t1,t2)
    for i=1,#t2 do
        t1[#t1+1] = t2[i]
    end
    return t1
end

function handleQuickPhrase(input)
    if #input < 1 then
        return nil
    end
    fcitx.log("quickphrase input " .. input)
    local command = string.sub(input, 1, 2)
    if #input >= 2 and commands[command] ~= nil then
        -- Prevent future handling.
        local fcitx_result = {{"", "", fcitx.QuickPhraseAction.Break}}
        callImeApiCallback(fcitx_result, commands[command].func, string.sub(input, 3), commands[command].leading)
        return fcitx_result
    end
    local fcitx_result = {}
    for _, trigger in ipairs(triggers) do
        if checkInputStringMatches(input, trigger.input_trigger_strings) then
            callImeApiCallback(fcitx_result, trigger.func, input)
        end
    end
    return fcitx_result
end

function candidateTrigger(input)
    if type(input) ~= "string" then
        return nil
    end
    fcitx_result = {}
    for _, trigger in ipairs(triggers) do
        if checkInputStringMatches(input, trigger.candidate_trigger_strings) then
            callImeApiCallback(fcitx_result, trigger.func, input)
        end
    end
    result = {}
    count = 0
    for _, cand in ipairs(fcitx_result) do
        if cand[3] == fcitx.QuickPhraseAction.Commit then
            result[tostring(count)] = cand[1]
            count = count + 1
        end
    end
    result.Length = tostring(count)
    return result
end

local function registerQuickPhrase()
    if state.quickphrase == nil then
        state.quickphrase = fcitx.addQuickPhraseHandler("handleQuickPhrase")
    end
end

---
-- Register a two character command to be used with fcitx Quickphrase.
function ime.register_command(
    command_name, -- Command string, can be only exactly two characters.
    lua_function_name, -- global function name.
    description, -- Description, not used in the implementation.
    leading, -- The candidate selection key, can be either, 'none', 'alpha', 'digit', or omit for digit.
    help -- Long help information, not used by the implementation.
)
    if #command_name ~= 2 then
        fcitx.log("Command need to be length 2")
        return
    end
    if commands[command_name] ~= nil then
        fcitx.log("Already registered command: " .. command_name)
        return
    end
    registerQuickPhrase()
    commands[command_name] = {
        func = lua_function_name,
        leading = leading,
        description = description,
        help = help,
    }
end

---
-- Register a trigger function, to be triggered by input or candidate.
function ime.register_trigger(
    lua_function_name, -- global function name.
    description, -- description, not used by the implementation.
    input_trigger_strings, -- A table of string to match input trigger.
    candidate_trigger_strings -- A table of string ot match candidate.
)
    registerQuickPhrase()
    table.insert(triggers, {func = lua_function_name, description = description, input_trigger_strings = input_trigger_strings, candidate_trigger_strings = candidate_trigger_strings})
end

--- Register a converter
function ime.register_converter(
    lua_function_name, -- global function name.
    description -- description string, not used by the implementation.
)
    fcitx.addConverter(lua_function_name)
end

--- Return version of fcitx.
function ime.get_version()
    return fcitx.version()
end

--- Return the string last being committed.
-- @treturn string
function ime.get_last_commit()
    return fcitx.lastCommit()
end

--- Convert an integer to hex string.
-- @int value
-- @int[opt=0] width
-- @treturn string Hex string.
function ime.int_to_hex_string(
    value,
    width
)
    if width == nil then
        width = 0
    end
    local result = string.format("%x", value)
    return string.rep("0", width - #result) .. result
end

--- Join a string list with separtor.
-- @tab str_list A list of string
-- @string sep A string separtor.
-- @treturn string Joined string.
function ime.join_string(str_list, sep)
    return table.concat(str_list, sep)
end

--- Parse a string that represents a key-to-multiple-value table.
-- @string src_string input string.
-- @string line_sep Line separator.
-- @string key_value_sep Separator between key and values.
-- @string values_sep Separator between different values.
-- @treturn table
function ime.parse_mapping (src_string, line_sep, key_value_sep, values_sep)
    local mapping = {}
    for _, line in ipairs(ime.split_string(src_string, line_sep)) do
        local kv = ime.split_string(line, key_value_sep)
        if #kv == 2 then
            mapping[kv[1]] = ime.split_string(kv[2], values_sep)
        end
    end
    return mapping
end

---
-- Split the string, empty string will be ignored.
--
--   ime.split_string('a,b,c', ',') == {'a', 'b', 'c'}
--
--   ime.split_string('a,b,,c', ',') == {'a', 'b', 'c'}
--
--   ime.split_string('', ',') == {}
--]]
-- @string str input string.
-- @string sep separtor string.
-- @treturn table A list of split string.
function ime.split_string(str, sep)
    return fcitx.splitString(str, sep)
end

--- Trim the white space.
-- @string s
-- @treturn string
function ime.trim_string(s)
    return (s:gsub("^%s*(.-)%s*$", "%1"))
end

--- Trim the white space on the left.
-- @string s
-- @treturn string
function ime.trim_string_left(s)
    return (s:gsub("^%s*", ""))
end

--- Trim the white space on the right.
-- @string s
-- @treturn string
function ime.trim_string_right(s)
    local n = #s
    while n > 0 and s:find("^%s", n) do n = n - 1 end
    return s:sub(1, n)
end

--- Helper function to convert UTF16 string to UTF8.
-- @string str UTF16 string.
-- @treturn string UTF8 string or empty string if it fails.
function ime.utf8_to_utf16 (str)
    return fcitx.UTF8ToUTF16(str)
end

--- Helper function to convert UTF8 string to UTF16.
-- @string str UTF8 string.
-- @treturn string UTF16 string or empty string if it fails.
function ime.utf16_to_utf8 (str)
    return fcitx.UTF16ToUTF8(str)
end

-- Load extensions.
local files = fcitx.standardPathLocate(fcitx.StandardPath.PkgData, "lua/imeapi/extensions", ".lua")
for _, file in ipairs(files) do
    fcitx.log("Loading imeapi extension: " .. file)
    local f = assert(loadfile(file))
    f()
end
