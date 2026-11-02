#include "engine/ClassArt.h"
#include "engine/PlayerBody.h"
#include "engine/Renderer.h"
#include "game/CharacterView.h"
#include "game/CharacterPose.h"
#include "game/CharacterRig.h"
#include "game/AuthoredLocomotion.h"
#include "game/FootPlacement.h"
#include "platform/Win32Window.h"
#include <filesystem>
#include <iostream>
#include <stdexcept>
namespace {
void Check(bool ok,const char* reason){if(!ok)throw std::runtime_error(reason);}
}
int main(int argc,char** argv){try {
 using namespace DirectX;
 const auto& library=classart::Collection();std::size_t bytes=0,triangles=0;
 for(const auto& item:library)for(const auto& mesh:item){bytes+=mesh.vertices.size()*sizeof(visual::Vertex)+mesh.indices.size()*4;triangles+=mesh.indices.size()/3;}
 for(const auto& item:library)for(const auto& mesh:item) {
  for(std::size_t i=0;i<mesh.indices.size();i+=3) {
   const auto& a=mesh.vertices[mesh.indices[i]],&b=mesh.vertices[mesh.indices[i+1]],&c=mesh.vertices[mesh.indices[i+2]];
   const auto normal=XMVector3Cross(XMLoadFloat3(&b.position)-XMLoadFloat3(&a.position),XMLoadFloat3(&c.position)-XMLoadFloat3(&a.position));
   Check(XMVectorGetX(XMVector3LengthSq(normal))>1e-16f,"Degenerate class attachment triangle");
   Check(XMVectorGetX(XMVector3Dot(normal,XMLoadFloat3(&a.normal)+XMLoadFloat3(&b.normal)+XMLoadFloat3(&c.normal)))>0,"Class attachment normal opposes winding");
  }
 }
 Check(library.size()==27&&bytes<16*1024*1024,"Old-style class library budget");
 Check(playerbody::Collection()[0].mesh.indices.size()/3>=18000,"Dense anatomical body was not embedded");
 // Waist anchors must remain attached while the hem follows the posed legs.
 // This catches accidental restoration of the old four rigid leg panels.
 for(unsigned cls=0;cls<3;++cls) {
  auto outfit=CharacterOutfit{};const auto mesh=classart::Mesh(classart::Part(unsigned(classart::Part::WarriorSkirt)+cls));
  const auto& garment=library[unsigned(classart::Part::WarriorSkirt)+cls][0];
  const auto& weights=classart::GarmentInfluences(mesh);Check(weights.size()==garment.vertices.size(),"Skirt palette alignment");
  animation::Sample move;move.air=.8f;move.rise=.7f;move.authoredGait=true;move.authoredEquipment=playerclips::Equipment::None;
  const auto pose=BuildCharacterPose(0,0,false,move);const auto palette=BuildCharacterRig(pose,outfit,{.3f,.3f,.3f,1});
  float hemTravel=0;unsigned waist=0;
  for(std::size_t i=0;i<weights.size();++i) {
   const auto& v=garment.vertices[i];const auto skinned=visual::SkinVertex(v,weights[i],palette);
   Check(std::isfinite(skinned.position.x)&&std::isfinite(skinned.position.y)&&std::isfinite(skinned.position.z),"Nonfinite garment skinning");
   if(v.position.y>.99f) {
    const XMFLOAT3 anchor{v.position.x+pose.bodyShift,v.position.y+pose.bodyLift,v.position.z};
    Check(XMVectorGetX(XMVector3Length(XMLoadFloat3(&skinned.position)-XMLoadFloat3(&anchor)))<.001f,"Garment waist detached from pelvis");++waist;
   }
   if(v.position.y<.77f)hemTravel=std::max(hemTravel,std::abs(skinned.position.z-v.position.z));
  }
  Check(waist>50&&hemTravel>.015f,"Garment no longer follows lower-body movement");
 }
 for(unsigned cls=0;cls<3;++cls) {
  const auto& a=library[cls*7+5][0];const auto& b=library[cls*7+6][0];
  bool different=a.vertices.size()!=b.vertices.size();
  for(std::size_t i=0;i<std::min(a.vertices.size(),b.vertices.size())&&!different;++i){const auto p=a.vertices[i].position,q=b.vertices[i].position;different=std::abs(p.x-q.x)+std::abs(p.y-q.y)+std::abs(p.z-q.z)>.0001f;}
  Check(different,"Weapon tiers need distinct geometry, not only a tint");
 }
 Win32Window window;Check(window.Create(GetModuleHandleW(nullptr),1200,1000,L"Old style character art",false),"Art review window");
 Renderer renderer;Check(renderer.Initialize(window.GetHandle(),1200,1000),"Art review renderer");
 const auto folder=std::filesystem::current_path()/"oldstyle-review";std::filesystem::create_directories(folder);
 const rpg::ClassId classes[]={rpg::ClassId::Warrior,rpg::ClassId::Scout,rpg::ClassId::Mage};
 // Orthographic-like front/profile and isolated shells expose intersections
 // and shading defects that a distant three-quarter equipment view can hide.
 for(unsigned cls=0;cls<3;++cls)for(unsigned view=0;view<3;++view) {
  auto kit=CharacterOutfit::From(rpg::Inventory{},classes[cls]);kit.belt=true;
  kit.shoulders=true;kit.gloves=true;
  renderer.BeginFrame();renderer.SetCamera(view==1?XMFLOAT3{-3.5f,1.15f,0}:XMFLOAT3{0,1.15f,-3.5f},{0,1.05f,0});
  renderer.DrawMesh(visual::Mesh::Box,{0,-.04f,0},{8,.08f,8},{.27f,.25f,.21f,1},visual::Material::Stone);
  if(view==2)renderer.DrawMeshTransformed(classart::Mesh(classart::Part(cls*7)),XMMatrixRotationY(XM_PI),{1,1,1,1},visual::Material::Count);
  else DrawCharacter(renderer,{0,0},0,XM_PI,{.32f,.30f,.25f,1},0,0,kit,{},1);
  Check(renderer.Capture((folder/("tailoring-"+std::to_string(cls)+"-"+std::to_string(view)+".png")).wstring())&&renderer.EndFrame(false)&&renderer.ValidateDebugMessages(),"Tailoring front/profile/shell capture");
 }
 if(argc>1&&std::string_view(argv[1])=="--tailoring-review")return 0;
 for(unsigned cls=0;cls<3;++cls)for(int veteran=0;veteran<2;++veteran)for(int view=0;view<4;++view){
  auto kit=CharacterOutfit::From(rpg::Inventory{},classes[cls]);kit.armor=veteran;kit.rewardWeapon=veteran;kit.helm=view==1;kit.shoulders=true;kit.belt=true;kit.cloak=true;kit.gloves=true;kit.shield=view==2;
  animation::State state;for(int k=0;k<90;++k){state.Update(1.f/60,0,0,0,true,0);state.locomotion.Update(1.f/60,0,0,0,0,true,0,0,true);animation::ApplyAuthoredLocomotion(state,1.f/60,cls==0?playerclips::Equipment::Sword:cls==2?playerclips::Equipment::Staff:playerclips::Equipment::Bow);}
  renderer.BeginFrame();renderer.SetCamera({view==3?1.7f:-1.8f,1.6f,view==3?3.2f:-3.2f},{0,1.04f,0});
  renderer.DrawMesh(visual::Mesh::Box,{0,-.04f,0},{8,.08f,8},{.27f,.25f,.21f,1},visual::Material::Stone);
  DrawCharacter(renderer,{0,0},0,XM_PI,{.32f,.30f,.25f,1},0,0,kit,state.pose,1);
  const auto path=folder/("class-"+std::to_string(cls)+"-tier-"+std::to_string(veteran)+"-view-"+std::to_string(view)+".png");
  Check(renderer.Capture(path.wstring())&&renderer.EndFrame(false)&&renderer.ValidateDebugMessages(),"Native class equipment capture");
 }
 for(unsigned cls=0;cls<3;++cls)for(bool heavy:{false,true}) {
  auto kit=CharacterOutfit::From(rpg::Inventory{},classes[cls]);kit.armor=true;kit.rewardWeapon=true;kit.shoulders=true;kit.belt=true;kit.gloves=true;
  animation::AuthoredWeaponController controller;
  for(int tick=0;tick<150;++tick) {
   const float t=tick/60.f;
   controller.Update(1.f/60,t<combat::Duration(heavy)?t:-1,heavy,cls==2,true,1,cls==1);
   animation::Sample sample;sample.authoredGait=true;sample.authoredBlend=1;
   sample.authoredEquipment=cls==0?playerclips::Equipment::Sword:cls==1?playerclips::Equipment::Bow:playerclips::Equipment::Staff;
   sample.authoredStrike=controller.frame;sample.authoredStrikeWeight=controller.weight;sample.authoredStrikePhase=controller.phase;sample.authoredStrikeHeavy=heavy;
   if(tick%3)continue;
   renderer.BeginFrame();renderer.SetCamera({-2.8f,1.55f,-3.7f},{0,1.02f,0});
   renderer.DrawMesh(visual::Mesh::Box,{0,-.04f,0},{8,.08f,8},{.27f,.25f,.21f,1},visual::Material::Stone);
   DrawCharacter(renderer,{0,0},0,XM_PI,{.32f,.30f,.25f,1},0,0,kit,sample,1);
   const auto path=folder/("attack-"+std::to_string(cls)+(heavy?"-heavy-":"-basic-")+std::to_string(tick/3)+".png");
   Check(renderer.Capture(path.wstring())&&renderer.EndFrame(false)&&renderer.ValidateDebugMessages(),"Native class attack capture");
  }
 }
 // Real presentation controllers: idle -> carry walk/run -> attack -> settle.
 // Capture at 20 Hz while updating at 60 Hz, including the supporting hand.
 for(unsigned cls=0;cls<3;++cls) {
  auto kit=CharacterOutfit::From(rpg::Inventory{},classes[cls]);kit.armor=true;kit.rewardWeapon=true;
  kit.shoulders=true;kit.belt=true;kit.gloves=true;kit.shield=false;
  animation::State state;animation::FootPlacement feet;rpg::Vec2 position{};feet.Reset(position,0);
  const auto equipment=cls==0?playerclips::Equipment::Sword:cls==1?playerclips::Equipment::Bow:playerclips::Equipment::Staff;
  for(int tick=0;tick<420;++tick) {
   const float t=tick/60.f,speed=t<1?0:t<2.4f?2.f:t<3.4f?5.f:0;
   position.z+=speed/60;
   state.Update(1.f/60,0,0,speed/60,true,0);
   state.locomotion.Update(1.f/60,speed/60,1,0,0,true,0,0,true);
   animation::ApplyAuthoredLocomotion(state,1.f/60,equipment);
   auto& weapon=state.authoredWeapon;const float attack=t-3.5f;
   weapon.Update(1.f/60,attack>=0&&attack<combat::Duration(false)?attack:-1,false,cls==2,true,1,cls==1);
   auto& pose=state.pose;pose.authoredStrike=weapon.frame;pose.authoredStrikeWeight=weapon.weight;
   pose.authoredStrikePhase=weapon.phase;pose.authoredStrikeHeavy=false;pose.authoredReady=weapon.ready;
   feet.Update(1.f/60,position,0,true,true,pose);
   if(tick%3)continue;
   renderer.BeginFrame();renderer.SetCamera({-2.8f,1.55f,-3.7f},{0,1.02f,0});
   renderer.DrawMesh(visual::Mesh::Box,{0,-.04f,0},{8,.08f,8},{.27f,.25f,.21f,1},visual::Material::Stone);
   DrawCharacter(renderer,{0,0},0,XM_PI,{.32f,.30f,.25f,1},0,0,kit,pose,1);
   const auto path=folder/("carry-"+std::to_string(cls)+"-"+std::to_string(tick/3)+".png");
   Check(renderer.Capture(path.wstring())&&renderer.EndFrame(false)&&renderer.ValidateDebugMessages(),"Carried weapon state transitions");
  }
 }
 // Armed flight: the blade remains attached to the wrist and clear of the
 // torso while the free arm balances ascent/descent. Inspect at 30 Hz.
 for(int frame=0;frame<36;++frame) {
  animation::Sample sample;sample.authoredGait=true;sample.authoredEquipment=playerclips::Equipment::Sword;
  sample.air=1;sample.rise=1-frame/17.5f;sample.authoredClock=frame/30.f;
  auto kit=CharacterOutfit::From(rpg::Inventory{},rpg::ClassId::Warrior);kit.armor=true;kit.rewardWeapon=true;
  const auto pose=BuildCharacterPose(0,0,false,sample);
  Check(pose.hasGripSocket&&pose.weaponDirection.y>.10f,"Airborne sword must stay braced above the ground");
  Check(pose.WeaponPoint(.70f).x>.20f,"Airborne blade crossed the torso");
  renderer.BeginFrame();renderer.SetCamera({-2.8f,1.55f,-3.7f},{0,1.02f,0});
  renderer.DrawMesh(visual::Mesh::Box,{0,-.04f,0},{8,.08f,8},{.27f,.25f,.21f,1},visual::Material::Stone);
  DrawCharacter(renderer,{0,0},.25f,XM_PI,{.32f,.30f,.25f,1},0,0,kit,sample,1);
  Check(renderer.Capture((folder/("sword-flight-"+std::to_string(frame)+".png")).wstring())&&renderer.EndFrame(false)&&renderer.ValidateDebugMessages(),"Armed flight capture");
 }
 renderer.BeginFrame();renderer.DrawHudRect(0,0,1,1,{.08f,.09f,.10f,1});
 Check(renderer.BeginPortrait(0,0,1,1,.80f),"Native lit face inspection");
 auto faceKit=CharacterOutfit::From(rpg::Inventory{},rpg::ClassId::Warrior);faceKit.weapon=false;
 DrawCharacter(renderer,{0,0},0,.25f,{.32f,.30f,.25f,1},0,0,faceKit,{},1);
 Check(renderer.EndPortrait()&&renderer.Capture((folder/"portrait-lit.png").wstring())&&renderer.EndFrame(false),"Native face portrait capture");
 std::cout<<"PASS: 27 Blender items, six distinct weapons, body detail, native equipment/shadow captures; "<<bytes<<" bytes, "<<triangles<<" triangles shared across all kits\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
