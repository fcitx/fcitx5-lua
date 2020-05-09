--
-- SPDX-FileCopyrightText: 2020 Weng Xuetian <wengxt@gmail.com>
--
-- SPDX-License-Identifier: GPL-2.0-or-later
--
local fcitx = require("fcitx")

print(ime.get_version())
assert(ime.get_version() == fcitx.version())
print(ime.int_to_hex_string(100, 6))
assert(ime.int_to_hex_string(100, 6) == "000064")

local utf16str = ime.utf8_to_utf16("你好")
local utf8str = ime.utf16_to_utf8(utf16str)
assert(utf8str == "你好")

assert(ime.trim_string("   Hello World!   ") == "Hello World!")
print(ime.trim_string_left("   Hello World!   ") == "Hello World!   ")
print(ime.trim_string_right("   Hello World!   ") == "   Hello World!")

tab = ime.split_string("aa..bb..cc", ".");
assert(tab[1] == "aa")
assert(tab[2] == "bb")
assert(tab[3] == "cc")

tab = ime.split_string("aa..bb..cc", "..");
assert(ime.join_string(tab, ", ") == "aa, bb, cc");

assert(ime.join_string({}, "..") == "");
assert(ime.join_string({"aa"}, "  ") == "aa");

_MAPPING_TABLE = [[
a 啊
b 不,吧
c 从,穿,出
]]

_MAPPING = ime.parse_mapping(_MAPPING_TABLE, "\n", " ", ",")
assert(_MAPPING["a"][1] == "啊")
assert(_MAPPING["b"][1] == "不")
assert(_MAPPING["b"][2] == "吧")
assert(_MAPPING["c"][1] == "从")
assert(_MAPPING["c"][2] == "穿")
assert(_MAPPING["c"][3] == "出")
