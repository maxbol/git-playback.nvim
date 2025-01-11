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
  type = "make",
  build_variables = {
    CFLAGS = "$(CFLAGS)",
    LIBFLAG = "$(LIBFLAG)",
    -- LUA_LIBDIR = "$(LUA_LIBDIR)",
    LUA_BINDIR = "$(LUA_BINDIR)",
    -- LUA_INCDIR = "$(LUA_INCDIR)",
    LUA = "$(LUA)",
    OPTS = "-DLUA_OUT",
  },
  install_variables = {
    INST_PREFIX = "$(PREFIX)",
    INST_BINDIR = "$(BINDIR)",
    INST_LIBDIR = "$(LIBDIR)",
    INST_LUADIR = "$(LUADIR)",
    INST_CONFDIR = "$(CONFDIR)",
  },
}
