#include <assert.h>
#include <math.h>
#include <raylib.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "plotter.h"

#define MIN(x, y) (x) < (y) ? (x) : (y)
#define MAX(x, y) (x) > (y) ? (x) : (y)

static void quad_tree_extend_rect(Rectangle* rect, Vector2 point);
static void quad_tree_split(quad_tree_t* rect, Vector2 const * all_points);
static void _quad_tree_draw_lines(quad_tree_t* qt);
static void quad_tree_draw_rects(quad_tree_t* qt, int debug);
static Vector2 quad_tree_mid_point(quad_tree_t const * qt, int n);
static void quad_tree_copy(quad_tree_t* dest, quad_tree_t const* src);
static void quad_tree_draw_rects_for_point(quad_tree_t* qt, Vector2 point, int debug);
static void quad_tree_check(quad_tree_t* rect, Vector2 const * all_points);

static int* _c;
static Rectangle _screen;
static Vector2 const * _all_points;
static int _all_points_count;
static Shader _shader;
static smol_mesh_t* _sm;

void quad_tree_balance(quad_tree_t* dest, quad_tree_t const * qt, Vector2 const * all_points) {
  if (qt->is_leaf) {
    memcpy(dest, qt, sizeof(*dest));
    return;
  }
  dest->is_leaf = false;
  _all_points = all_points;
  dest->node.split_point = quad_tree_mid_point(qt, qt->count);
  dest->node.children = malloc(QUAD_TREE_DIR_COUNT * sizeof(quad_tree_t));
  for (int i = 0; i < QUAD_TREE_DIR_COUNT; ++i) {
    quad_tree_init(&dest->node.children[i]);
  }
  quad_tree_copy(dest, qt);
  for (int i = 0; i < QUAD_TREE_DIR_COUNT; ++i) {
    quad_tree_t t = {0};
    quad_tree_balance(&t, &dest->node.children[i], all_points);
    //quad_tree_free(&dest->node.children[i]);
    memcpy(&dest->node.children[i], &t, sizeof(t));
  }
}

static void quad_tree_copy(quad_tree_t* dest, quad_tree_t const * src) {
  if (src->is_leaf) {
    for (int i = 0; i < QUAD_TREE_MAX_GROUPS; ++i) {
      if (src->groups[i].length == 0) break;
      for (int j = 0; j < src->groups[i].length; ++j) {
        quad_tree_add_point(dest, _all_points, _all_points[src->groups[i].start_index + j], src->groups[i].start_index + j);
      }
    }
    return;
  }
  for (int i = 0; i < QUAD_TREE_DIR_COUNT; ++i) {
    quad_tree_copy(dest, &src->node.children[i]);
  }
  return;
}

static Vector2 quad_tree_mid_point(quad_tree_t const * qt, int n) {
  return (Vector2){
    .x = qt->bounds.x + qt->bounds.width/2,
    .y = qt->bounds.y + qt->bounds.height/2 };
//  Vector2 ret = {0};
//  if (qt->is_leaf) {
//    for (int i = 0; i < QUAD_TREE_MAX_GROUPS; ++i) {
//      for (int j = 0; j < qt->groups[i].length; ++j) {
//        ret.x += _all_points[qt->groups[i].start_index + j].x / n;
//        ret.y += _all_points[qt->groups[i].start_index + j].y / n;
//      }
//    }
//    return ret;
//  }
//  for (int i = 0; i < QUAD_TREE_DIR_COUNT; ++i) {
//    Vector2 r = quad_tree_mid_point(&qt->node.children[i], n);
//    ret.x += r.x;
//    ret.y += r.y;
//  }
//  return ret;
}

int quad_tree_draw_lines(quad_tree_t* qt, Rectangle screen, Vector2* all_points, int all_points_count, Shader shader, int debug) {
  int c = 0;
  _c = &c;
  _screen = screen;
  _screen.y -= _screen.height;
  _all_points = all_points;
  _all_points_count = all_points_count;
  _shader = shader;
  _sm = smol_mesh_get_temp();
  _quad_tree_draw_lines(qt);
  if (debug == 1 || debug == 2) quad_tree_draw_rects(qt, debug);
  if (debug == 3 || debug == 4) quad_tree_draw_rects_for_point(qt, graph_mouse_position, debug);
  return c;
}

