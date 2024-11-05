package = "git-playback.nvim"
version = "dev-1"
source = {
  url = "https://github.com/maxbol/git-playback.nvim",
}
description = {
  homepage = "https://github.com/maxbol/git-playback.nvim",
  license = "MIT",
}
build = {
  type = "builtin",
  modules = {
    ["git-playback"] = "lua/git-playback/init.lua",
    ["playback"] = "c/luabridge.c",
  },
}
