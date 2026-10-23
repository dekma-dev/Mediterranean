#pragma once
#include <DirectXMath.h>
#include <string>
#include <vector>

namespace animation {
// Canonical engine space: metres, left-handed, Y up, +Z forward.
// Importers must convert all transforms
// consistently before constructing this representation. No file IO at runtime.
struct Transform {
  DirectX::XMFLOAT3 translation{0,0,0};
  DirectX::XMFLOAT4 rotation{0,0,0,1};
  DirectX::XMFLOAT3 scale{1,1,1};
};
struct Joint { std::string name; int parent=-1; Transform local; };
struct Pose {
  std::vector<DirectX::XMFLOAT4X4> global,skin,normal;
};
struct Key { float time=0; Transform value; };
using Track=std::vector<Key>;
struct Clip { float duration=0; std::vector<Track> tracks; };

// Immutable after construction; validation failures throw std::runtime_error.
// Arbitrary joint order is accepted, but cycles/multiple roots are rejected.
class Skeleton {
public:
  explicit Skeleton(std::vector<Joint> joints,
      std::vector<DirectX::XMFLOAT4X4> inverseBind={});
  std::size_t Size() const { return joints_.size(); }
  Pose RestPose() const;
  Pose Evaluate(const std::vector<Transform>& locals) const;
  void ValidateClip(const Clip& clip) const;
  // Clamp for one-shots; positive modulo for loops, including negative time.
  // Empty tracks retain the rest transform. Root motion is not applied here.
  Pose Sample(const Clip& clip,double seconds,bool loop) const;
private:
  std::vector<Joint> joints_;
  std::vector<std::size_t> order_;
  std::vector<DirectX::XMFLOAT4X4> inverseBind_;
};
}
