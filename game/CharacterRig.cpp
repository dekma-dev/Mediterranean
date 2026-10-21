#include "game/CharacterRig.h"

visual::BodyPalette BuildCharacterRig(const CharacterPose& pose,const CharacterOutfit& outfit,DirectX::XMFLOAT4 cloth) {
  using namespace DirectX;
  visual::BodyPalette result{};
  const auto& bind=visual::BodyBindPose();
  const auto store=[&](unsigned bone,const XMMATRIX& matrix) {
    XMStoreFloat4x4(&result.joints[bone],matrix);
    XMStoreFloat4x4(&result.normals[bone],XMMatrixTranspose(XMMatrixInverse(nullptr,matrix)));
  };
  store(0,XMMatrixTranslation(pose.bodyShift,pose.bodyLift,0));
  store(1,XMMatrixTranslation(0,-.95f,0)*XMMatrixRotationX(pose.upperPitch)*XMMatrixRotationZ(pose.upperRoll)*
    XMMatrixRotationY(pose.upperYaw)*XMMatrixTranslation(pose.bodyShift,.95f+pose.bodyLift,0));
  for(unsigned side=0;side<2;++side) for(unsigned limb=0;limb<2;++limb) {
    const auto& chain=limb?pose.arms[side]:pose.legs[side];
    const auto torso=limb?XMMatrixRotationX(pose.upperPitch)*XMMatrixRotationZ(pose.upperRoll)*XMMatrixRotationY(pose.upperYaw):XMMatrixIdentity();
    const unsigned first=(limb?6:2)+side*2;
    for(unsigned segment=0;segment<2;++segment) {
      const auto& rest=bind[first+segment];
      const auto bindFrame=visual::BoneFrame(rest.from,rest.to);
      const auto reference=bindFrame*torso;
      const auto from=XMLoadFloat3(segment?&chain.joint:&chain.root),to=XMLoadFloat3(segment?&chain.end:&chain.joint);
      const auto delta=to-from,up=XMVector3Normalize(delta),restUp=XMVector3Normalize(reference.r[1]);
      const float alignment=std::clamp(XMVectorGetX(XMVector3Dot(restUp,up)),-1.f,1.f);
      // Transport bind twist by the shortest rotation. Projecting a fixed
      // forward axis flips 180 degrees when a forearm points through that axis.
      const auto rotation=alignment<-.9999f?XMQuaternionRotationAxis(reference.r[0],XM_PI):
        XMQuaternionNormalize(XMVectorSetW(XMVector3Cross(restUp,up),1+alignment));
      const auto transported=XMVector3Rotate(reference.r[0],rotation);
      const auto right=XMVector3Normalize(transported-up*XMVector3Dot(transported,up));
      const XMMATRIX posedFrame(right,delta,XMVector3Cross(right,up),XMVectorSetW(from,1));
      store(first+segment,XMMatrixInverse(nullptr,bindFrame)*posedFrame);
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