static void quad_tree_draw_rects_for_point(quad_tree_t* qt, Vector2 point, int debug) {
  if (CheckCollisionPointRec(point, qt->bounds)) {
    if (qt->is_leaf || debug == 4) {
      Vector2 vec[5] = { {.x = qt->bounds.x, .y = qt->bounds.y},
        {.x = qt->bounds.x + qt->bounds.width, .y = qt->bounds.y},
        {.x = qt->bounds.x + qt->bounds.width, .y = qt->bounds.y + qt->bounds.height},
        {.x = qt->bounds.x, .y = qt->bounds.y + qt->bounds.height},
        {.x = qt->bounds.x, .y = qt->bounds.y} };
      smol_mesh_gen_line_strip(_sm, vec, 5, 0);
      smol_mesh_update(_sm);
      smol_mesh_draw_line_strip(_sm, _shader);
    }
  }
  if (qt->is_leaf) return;
  for(int i = 0; i < QUAD_TREE_DIR_COUNT; ++i) {
    quad_tree_draw_rects_for_point(&qt->node.children[i], point, debug); 
  }
}

static void quad_tree_draw_rects(quad_tree_t* qt, int debug) {
  if (qt->is_leaf) {
      Vector2 vec[5] = { {.x = qt->bounds.x, .y = qt->bounds.y},
        {.x = qt->bounds.x + qt->bounds.width, .y = qt->bounds.y},
        {.x = qt->bounds.x + qt->bounds.width, .y = qt->bounds.y + qt->bounds.height},
        {.x = qt->bounds.x, .y = qt->bounds.y + qt->bounds.height},
        {.x = qt->bounds.x, .y = qt->bounds.y} };
      smol_mesh_gen_line_strip(_sm, vec, 5, 0);
      smol_mesh_update(_sm);
      smol_mesh_draw_line_strip(_sm, _shader);
  } else {
    Vector2 vec[10] = {
      {.x = qt->bounds.x, .y = qt->bounds.y},
      {.x = qt->bounds.x + qt->bounds.width, .y = qt->bounds.y},
      {.x = qt->bounds.x + qt->bounds.width, .y = qt->bounds.y + qt->bounds.height},
      {.x = qt->bounds.x, .y = qt->bounds.y + qt->bounds.height},
      {.x = qt->bounds.x, .y = qt->bounds.y},
      {.x = qt->bounds.x, .y = qt->node.split_point.y},
      {.x = qt->bounds.x + qt->bounds.width, .y = qt->node.split_point.y},
      {.x = qt->node.split_point.x, .y = qt->node.split_point.y},
      {.x = qt->node.split_point.x, .y = qt->bounds.y + qt->bounds.height},
      {.x = qt->node.split_point.x, .y = qt->bounds.y}
    };
    smol_mesh_gen_line_strip(_sm, vec, 10, 0);
    smol_mesh_update(_sm);
    smol_mesh_draw_line_strip(_sm, _shader);
  }
  if (qt->is_leaf) {
    return;
  }
  for(int i = 0; i < QUAD_TREE_DIR_COUNT; ++i) {
    quad_tree_draw_rects(&qt->node.children[i], debug); 
  }
}

static void _quad_tree_draw_lines(quad_tree_t* qt) {
  if (!CheckCollisionRecs(qt->bounds, _screen)) return;
  if (!qt->is_leaf) {
    for (int i = 0; i < QUAD_TREE_DIR_COUNT; ++i) {
      _quad_tree_draw_lines(&qt->node.children[i]);
    }
    return;
  }
  ++*_c;
  for (int i = 0; i < QUAD_TREE_MAX_GROUPS; ++i) {
    if (qt->groups->length == 0) return;
    Vector2 const * ps = &_all_points[qt->groups[i].start_index];
    int l = qt->groups[i].length;
    if (ps != _all_points) {
      ps -= 1;
      l += 1;
    }
    for (int j = 0; j < l; j+=PTOM_COUNT) {
      int len = MIN(l - j, PTOM_COUNT);
      if (smol_mesh_gen_line_strip(_sm, ps + j, len, 0)) {
        smol_mesh_update(_sm);
        smol_mesh_draw_line_strip(_sm, _shader);
      }
    }
  }
}

static int _quad_tree_print_dot(quad_tree_t* t, int* n) {
  int id = ++*n;
  if (t->is_leaf) {
    printf(" id%d [label=\"%f,%f,%f,%f\\n%d\"]\n", id, t->bounds.x, t->bounds.y, t->bounds.width, t->bounds.height, t->count);
    return id;
  } else {
    printf(" id%d [label=\"%f,%f,%f,%f\\l%f,%f\\l%d\"]\n", id, t->bounds.x, t->bounds.y, t->bounds.width, t->bounds.height, t->node.split_point.x, t->node.split_point.y, t->count);
    for (int i = 0; i < QUAD_TREE_DIR_COUNT; ++i) {
      int c = _quad_tree_print_dot(&t->node.children[i], n);
      printf(" id%d -> id%d;\n", id, c);
    }
  }
  return id;
}

void quad_tree_print_dot(quad_tree_t* t) {
  int id = 0;
  printf("digraph L {\n");
  printf("node [shape=record fontname=Arial];\n");
  _quad_tree_print_dot(t, &id);
  printf("}\n");
  return;
}

