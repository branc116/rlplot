# This are the configuration options.
# default configuration (calling make with 0 arguments) call is:
# $ make CONFIG=RELEASE PLATFORM=LINUX GUI=IMGUI TYPE=EXE COMPILER=GCC
# But you can change it by changing one param e.g. use clang:
# $ make COMPILER=CLANG

# DEBUG | RELEASE
CONFIG    ?= RELEASE
# LINUX | WEB | WINDOWS
PLATFORM  ?= LINUX
HEADLESS  ?= NO
# EXE | LIB
TYPE      ?= EXE
# GCC | CLANG ( Only for linux build )
COMPILER  ?= GCC
NATIVE_CC ?= gcc
# YES | NO
COVERAGE  ?= NO
# YES | NO
FUZZ      ?= NO
# YES | NO
TRACY     ?= NO
# YES | NO
LTO       ?= YES
# X11 | WAYLAND
BACKEND   ?= X11
# Only when TYPE=LIB
STATIC   ?= NO

IM                 = external/imgui-docking
SOURCE             = src/main.c           src/help.c       src/data.c        src/smol_mesh.c   src/q.c       src/read_input.c \
										 src/keybindings.c    src/str.c        src/resampling2.c src/graph_utils.c src/shaders.c src/plotter.c    \
										 src/plot.c           src/permastate.c src/filesystem.c  src/gui.c         src/text_renderer.c \
										 src/data_generator.c src/platform.c   src/threads.c     src/gl.c          src/icons.c
COMMONFLAGS        = -I. -Iexternal/glfw/include/ -Iexternal/Tracy -MMD -MP -fvisibility=hidden
WARNING_FLAGS      = -Wconversion -Wall -Wpedantic -Wextra -Wshadow
LD_FLAGS           =

ifeq ($(PLATFORM)_$(COMPILER), LINUX_CLANG)
	ifeq ($(COVERAGE), YES)
		COMMONFLAGS+= -fprofile-instr-generate -fcoverage-mapping -mllvm -runtime-counter-relocation
	endif
	ifeq ($(FUZZ), YES)
		COMMONFLAGS+= -fsanitize=fuzzer
	endif
	WARNING_FLAGS+= -Wno-nested-anon-types -Wno-gnu-anonymous-struct -Wno-newline-eof
	CC= clang
else ifeq ($(PLATFORM)_$(COMPILER), LINUX_GCC)
	CC= gcc
else ifeq ($(PLATFORM)_$(COMPILER), LINUX_COSMO)
	CC= cosmocc
endif

ifeq ($(HEADLESS), YES)
	COMMONFLAGS+= -DNUMBER_OF_STEPS=100 -DHEADLESS
endif

ifeq ($(PLATFORM), LINUX)
	ifeq ($(BACKEND), WAYLAND)
		COMMONFLAGS+= -Iexternal/glfw/include -D_GLFW_WAYLAND
	endif
	SHADERS_HEADER= .generated/shaders.h
	ifeq ($(TYPE), LIB)
		COMMONFLAGS+= -fPIC -DLIB
		LD_FLAGS+= -fPIC -shared
	endif
	LIBS+= -lm
	LD= gcc

else ifeq ($(PLATFORM), WINDOWS)
	BACKEND= GLFW
	# LIBS= -lopengl32 -lgdi32 -lwinmm
	CC= x86_64-w64-mingw32-gcc
	LD= x86_64-w64-mingw32-gcc
	COMMONFLAGS+= -Iexternal/glfw/include -D_WIN32=1 -DWIN32_LEAN_AND_MEAN
	SHADERS_HEADER= .generated/shaders.h
	COMPILER= MINGW

