#include "br_text_renderer.h"
#include "br_shaders.h"
#include "br_pp.h"
#include "br_da.h"

#include "external/stb_rect_pack.h"

#ifdef __GNUC__
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wsign-conversion"
#  pragma GCC diagnostic ignored "-Wconversion"
#endif

#define STB_TRUETYPE_IMPLEMENTATION
#include "external/stb_truetype.h"

#define STB_DS_IMPLEMENTATION
#include "external/stb_ds.h"

#ifdef __GNUC__
#  pragma GCC diagnostic pop
#endif

#include "raylib.h"

#include <stdlib.h>
#include <stdio.h>
#include <float.h>

#define IMG_SIZE 512
#define CHARS 30

typedef struct {
  char key;
  stbtt_packedchar value;
} key_to_packedchar_t;

typedef struct {
  int key;
  key_to_packedchar_t* value;
} size_to_font;

typedef struct {
  int size;
  char ch;
} char_sz;

typedef struct {
  char_sz key;
  unsigned char value; 
} to_bake_t;

typedef struct br_text_renderer_t {
  br_shader_font_t** shader;
  size_to_font* sizes;
  to_bake_t* to_bake;
  unsigned char* bitmap_pixels;
  int bitmap_pixels_len;
  int bitmap_pixels_height;
  int bitmap_pixels_width;
  unsigned int bitmap_texture_id;
  stbtt_pack_context pack_cntx;
  unsigned char const* font_data;

  struct {stbtt_aligned_quad* arr; size_t len, cap; } tmp_quads;
} br_text_renderer_t;

br_text_renderer_t* br_text_renderer_malloc(int bitmap_width, int bitmap_height, unsigned char const* font_data, br_shader_font_t** shader) {
  size_t total_mem = sizeof(br_text_renderer_t) + (size_t)bitmap_width * (size_t)bitmap_height;
  struct tmp {
    br_text_renderer_t r;
    unsigned char data[];
  };
  
  struct tmp* t = BR_MALLOC(total_mem);
  t->r = (br_text_renderer_t) {
    .shader = shader,
    .sizes = NULL,
    .to_bake = NULL,
    .bitmap_pixels = t->data,
    .bitmap_pixels_len = bitmap_width * bitmap_height,
    .bitmap_pixels_width = bitmap_width,
    .bitmap_pixels_height = bitmap_height,
    .bitmap_texture_id = 0,
    .pack_cntx = {0},
    .font_data = font_data
  };
  int res = stbtt_PackBegin(&t->r.pack_cntx, t->r.bitmap_pixels, bitmap_width, bitmap_height, 0, 1, NULL);
  stbtt_PackSetOversampling(&t->r.pack_cntx, 2, 2);
  if (res == 0) fprintf(stderr, "Failed to pack begin\n");
  return &t->r;
}

void br_text_renderer_free(br_text_renderer_t* r) {
  stbds_hmfree(r->to_bake);
  for (long i = 0; i < stbds_hmlen(r->sizes); ++i) {
    stbds_hmfree(r->sizes[i].value);
  }
  stbds_hmfree(r->sizes);
  stbtt_PackEnd(&r->pack_cntx);
  rlUnloadTexture((*r->shader)->uvs.atlas_uv);
  br_da_free(r->tmp_quads);
  BR_FREE(r);
}

static int br_text_renderer_sort_by_size(const void* s1, const void* s2) {
  to_bake_t a = *(to_bake_t*)s1, b = *(to_bake_t*)s2;
  if (a.key.size < b.key.size) return -1;
  else if (a.key.size > b.key.size) return 1;
  else if (a.key.ch < b.key.ch) return -1;
  else if (a.key.ch > b.key.ch) return 1;
  else return 0;
}

