#pragma once
#include "engine/Geometry.h"
#include <array>

class Renderer;
namespace visual {
// CPU records only. Camera visibility and shadow passes are evaluated afresh
// when submitted; these records own no device, viewport or frame resources.
struct SceneInstance { DirectX::XMFLOAT4X4 world,normal; DirectX::XMFLOAT4 color,surface; };
struct SceneItem {
  SceneInstance instance;
  DirectX::XMFLOAT3 center;
  float radius;
  unsigned visibility=0;
};
class StaticScene {
public:
  void Invalidate() {
    ready_=false;
    for (auto& batch : batches_) batch.clear();
  }
  std::size_t Instances() const {
    std::size_t count=0;
    for (const auto& batch : batches_) count+=batch.size();
    return count;
  }
  std::size_t Bytes() const {
    std::size_t bytes=0;
    for (const auto& batch : batches_) bytes+=batch.capacity()*sizeof(SceneItem);
    return bytes;
  }
private:
  friend class ::Renderer;
  bool ready_=false;
  std::array<std::vector<SceneItem>,static_cast<unsigned>(Mesh::Count)> batches_;
};
}
