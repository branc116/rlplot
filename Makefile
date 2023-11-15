RL= raylib/src
# DEBUG | RELEASE
CONFIG?= DEBUG
# LINUX | WEB
PLATFORM?= LINUX
# IMGUI | RAYLIB | HEADLESS
GUI?= IMGUI

RAYLIB_SOURCES= $(RL)/rmodels.c $(RL)/rshapes.c $(RL)/rtext.c $(RL)/rtextures.c $(RL)/utils.c $(RL)/rcore.c
ADDITIONAL_HEADERS= src/misc/default_font.h

COMMONFLAGS= -I$(RL) -I./imgui -I./imgui/backends -I.

ifeq ($(GUI), IMGUI)
	SOURCE= imgui/imgui.cpp imgui/imgui_draw.cpp imgui/imgui_tables.cpp \
				  imgui/imgui_widgets.cpp imgui/backends/imgui_impl_glfw.cpp imgui/backends/imgui_impl_opengl3.cpp \
				  src/imgui/gui.cpp src/imgui/imgui_extensions.cpp $(RAYLIB_SOURCES)
	COMMONFLAGS+= -DIMGUI
else ifeq ($(GUI), RAYLIB)
	SOURCE= src/raylib/gui.c src/raylib/ui.c $(RAYLIB_SOURCES)
else ifeq ($(GUI), HEADLESS)
	SOURCE= src/headless/raylib_headless.c src/headless/gui.c
	PLATFORM= LINUX
	COMMONFLAGS+= -DNUMBER_OF_STEPS=100
else
	echo "Valid GUI parameters are IMGUI, RAYLIB, HEADLESS" && exit -1
endif
	
SOURCE+= src/main.c src/help.c \
				 src/points_group.c src/resampling.c src/smol_mesh.c src/q.c \
				 src/read_input.c src/memory.cpp src/gui.c src/keybindings.c

ifeq ($(PLATFORM), LINUX)
	LIBS= `pkg-config --static --libs glfw3` -lGL
	CXX= g++
	CC= gcc 
	COMMONFLAGS+= -DLINUX -DPLATFORM_DESKTOP
	SOURCE+= src/desktop/linux/read_input.c
	SHADERS_HEADER= src/misc/shaders.h
	SHADERS_LIST= src/desktop/shaders/grid.fs src/desktop/shaders/line.fs src/desktop/shaders/line.vs src/desktop/shaders/quad.vs src/desktop/shaders/quad.fs

else ifeq ($(PLATFORM), WINDOWS)
	LIBS= -lopengl32 -lgdi32 -lwinmm
	CXX= x86_64-w64-mingw32-g++
	CC= x86_64-w64-mingw32-gcc
	COMMONFLAGS+= -Iraylib/src/external/glfw/include -DWINDOWS -DPLATFORM_DESKTOP -D_WIN32=1 \
								-DSUPPORT_TEXT_MANIPULATION=1 \
								-DSUPPORT_DEFAULT_FONT=1

	SOURCE+= $(RL)/rglfw.c src/desktop/win/read_input.c
	SHADERS_HEADER= src/misc/shaders.h
	SHADERS_LIST= src/desktop/shaders/grid.fs src/desktop/shaders/line.fs src/desktop/shaders/line.vs src/desktop/shaders/quad.vs src/desktop/shaders/quad.fs

else ifeq ($(PLATFORM), WEB)
	CXX= $(EMSCRIPTEN)em++
	CC= $(EMSCRIPTEN)emcc
	COMMONFLAGS+= -DGRAPHICS_API_OPENGL_ES2 -DPLATFORM_WEB --memory-init-file 1 --closure 1  -s "EXPORTED_RUNTIME_METHODS=['FS']" -s FORCE_FILESYSTEM -s WASM_BIGINT -s ENVIRONMENT=web -sALLOW_MEMORY_GROWTH -s USE_GLFW=3 -s ASYNCIFY --shell-file=src/web/minshell.html
	SOURCE+= src/web/read_input.c
	SHADERS_LIST= src/web/shaders/grid.fs src/web/shaders/line.fs src/web/shaders/line.vs src/web/shaders/quad.fs src/web/shaders/quad.vs
	SHADERS_HEADER= src/misc/shaders_web.h
	OUTPUT= www/index.html

else
	echo "Valid PLATFORM parameter values are LINUX, WINDOWS, WEB" && exit -1
endif

ifeq ($(CONFIG), DEBUG)
	COMMONFLAGS+= -g -pg -Wconversion -Wall -Wpedantic -Wextra -DUNIT_TEST
	ifeq ($(PLATFORM), LINUX)
		SOURCE+= src/desktop/linux/refresh_shaders.c
		COMMONFLAGS+= -rdynamic -fpie \
	   -fsanitize=address -fsanitize=leak \
		 -fsanitize=undefined -fsanitize=bounds-strict -fsanitize=signed-integer-overflow \
		 -fsanitize=integer-divide-by-zero -fsanitize=shift -fsanitize=float-divide-by-zero -fsanitize=float-cast-overflow
	endif
	ifeq ($(PLATFORM), WINDOWS)
		SOURCE+= src/desktop/win/refresh_shaders.c
	endif
	ifeq ($(PLATFORM), WEB)
		SOURCE+= src/web/refresh_shaders.c
	endif
	ifeq ($(GUI), IMGUI)
		SOURCE+= imgui/imgui_demo.cpp
		ifeq ($(PLATFORM), LINUX)
			SOURCE+= src/imgui/hotreload.c
		endif
	endif
