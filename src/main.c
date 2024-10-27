#include "src/br_plotter.h"
#include "src/br_pp.h"
#include "src/br_q.h"
#include "src/br_permastate.h"
#include "src/br_text_renderer.h"
#include "src/br_help.h"

#include "tracy/TracyC.h"
#include "raylib.h"

void br_gui_init_specifics_gui(br_plotter_t* plotter);
static void* main_gui(void* plotter) {
  br_plotter_t* br = (br_plotter_t*)plotter;
  br_plotter_init_specifics_platform(br);
  while(br->should_close == false) {
    TracyCFrameMark;
    br_plotter_draw(br);
    //br_dagens_handle(&br->groups, &br->dagens, &br->plots, GetTime() + 0.010);
    TracyCFrameMarkStart("plotter_frame_end");
    br_plotter_frame_end(br);
    TracyCFrameMarkEnd("plotter_frame_end");
  }
  br->should_close = true;
  
  return 0;
}

#if !defined(LIB)
#include "br_plot.h"
#include "br_pp.h"

#include "raylib.h"

#define WIDTH 1280
#define HEIGHT 720

int main(void) {
#if !defined(RELEASE)
  SetTraceLogLevel(LOG_ALL);
#else
  SetTraceLogLevel(LOG_ERROR);
#endif
  br_plotter_t* br = br_plotter_malloc();
  if (NULL == br) {
    LOGE("Failed to malloc br plotter, exiting...\n");
    exit(1);
  }
  br_plotter_init(br, true);
  br->height = HEIGHT;
  br->width = WIDTH;
#if BR_HAS_SHADER_RELOAD
  start_refreshing_shaders(br);
#endif
  read_input_start(br);
  SetExitKey(KEY_NULL);
  main_gui(br);

  // CLEAN UP
  read_input_stop();
  br_permastate_save(br);
  br_plotter_free(br);
  BR_FREE(br);
  CloseWindow();
  return 0;
}
#endif

#if defined(__linux__) && !defined(RELEASE)
const char* __asan_default_options(void) {
  return "verbosity=0:"
    "sleep_before_dying=120:"
    "print_scariness=true:"
    "allocator_may_return_null=1:"
    "soft_rss_limit_mb=512222"
    ;
}
#endif

#if defined(LIB)
#include "lib.c"
#endif

