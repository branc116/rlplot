#include "src/br_da.h"
#include "src/br_plotter.h"
#include "src/br_gui_internal.h"
#include "src/br_shaders.h"
#include "src/br_smol_mesh.h"
#include "src/br_text_renderer.h"

#define PORTABLEGL_IMPLEMENTATION
#include "external/portablegl.h"


#ifndef NUMBER_OF_STEPS
#define NUMBER_OF_STEPS 100
#endif


void br_gui_init_specifics_gui(br_plotter_t* br) {
  br_plot_t plot = { 
    .kind = br_plot_kind_2d,
    .graph_screen_rect = BR_EXTENTI(0, 0, 1280, 720),
    .dd = (br_plot_2d_t) {
      .zoom = BR_VEC2(1, 1),
      .offset = {0},
    }
  };
  br_da_push_t(int, br->plots, plot);
}

void br_plotter_draw(br_plotter_t* br) {
  br_plot_t* plot = &br->plots.arr[0];
  br_plotter_begin_drawing(br);
  help_draw_fps(br->text, 0, 0);
  draw_grid_numbers(br->text, plot);
  smol_mesh_grid_draw(plot, &br->shaders);
  br_datas_draw(br->groups, plot, &br->shaders);
  br_plotter_end_drawing(br);
}

void br_plot_screenshot(br_text_renderer_t* text, br_plot_t* br, br_shaders_t* shaders, br_datas_t groups, const char* path) {
  (void)text; (void)br; (void)path; (void)groups; (void)shaders;
  LOGI("Grabbing screenshot in headless, NOP\n");
  return;
}
