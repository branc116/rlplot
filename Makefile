RL= raylib/src
CCFLAGS= -DRELEASE -I$(RL) -Wpedantic -Wextra -Os -flto=auto
#raylib configs
CCFLAGS+= -DEXTERNAL_CONFIG_FLAGS=1 \
  -DSUPPORT_MODULE_RSHAPES=1 \
  -DSUPPORT_MODULE_RTEXTURES=1 \
  -DSUPPORT_MODULE_RTEXT=1 \
  -DSUPPORT_MODULE_RMODELS=1 \
  -DSUPPORT_SSH_KEYBOARD_RPI=1 \
  -DMAX_FILEPATH_CAPACITY=64 \
  -DRL_DEFAULT_BATCH_BUFFERS=1 \
  -DRL_MAX_MATRIX_STACK_SIZE=2 \
  -DSUPPORT_FILEFORMAT_FNT=1 \
  -DSUPPORT_FILEFORMAT_TTF=1 \
  -DMAX_MATERIAL_MAPS=0 \
  -DMAX_MESH_VERTEX_BUFFERS=7 \
	-DSUPPORT_TEXT_MANIPULATION=1 \
	-DSUPPORT_DEFAULT_FONT=1 \
	-DSUPPORT_TRACELOG=1

RAYLIB_SOURCE= $(RL)/rmodels.c $(RL)/rshapes.c $(RL)/rtext.c $(RL)/rtextures.c $(RL)/utils.c $(RL)/rcore.c

CCFLAGS_LINUX= -Wl,-z,now -DLINUX -DPLATFORM_DESKTOP $(CCFLAGS)
SHADERS= SHADER_GRID_FS:src/desktop/shaders/grid.fs SHADER_LINE_FS:src/desktop/shaders/line.fs SHADER_LINE_VS:src/desktop/shaders/line.vs SHADER_QUAD_FS:src/desktop/shaders/quad.fs SHADER_QUAD_VS:src/desktop/shaders/quad.vs SHADER_FONT_SDF:src/desktop/shaders/sdf_font.fs
SOURCE= $(RAYLIB_SOURCE) \
        ./src/desktop/linux/read_input.c ./src/desktop/linux/refresh_shaders.c src/smol_mesh.c src/main.c src/points_group.c src/graph.c src/q.c src/read_input.c src/resampling.c src/ui.c src/help.c

CCFLAGS_DBG = -I$(RL) -Wconversion -Wall -Wpedantic -Wextra -g -DLINUX -DPLATFORM_DESKTOP -pg -fsanitize=address -fsanitize=leak \
							-fsanitize=undefined -fsanitize=bounds-strict -fsanitize=signed-integer-overflow \
							-fsanitize=integer-divide-by-zero -fsanitize=shift -fsanitize=float-divide-by-zero -fsanitize=float-cast-overflow -DUNIT_TEST
SOURCE_DBG= src/desktop/linux/refresh_shaders.c ./src/desktop/linux/read_input.c src/smol_mesh.c src/main.c src/points_group.c src/graph.c src/q.c src/read_input.c src/resampling.c src/ui.c src/help.c

SOURCE_WEB= $(RL)/rmodels.c $(RL)/rshapes.c $(RL)/rtext.c $(RL)/rtextures.c $(RL)/utils.c $(RL)/rcore.c \
        src/web/refresh_shaders.c src/smol_mesh.c src/main.c src/points_group.c src/graph.c src/q.c src/web/read_input.c src/resampling.c src/ui.c src/help.c
SHADERS_WEB= SHADER_GRID_FS:src/web/shaders/grid.fs SHADER_LINE_FS:src/web/shaders/line.fs SHADER_LINE_VS:src/web/shaders/line.vs SHADER_QUAD_VS:src/web/shaders/quad.vs SHADER_QUAD_FS:src/web/shaders/quad.fs SHADER_FONT_SDF:src/web/shaders/sdf_font.fs
CCFLAGS_WASM= -DGRAPHICS_API_OPENGL_ES2 -DPLATFORM_WEB --memory-init-file 1 --closure 1 -s WASM_BIGINT -s ENVIRONMENT=web -sALLOW_MEMORY_GROWTH -s USE_GLFW=3 -s ASYNCIFY $(CCFLAGS)

# If emscript is not in this location call: make EMSCRIPTEN=<path to your EMSCRIPTEN> ...
EMSCRIPTEN?=/usr/lib/emscripten

OBJS= $(patsubst %.c, build/%.o, $(SOURCE))
OBJSDIR= $(sort $(dir $(OBJS)))

