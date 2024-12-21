# build
UNITY_ROOT		:= ./Unity
C_SRC					:= ./c
CC		 				:= clang
CFLAGS 				:= -g -O0
CFLAGS_ALL		:= -I$(C_SRC) -I$(UNITY_ROOT)/src -I$(UNITY_ROOT)/extras/fixture/src -Wall -Werror
# OPTS					:= -DENABLE_DEBUG_LOGGING
INCLUDES			:= `pkg-config --cflags lua-5.1 libgit2`
LDFLAGS 			:= `pkg-config --libs libgit2`
LIBFLAG				:= -bundle -undefined dynamic_lookup -all_load
LUA_BINDIR		:= /usr/local/bin
LUA_INCDIR		:= /usr/local/include
VPATH					:= c

# install
INST_PREFIX 	:= ./lua_modules
INST_BINDIR 	:= ./lua_modules/bin
INST_LIBDIR 	:= ./lua_modules/lib/lua/5.1
INST_LUADIR 	:= ./lua_modules/share/lua/5.1
INST_CONFDIR 	:= ./lua_modules/etc

DEPS := arrays.h assert.h constants.h diff.h error.h escapestr.h flags.h log.h luabridge.h patch.h slice.h show.h words.h writestr.h
SRC := out/obj/diff.o out/obj/error.o out/obj/escapestr.o out/obj/flags.o out/obj/patch.o out/obj/slice.o out/obj/show.o out/obj/words.o out/obj/writestr.o 

.PHONY: all clean test install

all: out/lib/playback.so

test: out/test/test_patch out/test/test_diff
	./out/test/test_patch
	./out/test/test_diff

clean:
	rm -rf out/obj/* out/test/* out/lib/* lua_modules/*

install: out/lib/playback.so lua/git-playback/init.lua
	mkdir -p $(INST_BINDIR) $(INST_LIBDIR) $(INST_LUADIR) $(INST_CONFDIR)
	cp out/lib/playback.so $(INST_LIBDIR)
	cp lua/git-playback/init.lua $(INST_LUADIR)/git-playback.lua

out/lib/playback.so: out/obj/luabridge.o $(SRC)
	$(CC) $(LIBFLAG) -o $@ out/obj/luabridge.o $(SRC) $(LDFLAGS)

out/obj/%.o: c/%.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS) $(CFLAGS_ALL) $(OPTS) $(INCLUDES)

out/test/test_%: $(UNITY_ROOT)/src/unity.c c/%_test.c test_runners/%.c $(SRC)
	$(CC) $(CFLAGS) $(CFLAGS_ALL) -o $@ $(UNITY_ROOT)/src/unity.c c/$*_test.c test_runners/$*.c $(SRC) `pkg-config --libs lua-5.1 libgit2`

test_runners/%.c: c/%_test.c
	mkdir -p ./test_runners
	ruby $(UNITY_ROOT)/auto/generate_test_runner.rb $< $@
