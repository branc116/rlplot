#include "br_plot.h"
#include "br_help.h"
#include "stdint.h"
#include "math.h"
#include "src/misc/tests.h"
#include "string.h"
#include "assert.h"

#define RESAMPLING_NODE_MAX_LEN 128

typedef enum {
  resampling2_kind_x,
  resampling2_kind_y,
  resampling2_kind_raw
} resampling2_node_kind_t;

typedef struct {
  float min, max;
  size_t index_start, len;
} resampling2_node_t;

typedef struct {
  size_t index_start, len;
} resampling2_raw_node_t;

typedef struct resampling2_nodes_s {
  resampling2_node_t* arr;
  struct resampling2_nodes_s* parent;
  unsigned int len;
  unsigned int cap;
  bool is_rising, is_falling;
} resampling2_nodes_t;

typedef struct {
  resampling2_node_kind_t kind;
  union {
    resampling2_nodes_t x;
    resampling2_nodes_t y;
    resampling2_raw_node_t raw;
  };
} resampling2_all_roots;

typedef struct resampling2_s {
  resampling2_all_roots* roots;
  size_t roots_len;
  size_t roots_cap;

  resampling2_nodes_t temp_root_x, temp_root_y;
  resampling2_raw_node_t temp_root_raw;
  bool temp_x_valid, temp_y_valid, temp_raw_valid;
} resampling2_t;

void resampling2_nodes_deinit(resampling2_nodes_t* nodes) {
  if (nodes == NULL) return;
  resampling2_nodes_deinit(nodes->parent);
  BR_FREE(nodes->arr);
  BR_FREE(nodes->parent);
}

resampling2_t* resampling2_malloc(void) {
   resampling2_t* r = BR_CALLOC(1, sizeof(resampling2_t));
   r->temp_x_valid = r->temp_y_valid = r->temp_raw_valid = true;
   return r;
}

void resampling2_free(resampling2_t* r) {
  for (size_t i = 0; i < r->roots_len; ++i) {
    if (r->roots[i].kind != resampling2_kind_raw)
    resampling2_nodes_deinit(&r->roots[i].x);
  }
  resampling2_nodes_deinit(&r->temp_root_x);
  resampling2_nodes_deinit(&r->temp_root_y);
  BR_FREE(r->roots);
  BR_FREE(r);
}

void resampling2_push_root(resampling2_t* r, resampling2_all_roots root) {
  if (r->roots_len == 0) {
    r->roots = BR_MALLOC(sizeof(resampling2_all_roots));
    r->roots_cap = 1;
  }
  if (r->roots_len == r->roots_cap) {
    size_t new_cap = r->roots_cap + 2;
    r->roots = BR_REALLOC(r->roots, new_cap * sizeof(resampling2_all_roots));
    r->roots_cap = new_cap;
  }
  r->roots[r->roots_len++] = root;
}

resampling2_nodes_t* resampling2_nodes_get_parent(resampling2_nodes_t* nodes) {
  if (nodes->parent == NULL) {
    nodes->parent = BR_CALLOC(1, sizeof(resampling2_nodes_t));
  }
  return nodes->parent;
}

resampling2_node_t* resampling2_get_last_node(resampling2_nodes_t* nodes) {
  if (nodes->len == 0) {
    nodes->arr = BR_CALLOC(1, sizeof(resampling2_node_t));
    nodes->len = 1;
    nodes->cap = 1;
  }
  return &nodes->arr[nodes->len - 1];
}

