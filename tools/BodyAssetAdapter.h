#pragma once
#include "tools/GltfRigImport.h"
#include "engine/Skinning.h"
#include <cmath>
#include <stdexcept>

namespace assets {
// Preview bridge to the existing ten-joint body shader. Larger imported rigs
// reject explicitly; this does not silently remap or discard influences.
inline visual::BodyGeometry BodyGeometry(const RigAsset& asset) {
  if(asset.skeleton.Size()>visual::bodyBones || asset.mesh.vertices.empty())
    throw std::runtime_error("Body preview requires a mesh and at most ten joints");
  DeformMesh(asset.mesh,asset.skeleton.RestPose());
  visual::BodyGeometry result;
  for(const auto& v:asset.mesh.vertices) {
    result.mesh.vertices.push_back({v.position,v.normal,{1,1,1,0}});
    visual::SkinInfluence influence;
    float sum=0;for(float w:v.weights) sum+=w;
    influence.bones={v.joints[0],v.joints[1],v.joints[2],v.joints[3]};
    influence.weights={v.weights[0]/sum,v.weights[1]/sum,v.weights[2]/sum,v.weights[3]/sum};
    result.influences.push_back(influence);
  }
  result.mesh.indices.assign(asset.mesh.indices.begin(),asset.mesh.indices.end());
  visual::ValidateBodyGeometry(result);
  return result;
}
inline visual::BodyPalette BodyPose(const RigAsset& asset,const animation::Pose& pose,
    DirectX::XMFLOAT4 tint={.45f,.62f,.75f,float(visual::Material::Cloth)}) {
  if(asset.skeleton.Size()>visual::bodyBones || pose.skin.size()!=asset.skeleton.Size())
    throw std::runtime_error("Body preview pose does not match its rig");
  DeformMesh(asset.mesh,pose); // Validate affine transforms and resulting normals.
  visual::BodyPalette result;
  for(unsigned i=0;i<visual::bodyBones;++i) {
    DirectX::XMStoreFloat4x4(&result.joints[i],DirectX::XMMatrixIdentity());
    DirectX::XMStoreFloat4x4(&result.normals[i],DirectX::XMMatrixIdentity());
    if(i<pose.skin.size()) {result.joints[i]=pose.skin[i];result.normals[i]=pose.normal[i];}
  }
  if(!std::isfinite(tint.x) || !std::isfinite(tint.y) || !std::isfinite(tint.z) ||
     !std::isfinite(tint.w) || tint.w<0 || tint.w>=float(visual::Material::Count) || std::floor(tint.w)!=tint.w)
    throw std::runtime_error("Invalid body preview material");
  result.colors.fill(tint);return result;
}
}
