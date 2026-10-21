#include "game/CharacterRig.h"
#include "engine/Renderer.h"
#include "platform/Win32Window.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <map>
#include <queue>
#include <limits>
#include <wincodec.h>

namespace {
using namespace DirectX;
void Check(bool ok,const char* message) { if(!ok) { std::cerr<<message<<'\n'; std::exit(1); } }
float Distance(XMFLOAT3 a,XMFLOAT3 b) { return XMVectorGetX(XMVector3Length(XMLoadFloat3(&a)-XMLoadFloat3(&b))); }
std::vector<unsigned char> Pixels(const std::wstring& path) {
  using Microsoft::WRL::ComPtr;
  ComPtr<IWICImagingFactory> factory; ComPtr<IWICBitmapDecoder> decoder;
  ComPtr<IWICBitmapFrameDecode> frame; ComPtr<IWICFormatConverter> converter;
  Check(SUCCEEDED(CoCreateInstance(CLSID_WICImagingFactory,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&factory))) &&
    SUCCEEDED(factory->CreateDecoderFromFilename(path.c_str(),nullptr,GENERIC_READ,WICDecodeMetadataCacheOnLoad,&decoder)) &&
    SUCCEEDED(decoder->GetFrame(0,&frame)) && SUCCEEDED(factory->CreateFormatConverter(&converter)) &&
    SUCCEEDED(converter->Initialize(frame.Get(),GUID_WICPixelFormat32bppRGBA,WICBitmapDitherTypeNone,nullptr,0,WICBitmapPaletteTypeCustom)),
    "LOD diagnostic PNG decode");
  UINT width=0,height=0; Check(SUCCEEDED(converter->GetSize(&width,&height)) && width==960 && height==720,"LOD image dimensions");
  std::vector<unsigned char> pixels(width*height*4);
  Check(SUCCEEDED(converter->CopyPixels(nullptr,width*4,UINT(pixels.size()),pixels.data())),"LOD pixel read"); return pixels;
}
void Silhouette(unsigned level) {
  const auto prefix=L"skin-lod-"+std::to_wstring(level);
  const auto a=Pixels(prefix+L"-auto.png"),b=Pixels(prefix+L"-full.png"),background=Pixels(prefix+L"-background.png");
  std::vector<bool> masks[2]; unsigned difference=0,unmatched=0,covered=0;
  for(auto& mask : masks) mask.resize(960*720);
  for(std::size_t pixel=0;pixel<masks[0].size();++pixel) {
    for(int channel=0;channel<3;++channel) {
      masks[0][pixel]=masks[0][pixel] || std::abs(int(a[pixel*4+channel])-int(background[pixel*4+channel]))>16;
      masks[1][pixel]=masks[1][pixel] || std::abs(int(b[pixel*4+channel])-int(background[pixel*4+channel]))>16;
    }
    covered+=masks[0][pixel] || masks[1][pixel]; difference+=masks[0][pixel]!=masks[1][pixel];
  }
  for(int side=0;side<2;++side) for(int y=2;y<718;++y) for(int x=2;x<958;++x) if(masks[side][y*960+x]) {
    bool nearby=false;
    for(int dy=-2;dy<=2;++dy) for(int dx=-2;dx<=2;++dx) nearby=nearby || masks[1-side][(y+dy)*960+x+dx];
    unmatched+=!nearby;
  }
  std::cout<<"LOD "<<level<<" silhouette: "<<difference<<" differing / "<<covered<<" covered pixels; "<<unmatched<<" beyond 2 px\n";
  Check(covered>100 && unmatched==0 && difference<float(covered)*.12f,"Body LOD silhouette must stay within two pixels and 12 percent coverage at its largest retained size");
  if(level==0) Check(a==b,"Near detail must remain pixel-identical when automatic LOD is enabled");
}
void Topology(unsigned level) {
  const auto& body=visual::CharacterBody(level); const auto& mesh=body.mesh;
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
  std::cout<<"Body level "<<level<<": "<<mesh.vertices.size()<<" vertices, "<<mesh.indices.size()/3<<" triangles, "<<bytes<<" immutable bytes\n";
  Check(bytes<1024*1024 && mesh.indices.size()/3<20000,"Shared body exceeds its explicit geometry budget");
  if(level) Check(mesh.indices.size()<visual::CharacterBody(level-1).mesh.indices.size()*.65,
    "Each reduced body must remove substantial geometry");
}
void Poses(unsigned level) {
  const auto& body=visual::CharacterBody(level);
  for(int frame=0;frame<=80;++frame) for(bool shield : {false,true}) {
    animation::Sample sample; sample.air=frame<20?frame/20.f:0; sample.rise=.7f;
    sample.landing=frame>=20 && frame<40?(frame-20)/20.f:0;
    sample.attackSeconds=frame>=40?(frame-40)*.02f:-1; sample.heavyAttack=frame%2==0;
    sample.bank=std::sin(frame*.2f); sample.hit=frame>65?.6f:0;
    const auto pose=BuildCharacterPose(0,std::sin(frame*.1f),shield,sample);
    const auto palette=BuildCharacterRig(pose,{}, {.22f,.37f,.54f,1});
    const auto& bind=visual::BodyBindPose();
    for(unsigned bone=0;bone<visual::bodyBones;++bone) {
      const auto reference=XMMatrixTranspose(XMMatrixInverse(nullptr,XMLoadFloat4x4(&palette.joints[bone])));
      for(const auto normal : {XMVectorSet(1,0,0,0),XMVectorSet(0,1,0,0),XMVectorSet(0,0,1,0),XMVectorSet(.3f,.7f,-.4f,0)}) {
        const auto a=XMVector3TransformNormal(normal,reference);
        const auto b=XMVector3TransformNormal(normal,XMLoadFloat4x4(&palette.normals[bone]));
        Check(XMVectorGetX(XMVector3Length(a-b))<.000003f,"Optimized rig normal must match the general inverse transpose");
      }
    }
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
void LodSelection() {
  Check(visual::SelectBodyLod(500)==0 && visual::SelectBodyLod(100)==1 && visual::SelectBodyLod(30)==2,"Projected-size detail selection");
  for(float invalid : {-1.f,std::numeric_limits<float>::quiet_NaN(),std::numeric_limits<float>::infinity()})
    Check(visual::SelectBodyLod(invalid)==0,"Invalid projections must retain full detail");
  visual::BodyLodHistory history;
  Check(history.Select(1,200)==0 && history.Select(2,100)==1 && history.Select(3,40)==2,"Actors need independent LOD history");
  for(int i=0;i<20;++i) {
    history.BeginFrame();
    Check(history.Select(1,i%2?149.f:151.f)==0 && history.Select(2,i%2?149.f:151.f)==1,
      "Small camera changes must not alternate body geometry");
  }
  Check(history.Select(1,20)==2 && history.Select(1,400)==0,"Large camera changes must cross all LOD boundaries immediately");
  history.BeginFrame(); history.BeginFrame(); history.BeginFrame();
  Check(history.Select(1,145)==1,"Expired actor history must not retain stale near detail");
  history.Reset(); Check(history.Select(1,145)==1,"Reset must discard LOD history");
  for(unsigned id=4;id<1000;++id) history.Select(id,40);
  Check(history.Select(1,400)==0,"Bounded history replacement must remain usable");
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
  RigContinuity(); LodSelection();
  for(unsigned level=0;level<visual::bodyLodCount;++level) { Topology(level); Poses(level); }
  Check(SUCCEEDED(CoInitializeEx(nullptr,COINIT_MULTITHREADED)),"COM initialization");
  Win32Window window; Check(window.Create(GetModuleHandle(nullptr),960,720,L"Skinned body checks",false),"Hidden skin test window");
  Renderer renderer; Check(renderer.Initialize(window.GetHandle(),960,720),"Skin renderer initialization");
  const auto idlePalette=BuildCharacterRig(BuildCharacterPose(0,0,false),{}, {.2f,.4f,.6f,1});
  for(int level=0;level<3;++level) {
    const float distance=level==0?5.f:level==1?9.6f:22.f;
    renderer.BeginFrame(); renderer.SetCamera({0,1,distance},{0,1,0});
    Check(renderer.Capture(L"skin-lod-"+std::to_wstring(level)+L"-background.png") && renderer.EndFrame(false),"LOD background capture");
    unsigned counts[2]{};
    for(int full=0;full<2;++full) {
      renderer.SetBodyLodEnabled(!full);
      // Seed the coarse level, then approach its upper hysteresis boundary:
      // this is its largest retained size and the strictest silhouette check.
      renderer.BeginFrame(); renderer.SetCamera({0,1,level==0?5.f:level==1?16.f:34.f},{0,1,0});
      renderer.DrawBody({0,0,0},0,idlePalette,42); Check(renderer.EndFrame(false),"LOD history seed");
      renderer.BeginFrame(); renderer.SetCamera({0,1,5},{0,1,0});
      renderer.DrawBody({0,0,0},0,idlePalette,42);
      // The final camera can arrive after queueing; selection must follow it.
      renderer.SetCamera({0,1,distance},{0,1,0});
      Check(renderer.Capture(L"skin-lod-"+std::to_wstring(level)+(full?L"-full.png":L"-auto.png")) && renderer.EndFrame(false),"LOD capture");
      Check(renderer.BodyLodCounts()[full?0:level]==1,"Rendered body must use the final camera's detail level");
      counts[full]=renderer.Triangles();
    }
    Check(level==0?counts[0]==counts[1]:counts[0]<counts[1]*.66f,"Distant bodies must reduce actual submitted triangles");
    Silhouette(level);
  }
  renderer.SetBodyLodEnabled(true);
  renderer.BeginFrame(); renderer.SetCamera({0,1,34},{0,1,0}); renderer.DrawBody({0,0,0},0,idlePalette,42);
  Check(renderer.BeginPortrait(.7f,.1f,.25f,.7f),"LOD world-to-portrait transition");
  const auto beforePortrait=renderer.Triangles();
  renderer.DrawBody({0,0,0},0,idlePalette,42); Check(renderer.EndPortrait(),"Full-detail portrait flush");
  Check(renderer.Triangles()-beforePortrait==visual::CharacterBody().mesh.indices.size()/3,"Portrait must always keep full body detail");
  renderer.DrawBody({0,0,0},0,idlePalette,42); Check(renderer.EndFrame(false),"Restore world detail after portrait");
  Check(renderer.BodyLodCounts()[2]==2,"Portrait must not overwrite the world's detail history");
  renderer.BeginFrame(); renderer.SetCamera({0,1,-34},{0,1,-35}); renderer.DrawBody({0,0,0},0,idlePalette,42);
  Check(renderer.EndFrame(false) && renderer.BodyLodCounts()[2]==1,"Distant behind-eye shadow casters must retain reduced detail");
  Check(renderer.Resize(960,1440),"LOD resolution change");
  renderer.SetBodyLodEnabled(true); renderer.BeginFrame(); renderer.SetCamera({0,1,16},{0,1,0}); renderer.DrawBody({0,0,0},0,idlePalette,42);
  Check(renderer.EndFrame(false) && renderer.BodyLodCounts()[0]==1,"Higher pixel coverage must increase detail at the same world distance");
  Check(renderer.Resize(960,720),"Restore LOD diagnostic size");
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