endif

ifeq ($(CONFIG), RELEASE)
	COMMONFLAGS+= -Os -DRELEASE -DEXTERNAL_CONFIG_FLAGS=1 \
		-DSUPPORT_MODULE_RSHAPES=1 \
		-DSUPPORT_MODULE_RTEXTURES=1 \
		-DSUPPORT_MODULE_RTEXT=1 \
		-DSUPPORT_MODULE_RMODELS=1 \
		-DMAX_FILEPATH_CAPACITY=64 \
		-DRL_DEFAULT_BATCH_BUFFERS=1 \
		-DRL_MAX_MATRIX_STACK_SIZE=2 \
		-DSUPPORT_FILEFORMAT_FNT=1 \
		-DSUPPORT_FILEFORMAT_TTF=1 \
		-DMAX_MATERIAL_MAPS=0 \
		-DSUPPORT_IMAGE_MANIPULATION=1 \
		-DMAX_MESH_VERTEX_BUFFERS=7 \
		-DSUPPORT_IMAGE_EXPORT=1 \
		-DSUPPORT_FILEFORMAT_PNG=1 \
		-DSUPPORT_STANDARD_FILEIO=1 \
		-DIMGUI_DISABLE_DEMO_WINDOWS \
		-DIMGUI_DISABLE_DEBUG_TOOLS
	ADDITIONAL_HEADERS+= $(SHADERS_HEADER)
	ifneq ($(PLATFORM), WINDOWS)
		COMMONFLAGS+= -flto=auto
	endif
endif

PREFIX_BUILD= $(shell echo 'build/$(PLATFORM)/$(CONFIG)' | tr '[A-Z]' '[a-z]')
OBJSA= $(patsubst %.cpp, $(PREFIX_BUILD)/%.o, $(SOURCE))
OBJS+= $(patsubst %.c, $(PREFIX_BUILD)/%.o, $(OBJSA))
CXXFLAGS= $(COMMONFLAGS)
CCFLAGS= $(COMMONFLAGS)
OUTPUT?= $(shell echo 'bin/brplot_$(GUI)_$(PLATFORM)_$(CONFIG)' | tr '[A-Z]' '[a-z]')

OBJSDIR= $(sort $(dir $(OBJS)))
$(shell $(foreach var,$(OBJSDIR), test -d $(var) || mkdir -p $(var);))
$(shell test -d $(dir $(OUTPUT)) || mkdir $(dir $(OUTPUT)))
$(shell test -d bin || mkdir bin)

$(OUTPUT): $(ADDITIONAL_HEADERS) $(OBJS)
	$(CXX) $(COMMONFLAGS) $(MY_COMMONFLAGS) -o $@ $(LIBS) $(OBJS) $(LIBS)

bin/upper: tools/upper.cpp
	g++ -O3 -o bin/upper tools/upper.cpp

bin/lower: tools/lower.cpp
	g++ -O3 -o bin/lower tools/lower.cpp

src/misc/default_font.h: bin/font_export fonts/PlayfairDisplayRegular-ywLOY.ttf
	bin/font_export fonts/PlayfairDisplayRegular-ywLOY.ttf > src/misc/default_font.h

bin/font_export: tools/font_export.c
	gcc -o bin/font_export tools/font_export.c -lm

$(SHADERS_HEADER): $(SHADERS_LIST) bin/upper bin/lower
	echo "" > $(SHADERS_HEADER)
	echo "#pragma once" >> $(SHADERS_HEADER)
	echo "//This file is autogenerated" >> $(SHADERS_HEADER)
	$(foreach s, $(SHADERS_LIST), echo "#define" | bin/lower >> $(SHADERS_HEADER) && \
																echo 'SHADER_$(word 4, $(subst /, , $(s))) \' | sed 's/\./_/' | bin/upper >> $(SHADERS_HEADER) && \
																cat $(s) | sed 's/\(.*\)/"\1\\n" \\/' >> $(SHADERS_HEADER) && \
																echo "" >> $(SHADERS_HEADER) && ) echo "OKi"
BR_HEADERS= src/br_plot.h br_gui_internal.h br_help.h

$(PREFIX_BUILD)/src/%.o:src/%.c $(BR_HEADERS) $(ADDITIONAL_HEADERS)
	$(CC) $(CCFLAGS) $(MY_COMMONFLAGS) -c -o $@ $<

$(PREFIX_BUILD)/src/%.o:src/%.cpp $(BR_HEADERS) $(ADDITIONAL_HEADERS)
	$(CC) $(CXXFLAGS) $(MY_COMMONFLAGS) -c -o $@ $<

$(PREFIX_BUILD)/%.o:%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(PREFIX_BUILD)/%.o:%.c
	$(CC) $(CCFLAGS) -c -o $@ $<

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
