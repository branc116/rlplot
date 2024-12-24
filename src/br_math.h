#pragma once
#include <math.h>
#include <stddef.h>
#include <float.h>
#include <stdbool.h>

#define BR_VEC2(X, Y) ((br_vec2_t) { .x = (X), .y = (Y) })
#define BR_VEC2I(X, Y) ((br_vec2i_t) { .x = (X), .y = (Y) })
#define BR_VEC2_TOI(V) ((br_vec2i_t) { .x = (V).x, .y = (V).y })
#define BR_VEC2I_SUB(A, B) ((br_vec2i_t) { .x = (A).x - (B).x, .y = (A).y - (B).y })
#define BR_VEC2I_SCALE(V, B) ((br_vec2i_t) { .x = (V).x * (B), .y = (V).y * (B) })
#define BR_SIZE(WIDTH, HEIGHT) ((br_size_t) { .width = (WIDTH), .height = (HEIGHT) })
#define BR_SIZEI(WIDTH, HEIGHT) ((br_sizei_t) { .width = (WIDTH), .height = (HEIGHT) })
#define BR_SIZEI_TOF(SZ) ((br_size_t) { .width = ((SZ).width), .height = ((SZ).height) })
#define BR_EXTENTI_TOF(E) ((br_extent_t) { .arr = { (float)(E).arr[0], (float)(E).arr[1], (float)(E).arr[2], (float)(E).arr[3] } })
#define BR_EXTENT_ASPECT(E) ((E).height / (E).width)
#define BR_EXTENT(X, Y, WIDTH, HEIGHT) (br_extent_t) { .arr = { (X), (Y), (WIDTH), (HEIGHT) } }
#define BR_EXTENTPS(POS, SIZE) (br_extent_t) { .size = (SIZE), .pos = (POS) }
#define BR_EXTENTI(X, Y, WIDTH, HEIGHT) (br_extenti_t) { .arr = { (X), (Y), (WIDTH), (HEIGHT) } }
#define BR_BB(Xm, Ym, XM, YM) (br_bb_t) { .arr = { (Xm), (Ym), (XM), (YM) } }
#define BR_BB_TOEX(BB) (br_extent_t) { .arr = { (BB).min_x, (BB).min_y, (BB).max_x - (BB).min_x, (BB).max_y - (BB).min_y } }
#define BR_VEC4(X, Y, Z, W) ((br_vec4_t) { .x = (X), .y = (Y), .z = (Z), .w = (W) }) 
#define BR_VEC3(X, Y, Z) ((br_vec3_t) { .x = (X), .y = (Y), .z = (Z) }) 
#define BR_VEC_ELS(X) (sizeof((X).arr) / sizeof((X).arr[0]))
#define BR_COLOR_TO4(X)  BR_VEC4(((X).r) / 255.f, ((X).g) / 255.f, ((X).b) / 255.f, ((X).a) / 255.f)
#define BR_COLOR(R, G, B, A) ((br_color_t) { .r = (R), .g = (G), .b = (B), .a = (A) })

#define BR_RED BR_COLOR(200, 10, 10, 255)
#define BR_LIME BR_COLOR(10, 200, 10, 255)
#define BR_ORANGE BR_COLOR(180, 180, 10, 255)
#define BR_WHITE BR_COLOR(180, 180, 180, 255)
#define BR_GREEN BR_COLOR(10, 200, 10, 255)
#define BR_BLUE BR_COLOR(10, 10, 200, 255)
#define BR_LIGHTGRAY BR_COLOR(100, 100, 100, 255)
#define BR_PINK BR_COLOR(150, 80, 80, 255)
#define BR_GOLD BR_COLOR(50, 180, 50, 255)
#define BR_VIOLET BR_COLOR(180, 10, 180, 255)
#define BR_DARKPURPLE BR_COLOR(80, 10, 80, 255)

#define BR_PI 3.14159265358979323846f

typedef struct {
  union {
    struct {
      float x, y;
    };
    float arr[2];
  };
} br_vec2_t;

typedef struct {
  union {
    struct {
      double x, y;
    };
    double arr[2];
  };
} br_vec2d_t;

typedef struct {
  union {
    struct {
      int x, y;
    };
    int arr[2];
  };
} br_vec2i_t;

typedef struct {
  union {
    struct {
      int width, height;
    };
    br_vec2i_t vec;
    int arr[2];
  };
} br_sizei_t;

typedef struct {
  union {
    struct {
      float width, height;
    };
    br_vec2_t vec;
    float arr[2];
  };
} br_size_t;

