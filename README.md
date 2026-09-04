fcitx5-lua
=====================================================
Lua support for fcitx.

[![Jenkins Build](https://img.shields.io/jenkins/build?jobUrl=https%3A%2F%2Fjenkins.fcitx-im.org%2Fjob%2Ffcitx5-lua%2F)](https://jenkins.fcitx-im.org/job/fcitx5-lua/)

[Documentation](https://fcitx.github.io/fcitx5-lua/index.html)

It tries to support lua in fcitx in two ways.
1. An addon loader for lua, which supports Type=Lua addon.
2. The googlepinyin api, which is provided by imeapi addon. You may put your
   lua file under $HOME/.local/share/fcitx5/lua/imeapi/extensions to make the
   addon find your scripts.

Cloud Pinyin providers
----------------------

`ime.cloudpinyin_provider_api_version` is `1`.
Extensions may register a provider for the Cloud Pinyin addon's `Lua` backend:

```lua
ime.register_cloudpinyin_provider("example",
    function(context)
        return {
            url = "https://example.invalid/pinyin?text=" ..
                  context.selected .. context.input,
            method = "POST", -- optional; defaults to GET
            headers = { ["Content-Type"] = "application/json" },
            timeout = 5, -- optional; seconds, defaults to 10
            body = context.before or "",
        }
    end,
    function(response)
        if response.status == 200 then
            return {
                text = response.body,
                comment = "Example annotation",
            }
        end
        return nil
    end)
```

The request callback receives a context table. Its `pinyin` field contains full pinyin syllables separated by apostrophes, including converted double pinyin and automatic completion.

`input` is the raw, unselected user input; for example, it is `f` when a user enters `buf` and selects `bu` as `部`.

`selected` contains the text already selected in the current input.

`first` contains the Pinyin engine's preferred candidate before custom, cloud, or Lua candidates are added.

When the frontend supplies surrounding text, `before` and `after` contain the text around the cursor.

`program` is the client program name and `session` is the input context UUID encoded as 32 lowercase hexadecimal characters; both are empty for the legacy request API without an input context.

The request table may specify `timeout` as a whole number of seconds from 1 to 60; it defaults to 10 seconds.

`body` requires a non-GET `method`; a GET request with a non-empty body is rejected.

The response callback receives numeric `status`, `headers`, and `body`, then returns a candidate table or `nil`.

The candidate table requires string `text` and may contain string `comment`, which is displayed as the candidate annotation.

Requests are performed asynchronously by the Cloud Pinyin addon; callbacks must not perform blocking I/O.
This v1 contract will remain compatible: future fields or APIs may be added, but existing fields and callback signatures will not change.

Keep API keys out of scripts and logs; load them from a user-owned file with suitable permissions instead.

Set Cloud Pinyin's `LuaProvider` option to a registered provider name to select it explicitly.
Its default empty value selects the first provider registered by Lua extensions.
