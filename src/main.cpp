#include <clipper.hpp>
#include <janet.h>

using namespace ClipperLib;

// static float idx_getfloat(JanetView idx, int index) {
//     if (index >= idx.len) {
//         janet_panicf("index %d outside of range [0, %d)", idx.len);
//     }
//     if (!janet_checktype(idx.items[index], JANET_NUMBER)) {
//         janet_panicf("expected number, got %v", idx.items[index]);
//     }
//     return (float) janet_unwrap_number(idx.items[index]);
// }

static float idx_getint(JanetView idx, int index) {
  if (index >= idx.len) {
      janet_panicf("index %d outside of range [0, %d)", idx.len);
  }
  if (!janet_checkint(idx.items[index])) {
    janet_panicf("expected number, got %v", idx.items[index]);
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

static Janet cfun_Intersect(int32_t argc, Janet *argv) {
  janet_fixarity(argc, 2);
  Path subject = clipper_getpath(argv, 0);
  Path clip = clipper_getpath(argv, 1);
  Paths result;

  Clipper clipper;

  Paths subjects = {subject};
  Paths clips = {clip};

  clipper.AddPaths(subjects, ptSubject, true);
  clipper.AddPaths(clips, ptClip, true);
  
  clipper.Execute(ctIntersection, result, pftEvenOdd);

  return clipper_wrap_paths(result);
}

static JanetReg cfuns[] = {
  {"intersect", cfun_Intersect, NULL},
  {NULL, NULL, NULL}
};

JANET_MODULE_ENTRY(JanetTable *env) {
  janet_cfuns(env, "clipper", cfuns);
}