typedef struct {
  union {
    struct {
      int x, y, width, height;
    };
    struct {
      br_vec2i_t pos;
      br_sizei_t size;
    };
    int arr[4];
  };
} br_extenti_t;

typedef struct {
  union {
    struct {
      float x, y, width, height;
    };
    struct {
      br_vec2_t pos;
      br_size_t size;
    };
    float arr[4];
  };
} br_extent_t;

typedef struct {
  union {
    struct {
      float min_x, min_y, max_x, max_y;
    };
    struct {
      br_vec2_t min;
      br_vec2_t max;
    };
    float arr[4];
  };
} br_bb_t;

typedef struct {
  union {
    struct {
      float x, y, z;
    };
    float arr[3];
  };
}  br_vec3_t;

typedef struct {
  union {
    struct {
      float x, y, z, w;
    };
    struct {
      br_vec2_t xy;
      br_vec2_t zw;
    };
    float arr[4];
  };
}  br_vec4_t;

typedef struct {
  unsigned char r, g, b, a;
} br_color_t;

typedef struct {
  union {
    struct {
      float m0, m4, m8, m12;
      float m1, m5, m9, m13;
      float m2, m6, m10, m14;
      float m3, m7, m11, m15;
    };
    float arr[16];
    br_vec4_t rows[4];
  };
} br_mat_t;

//------------------------vec2------------------------------

static inline br_vec2_t br_vec2i_tof(br_vec2i_t v) {
  return (br_vec2_t) { .x = (float)v.x, .y = (float)v.y };
}

static inline br_vec2i_t br_vec2_toi(br_vec2_t v) {
  return (br_vec2i_t) { .x = (int)v.x, .y = (int)v.y };
}

static inline br_vec2_t br_vec2_scale(br_vec2_t v, float s) {
  return ((br_vec2_t) { .x = v.x * s, .y = v.y * s });
}

static inline br_vec2_t br_vec2_adds(br_vec2_t a, br_size_t b) {
  return BR_VEC2(a.x + b.width, a.y + b.height);
}

static inline br_vec2_t br_vec2_sub(br_vec2_t a, br_vec2_t b) {
  return BR_VEC2(a.x - b.x, a.y - b.y);
}

static inline br_vec2i_t br_vec2i_add(br_vec2i_t a, br_vec2i_t b) {
  return BR_VEC2I(a.x + b.x, a.y + b.y);
}

static inline br_vec2i_t br_vec2i_sub(br_vec2i_t a, br_vec2i_t b) {
  return BR_VEC2I(a.x - b.x, a.y - b.y);
}

static inline float br_vec2_len2(br_vec2_t a) {
  float sum = 0.f;
  for (size_t i = 0; i < BR_VEC_ELS(a); ++i) sum += a.arr[i] * a.arr[i];
  return sum;
}

static inline float br_vec2_len(br_vec2_t a) {
  return sqrtf(br_vec2_len2(a));
}

static inline br_vec2_t br_vec2_mul(br_vec2_t a, br_vec2_t b) {
  return BR_VEC2(a.x * b.x, a.y * b.y);
}

static inline float br_vec2_dist2(br_vec2_t a, br_vec2_t b) {
  float len2 = br_vec2_len2(br_vec2_sub(a, b));
  return len2;
}

static inline br_vec3_t br_vec2_transform_scale(br_vec2_t v, br_mat_t mat) {
  br_vec3_t result = { 0 };
  float x = v.x;
  float y = v.y;
  result.x = (mat.m0*x + mat.m4*y + mat.m12);
  result.y = (mat.m1*x + mat.m5*y + mat.m13);
  result.z = (mat.m2*x + mat.m6*y + mat.m14);
  if (fabsf(result.z) > 0.00001f) {
    result.x /= fabsf(result.z);
    result.y /= fabsf(result.z);
  }
  return result;
}

static inline br_vec2_t br_vec2_stog(br_vec2_t v, br_sizei_t screen) {
  return BR_VEC2(v.x / (float)screen.width * 2.f - 1.f, (1.f - v.y / (float)screen.height) * 2.f - 1.f);
}

//------------------------size------------------------------

static inline br_sizei_t br_sizei_sub(br_sizei_t a, br_sizei_t b) {
  return BR_SIZEI(a.width - b.width, a.height - b.height);
}

//------------------------vec3------------------------------

static inline br_vec3_t br_vec3_add(br_vec3_t a, br_vec3_t b) {
  return BR_VEC3(a.x + b.x, a.y + b.y, a.z + b.z);
}

static inline br_vec3_t br_vec3_sub(br_vec3_t a, br_vec3_t b) {
  return BR_VEC3(a.x - b.x, a.y - b.y, a.z - b.z);
}

