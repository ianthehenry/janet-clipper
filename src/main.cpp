#include <clipper.hpp>
#include <janet.h>

using namespace ClipperLib;

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

static Path clipper_getpath(const Janet *argv, int32_t n) {
  JanetView points = janet_getindexed(argv, n);
  Path path;
  path.resize(points.len);

  for (int i = 0; i < points.len; i++) {
    JanetView point = idx_getindexed(points, i);
    path[i].X = idx_getint(point, 0);
    path[i].Y = idx_getint(point, 1);
  }

  return path;
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
  return janet_wrap_boolean(paths_area(intersection_helper(argc, argv)) > 0);
}

static JanetReg cfuns[] = {
  {"intersection", cfun_Intersection, NULL},
  {"intersects?", cfun_Intersects, NULL},
  {NULL, NULL, NULL}
};

JANET_MODULE_ENTRY(JanetTable *env) {
  janet_cfuns(env, "clipper", cfuns);
}