static void resampling2_nodes_push_point(resampling2_nodes_t* nodes, size_t index, float p, uint8_t depth) {
  resampling2_node_t* node = resampling2_get_last_node(nodes);
  unsigned int node_i = nodes->len - 1;
  if (node->len == 0) {
    node->min = p;
    node->max = p;
    node->len = 1;
    node->index_start = index;
  } else if (node->len < (size_t)(RESAMPLING_NODE_MAX_LEN * ((size_t)1 << (size_t)2*depth))) {
    node->min = fminf(p, node->min);
    node->max = fmaxf(p, node->max);
    ++node->len;
  } else if (node->len == (size_t)(RESAMPLING_NODE_MAX_LEN * ((size_t)1 << (size_t)2*depth))) {
    unsigned int new_cap = nodes->cap + 2;
    if (nodes->len == nodes->cap) {
      nodes->arr = BR_REALLOC(nodes->arr, sizeof(resampling2_node_t) * (nodes->cap + 2));
      for (unsigned int i = nodes->cap; i < new_cap; ++i) {
        nodes->arr[i] = (resampling2_node_t){0};
      }
      nodes->cap += 2;
    }
    ++nodes->len;
    node = &nodes->arr[node_i];
    if (nodes->parent == NULL) {
      nodes->parent = BR_CALLOC(1, sizeof(resampling2_nodes_t));
      nodes->parent->arr = BR_CALLOC(1, sizeof(resampling2_node_t));
      nodes->parent->len = 1;
      nodes->parent->cap = 1;
      memcpy(nodes->parent->arr, node, sizeof(resampling2_node_t));
      resampling2_nodes_push_point(nodes, index, p, depth);
    } else {
      resampling2_nodes_push_point(nodes, index, p, depth);
    }
    return;
  } else {
    printf("%d\n", *(int*)0);
  }
  if (nodes->parent != NULL) resampling2_nodes_push_point(nodes->parent, index, p, depth + 1);
}

static bool resampling2_add_point_raw(resampling2_raw_node_t* node, size_t index) {
  if (node->len == RESAMPLING_NODE_MAX_LEN) return false;
  if (node->len == 0) node->index_start = index;
  ++node->len;
  return true;
}

static bool resampling2_add_point_y(resampling2_nodes_t* nodes, const points_group_t *pg, size_t index) {
  Vector2 p = pg->points[index];
  if (nodes->len == 0) nodes->is_rising = nodes->is_falling = true;
  if (index == 0) {
    nodes->is_rising = true;
    nodes->is_falling = true;
    resampling2_nodes_push_point(nodes, index, p.x, 0);
    return true;
  } else if (p.y > pg->points[index - 1].y) {
    nodes->is_falling = false;
    if (nodes->is_rising) {
      resampling2_nodes_push_point(nodes, index, p.x, 0);
    } else {
      return false;
    }
  } else if (p.y < pg->points[index - 1].y) {
    nodes->is_rising = false;
    if (nodes->is_falling) {
      resampling2_nodes_push_point(nodes, index, p.x, 0);
    } else {
      return false;
    }
  } else if (p.y == pg->points[index - 1].y) {
      resampling2_nodes_push_point(nodes, index, p.x, 0);
  }
  // point must be nan, so let's just ignore it...
  return true;
}

bool resampling2_add_point_x(resampling2_nodes_t* nodes, const points_group_t *pg, size_t index) {
  Vector2 p = pg->points[index];
  if (nodes->len == 0) nodes->is_rising = nodes->is_falling = true;
  if (index == 0) {
    nodes->is_rising = true;
    nodes->is_falling = true;
    resampling2_nodes_push_point(nodes, index, p.y, 0);
    return true;
  } else if (p.x > pg->points[index - 1].x) {
    nodes->is_falling = false;
    if (nodes->is_rising) {
      resampling2_nodes_push_point(nodes, index, p.y, 0);
    } else {
      return false;
    }
  } else if (p.x < pg->points[index - 1].x) {
    nodes->is_rising = false;
    if (nodes->is_falling) {
      resampling2_nodes_push_point(nodes, index, p.y, 0);
    } else {
      return false;
    }
  } else if (p.x == pg->points[index - 1].x) {
      resampling2_nodes_push_point(nodes, index, p.y, 0);
  }
  // point must be nan, so let's just ignore it...
  return true;
}

