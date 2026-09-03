--
-- SPDX-FileCopyrightText: 2026 CSSlayer <wengxt@gmail.com>
--
-- SPDX-License-Identifier: LGPL-2.1-or-later
--

assert(ime.register_cloudpinyin_provider(
    "test",
    function(context)
        return {
            url = "https://example.invalid/pinyin",
            method = "POST",
            headers = {
                ["Content-Type"] = "application/json",
                ["X-Program"] = context.program,
                ["X-Session"] = context.session,
            },
            timeout = 5,
            body = table.concat({
                context.pinyin,
                context.input,
                context.selected,
                context.first,
                context.before or "",
                context.after or "",
            }, "|"),
        }
    end,
    function(response)
        if response.status == 200 then
            return {
                text = response.body,
                comment = "test comment",
            }
        end
        return nil
    end
))
