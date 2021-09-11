#include <clipper.hpp>
#include <janet.h>

using namespace ClipperLib;

static float numerical_tolerance = 0.00001;

static float idx_getint(JanetView idx, int index) {
  if (index >= idx.len) {
      janet_panicf("index %d outside of range [0, %d)", idx.len);
  }
  if (!janet_checkint(idx.items[index])) {
    janet_panicf("expected integer, got %v", idx.items[index]);
  }
  return janet_unwrap_integer(idx.items[index]);
}

static JanetView idx_getindexed(JanetView idx, int index) {
    Janet x = idx.items[index];
    JanetView view;
    if (!janet_indexed_view(x, &view.items, &view.len)) {
        janet_panic_type(x, index, JANET_TFLAG_INDEXED);
    }
    return view;
}

static IntPoint parse_point(JanetView janet_point) {
  IntPoint point;
  point.X = idx_getint(janet_point, 0);
  point.Y = idx_getint(janet_point, 1);
  return point;
}

static Path parse_path(JanetView janet_path) {
  Path path;
  path.resize(janet_path.len);

  for (int i = 0; i < janet_path.len; i++) {
    path[i] = parse_point(idx_getindexed(janet_path, i));
  }

  return path;
}

static Paths parse_paths(JanetView janet_paths) {
  Paths paths;
  paths.resize(janet_paths.len);

  for (int i = 0; i < janet_paths.len; i++) {
    paths[i] = parse_path(idx_getindexed(janet_paths, i));
  }

  return paths;
}

static Path clipper_getpath(const Janet *argv, int32_t n) {
  return parse_path(janet_getindexed(argv, n));
}

static Paths clipper_getpaths(const Janet *argv, int32_t n) {
  return parse_paths(janet_getindexed(argv, n));
}

static Janet clipper_wrap_point(IntPoint p) {
  Janet *tup = janet_tuple_begin(2);
  tup[0] = janet_wrap_integer(p.X);
  tup[1] = janet_wrap_integer(p.Y);
  return janet_wrap_tuple(janet_tuple_end(tup));
}

static Janet clipper_wrap_path(Path path) {
  Janet *tup = janet_tuple_begin(path.size());
  for (size_t i = 0; i < path.size(); i++) {
    tup[i] = clipper_wrap_point(path[i]);
  }

  return janet_wrap_tuple(janet_tuple_end(tup));
}

static Janet clipper_wrap_paths(Paths paths) {
  Janet *tup = janet_tuple_begin(paths.size());
  for (size_t i = 0; i < paths.size(); i++) {
    tup[i] = clipper_wrap_path(paths[i]);
  }

  return janet_wrap_tuple(janet_tuple_end(tup));
}

static Paths intersection_helper(int32_t argc, Janet *argv) {
  janet_fixarity(argc, 2);
  Clipper clipper;
  Path subject = clipper_getpath(argv, 0);
  Path clip = clipper_getpath(argv, 1);
  Paths subjects = {subject};
  Paths clips = {clip};
  clipper.AddPaths(subjects, ptSubject, true);
  clipper.AddPaths(clips, ptClip, true);  
  Paths result;
  clipper.Execute(ctIntersection, result, pftNonZero);
  return result;
}

static Paths intersection_multi_helper(int32_t argc, Janet *argv) {
  janet_fixarity(argc, 2);
  Clipper clipper;
  Paths subjects = clipper_getpaths(argv, 0);
  Paths clips = clipper_getpaths(argv, 1);
  clipper.AddPaths(subjects, ptSubject, true);
  clipper.AddPaths(clips, ptClip, true);  
  Paths result;
  clipper.Execute(ctIntersection, result, pftNonZero);
  return result;
}

static double paths_area(Paths paths) {
  double area = 0;
  for (size_t i = 0; i < paths.size(); i++) {
    area += Area(paths[i]);
  }
  return area;
}

static Janet cfun_Intersection(int32_t argc, Janet *argv) {
  return clipper_wrap_paths(intersection_helper(argc, argv));
}

// TODO: maybe instead of 0 we should have some numeric epsilon thing
static Janet cfun_Intersects(int32_t argc, Janet *argv) {
  return janet_wrap_boolean(paths_area(intersection_helper(argc, argv)) > numerical_tolerance);
}

static Janet cfun_IntersectsAny(int32_t argc, Janet *argv) {
  return janet_wrap_boolean(paths_area(intersection_multi_helper(argc, argv)) > numerical_tolerance);
}

static JanetReg cfuns[] = {
  {"intersection", cfun_Intersection, NULL},
  {"intersects?", cfun_Intersects, NULL},
  {"intersects-any?", cfun_IntersectsAny, NULL},
  {NULL, NULL, NULL}
};

JANET_MODULE_ENTRY(JanetTable *env) {
  janet_cfuns(env, "clipper", cfuns);
}