void resampling2_add_point(resampling2_t* r, const points_group_t *pg, size_t index) {
  bool was_valid_x = r->temp_x_valid, was_valid_y = r->temp_y_valid, was_valid_raw = r->temp_raw_valid;
  if (was_valid_x)   r->temp_x_valid   = resampling2_add_point_x(&r->temp_root_x, pg, index);
  if (was_valid_y)   r->temp_y_valid   = resampling2_add_point_y(&r->temp_root_y, pg, index);
  if (was_valid_raw) r->temp_raw_valid = resampling2_add_point_raw(&r->temp_root_raw, index);
  if (r->temp_x_valid || r->temp_y_valid || r->temp_raw_valid) return;
  if (was_valid_x) {
    resampling2_push_root(r, (resampling2_all_roots) {.kind = resampling2_kind_x, .x = r->temp_root_x} );
    resampling2_nodes_deinit(&r->temp_root_y);
  } else if (was_valid_y) {
    resampling2_push_root(r, (resampling2_all_roots) {.kind = resampling2_kind_y, .y = r->temp_root_y} );
    resampling2_nodes_deinit(&r->temp_root_x);
  } else if (was_valid_raw) {
    resampling2_push_root(r, (resampling2_all_roots) {.kind = resampling2_kind_raw, .raw = r->temp_root_raw} );
    resampling2_nodes_deinit(&r->temp_root_x);
    resampling2_nodes_deinit(&r->temp_root_y);
  } else printf("%d\n", *(int*)0);
  r->temp_root_x = (resampling2_nodes_t){0};
  r->temp_root_y = (resampling2_nodes_t){0};
  r->temp_root_raw = (resampling2_raw_node_t){0};
  r->temp_raw_valid = r->temp_y_valid = r->temp_x_valid = true;
  resampling2_add_point(r, pg, index);
}

bool resampling2_nodex_inside_rect(resampling2_node_t* node, Vector2* points, Rectangle rect) {
  float firstx = points[node->index_start].x;
  float lastx = points[node->index_start + node->len - 1].x;
  if (lastx < firstx) {
    float tmp = firstx;
    firstx = lastx;
    lastx = tmp;
  }
  if (lastx < rect.x) return false;
  if (firstx > rect.x + rect.width) return false;
  if (node->min > rect.y + rect.height) return false;
  if (node->max < rect.y) return false;
  return true;
}

bool resampling2_nodey_inside_rect(resampling2_node_t* node, Vector2* points, Rectangle rect) {
  float firsty = points[node->index_start].y;
  float lasty = points[node->index_start + node->len - 1].y;
  if (lasty < firsty) {
    float tmp = firsty;
    firsty = lasty;
    lasty = tmp;
  }
  if (lasty < rect.y) return false;
  if (firsty > rect.y + rect.height) return false;
  if (node->min > rect.x + rect.width) return false;
  if (node->max < rect.x) return false;
  return true;
}

void resampling2_draw_raw(resampling2_raw_node_t raw, points_group_t *pg, points_groups_draw_in_t *rdi) {
  smol_mesh_gen_line_strip(rdi->line_mesh, &pg->points[raw.index_start], raw.len, pg->color);
}

void resampling2_draw_x(resampling2_nodes_t* nodes, points_group_t *pg, points_groups_draw_in_t *rdi) {
  //float step = rdi->rect.width / 1024;
  for (size_t j = 0; j < nodes->len - 1; ++j) {
    if (!resampling2_nodex_inside_rect(&nodes->arr[j], pg->points, rdi->rect)) {
      continue;
    }
    //smol_mesh_gen_line_strip(rdi->line_mesh, &pg->points[nodes->arr[j].index_start], nodes->arr[j].len, pg->color);
    resampling2_node_t n = nodes->arr[j];
    Vector2* ps = pg->points;
    Vector2 p1 = { ps[n.index_start].x, n.min };
    Vector2 p2 = { ps[n.index_start].x, n.max };
    smol_mesh_gen_line(rdi->line_mesh, p1, p2, pg->color);
    p1 = (Vector2){ ps[n.index_start].x, n.max };
    p2 = (Vector2){ ps[n.index_start + n.len - 1].x, n.max };
    smol_mesh_gen_line(rdi->line_mesh, p1, p2, pg->color);
    p1 = (Vector2){ ps[n.index_start + n.len - 1].x, n.max };
    p2 = (Vector2){ ps[n.index_start + n.len - 1].x, n.min };
    smol_mesh_gen_line(rdi->line_mesh, p1, p2, pg->color);
  }
}