static inline br_vec3_t br_vec3_scale(br_vec3_t a, float s) {
  return BR_VEC3(a.x * s, a.y * s, a.z * s);
}

static inline br_vec3_t br_vec3_pow(br_vec3_t a, float exponent) {
  return BR_VEC3(powf(a.x, exponent), powf(a.y, exponent), powf(a.z, exponent));
}

static inline float br_vec3_len2(br_vec3_t a) {
  float sum = 0.f;
  for (size_t i = 0; i < BR_VEC_ELS(a); ++i) sum += a.arr[i] * a.arr[i];
  return sum;
}

static inline float br_vec3_len(br_vec3_t a) {
  return sqrtf(br_vec3_len2(a));
}

static inline float br_vec3_dist2(br_vec3_t a, br_vec3_t b) {
  float len2 = br_vec3_len2(br_vec3_sub(a, b));
  return len2;
}

static inline float br_vec3_dist(br_vec3_t a, br_vec3_t b) {
  return sqrtf(br_vec3_dist2(a, b));
}

static inline br_vec3_t br_vec3_mul(br_vec3_t a, br_vec3_t b) {
  return BR_VEC3(a.x*b.x, a.y*b.y, a.z*b.z);
}

static inline float br_vec3_dot(br_vec3_t a, br_vec3_t b) {
  return a.x*b.x + a.y*b.y + a.z*b.z;
}

static inline br_vec3_t br_vec3_cross(br_vec3_t a, br_vec3_t b) {
  return BR_VEC3(
    a.y*b.z - a.z*b.y,
    a.z*b.x - a.x*b.z,
    a.x*b.y - a.y*b.x);
}

static inline br_vec3_t br_vec3_normalize(br_vec3_t a) {
  float len2 = br_vec3_len2(a);
  if (fabsf(len2) > FLT_EPSILON) {
    float len = sqrtf(len2);
    for (size_t i = 0; i < BR_VEC_ELS(a); ++i) a.arr[i] /= len;
    return a;
  }
  return BR_VEC3(0,0,0);
}

// TODO: This look sus...
static inline br_vec3_t br_vec3_transform_scale(br_vec3_t v, br_mat_t mat) {
  br_vec3_t result = { 0 };

  float x = v.x, y = v.y, z = v.z;

  result.x = mat.m0*x + mat.m4*y + mat.m8*z + mat.m12;
  result.y = mat.m1*x + mat.m5*y + mat.m9*z + mat.m13;
  result.z = mat.m2*x + mat.m6*y + mat.m10*z + mat.m14;
  //float w =  mat.m3*x + mat.m7*y + mat.m11*z + mat.m15;

  if (fabsf(result.z) > 0.00001f) {
    result.x /= fabsf(result.z);
    result.y /= fabsf(result.z);
  }

  return result;
}

static inline float br_vec3_angle(br_vec3_t v1, br_vec3_t v2) {
  float len = br_vec3_len(br_vec3_cross(v1, v2));
  float dot = br_vec3_dot(v1, v2);
  return atan2f(len, dot);
}

static inline br_vec3_t br_vec3_rot(br_vec3_t v, br_vec3_t axis, float angle) { 
  angle *= .5f;
  br_vec3_t w = br_vec3_scale(axis, sinf(angle));
  br_vec3_t wv = br_vec3_cross(w, v);
  br_vec3_t wwv = br_vec3_cross(w, wv);
  wv = br_vec3_scale(wv, cosf(angle) * 2.f);
  v = br_vec3_add(v, wv);
  v = br_vec3_add(v, br_vec3_scale(wwv, 2.f));
  return v;
}

static inline br_vec3_t br_vec3_perpendicular(br_vec3_t v) {
  size_t min = 0;
  float min_val = v.arr[0];
  br_vec3_t ord = { 0 };

  for (size_t i = 1; i < 3; ++i) {
    float c = fabsf(v.arr[i]);
    if (c < min_val) min = i, min_val = c;
  }
  ord.arr[min] = 1;
  return br_vec3_cross(v, ord);
}
// ------------------br_extent_t-----------------

static inline br_vec2_t br_extent_top_right(br_extent_t extent) {
  return BR_VEC2(extent.x + extent.width, extent.y);
}

static inline br_vec2_t br_extent_bottom_left(br_extent_t extent) {
  return BR_VEC2(extent.x, extent.y + extent.height);
}

static inline br_vec2_t br_extent_bottom_right(br_extent_t extent) {
  return BR_VEC2(extent.x + extent.width, extent.y + extent.height);
}

static inline br_vec2_t br_extent_tl(br_extent_t extent, float x, float y) {
  return BR_VEC2(extent.x + x, extent.y + y);
}

