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