void quad_tree_init(quad_tree_t* qt) {
  *qt = (quad_tree_t){ 0 };
  qt->is_leaf = true;
}

bool quad_tree_add_point(quad_tree_t* root, Vector2 const * all_points, Vector2 point, int index) {
  // Check for NaN
  if (point.x != point.x) return false;
  if (point.y != point.y) return false;

  if (root->is_leaf) {
    bool is_ok = false;
    for (int i = 0; i < QUAD_TREE_MAX_GROUPS; ++i) {
      if (root->groups[i].length == 0) {
        root->groups[i].start_index = index;
        root->groups[i].length = 1;
        is_ok = true;
        break;
      } else if (root->groups[i].start_index == index - root->groups[i].length) {
        ++root->groups[i].length;
        is_ok = true;
        break;
      }
    }
    if (root->count == 0) {
      root->bounds = (Rectangle){.x = point.x, .y = point.y, .width = 0, .height = 0};
    } else {
      quad_tree_extend_rect(&root->bounds, point);
    }
    if (!is_ok || root->count > QUAD_TREE_SPLIT_COUNT) {
      quad_tree_split(root, all_points);
    } else {
      ++root->count;
    }
  } else {
    bool down = point.y < root->node.split_point.y, right = point.x > root->node.split_point.x;
    int node_index = (down << 1) | (right);
    if (root->count == 0) {
      root->bounds = (Rectangle){.x = point.x, .y = point.y, .height = 0, .width = 0};
    } else {
      quad_tree_extend_rect(&root->bounds, point);
    }
    ++root->count;
    assert(quad_tree_add_point(&root->node.children[node_index], all_points, point, index));
  }
  //quad_tree_check(root, all_points);
  return true;
}

static void quad_tree_check(quad_tree_t* rect, Vector2 const * all_points) {
  if (rect->is_leaf) {
    for (int i = 0; i < QUAD_TREE_MAX_GROUPS; ++i) {
      for (int j = 0; j < rect->groups[i].length; ++j) {
        assert(CheckCollisionPointRec(all_points[rect->groups[i].start_index + j], rect->bounds));
      }
    }
    return;
  }
  for (int i = 0; i < QUAD_TREE_DIR_COUNT; ++i) {
    assert(rect->count >= rect->node.children[i].count);
    assert(rect->bounds.x <= rect->node.children[i].bounds.x || rect->node.children[i].count == 0);
    assert(rect->bounds.y <= rect->node.children[i].bounds.y || rect->node.children[i].count == 0);
    assert(rect->bounds.width >= rect->node.children[i].bounds.width);
    assert(rect->bounds.height >= rect->node.children[i].bounds.height);
    quad_tree_check(&rect->node.children[i], all_points);
  }
}

static void quad_tree_split(quad_tree_t* rect, Vector2 const * all_points) {
  assert(rect->is_leaf);

  Vector2 sp = quad_tree_mid_point(rect, rect->count);
  struct {
    int start;
    int length;
  } groups[QUAD_TREE_MAX_GROUPS];
  memcpy(groups, rect->groups, sizeof(rect->groups));
  rect->is_leaf = false;
  rect->node.split_point = sp;
  rect->node.children = malloc(QUAD_TREE_DIR_COUNT * sizeof(quad_tree_t));
  rect->count = 0;
  for (int i = 0; i < QUAD_TREE_DIR_COUNT; ++i) {
    quad_tree_init(&rect->node.children[i]);
  }
  for (int i = 0; i < QUAD_TREE_MAX_GROUPS; ++i) {
    for (int j = 0; j < groups[i].length; ++j) {
      quad_tree_add_point(rect, all_points, all_points[groups[i].start + j], groups[i].start + j);
    }
  }
}

static void quad_tree_extend_rect(Rectangle* rect, Vector2 point) {
  if (CheckCollisionPointRec(point, *rect)) return;
  float dx = point.x - rect->x;
  float dy = point.y - rect->y;
  if (dx > 0) {
    rect->width = MAX(dx, rect->width);
    while (rect->width + rect->x < point.x) rect->width = nextafterf(rect->width, INFINITY);
  } else if (dx < 0) {
    float old_reach = rect->width + rect->x;
    rect->x = point.x;
    rect->width -= dx;
    while (rect->width + rect->x < old_reach) rect->width = nextafterf(rect->width, INFINITY);
  }
  if (dy > 0) {
    rect->height = MAX(dy, rect->height);
    while (rect->height + rect->y < point.y) rect->height = nextafterf(rect->height, INFINITY);
  } else if (dy < 0) {
    float old_reach = rect->height + rect->y;
    rect->y = point.y;
    rect->height -= dy;
    while (rect->height + rect->y < old_reach) rect->height = nextafterf(rect->height, INFINITY);
  }
}