bin/rlplot: $(SOURCE) src/shaders.h src/plotter.h src/default_font.h
	$(foreach var,$(OBJSDIR), test -d $(var) || mkdir -p $(var);)
	$(foreach var,$(SOURCE), gcc $(CCFLAGS_LINUX) -c -o $(patsubst %.c, build/%.o, $(var)) $(var) && ) echo "OK"
	test -d bin || mkdir bin;
	gcc $(CCFLAGS_LINUX) -o bin/rlplot $(OBJS) -lm -lglfw

bin/rlplot_dbg: $(SOURCE_DBG) src/plotter.h src/default_font.h
	test -d bin || mkdir bin;
	gcc $(CCFLAGS_DBG) -o bin/rlplot_dbg $(SOURCE_DBG) -lraylib -lm -lglfw

bin/font_export: $(RAYLIB_SOURCE) tools/font_export.c
	test -d bin || mkdir bin
	gcc -Wl,-z,now -O3 -I$(RL) -DLINUX -DPLATFORM_DESKTOP -o bin/font_export $(RAYLIB_SOURCE) tools/font_export.c -lm -lglfw

.PHONY: test
test: bin/rlplot_dbg
	./bin/rlplot_dbg --unittest

src/shaders.h: ./src/desktop/shaders/line.vs ./src/desktop/shaders/line.fs ./src/desktop/shaders/grid.fs ./src/desktop/shaders/quad.fs ./src/desktop/shaders/quad.vs ./src/desktop/shaders/sdf_font.fs
	# This will break if there are `"` characters in shaders
	echo "// This file is autogenerated!\n" > src/shaders.h;
	$(foreach s, $(SHADERS), echo '#define $(word 1, $(subst :, ,$(s))) \' >> src/shaders.h && \
		                       cat "$(word 2, $(subst :, ,$(s)))" | sed 's/\(.*\)/"\1\\n"\\/' >> src/shaders.h  && \
													 echo "" >> src/shaders.h &&) echo "DONE with shaders";

src/default_font.h: bin/font_export fonts/PlayfairDisplayRegular-ywLOY.ttf
	bin/font_export fonts/PlayfairDisplayRegular-ywLOY.ttf > src/default_font.h

www/index.wasm: $(SOURCE_WEB) src/plotter.h src/shaders_web.h
	test -f $(EMSCRIPTEN)/emcc || test -f $(which emcc) || (echo "Set EMSCRIPTEN variable (to directory where emcc is located) if you want to build for web" && kill -1)
	test -d www || mkdir www;
	$(EMSCRIPTEN)/emcc --shell-file=src/web/minshell.html  $(CCFLAGS_WASM) -o www/index.html $(SOURCE_WEB) || \
		emcc --shell-file=src/web/minshell.html  $(CCFLAGS_WASM) -o www/index.html $(SOURCE_WEB)


src/shaders_web.h: src/web/shaders/grid.fs src/web/shaders/line.fs src/web/shaders/line.vs src/web/shaders/sdf_font.fs
	# This will break if there are `"` characters in shaders
	echo "// This file is autogenerated!\n" > src/shaders_web.h;
	$(foreach s, $(SHADERS_WEB), echo '#define $(word 1, $(subst :, ,$(s))) \' >> src/shaders_web.h && \
		                       cat "$(word 2, $(subst :, ,$(s)))" | sed 's/\(.*\)/"\1\\n"\\/' >> src/shaders_web.h  && \
													 echo "" >> src/shaders_web.h &&) echo "DONE with shaders";

.PHONY: install
install: bin/rlplot
	install bin/rlplot /bin/rlplot

.PHONY: clean
clean:
	test -d build &&  rm build -rdf || echo "done"
	test -d bin &&  rm bin -rdf || echo "done"
	test -d www &&  rm www -rdf || echo "done"
	test -d zig-cache && rm zig-cache -rdf || echo "done"
	test -d zig-out && rm zig-out -rdf || echo "done"
	test -f src/shaders.h && rm src/shaders.h || echo "done"
	test -f src/shaders_web.h && rm src/shaders_web.h || echo "done"
	test -f src/default_font.h && rm src/default_font.h || echo "done"

.PHONY: bench
bench:
	gcc $(CCFLAGS_DBG) -o bin/bench ./src/benchmark.c ./src/quad_tree.c ./src/smol_mesh.c ./src/graph.c ./src/points_group.c ./src/q.c -lm -lraylib

.PHONY: windows
windows:
	zig build -Dtarget=x86_64-windows-gnu -Doptimize=ReleaseSmall