else ifeq ($(PLATFORM), WEB)
	BACKEND= GLFW
	CC= $(EMSCRIPTEN)emcc
	LD= $(EMSCRIPTEN)emcc
	COMMONFLAGS+= -DGRAPHICS_API_OPENGL_ES3=1
	WARNING_FLAGS+= -Wno-nested-anon-types -Wno-gnu-anonymous-struct -Wno-newline-eof
	LD_FLAGS= -sWASM_BIGINT -sENVIRONMENT=web -sALLOW_MEMORY_GROWTH -sUSE_GLFW=3 -sUSE_WEBGL2=1 -sGL_ENABLE_GET_PROC_ADDRESS --shell-file=src/web/minshell.html
	LD_FLAGS+= -sCHECK_NULL_WRITES=0 -sDISABLE_EXCEPTION_THROWING=1 -sFILESYSTEM=0 -sDYNAMIC_EXECUTION=0
	SHADERS_HEADER= .generated/shaders_web.h
	COMPILER= EMCC
	ifeq ($(TYPE), LIB)
		COMMONFLAGS+= -DLIB
		LD_FLAGS+= -sMODULARIZE=1 -sEXPORT_ES6=1
		OUTPUT= $(shell echo 'www/brplot_$(HEADLESS)_$(CONFIG)_lib.js' | tr '[A-Z]' '[a-z]')
	else ifeq ($(TYPE), EXE)
		OUTPUT= $(shell echo 'www/brplot_$(HEADLESS)_$(CONFIG).html' | tr '[A-Z]' '[a-z]')
		LD_FLAGS+= -sASYNCIFY
	else
		echo "Valid TYPE parameter values are LIB, EXE" && exit -1
	endif
else
	echo "Valid PLATFORM parameter values are LINUX, WINDOWS, WEB" && exit -1
endif

ifeq ($(CONFIG), DEBUG)
	COMMONFLAGS+= -ggdb
	SHADERS_HEADER=
	ifeq ($(PLATFORM), LINUX)
		COMMONFLAGS+= -DUNIT_TEST
		SOURCE+= tests/src/math.c
		ifeq ($(COMPILER), GCC)
		  COMMONFLAGS+= -fsanitize=bounds-strict
		endif
		ifeq ($(COVERAGE), NO)
			LD_FLAGS+= -rdynamic
			ifeq ($(TYPE), EXE)
				COMMONFLAGS+= -fpie \
				 -fsanitize=address -fsanitize=leak \
				 -fsanitize=undefined -fsanitize=signed-integer-overflow \
				 -fsanitize=integer-divide-by-zero -fsanitize=shift -fsanitize=float-divide-by-zero -fsanitize=float-cast-overflow
			endif
			COMMONFLAGS+= -pg
		endif
	endif
else ifeq ($(CONFIG), RELEASE)
	COMMONFLAGS+= -fdata-sections -ffunction-sections -Os -DRELEASE=1
	LD_FLAGS+= -fdata-sections -ffunction-sections -Wl,--gc-sections
	ifeq ($(PATFORM)_$(LTO), LINUX_YES)
		LD_FLAGS+= -flto=auto
	endif
else
	$(error BadCONFIG)
endif

ifeq ($(TRACY), YES)
	COMMONFLAGS+= -DTRACY_ENABLE=1
	LD_FLAGS+= -ltracy
endif

CXXFLAGS= $(COMMONFLAGS) -fno-exceptions -std=gnu++17
CCFLAGS= $(COMMONFLAGS) -std=gnu11
ifeq ($(TYPE), EXE)
	PREFIX_BUILD= $(shell echo 'build/$(PLATFORM)/$(CONFIG)/$(HEADLESS)/$(BACKEND)/$(COMPILER)' | tr '[A-Z]' '[a-z]')
	OUTPUT?= $(shell echo 'bin/brplot_$(HEADLESS)_$(BACKEND)_$(PLATFORM)_$(CONFIG)_$(COMPILER)' | tr '[A-Z]' '[a-z]')
