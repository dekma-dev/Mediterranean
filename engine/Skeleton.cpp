#include "engine/Skeleton.h"
#include <algorithm>
#include <cmath>
#include <functional>
#include <stdexcept>
#include <unordered_set>

namespace animation {
using namespace DirectX;
namespace {
void Require(bool ok,const char* message) { if(!ok) throw std::runtime_error(message); }
void Check(const Transform& t) {
  for(float v:{t.translation.x,t.translation.y,t.translation.z})
    Require(std::isfinite(v) && std::abs(v)<=1000,"Skeleton: invalid translation");
  for(float v:{t.scale.x,t.scale.y,t.scale.z})
    Require(std::isfinite(v) && v>=.01f && v<=100,"Skeleton: invalid scale");
  double length=0;
  for(float v:{t.rotation.x,t.rotation.y,t.rotation.z,t.rotation.w}) {
    Require(std::isfinite(v),"Skeleton: non-finite quaternion");length+=double(v)*v;
  }
  Require(std::abs(length-1)<.001,"Skeleton: quaternion must be normalized");
}
XMMATRIX Matrix(const Transform& t) {
  return XMMatrixScaling(t.scale.x,t.scale.y,t.scale.z)*
    XMMatrixRotationQuaternion(XMQuaternionNormalize(XMLoadFloat4(&t.rotation)))*
    XMMatrixTranslation(t.translation.x,t.translation.y,t.translation.z);
}
void CheckMatrix(FXMMATRIX matrix) {
  XMFLOAT4X4 m;XMStoreFloat4x4(&m,matrix);
  for(const auto& row:m.m) for(float v:row)
    Require(std::isfinite(v) && std::abs(v)<=1e8f,"Skeleton: hierarchy exceeds transform budget");
}
Transform Blend(const Transform& a,const Transform& b,float f) {
  Transform t;
  XMStoreFloat3(&t.translation,XMVectorLerp(XMLoadFloat3(&a.translation),XMLoadFloat3(&b.translation),f));
  XMStoreFloat3(&t.scale,XMVectorLerp(XMLoadFloat3(&a.scale),XMLoadFloat3(&b.scale),f));
  // Slerp assumes unit endpoints. Normalizing only its result cannot remove
  // weighting/angle errors introduced by accepted export-rounding differences.
  const auto from=XMQuaternionNormalize(XMLoadFloat4(&a.rotation));
  const auto to=XMQuaternionNormalize(XMLoadFloat4(&b.rotation));
  XMStoreFloat4(&t.rotation,XMQuaternionNormalize(XMQuaternionSlerp(from,to,f)));
  return t;
}
}
Skeleton::Skeleton(std::vector<Joint> joints,std::vector<XMFLOAT4X4> suppliedBind):joints_(std::move(joints)) {
  Require(!joints_.empty() && joints_.size()<=64,"Skeleton: joint budget is 1..64");
  Require(suppliedBind.empty() || suppliedBind.size()==joints_.size(),"Skeleton: inverse bind count mismatch");
  std::unordered_set<std::string> names;unsigned roots=0;
  for(std::size_t i=0;i<joints_.size();++i) {
    const auto& j=joints_[i];Check(j.local);
    Require(!j.name.empty() && j.name.size()<=64 && names.insert(j.name).second,"Skeleton: missing, long or duplicate joint name");
    Require(j.parent>=-1 && j.parent<int(joints_.size()) && j.parent!=int(i),"Skeleton: invalid parent");
    roots+=j.parent==-1;
  }
  Require(roots==1,"Skeleton: exactly one root required");
  std::vector<unsigned char> visited(joints_.size());
  const std::function<void(std::size_t)> visit=[&](std::size_t i) {
    Require(visited[i]!=1,"Skeleton: hierarchy cycle");if(visited[i]==2) return;
    visited[i]=1;if(joints_[i].parent>=0) visit(std::size_t(joints_[i].parent));
    visited[i]=2;order_.push_back(i);
  };
  for(std::size_t i=0;i<joints_.size();++i) visit(i);
  std::vector<XMFLOAT4X4> global(joints_.size());inverseBind_.resize(joints_.size());
  for(auto i:order_) {
    auto m=Matrix(joints_[i].local);
    if(joints_[i].parent>=0) m=m*XMLoadFloat4x4(&global[joints_[i].parent]);
    CheckMatrix(m);XMStoreFloat4x4(&global[i],m);
    XMVECTOR determinant;const auto inverse=XMMatrixInverse(&determinant,m);
    Require(std::isfinite(XMVectorGetX(determinant)) && XMVectorGetX(determinant)>1e-12f,"Skeleton: singular bind hierarchy");
    CheckMatrix(inverse);XMStoreFloat4x4(&inverseBind_[i],inverse);
  }
  if(!suppliedBind.empty()) {
    for(const auto& bind:suppliedBind) {
      const auto matrix=XMLoadFloat4x4(&bind);CheckMatrix(matrix);
      Require(std::abs(bind._14)<1e-6f && std::abs(bind._24)<1e-6f && std::abs(bind._34)<1e-6f && std::abs(bind._44-1)<1e-6f,
        "Skeleton: inverse bind must be affine");
      XMVECTOR determinant;const auto inverse=XMMatrixInverse(&determinant,matrix);
      Require(std::isfinite(XMVectorGetX(determinant)) && XMVectorGetX(determinant)>1e-12f,"Skeleton: invalid inverse bind");
      CheckMatrix(inverse);
    }
    inverseBind_=std::move(suppliedBind);
  }
}
Pose Skeleton::RestPose() const {
  std::vector<Transform> locals;locals.reserve(Size());for(const auto& j:joints_) locals.push_back(j.local);
  return Evaluate(locals);
}
Pose Skeleton::Evaluate(const std::vector<Transform>& locals) const {
  Require(locals.size()==Size(),"Skeleton: pose joint count mismatch");
  Pose pose;pose.global.resize(Size());pose.skin.resize(Size());pose.normal.resize(Size());
  for(auto i:order_) {
    Check(locals[i]);auto global=Matrix(locals[i]);
    if(joints_[i].parent>=0) global=global*XMLoadFloat4x4(&pose.global[joints_[i].parent]);
    CheckMatrix(global);XMStoreFloat4x4(&pose.global[i],global);
    const auto skin=XMLoadFloat4x4(&inverseBind_[i])*global;CheckMatrix(skin);
    XMStoreFloat4x4(&pose.skin[i],skin);
    XMVECTOR determinant;const auto normal=XMMatrixTranspose(XMMatrixInverse(&determinant,skin));
    Require(std::isfinite(XMVectorGetX(determinant)) && XMVectorGetX(determinant)>1e-12f,"Skeleton: singular animated hierarchy");
    CheckMatrix(normal);XMStoreFloat4x4(&pose.normal[i],normal);
  }
  return pose;
}
void Skeleton::ValidateClip(const Clip& clip) const {
  Require(std::isfinite(clip.duration) && clip.duration>0 && clip.duration<=600,"Clip: invalid duration");
  Require(clip.tracks.size()==Size(),"Clip: track count mismatch");
  std::size_t keys=0;
  for(const auto& track:clip.tracks) {
    keys+=track.size();Require(keys<=65536,"Clip: key budget exceeded");
    float previous=-1;
    for(const auto& key:track) {
      Require(std::isfinite(key.time) && key.time>=0 && key.time<=clip.duration && key.time>previous,"Clip: invalid or unordered key time");
      Check(key.value);previous=key.time;
    }
  }
}
Pose Skeleton::Sample(const Clip& clip,double seconds,bool loop) const {
  ValidateClip(clip);Require(std::isfinite(seconds),"Clip: non-finite sample time");
  double time=loop?std::fmod(seconds,double(clip.duration)):std::clamp(seconds,0.0,double(clip.duration));
  if(time<0) time+=clip.duration;
  std::vector<Transform> locals;locals.reserve(Size());
  for(std::size_t i=0;i<Size();++i) {
    const auto& track=clip.tracks[i];
    if(track.empty()) {locals.push_back(joints_[i].local);continue;}
    const auto next=std::upper_bound(track.begin(),track.end(),time,[](double t,const Key& k){return t<k.time;});
    if(next==track.begin()) locals.push_back(next->value);
    else if(next==track.end()) locals.push_back(track.back().value);
    else {const auto& prev=*(next-1);locals.push_back(Blend(prev.value,next->value,float((time-prev.time)/(next->time-prev.time))));}
  }
  return Evaluate(locals);
}
}
