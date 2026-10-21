#include "game/CharacterRig.h"

namespace {
struct BindTransform { DirectX::XMFLOAT4X4 inverse,normal; };
const std::array<BindTransform,visual::bodyBones>& BindTransforms() {
  static const auto transforms=[] {
    std::array<BindTransform,visual::bodyBones> result{};
    const auto& bones=visual::BodyBindPose();
    for(unsigned i=0;i<visual::bodyBones;++i) {
      const auto frame=visual::BoneFrame(bones[i].from,bones[i].to);
      DirectX::XMStoreFloat4x4(&result[i].inverse,DirectX::XMMatrixInverse(nullptr,frame));
      DirectX::XMStoreFloat4x4(&result[i].normal,DirectX::XMMatrixTranspose(frame));
    }
    return result;
  }();
  return transforms;
}
}
visual::BodyPalette BuildCharacterRig(const CharacterPose& pose,const CharacterOutfit& outfit,DirectX::XMFLOAT4 cloth) {
  using namespace DirectX;
  visual::BodyPalette result{};
  const auto& bind=BindTransforms();
  const auto store=[&](unsigned bone,const XMMATRIX& matrix,const XMMATRIX& normal) {
    XMStoreFloat4x4(&result.joints[bone],matrix);
    XMStoreFloat4x4(&result.normals[bone],normal);
  };
  store(0,XMMatrixTranslation(pose.bodyShift,pose.bodyLift,0),XMMatrixIdentity());
  const auto torso=XMMatrixRotationX(pose.upperPitch)*XMMatrixRotationZ(pose.upperRoll)*XMMatrixRotationY(pose.upperYaw);
  store(1,XMMatrixTranslation(0,-.95f,0)*torso*XMMatrixTranslation(pose.bodyShift,.95f+pose.bodyLift,0),torso);
  for(unsigned side=0;side<2;++side) for(unsigned limb=0;limb<2;++limb) {
    const auto& chain=limb?pose.arms[side]:pose.legs[side];
    // The IK elbow/knee plane is continuous through raised/forward limbs.
    // Both segments must share it; a fixed forward axis or a shortest swing
    // from the hanging arm has a pole in the playable attack range.
    const auto upper=XMLoadFloat3(&chain.joint)-XMLoadFloat3(&chain.root);
    const auto lower=XMLoadFloat3(&chain.end)-XMLoadFloat3(&chain.joint);
    const auto plane=XMVector3Normalize(XMVector3Cross(upper,lower))*(limb?1.f:-1.f);
    const unsigned first=(limb?6:2)+side*2;
    for(unsigned segment=0;segment<2;++segment) {
      const auto& rest=bind[first+segment];
      const auto from=XMLoadFloat3(segment?&chain.joint:&chain.root),to=XMLoadFloat3(segment?&chain.end:&chain.joint);
      const auto delta=to-from,up=XMVector3Normalize(delta);
      const auto right=XMVector3Normalize(plane-up*XMVector3Dot(plane,up));
      const auto forward=XMVector3Cross(right,up);
      const XMMATRIX posedFrame(right,delta,forward,XMVectorSetW(from,1));
      // The solved frame is orthogonal, with length only on its Y axis.
      // Reciprocal squared length gives its normal transform without a general
      // inverse. The bind inverse/normal factor is immutable and cached once.
      const XMMATRIX posedNormal(right,delta/XMVector3LengthSq(delta),forward,XMVectorSet(0,0,0,1));
      store(first+segment,XMLoadFloat4x4(&rest.inverse)*posedFrame,XMLoadFloat4x4(&rest.normal)*posedNormal);
    }
  }
  const XMFLOAT4 skin{.72f,.51f,.35f,float(visual::Material::Skin)},steel{.52f,.59f,.61f,float(visual::Material::Metal)},
    leather{.20f,.14f,.095f,float(visual::Material::Cloth)},trousers{.24f,.25f,.23f,float(visual::Material::Cloth)};
  cloth.w=float(visual::Material::Cloth);
  if(!outfit.tunic) cloth={.61f,.58f,.46f,float(visual::Material::Cloth)};
  result.colors={outfit.armor?steel:cloth,cloth,outfit.armor?steel:skin,
    outfit.trousers?trousers:skin,outfit.boots?leather:outfit.trousers?trousers:skin,skin};
  return result;
}