else
	ifeq ($(STATIC), YES)
		PREFIX_BUILD= $(shell echo 'build/slib/$(PLATFORM)/$(CONFIG)/$(HEADLESS)/$(COMPILER)' | tr '[A-Z]' '[a-z]')
		OUTPUT= bin/$(shell echo 'bin/libbrplot_$(HEADLESS)_$(PLATFORM)_$(CONFIG)_$(COMPILER).a' | tr '[A-Z]' '[a-z]')
	else
		PREFIX_BUILD= $(shell echo 'build/lib/$(PLATFORM)/$(CONFIG)/$(HEADLESS)/$(COMPILER)' | tr '[A-Z]' '[a-z]')
		OUTPUT= $(shell echo 'bin/libbrplot_$(HEADLESS)_$(PLATFORM)_$(CONFIG)_$(COMPILER).so' | tr '[A-Z]' '[a-z]')
	endif
endif

OBJS= $(patsubst %.c, $(PREFIX_BUILD)/%.o, $(SOURCE))
MAKE_INCLUDES= $(patsubst %.o, %.d, $(OBJS))
NOBS= $(patsubst %.o, %.nob.dir, $(OBJS))
NOBS+= bin/.nob.dir

$(OUTPUT):

ifeq ($(STATIC), YES)
%.a: $(OBJS)
	ar rcs $@ $^
else
$(OUTPUT): $(OBJS)
	$(LD) $(COMMONFLAGS) $(LD_FLAGS) -o $@ $(OBJS) $(LIBS)
ifeq ($(TYPE), EXE)
	ln -fs $@ brplot
else
	ln -fs $@ libbrplot.so
endif
endif

$(PREFIX_BUILD)/src/%.o: src/%.c
	$(CC) $(CCFLAGS) $(WARNING_FLAGS) -c -o $@ $<

$(PREFIX_BUILD)/%.o: %.c
	$(CC) $(CCFLAGS) -c -o $@ $<

$(OBJS): $(NOBS)

src/icons.c: .generated/icons.h .generated/icons.c
src/help.c: .generated/default_font.h
src/shaders.c: $(SHADERS_HEADER)

.PHONY: clean
clean:
	test -d build &&  rm build -rdf || echo "done"
	test -d bin &&  rm bin -rdf || echo "done"
	test -d www &&  rm www -rdf || echo "done"
	test -d zig-cache && rm zig-cache -rdf || echo "done"
	test -d zig-out && rm zig-out -rdf || echo "done"
	test -f src/misc/shaders.h && rm src/misc/shaders.h || echo "done"
	test -f src/misc/shaders_web.h && rm src/misc/shaders_web.h || echo "done"
	test -f src/misc/default_font.h && rm src/misc/default_font.h || echo "done"

.PHONY: fuzz
fuzz:
	make CONFIG=DEBUG HEADLESS=YES && \
	cat /dev/random | ./bin/brplot_headless_linux_debug_gcc > /dev/null && echo "Fuzz test OK"

.PHONY: test
test:
	make HEADLESS=YES CONFIG=DEBUG && \
	./bin/brplot_headless_linux_debug_gcc --unittest

.PHONY: test-gdb
test-gdb:
	make HEADLESS=YES CONFIG=DEBUG && \
	gdb -ex "r --unittest" ./bin/brplot_headless_linux_debug_gcc --tui

.PHONY: npm-imgui
npm-imgui:
	echo "TODO"
	exit 1
	make GUI=IMGUI CONFIG=RELEASE TYPE=LIB PLATFORM=WEB && \
	cp ./www/brplot_imgui_release_lib.js packages/npm/brplot.js && \
	cp ./www/brplot_imgui_release_lib.wasm packages/npm && \
	  ((cd packages/npm && \
	   npm publish || cd ../..) && cd ../..)

.generated/default_font.h: bin/font_bake content/PlayfairDisplayRegular-ywLOY.ttf
	test -d .generated || mkdir .generated
	bin/font_bake content/PlayfairDisplayRegular-ywLOY.ttf > .generated/default_font.h

