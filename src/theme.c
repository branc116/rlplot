#include "src/br_theme.h"

BR_THREAD_LOCAL br_theme_t br_theme;

void br_theme_dark(void) {
  br_color_t bg = BR_COLOR(0x0f, 0x0f, 0x0f, 0xff);
  br_color_t txt = BR_COLOR(0xe0, 0xe0, 0xe0, 0xff);
  
  br_theme.colors.btn_inactive = br_color_lighter(bg, 0.4f);
  br_theme.colors.btn_hovered = br_color_greener(br_color_lighter(br_theme.colors.btn_inactive, 2.0f), 0.3f);
  br_theme.colors.btn_active = br_color_lighter(br_theme.colors.btn_hovered, 2.0f);
  br_theme.colors.btn_txt_inactive = txt;
  br_theme.colors.btn_txt_hovered = br_color_darker(txt, 0.2f);
  br_theme.colors.btn_txt_active = txt;

  br_theme.colors.plot_bg = br_color_lighter(bg, 0.2f);
  br_theme.colors.plot_menu_color = br_color_lighter(bg, .3f);

  br_theme.colors.grid_nums = txt;
  br_theme.colors.grid_nums_bg = br_theme.colors.plot_bg;
  br_theme.colors.grid_lines = br_color_greener(br_color_darker(txt, 0.7f), 0.02f);

  br_theme.colors.bg = bg;
  br_theme.colors.bg.a = 0x00;
}

void br_theme_light(void) {
  br_color_t bg = BR_COLOR(0xa0, 0xa0, 0xa0, 0xff);
  br_color_t txt = BR_COLOR(0x0f, 0x0f, 0x0f, 0xff);
  
  br_theme.colors.btn_inactive = br_color_darker(bg, 0.4f);
  br_theme.colors.btn_hovered = br_color_greener(br_color_darker(br_theme.colors.btn_inactive, .5f), 0.3f);
  br_theme.colors.btn_active = br_color_darker(br_theme.colors.btn_hovered, .5f);
  br_theme.colors.btn_txt_inactive = txt;
  br_theme.colors.btn_txt_hovered = br_color_lighter(txt, 0.2f);
  br_theme.colors.btn_txt_active = txt;

  br_theme.colors.plot_bg = br_color_darker(bg, .1f);
  br_theme.colors.plot_menu_color = br_color_darker(bg, .2f);

  br_theme.colors.grid_nums = txt;
  br_theme.colors.grid_nums_bg = br_theme.colors.plot_bg;
  br_theme.colors.grid_lines = br_color_greener(br_color_darker(bg, 0.5f), 0.02f);

  br_theme.colors.bg = bg;
  br_theme.colors.bg.a = 0x00;
}