void resampling2_draw_y(resampling2_nodes_t* nodes, points_group_t *pg, points_groups_draw_in_t *rdi) {
  return;
  //float step = rdi->rect.width / 1024;
  for (size_t j = 0; j < nodes->len; ++j) {
    if (resampling2_nodey_inside_rect(&nodes->arr[j], pg->points, rdi->rect)) {
      smol_mesh_gen_line_strip(rdi->line_mesh, &pg->points[nodes->arr[j].index_start], nodes->arr[j].len, pg->color);
    }
  }
}

void resampling2_draw_r(resampling2_all_roots r, points_group_t *pg, points_groups_draw_in_t *rdi) {
  if (r.kind == resampling2_kind_raw) {
    resampling2_draw_raw(r.raw, pg, rdi);
  } else if (r.kind == resampling2_kind_x) {
    resampling2_draw_x(&r.x, pg, rdi);
  } else if (r.kind == resampling2_kind_y) {
    resampling2_draw_y(&r.y, pg, rdi);
  }
}

void resampling2_draw(resampling2_t *res, points_group_t *pg, points_groups_draw_in_t *rdi) {
  for (size_t i = 0; i < res->roots_len; ++i) {
    resampling2_draw_r(res->roots[i], pg, rdi);
  }
  if (res->temp_x_valid) resampling2_draw_x(&res->temp_root_x, pg, rdi);
  else if (res->temp_y_valid) resampling2_draw_y(&res->temp_root_y, pg, rdi);
  else resampling2_draw_raw(res->temp_root_raw, pg, rdi);
}

static void resampling2_node_debug_print(FILE* file, resampling2_node_t* r, int depth) {
  if (r == NULL) return;
  for (int i = 0; i < depth; ++i) {
    fprintf(file, "  ");
  }
  fprintf(file, "Node len: %lu\n", r->len);
}

static void resampling2_nodes_debug_print(FILE* file, resampling2_nodes_t* r, int depth) {
  if (r == NULL) return;
  for (int i = 0; i < depth; ++i) {
    fprintf(file, "  ");
  }
  fprintf(file, "len: %u, cap: %u, rising: %d, falling: %d\n", r->len, r->cap, r->is_rising, r->is_falling);
  for (unsigned int i = 0; i < r->len; ++i) {
    resampling2_node_debug_print(file, &r->arr[i], depth + 1);
  }
  resampling2_nodes_debug_print(file, r->parent, depth + 1);
}

static void resampling2_raw_node_debug_print(FILE* file, resampling2_raw_node_t* node, int depth) {
  for (int i = 0; i < depth; ++i) {
    fprintf(file, "  ");
  }
  fprintf(file, "Raw node, from: %lu, len: %lu\n", node->index_start, node->len);
}

static void resampling2_all_nodes_debug_print(FILE* file, resampling2_all_roots* r) {
  const char* str;
  if (r->kind == resampling2_kind_x) str = "x";
  if (r->kind == resampling2_kind_y) str = "y";
  if (r->kind == resampling2_kind_raw) str = "raw";
  fprintf(file, "Nodes %s\n", str);
}