void br_text_renderer_dump(br_text_renderer_t* r) {
  stbtt_packedchar charz[256] = {0};
  long hm_len = stbds_hmlen(r->to_bake);
  if (hm_len > 0) {
    qsort(r->to_bake, (size_t)hm_len, sizeof(r->to_bake[0]), br_text_renderer_sort_by_size);
    int old_size = r->to_bake[0].key.size;
    int pack_from = 0;

    for (int i = 1; i < hm_len; ++i) {
      int new_size = r->to_bake[i].key.size;
      int p_len = r->to_bake[i].key.ch - r->to_bake[i - 1].key.ch;
      if (new_size == old_size && p_len < 10) continue;
      p_len = 1 + r->to_bake[i - 1].key.ch - r->to_bake[pack_from].key.ch;
      stbtt_PackFontRange(&r->pack_cntx, r->font_data, 0, (float)old_size, r->to_bake[pack_from].key.ch, p_len, &charz[0]);
      long k = stbds_hmgeti(r->sizes, old_size);
      if (k == -1) {
        stbds_hmput(r->sizes, old_size, NULL);
        k = stbds_hmgeti(r->sizes, old_size);
      }
      for (int j = 0; j < p_len; ++j) {
        stbds_hmput(r->sizes[k].value, (char)(r->to_bake[pack_from].key.ch + j), charz[j]);
      }
      old_size = new_size;
      pack_from = i;
    }
    if (pack_from < hm_len) {
      int s_len = 1 + r->to_bake[hm_len - 1].key.ch - r->to_bake[pack_from].key.ch;
      stbtt_PackFontRange(&r->pack_cntx, r->font_data, 0, (float)old_size, r->to_bake[pack_from].key.ch, s_len, &charz[0]);
      long k = stbds_hmgeti(r->sizes, old_size);
      if (k == -1) {
        stbds_hmput(r->sizes, old_size, NULL);
        k = stbds_hmgeti(r->sizes, old_size);
      }
      for (int j = 0; j < s_len; ++j) {
        stbds_hmput(r->sizes[k].value, r->to_bake[pack_from].key.ch + (char)j, charz[j]);
      }
    }

    stbds_hmfree(r->to_bake);
    rlUnloadTexture(r->bitmap_texture_id);
    r->bitmap_texture_id = rlLoadTexture(r->bitmap_pixels, r->bitmap_pixels_width, r->bitmap_pixels_height, RL_PIXELFORMAT_UNCOMPRESSED_GRAYSCALE, 1);
    Texture tex = {
        .id = r->bitmap_texture_id,
        .width = r->bitmap_pixels_width,
        .height = r->bitmap_pixels_height,
        .mipmaps = 1,
        .format = RL_PIXELFORMAT_UNCOMPRESSED_GRAYSCALE
    };
    DrawTexture(tex, 10000, 10000, WHITE);
  }
  br_shader_font_t* simp = *r->shader;
  simp->uvs.resolution_uv = (Vector2) { (float)GetScreenWidth(), (float)GetScreenHeight() };
  simp->uvs.atlas_uv = r->bitmap_texture_id;
  simp->uvs.sub_pix_aa_map_uv = (Vector3) { -1, 0, 1};
  simp->uvs.sub_pix_aa_scale_uv = 0.2f;
#define GL_SRC_ALPHA 0x0302
#define GL_DST_ALPHA 0x0304
#define GL_MAX 0x8008
  rlSetBlendMode(BLEND_CUSTOM);
  rlSetBlendFactors(GL_DST_ALPHA, GL_DST_ALPHA, GL_MAX); // Set blending mode factor and equation (using OpenGL factors)
  br_shader_font_draw(*r->shader);
  simp->len = 0;
}

static void br_text_draw_quad(Vector4* v, int* len, float x0, float y0, float s0, float t0,
                                                    float x1, float y1, float s1, float t1) {
  v[(*len)++] = (Vector4) { x0, y0, s0, t0 };
  v[(*len)++] = (Vector4) { x0, y1, s0, t1 };
  v[(*len)++] = (Vector4) { x1, y1, s1, t1 };
  v[(*len)++] = (Vector4) { x0, y0, s0, t0 };
  v[(*len)++] = (Vector4) { x1, y1, s1, t1 };
  v[(*len)++] = (Vector4) { x1, y0, s1, t0 };
}

br_text_renderer_extent_t br_text_renderer_push(br_text_renderer_t* r, float x, float y, int font_size, br_color_t color, const char* text) {
  return br_text_renderer_push2(r, x, y, font_size, color, br_strv_from_c_str(text), br_text_renderer_ancor_left_up);
}

br_text_renderer_extent_t br_text_renderer_push_strv(br_text_renderer_t* r, float x, float y, int font_size, br_color_t color, br_strv_t text) {
  return br_text_renderer_push2(r, x, y, font_size, color, text, br_text_renderer_ancor_left_up);
}

