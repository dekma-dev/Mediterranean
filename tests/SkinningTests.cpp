#include "game/CharacterRig.h"
#include "engine/Renderer.h"
#include "platform/Win32Window.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <map>
#include <queue>

namespace {
using namespace DirectX;
void Check(bool ok,const char* message) { if(!ok) { std::cerr<<message<<'\n'; std::exit(1); } }
float Distance(XMFLOAT3 a,XMFLOAT3 b) { return XMVectorGetX(XMVector3Length(XMLoadFloat3(&a)-XMLoadFloat3(&b))); }
void Topology() {
  const auto& body=visual::CharacterBody(); const auto& mesh=body.mesh;
  Check(mesh.vertices.size()==body.influences.size(),"Every body vertex needs skin weights");
  std::map<std::pair<unsigned,unsigned>,unsigned> edges;
  std::vector<std::vector<unsigned>> links(mesh.vertices.size());
  for(std::size_t t=0;t<mesh.indices.size();t+=3) for(unsigned k=0;k<3;++k) {
    const auto a=mesh.indices[t+k],b=mesh.indices[t+(k+1)%3];
    Check(a<mesh.vertices.size() && b<mesh.vertices.size() && a!=b,"Invalid body index");
    ++edges[std::minmax(a,b)]; links[a].push_back(b); links[b].push_back(a);
  }
  for(const auto& edge : edges) Check(edge.second==2,"Continuous body must be a closed two-manifold surface");
  std::vector<bool> visited(mesh.vertices.size()); std::queue<unsigned> pending; pending.push(0); visited[0]=true;
  unsigned connected=0;
  while(!pending.empty()) { const auto a=pending.front(); pending.pop(); ++connected;
    for(auto b : links[a]) if(!visited[b]) { visited[b]=true; pending.push(b); }
  }
  Check(connected==mesh.vertices.size(),"Chest, pelvis, arms and legs must belong to one connected mesh");
  visual::BodyPalette identity{};
  for(unsigned bone=0;bone<visual::bodyBones;++bone) {
    XMStoreFloat4x4(&identity.joints[bone],XMMatrixIdentity()); XMStoreFloat4x4(&identity.normals[bone],XMMatrixIdentity());
  }
  for(std::size_t i=0;i<mesh.vertices.size();++i) {
    const auto& v=mesh.vertices[i]; const auto& influence=body.influences[i];
    const float weights[]={influence.weights.x,influence.weights.y,influence.weights.z,influence.weights.w};
    float sum=0; for(unsigned k=0;k<4;++k) { Check(influence.bones[k]<visual::bodyBones && weights[k]>=0,"Invalid bone weight"); sum+=weights[k]; }
    Check(std::abs(sum-1)<.000001f,"Skin weights must sum to one");
    const auto rest=visual::SkinVertex(v,influence,identity);
    Check(Distance(rest.position,v.position)<.000001f && Distance(rest.normal,v.normal)<.000001f,"Identity skinning must preserve the bind mesh");
    Check(v.color.w>=0 && v.color.w<visual::bodyRegions,"Invalid body material region");
  }
  const auto bytes=mesh.vertices.size()*(sizeof(visual::Vertex)+sizeof(visual::SkinInfluence))+mesh.indices.size()*4;
  std::cout<<"Body: "<<mesh.vertices.size()<<" vertices, "<<mesh.indices.size()/3<<" triangles, "<<bytes<<" immutable bytes\n";
  Check(bytes<1024*1024 && mesh.indices.size()/3<20000,"Shared body exceeds its explicit geometry budget");
}
void Poses() {
  const auto& body=visual::CharacterBody();
  for(int frame=0;frame<=80;++frame) for(bool shield : {false,true}) {
    animation::Sample sample; sample.air=frame<20?frame/20.f:0; sample.rise=.7f;
    sample.landing=frame>=20 && frame<40?(frame-20)/20.f:0;
    sample.attackSeconds=frame>=40?(frame-40)*.02f:-1; sample.heavyAttack=frame%2==0;
    sample.bank=std::sin(frame*.2f); sample.hit=frame>65?.6f:0;
    const auto pose=BuildCharacterPose(0,std::sin(frame*.1f),shield,sample);
    const auto palette=BuildCharacterRig(pose,{}, {.22f,.37f,.54f,1});
    const auto& bind=visual::BodyBindPose();
    for(unsigned side=0;side<2;++side) for(unsigned limb=0;limb<2;++limb) {
      const unsigned first=(limb?6:2)+side*2; const auto& chain=limb?pose.arms[side]:pose.legs[side];
      for(unsigned segment=0;segment<2;++segment) {
        XMFLOAT3 a,b; const auto matrix=XMLoadFloat4x4(&palette.joints[first+segment]);
        XMStoreFloat3(&a,XMVector3TransformCoord(XMLoadFloat3(&bind[first+segment].from),matrix));
        XMStoreFloat3(&b,XMVector3TransformCoord(XMLoadFloat3(&bind[first+segment].to),matrix));
        Check(Distance(a,segment?chain.joint:chain.root)<.000001f && Distance(b,segment?chain.end:chain.joint)<.000001f,
          "Rig must follow solved feet, hands and fixed-length joints");
      }
    }
    for(std::size_t i=0;i<body.mesh.vertices.size();++i) {
      const auto v=visual::SkinVertex(body.mesh.vertices[i],body.influences[i],palette);
      Check(std::isfinite(v.position.x) && std::isfinite(v.position.y) && std::isfinite(v.position.z),"Skinning must remain finite");
      Check(Distance(v.position,{0,0,0})<3 && v.position.y>.01f,"Deformed body must fit culling bounds and clear the floor");
      Check(std::abs(Distance(v.normal,{0,0,0})-1)<.00001f,"Deformed normals must remain unit length");
    }
  }
}
void RigContinuity() {
  float worst=0,worstJoint=0,worstExcess=0; unsigned worstBone=0; int worstFrame=0,worstLayer=0; bool worstHeavy=false;
  for(bool heavy : {false,true}) for(bool shield : {false,true}) for(int layer=0;layer<4;++layer) {
    visual::BodyPalette previous{};
    for(int frame=0;frame<=800;++frame) {
      animation::Sample sample; sample.attackSeconds=frame*.001f; sample.heavyAttack=heavy;
      sample.air=layer==1?1.f:0; sample.rise=.5f; sample.cast=layer==2?1.f:0; sample.landing=layer==3?1.f:0;
      const auto rig=BuildCharacterRig(BuildCharacterPose(0,0,shield,sample),{}, {.2f,.4f,.6f,1});
      if(frame) for(unsigned bone=0;bone<visual::bodyBones;++bone) {
        const auto& rest=visual::BodyBindPose()[bone];
        float jointStep=0;
        for(const auto endpoint : {rest.from,rest.to}) {
          const auto p=XMLoadFloat3(&endpoint);
          jointStep=std::max(jointStep,XMVectorGetX(XMVector3Length(XMVector3TransformCoord(p,XMLoadFloat4x4(&rig.joints[bone]))-
            XMVector3TransformCoord(p,XMLoadFloat4x4(&previous.joints[bone])))));
        }
        // Off-axis skin points reveal twist that joint endpoint tests miss.
        for(const auto offset : {XMFLOAT3{.08f,0,0},XMFLOAT3{0,0,.08f}}) {
          const auto p=XMVectorSet((rest.from.x+rest.to.x)*.5f+offset.x,(rest.from.y+rest.to.y)*.5f,
            (rest.from.z+rest.to.z)*.5f+offset.z,1);
          const float step=XMVectorGetX(XMVector3Length(XMVector3TransformCoord(p,XMLoadFloat4x4(&rig.joints[bone]))-
            XMVector3TransformCoord(p,XMLoadFloat4x4(&previous.joints[bone]))));
          // Fast elbow travel at a nearly extended attack is legitimate. At
          // an 8 cm radius, bound skin motion relative to both joint endpoints
          // instead of imposing a speed cap on the authored attack itself.
          worstExcess=std::max(worstExcess,step-2*jointStep-.002f);
          if(step>worst) {
            worst=step; worstBone=bone; worstFrame=frame; worstLayer=layer; worstHeavy=heavy;
            worstJoint=jointStep;
          }
        }
      }
      previous=rig;
    }
  }
  std::cout<<"Rig 1 ms maximum surface probe change: "<<worst<<" on bone "<<worstBone
    <<", frame "<<worstFrame<<", layer "<<worstLayer<<", heavy "<<worstHeavy<<", joint change "<<worstJoint
    <<", excess skin motion "<<worstExcess<<'\n';
  Check(worstExcess==0,"Skinned limbs must not flip independently of their solved joints through attack poses");
}
}
int main() {
  RigContinuity(); Topology(); Poses();
  Check(SUCCEEDED(CoInitializeEx(nullptr,COINIT_MULTITHREADED)),"COM initialization");
  Win32Window window; Check(window.Create(GetModuleHandle(nullptr),960,720,L"Skinned body checks",false),"Hidden skin test window");
  Renderer renderer; Check(renderer.Initialize(window.GetHandle(),960,720),"Skin renderer initialization");
  const wchar_t* names[]={L"idle",L"stride",L"jump",L"landing",L"strike",L"heavy",L"cast",L"recoil"};
  for(int view=0;view<8;++view) {
    animation::Sample sample; sample.air=view==2?1.f:0; sample.rise=.5f; sample.landing=view==3?.8f:0;
    sample.attackSeconds=view==4?.18f:view==5?.30f:-1; sample.heavyAttack=view==5;
    sample.cast=view==6?1.f:0; sample.hit=view==7?1.f:0;
    renderer.BeginFrame(); renderer.SetCamera({2.7f,1.7f,3.7f},{0,.9f,0});
    renderer.DrawMesh(visual::Mesh::Box,{0,-.075f,0},{8,.15f,8},{.50f,.50f,.47f,1},visual::Material::Stone);
    CharacterOutfit outfit; outfit.armor=outfit.shoulders=outfit.helm=outfit.cloak=view%2==1;
    DrawCharacter(renderer,{0,0},0,0,{.22f,.37f,.54f,1},0,view==1?.8f:0,outfit,sample);
    const auto path=L"skin-"+std::wstring(names[view])+L".png";
    Check(renderer.Capture(path) && renderer.EndFrame(false) && renderer.ValidateDebugMessages(),"Skinned color/shadow capture");
  }
  // Multiple actors must retain distinct palettes; portraits may flush/reuse
  // slots without corrupting the world or the next frame's bones.
  renderer.BeginFrame(); renderer.SetCamera({0,2,6},{0,1,0});
  DrawCharacter(renderer,{-1,0},0,0,{.2f,.4f,.6f,1},0,.8f);
  DrawCharacter(renderer,{1,0},0,0,{.6f,.2f,.2f,1},.8f,-.8f);
  Check(renderer.BeginPortrait(.70f,.05f,.28f,.80f),"Portrait after skinned actors");
  DrawCharacter(renderer,{0,0},0,0,{.3f,.6f,.3f,1},0,0);
  Check(renderer.EndPortrait() && renderer.Capture(L"skin-world-portrait.png") && renderer.EndFrame(false) && renderer.ValidateDebugMessages(),
    "Palette reuse across world/portrait flushes");
  const auto palette=BuildCharacterRig(BuildCharacterPose(0,0,false),{}, {.2f,.4f,.6f,1});
  renderer.BeginFrame();
  for(unsigned i=0;i<visual::maxBodies+1;++i) renderer.DrawBody({float(i),0,0},0,palette);
  Check(!renderer.EndFrame(false),"Palette overflow must fail safely rather than address beyond the constant buffer");
  renderer.BeginFrame(); renderer.DrawBody({0,0,0},0,palette);
  Check(renderer.EndFrame(false) && renderer.ValidateDebugMessages(),"A new frame must recover after palette-budget failure");
  std::cout<<"PASS: connected weighted topology, bind/posed rig, finite normals/bounds, GPU color/shadow/portrait rendering\n";
  CoUninitialize();
}