static void resampling2_debug_print(FILE* file, resampling2_t* r) {
  fprintf(file, "\nRoots len: %lu, Roots cap: %lu, validxyr = %d,%d,%d\n", r->roots_len, r->roots_cap, r->temp_x_valid, r->temp_y_valid, r->temp_raw_valid);
  for (size_t i = 0; i < r->roots_len; ++i) {
    resampling2_all_nodes_debug_print(file, &r->roots[i]);
  }
  resampling2_nodes_debug_print(file, &r->temp_root_x, 1);
  resampling2_nodes_debug_print(file, &r->temp_root_y, 1);
  resampling2_raw_node_debug_print(file, &r->temp_root_raw, 1);
  return;
}

#define PRINT_ALLOCS(prefix) \
  printf("\n%s ALLOCATIONS: %lu ( %luKB ) | %lu (%luKB)\n", prefix, \
      context.alloc_count, context.alloc_size >> 10, context.alloc_total_count, context.alloc_total_size >> 10);

TEST_CASE(resampling) {
  Vector2 points[] = { {0, 1}, {1, 2}, {2, 4}, {3, 2} };
  points_group_t pg = {
    .len = 4,
    .points = points,
    .resampling = NULL,
    .cap = 4
  };
  resampling2_t* r = resampling2_malloc();
  for (int i = 0; i < 2*1024; ++i) resampling2_add_point(r, &pg, 3);
  resampling2_debug_print(stdout, r);
  printf("\nALLOCATIONS: %lu ( %luKB ) | %lu (%luKB)\n", context.alloc_count, context.alloc_size >> 10, context.alloc_total_count, context.alloc_total_size >> 10);
  resampling2_add_point(r, &pg, 3);
  printf("\nALLOCATIONS: %lu ( %luKB ) | %lu (%luKB)\n", context.alloc_count, context.alloc_size >> 10, context.alloc_total_count, context.alloc_total_size >> 10);
  for (int i = 0; i < 64*1024; ++i) resampling2_add_point(r, &pg, 3);
  printf("\nALLOCATIONS: %lu ( %luKB ) | %lu (%luKB)\n", context.alloc_count, context.alloc_size >> 10, context.alloc_total_count, context.alloc_total_size >> 10);
  resampling2_free(r);
}

#define N 2048
TEST_CASE(resampling2) {
  Vector2 points[N];
  for (int i = 0; i < N; ++i) {
    points[i].x = sinf(3.14f / 4.f * (float)i);
    points[i].y = cosf(3.14f / 4.f * (float)i);
  }
  points_group_t pg = {
    .len = N,
    .points = points,
    .resampling = NULL,
    .cap = N 
  };
  resampling2_t* r = resampling2_malloc();
  for (int i = 0; i < N; ++i) resampling2_add_point(r, &pg, (size_t)i);
  resampling2_debug_print(stdout, r);
  printf("\nALLOCATIONS: %lu ( %luKB ) | %lu (%luKB)\n", context.alloc_count, context.alloc_size >> 10, context.alloc_total_count, context.alloc_total_size >> 10);
  resampling2_add_point(r, &pg, 3);
  printf("\nALLOCATIONS: %lu ( %luKB ) | %lu (%luKB)\n", context.alloc_count, context.alloc_size >> 10, context.alloc_total_count, context.alloc_total_size >> 10);
  for (int i = 0; i < 64*1024; ++i) resampling2_add_point(r, &pg, 3);
  printf("\nALLOCATIONS: %lu ( %luKB ) | %lu (%luKB)\n", context.alloc_count, context.alloc_size >> 10, context.alloc_total_count, context.alloc_total_size >> 10);
  for (int i = 0; i < 1024*1024; ++i) resampling2_add_point(r, &pg, 3);
  printf("\nALLOCATIONS: %lu ( %luKB ) | %lu (%luKB)\n", context.alloc_count, context.alloc_size >> 10, context.alloc_total_count, context.alloc_total_size >> 10);
  resampling2_free(r);
}
