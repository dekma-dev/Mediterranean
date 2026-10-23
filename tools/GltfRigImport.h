#pragma once
#include "engine/Skeleton.h"
#include <string_view>

namespace assets {
// Offline, rig-only GLB 2.0 subset. Never follows paths or opens resources.
// Accepts one skin with TRS joint nodes and optional embedded FLOAT/MAT4 binds.
// Meshes, matrix nodes, sparse/strided accessors and extensions reject.
struct NamedClip { std::string name; animation::Clip clip; };
struct RigAsset { animation::Skeleton skeleton; std::vector<NamedClip> clips; };
// Also accepts bounded LINEAR translation/rotation/scale animation channels.
RigAsset ImportGlbAsset(std::string_view bytes);
// Static-rig convenience: refuses animated files rather than discarding clips.
animation::Skeleton ImportGlbRig(std::string_view bytes);
}
