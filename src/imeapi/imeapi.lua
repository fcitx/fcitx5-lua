local fcitx = require("fcitx")

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

local function callImeApiCallback(fcitx_result, func, leading, input)
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
        callImeApiCallback(fcitx_result, commands[command].func, commands[command].leading, string.sub(input, 3))
        return fcitx_result
    end
    local fcitx_result = {}
    for _, trigger in ipairs(triggers) do
        if checkInputStringMatches(input, trigger.input_trigger_strings) then
            callImeApiCallback(fcitx_result, trigger.func, nil, input)
        end
    end
    return fcitx_result
end

local function registerQuickPhrase()
    if state.quickphrase == nil then
        state.quickphrase = fcitx.addQuickPhraseHandler("handleQuickPhrase")
    end
end

function ime.register_command(command_name, lua_function_name, description, leading, help)
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

function ime.register_trigger(lua_function_name, description, input_trigger_strings, candidate_trigger_strings)
    registerQuickPhrase()
    table.insert(triggers, {func = lua_function_name, description = description, input_trigger_strings = input_trigger_strings, candidate_trigger_strings = candidate_trigger_strings})
end

function ime.register_converter(lua_function_name, description)
    fcitx.addConverter(lua_function_name)
end

files = fcitx.standardPathLocate(fcitx.StandardPath.PkgData, "lua/imeapi/extensions", ".lua")
for _, file in ipairs(files) do
    fcitx.log("Loading imeapi extension: " .. file)
    f = assert(loadfile(file))
    f()
end
