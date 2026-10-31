#pragma once
#include "engine/Skinning.h"
#include <string_view>

namespace playerbody {
using Library=std::array<visual::BodyGeometry,visual::bodyLodCount>;
Library Decode(std::string_view bytes);
std::string_view EmbeddedBytes();
const Library& Collection();
}
