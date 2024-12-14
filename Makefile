# build
CC		 				:= clang
CFLAGS 				:= -g -O0
CFLAGS_ALL		:= -I./c -Wall -Werror
OPTS					:= -DENABLE_DEBUG_LOGGING
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
OBJ_C_TEST := c/test.o c/diff.o c/error.o c/escapestr.o c/flags.o c/patch.o c/slice.o c/show.o c/words.o c/writestr.o
OBJ_LUABRIDGE := c/luabridge.o c/assert.o c/diff.o c/escapestr.o c/patch.o c/slice.o c/show.o c/words.o c/writestr.o

.PHONY: all clean test install

all: c_test playback.so
test: c_test
	./c_test
clean:
	rm -f c/*.o playback.so c_test
	rm -rf c/test.dSYM
	rm -rf lua_modules

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS) $(CFLAGS_ALL) $(OPTS) $(INCLUDES)

c_test: $(OBJ_C_TEST)
	$(CC) $(CFLAGS) $(CFLAGS_ALL) -o c_test $(OBJ_C_TEST) `pkg-config --libs lua-5.1 libgit2`

playback.so: $(OBJ_LUABRIDGE)
	$(CC) $(LIBFLAG) -o playback.so $(OBJ_LUABRIDGE) $(LDFLAGS)

install: playback.so lua/git-playback/init.lua
	cp playback.so $(INST_LIBDIR)
	cp lua/git-playback/init.lua $(INST_LUADIR)/git-playback.lua
