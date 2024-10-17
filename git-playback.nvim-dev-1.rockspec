package = "git-playback.nvim"
version = "dev-1"
source = {
   url = "*** please add URL for source tarball, zip or repository here ***"
}
description = {
   homepage = "*** please enter a project homepage ***",
   license = "*** please specify a license ***"
}
build = {
   type = "builtin",
   modules = {
      ["git-playback.git"] = "lua/git-playback/git.lua",
      types = "lua/types.lua"
   }
}
