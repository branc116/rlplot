#include "src/br_str.h"
#include "src/br_filesystem.h"

#include "external/tests.h"

bool br_fs_up_dir(br_str_t* cwd) {
start:
  if (cwd->len == 0) return br_str_push_c_str(cwd, "..");
  if (cwd->len == 1) {
    if (cwd->str[0] == '.') return br_str_push_c_str(cwd, ".");
    else {
      cwd->len = 0;
      return true;
    }
  }
  while (cwd->len > 1 && cwd->str[cwd->len - 1] == '/') --cwd->len;
  if (cwd->len >= 2 &&
      cwd->str[cwd->len - 1] == '.' &&
      cwd->str[cwd->len - 2] == '.') return br_str_push_c_str(cwd, "/..");
  else if (cwd->len >= 2 &&
      cwd->str[cwd->len - 1] == '.' &&
      cwd->str[cwd->len - 2] == '/') {
    cwd->len -= 2;
    goto start;
  }
  while (cwd->len != 0 && cwd->str[cwd->len - 1] != '/') --cwd->len;
  if (cwd->len == 0) {
    return true;
  }
  if (cwd->len == 1) {
    if (cwd->str[0] == '.') return br_str_push_c_str(cwd, ".");
    else {
      cwd->len = 0;
      return true;
    }
  }
  --cwd->len;
  return true;
}

bool br_fs_cd(br_str_t* cwd, br_strv_t path) {
  if (path.str[0] == '/') {
    cwd->len = 0;
    br_str_push_br_strv(cwd, path);
    return true;
  }

  while (path.len != 0) {
    br_strv_t next_name = { path.str, 0 };
    for (; next_name.len < path.len && path.str[next_name.len] != '/'; ++next_name.len);
    if (next_name.len == 0) /* DO NOTHING */;
    else if (br_strv_eq(next_name, br_strv_from_c_str(".."))) br_fs_up_dir(cwd);
    else if (br_strv_eq(next_name, br_strv_from_c_str("."))) /* DO NOTHING */;
    else if (cwd->len == 0) br_str_push_br_strv(cwd, next_name);
    else if (cwd->str[cwd->len - 1] == '/') br_str_push_br_strv(cwd, next_name);
    else {
      br_str_push_char(cwd, '/');
      br_str_push_br_strv(cwd, next_name);
    }
    if (next_name.len + 1 <= path.len) {
      path.str += next_name.len + 1;
      path.len -= next_name.len + 1;
    } else return true;
  }
  return true;
}

TEST_CASE(paths) {
  char c[128];
  br_str_t br = br_str_malloc(2);
  br_fs_cd(&br, br_strv_from_c_str("foo")); br_str_to_c_str1(br, c);
  TEST_STREQUAL(c, "foo");
  br_fs_cd(&br, br_strv_from_c_str("bar")); br_str_to_c_str1(br, c);
  TEST_STREQUAL(c, "foo/bar");
  br_fs_cd(&br, br_strv_from_c_str("../baz")); br_str_to_c_str1(br, c);
  TEST_STREQUAL(c, "foo/baz");
  br_fs_cd(&br, br_strv_from_c_str("../../baz")); br_str_to_c_str1(br, c);
  TEST_STREQUAL(c, "baz");
  br_fs_cd(&br, br_strv_from_c_str("../")); br_str_to_c_str1(br, c);
  TEST_STREQUAL(c, "");
  br_fs_cd(&br, br_strv_from_c_str(".///")); br_str_to_c_str1(br, c);
  TEST_STREQUAL(c, "");
  br_fs_cd(&br, br_strv_from_c_str("./aa//")); br_str_to_c_str1(br, c);
  TEST_STREQUAL(c, "aa");
  br_fs_cd(&br, br_strv_from_c_str("./a//")); br_str_to_c_str1(br, c);
  TEST_STREQUAL(c, "aa/a");
  br_fs_cd(&br, br_strv_from_c_str("../../../")); br_str_to_c_str1(br, c);
  TEST_STREQUAL(c, "..");
  br_fs_cd(&br, br_strv_from_c_str("./..")); br_str_to_c_str1(br, c);
  TEST_STREQUAL(c, "../..");

  br.len = 0;
  br_str_push_c_str(&br, "./././././");
  br_fs_cd(&br, br_strv_from_c_str("./..")); br_str_to_c_str1(br, c);
  TEST_STREQUAL(c, "..");

  br.len = 0;
  br_str_push_c_str(&br, "../../././");
  br_fs_cd(&br, br_strv_from_c_str("./..")); br_str_to_c_str1(br, c);
  TEST_STREQUAL(c, "../../..");
  br_str_free(br);
}