static inline br_vec2_t br_extent_tr(br_extent_t extent, float x, float y) {
  return BR_VEC2(extent.x + extent.width - x, extent.y + y);
}

// ------------------br_mat_t--------------------
static inline br_mat_t br_mat_perspective(float fovY, float aspect, float nearPlane, float farPlane) {
  br_mat_t result = { 0 };

  float top = nearPlane*tanf(fovY*0.5f);
  float bottom = -top;
  float right = top*aspect;
  float left = -right;

  // TODO: Check this out..
  // MatrixFrustum(-right, right, -top, top, near, far);
  float rl = (right - left);
  float tb = (top - bottom);
  float fn = (farPlane - nearPlane);

  result.m0 = (nearPlane*2.0f)/rl;
  result.m5 = (nearPlane*2.0f)/tb;
  result.m8 = (right + left)/rl;
  result.m9 = (top + bottom)/tb;
  result.m10 = -(farPlane + nearPlane)/fn;
  result.m11 = -1.0f;
  result.m14 = -(farPlane*nearPlane*2.0f)/fn;

  return result;
}

static inline br_mat_t br_mat_look_at(br_vec3_t eye, br_vec3_t target, br_vec3_t up) {
  br_vec3_t vz = br_vec3_normalize(br_vec3_sub(eye, target));
  br_vec3_t vx = br_vec3_normalize(br_vec3_cross(up, vz));
  br_vec3_t vy = br_vec3_cross(vz, vx);

  return (br_mat_t) { .rows = {
    BR_VEC4(vx.x, vx.y, vx.z, -br_vec3_dot(vx, eye)),
    BR_VEC4(vy.x, vy.y, vy.z, -br_vec3_dot(vy, eye)),
    BR_VEC4(vz.x, vz.y, vz.z, -br_vec3_dot(vz, eye)),
    BR_VEC4(   0,    0,    0,                   1.f)
  }};
}

static inline br_mat_t br_mat_mul(br_mat_t left, br_mat_t right) {
  br_mat_t result = { 0 };

  result.m0 = left.m0*right.m0 + left.m1*right.m4 + left.m2*right.m8 + left.m3*right.m12;
  result.m1 = left.m0*right.m1 + left.m1*right.m5 + left.m2*right.m9 + left.m3*right.m13;
  result.m2 = left.m0*right.m2 + left.m1*right.m6 + left.m2*right.m10 + left.m3*right.m14;
  result.m3 = left.m0*right.m3 + left.m1*right.m7 + left.m2*right.m11 + left.m3*right.m15;
  result.m4 = left.m4*right.m0 + left.m5*right.m4 + left.m6*right.m8 + left.m7*right.m12;
  result.m5 = left.m4*right.m1 + left.m5*right.m5 + left.m6*right.m9 + left.m7*right.m13;
  result.m6 = left.m4*right.m2 + left.m5*right.m6 + left.m6*right.m10 + left.m7*right.m14;
  result.m7 = left.m4*right.m3 + left.m5*right.m7 + left.m6*right.m11 + left.m7*right.m15;
  result.m8 = left.m8*right.m0 + left.m9*right.m4 + left.m10*right.m8 + left.m11*right.m12;
  result.m9 = left.m8*right.m1 + left.m9*right.m5 + left.m10*right.m9 + left.m11*right.m13;
  result.m10 = left.m8*right.m2 + left.m9*right.m6 + left.m10*right.m10 + left.m11*right.m14;
  result.m11 = left.m8*right.m3 + left.m9*right.m7 + left.m10*right.m11 + left.m11*right.m15;
  result.m12 = left.m12*right.m0 + left.m13*right.m4 + left.m14*right.m8 + left.m15*right.m12;
  result.m13 = left.m12*right.m1 + left.m13*right.m5 + left.m14*right.m9 + left.m15*right.m13;
  result.m14 = left.m12*right.m2 + left.m13*right.m6 + left.m14*right.m10 + left.m15*right.m14;
  result.m15 = left.m12*right.m3 + left.m13*right.m7 + left.m14*right.m11 + left.m15*right.m15;

  return result;
}

// collisions
static inline bool br_col_vec2_extent(br_extent_t ex, br_vec2_t point) {
  return (point.x >= ex.x) && (point.x < (ex.x + ex.width)) && (point.y >= ex.y) && (point.y < (ex.y + ex.height));
}

static inline bool br_col_extents(br_extent_t a, br_extent_t b) {
  return ((a.x < (b.x + b.width) && (a.x + a.width) > b.x) &&
          (a.y < (b.y + b.height) && (a.y + a.height) > b.y));
}