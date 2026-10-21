#pragma once
#include "engine/Geometry.h"
#include <string_view>

namespace assets {
struct ObjLimits {
  std::size_t bytes=2*1024*1024,records=16384,triangles=8192,vertices=32768;
  float coordinate=1000;
};
// Geometry-only, triangulated OBJ. No material files or other paths are opened.
// Errors include the source line. Coordinates and winding are preserved.
visual::Geometry ImportObj(std::string_view source,ObjLimits limits={});
}