bin/font_bake: tools/font_bake.c $(NOBS)
	$(NATIVE_CC) -O3 -o bin/font_bake tools/font_bake.c

bin/shaders_bake: tools/shaders_bake.c src/br_shaders.h src/str.c $(NOBS)
	$(NATIVE_CC) -I. -O3 -o bin/shaders_bake src/str.c tools/shaders_bake.c

$(SHADERS_HEADER): ./src/shaders/* bin/shaders_bake
	bin/shaders_bake $(PLATFORM) > $(SHADERS_HEADER)

.generated/icons.h .generated/icons.c: bin/pack_icons content/*.png
	bin/pack_icons

bin/pack_icons: tools/pack_icons.c $(NOBS)
	$(NATIVE_CC) -I. -O0 -o bin/pack_icons tools/pack_icons.c -lm

.PHONY: bench
bench: bin/bench
	date >> bench.txt
	./bin/bench >> bench.txt
	cat bench.txt

SOURCE_BENCH= ./src/misc/benchmark.c ./src/resampling2.c ./src/smol_mesh.c ./src/shaders.c ./src/plotter.c ./src/help.c ./src/gui.c ./src/data.c ./src/str.c ./src/plot.c ./src/q.c ./src/keybindings.c ./src/graph_utils.c ./src/data_generator.c ./src/platform.c  ./src/permastate.c ./src/filesystem.c
OBJSA_BENCH= $(patsubst %.cpp, $(PREFIX_BUILD)/%.o, $(SOURCE_BENCH))
OBJS_BENCH+= $(patsubst %.c, $(PREFIX_BUILD)/%.o, $(OBJSA_BENCH))

bin/bench: $(OBJS_BENCH) $(NOBS)
	$(CXX) $(LD_FLAGS) -o bin/bench $(COMMONFLAGS) $(LIBS) $(OBJS_BENCH)

COMPILE_FLAGS_JSONA= $(patsubst %.cpp, $(PREFIX_BUILD)/%.json, $(SOURCE))
COMPILE_FLAGS_JSON= $(patsubst %.c, $(PREFIX_BUILD)/%.json, $(COMPILE_FLAGS_JSONA))

PWD= $(shell pwd)

compile_commands.json: $(COMPILE_FLAGS_JSON)
	echo "[" > compile_commands.json
	cat $(COMPILE_FLAGS_JSON) >> compile_commands.json
	echo "]" >> compile_commands.json

$(PREFIX_BUILD)/src/%.json:src/%.c
	echo '{' > $@ && \
  echo '"directory": "$(PWD)",' >> $@ && \
  echo '"command": "$(CC) $(CCFLAGS) $(WARNING_FLAGS) -c $<",' >> $@ && \
  echo '"file": "$<"' >> $@ && \
	echo '},' >> $@

$(PREFIX_BUILD)/src/%.json:src/%.cpp
	echo '{' > $@ && \
  echo '"directory": "$(PWD)",' >> $@ && \
  echo '"command": "$(CXX) $(CXXFLAGS) $(WARNING_FLAGS) -c $<",' >> $@ && \
  echo '"file": "$<"' >> $@ && \
	echo '},' >> $@

$(PREFIX_BUILD)/%.json:%.c
	echo '{' > $@ && \
  echo '"directory": "$(PWD)",' >> $@ && \
  echo '"command": "$(CC) $(CCFLAGS) -c $<",' >> $@ && \
  echo '"file": "$<"' >> $@ && \
	echo '},' >> $@

$(PREFIX_BUILD)/%.json:%.cpp
	echo '{' > $@ && \
  echo '"directory": "$(PWD)",' >> $@ && \
  echo '"command": "$(CXX) $(CXXFLAGS) -c $<",' >> $@ && \
  echo '"file": "$<"' >> $@ && \
	echo '},' >> $@

-include $(MAKE_INCLUDES)

%nob.dir:
	mkdir -p $(dir $@)
	touch $@