br_text_renderer_extent_t br_text_renderer_push2(br_text_renderer_t* r, float x, float y, int font_size, br_color_t color, br_strv_t text, br_text_renderer_ancor_t ancor) {
  Vector2 loc = { x, y };
  long size_index = stbds_hmgeti(r->sizes, font_size);
  float og_x = loc.x;
  br_shader_font_t* simp = *r->shader;
  int len_pos = simp->len * 3;
  Vector4* pos = (Vector4*)simp->pos_vbo;
  Vector4* colors = (Vector4*)simp->color_vbo;
  r->tmp_quads.len = 0;

  if (size_index == -1) {
    for (size_t i = 0; i < text.len; ++i) {
      stbds_hmput(r->to_bake, ((char_sz){ .size = font_size, .ch = text.str[i] }), 0);
    }
  } else {
    size_to_font s = r->sizes[size_index];

    for (size_t i = 0; i < text.len; ++i) {
      long char_index = stbds_hmgeti(s.value, text.str[i]);
      if (text.str[i] == '\n') {
        loc.y += (float)font_size * 1.1f;
        loc.x = og_x;
        continue;
      }
      if (text.str[i] == '\r') continue;
      if (char_index == -1) {
        stbds_hmput(r->to_bake, ((char_sz){ .size = font_size, .ch = text.str[i] }), 0);
      } else {
        stbtt_aligned_quad q;
        stbtt_packedchar ch = s.value[char_index].value;
        stbtt_GetPackedQuad(&ch, r->bitmap_pixels_width, r->bitmap_pixels_height, 0, &loc.x, &loc.y, &q, false);
        br_da_push(r->tmp_quads, q);
      }
    }
  }
#define MMIN(x, y) (x < y ? x : y)
#define MMAX(x, y) (x > y ? x : y)
  float min_y = FLT_MAX, max_y = FLT_MIN, min_x = FLT_MAX, max_x = FLT_MIN;
  for (size_t i = 0; i < r->tmp_quads.len; ++i) {
    min_y = MMIN(r->tmp_quads.arr[i].y0, min_y);
    min_x = MMIN(r->tmp_quads.arr[i].x0, min_x);
    max_y = MMAX(r->tmp_quads.arr[i].y1, max_y);
    max_x = MMAX(r->tmp_quads.arr[i].x1, max_x);
  }
#undef MMAX
#undef MMIN
  float y_off = 0.f;
  float x_off = 0.f;
  if (ancor & br_text_renderer_ancor_y_up)         y_off = min_y - y;
  else if (ancor & br_text_renderer_ancor_y_mid)   y_off = (max_y + min_y) * 0.5f - y;
  else if (ancor & br_text_renderer_ancor_y_down)  y_off = max_y - y;
  if (ancor & br_text_renderer_ancor_x_left)       x_off = min_x - x;
  else if (ancor & br_text_renderer_ancor_x_mid)   x_off = (max_x + min_x) * 0.5f - x;
  else if (ancor & br_text_renderer_ancor_x_right) x_off = max_x - x;
  for (size_t i = 0; i < r->tmp_quads.len; ++i) {
    if (simp->len * 3 >= simp->cap) {
      br_text_renderer_dump(r);
      len_pos = 0;
      pos = (void*)simp->pos_vbo;
      colors = (Vector4*)simp->color_vbo;
    }
    for (int j = 0; j < 6; ++j) colors[len_pos + j] = BR_COLOR_TO_VEC4(color);
    br_text_draw_quad(pos, &len_pos,
        r->tmp_quads.arr[i].x0 - x_off, 
        r->tmp_quads.arr[i].y0 - y_off,
        r->tmp_quads.arr[i].s0,
        r->tmp_quads.arr[i].t0,
        r->tmp_quads.arr[i].x1 - x_off, 
        r->tmp_quads.arr[i].y1 - y_off,
        r->tmp_quads.arr[i].s1,
        r->tmp_quads.arr[i].t1);
    simp->len += 2;
  }
  return (br_text_renderer_extent_t){ min_x - x_off, min_y - y_off, max_x - x_off, max_y - y_off };
}

// gcc -fsanitize=address -ggdb main.c -lm -lraylib && ./a.out
// gcc -O3 main.c -lm -lraylib && ./a.out
// C:\cygwin64\bin\gcc.exe -O3 main.c -lm
// clang -L .\external\lib -lraylib main.c
