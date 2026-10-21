#include "game/HandPose.h"
#include "game/CharacterView.h"
#include "engine/Renderer.h"
#include "platform/Win32Window.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <map>
#include <queue>
#include <wincodec.h>

namespace {
using namespace DirectX;
void Check(bool ok,const char* message) { if(!ok) { std::cerr<<message<<'\n'; std::exit(1); } }
float Distance(XMFLOAT3 a,XMFLOAT3 b) { return XMVectorGetX(XMVector3Length(XMLoadFloat3(&a)-XMLoadFloat3(&b))); }
std::vector<unsigned char> Pixels(const wchar_t* path) {
  using Microsoft::WRL::ComPtr;
  ComPtr<IWICImagingFactory> factory; ComPtr<IWICBitmapDecoder> decoder;
  ComPtr<IWICBitmapFrameDecode> frame; ComPtr<IWICFormatConverter> converter;
  Check(SUCCEEDED(CoCreateInstance(CLSID_WICImagingFactory,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&factory))) &&
    SUCCEEDED(factory->CreateDecoderFromFilename(path,nullptr,GENERIC_READ,WICDecodeMetadataCacheOnLoad,&decoder)) &&
    SUCCEEDED(decoder->GetFrame(0,&frame)) && SUCCEEDED(factory->CreateFormatConverter(&converter)) &&
    SUCCEEDED(converter->Initialize(frame.Get(),GUID_WICPixelFormat32bppRGBA,WICBitmapDitherTypeNone,nullptr,0,WICBitmapPaletteTypeCustom)),"Hand PNG decode");
  UINT width=0,height=0; Check(SUCCEEDED(converter->GetSize(&width,&height)) && width==960 && height==720,"Hand diagnostic dimensions");
  std::vector<unsigned char> pixels(width*height*4);
  Check(SUCCEEDED(converter->CopyPixels(nullptr,width*4,UINT(pixels.size()),pixels.data())),"Hand diagnostic pixels"); return pixels;
}
void Topology(unsigned level,bool mirrored) {
  const auto& hand=visual::CharacterHand(level,mirrored); const auto& mesh=hand.mesh;
  Check(mesh.vertices.size()==hand.influences.size(),"Missing hand weights");
  std::map<std::pair<unsigned,unsigned>,unsigned> edges;
  std::vector<std::vector<unsigned>> links(mesh.vertices.size());
  for(std::size_t t=0;t<mesh.indices.size();t+=3) {
    for(unsigned k=0;k<3;++k) {
      const auto a=mesh.indices[t+k],b=mesh.indices[t+(k+1)%3];
      Check(a<mesh.vertices.size() && b<mesh.vertices.size() && a!=b,"Invalid hand triangle");
      ++edges[std::minmax(a,b)]; links[a].push_back(b); links[b].push_back(a);
    }
  }
  for(const auto& edge : edges) Check(edge.second==2,"Hand must have no open seams or nonmanifold edges");
  std::vector<bool> visited(mesh.vertices.size()); std::queue<unsigned> pending; pending.push(0); visited[0]=true;
  unsigned connected=0;
  while(!pending.empty()) { const auto a=pending.front(); pending.pop(); ++connected;
    for(auto b : links[a]) if(!visited[b]) { visited[b]=true; pending.push(b); }
  }
  Check(connected==mesh.vertices.size(),"Palm and all five fingers must form one connected surface");
  Check(int(mesh.vertices.size())-int(edges.size())+int(mesh.indices.size()/3)==2,"Fingers must not accidentally bridge into handles or tunnels");
  for(std::size_t v=0;v<mesh.vertices.size();++v) {
    const auto& influence=hand.influences[v]; const float w[]={influence.weights.x,influence.weights.y,influence.weights.z,influence.weights.w};
    float sum=0;
    for(unsigned k=0;k<4;++k) { Check(influence.bones[k]<visual::handBones && std::isfinite(w[k]) && w[k]>=0,"Invalid hand influence"); sum+=w[k]; }
    Check(std::abs(sum-1)<.000001f,"Hand weights do not sum to one");
  }
  const auto bytes=mesh.vertices.size()*(sizeof(visual::Vertex)+sizeof(visual::SkinInfluence))+mesh.indices.size()*4;
  std::cout<<"Hand "<<level<<" mirror "<<mirrored<<": "<<mesh.vertices.size()<<" vertices, "<<mesh.indices.size()/3<<" triangles, "<<bytes<<" bytes\n";
  Check(bytes<400*1024 && mesh.indices.size()/3<8500,"Hand exceeded its immutable geometry budget");
  if(level) Check(mesh.indices.size()<visual::CharacterHand(level-1,mirrored).mesh.indices.size()*.65,"Hand LOD must remove substantial geometry");
}
void Deformation() {
  const auto pose=BuildCharacterPose(0,0,false);
  float maximumStep=0;
  for(unsigned side=0;side<2;++side) for(bool gripping : {false,true}) {
    std::vector<XMFLOAT3> previous;
    for(unsigned step=0;step<=100;++step) {
      const auto hand=BuildHandPose(pose,side,gripping,gripping,step*.01f);
      const auto palette=BuildHandRig(hand); const auto& geometry=visual::CharacterHand(0,hand.mirrored);
      for(unsigned b=1;b<visual::handBones;++b) {
        auto a=visual::HandBindPose()[b].from,c=visual::HandBindPose()[b].to;
        if(hand.mirrored) { a.x=-a.x; c.x=-c.x; }
        const auto matrix=XMLoadFloat4x4(&palette.joints[b]);
        Check(std::abs(XMVectorGetX(XMMatrixDeterminant(matrix))-1)<.00001f,"Finger transform must stay rigid and preserve winding");
        XMStoreFloat3(&a,XMVector3TransformCoord(XMLoadFloat3(&a),matrix));
        XMStoreFloat3(&c,XMVector3TransformCoord(XMLoadFloat3(&c),matrix));
        const auto& finger=hand.fingers[(b-1)/3];
        Check(Distance(a,finger.joints[(b-1)%3])<.000001f && Distance(c,finger.joints[(b-1)%3+1])<.000001f,"Skinned finger lost solved joint anchors");
      }
      for(std::size_t v=0;v<geometry.mesh.vertices.size();++v) {
        const auto& bind=geometry.mesh.vertices[v]; const auto skinned=visual::SkinHandVertex(bind,geometry.influences[v],palette);
        Check(std::isfinite(skinned.position.x) && Distance(skinned.position,{0,0,0})<.23f,"Hand escaped its culling bound");
        Check(std::abs(Distance(skinned.normal,{0,0,0})-1)<.00001f,"Hand normal is invalid");
        if(!gripping && step==100) Check(Distance(bind.position,skinned.position)<.000001f,"Open hand must reproduce its bind surface");
        if(step) maximumStep=std::max(maximumStep,Distance(previous[v],skinned.position));
        else previous.push_back(skinned.position);
        previous[v]=skinned.position;
      }
    }
  }
  Check(maximumStep<.003f,"Hand surface snapped during casting");
  std::cout<<"Maximum skin step over 1% casting change: "<<maximumStep<<" m\n";
}
}
int main() {
  for(unsigned level=0;level<3;++level) for(bool mirrored : {false,true}) Topology(level,mirrored);
  Deformation();
  Check(SUCCEEDED(CoInitializeEx(nullptr,COINIT_MULTITHREADED)),"Hand skin COM");
  Win32Window window; Check(window.Create(GetModuleHandle(nullptr),960,720,L"Continuous hand checks",false),"Hand skin window");
  Renderer renderer; Check(renderer.Initialize(window.GetHandle(),960,720),"Hand skin renderer");
  const auto pose=BuildCharacterPose(0,0,false);
  for(unsigned sample=0;sample<6;++sample) {
    const bool grip=sample<2; const unsigned side=sample%2;
    const auto hand=BuildHandPose(pose,side,grip,grip,sample>=4?1.f:0.f);
    const auto palette=BuildHandRig(hand);
    renderer.BeginFrame(); renderer.SetCamera({.10f,.09f,.33f},{0,.012f,-.035f});
    renderer.DrawMesh(visual::Mesh::Box,{0,-.105f,0},{.7f,.015f,.7f},{.4f,.42f,.38f,1},visual::Material::Stone);
    renderer.DrawHand(XMMatrixIdentity(),palette,hand.mirrored,{.72f,.51f,.35f,1},visual::Material::Skin);
    if(grip) renderer.DrawMesh(visual::Mesh::Limb,{0,0,0},{.065f,.25f,.065f},{.16f,.10f,.06f,1},visual::Material::Cloth,0,0,visual::ShadowMode::Cast,XM_PIDIV2);
    Check(renderer.Capture(L"hand-skin-"+std::to_wstring(sample)+L".png") && renderer.EndFrame(false) && renderer.ValidateDebugMessages(),"Skinned hand color/shadow capture");
  }
  const auto hand=BuildHandPose(pose,0,false,false,1); const auto palette=BuildHandRig(hand);
  float light[2]{};
  for(unsigned blocked=0;blocked<2;++blocked) {
    renderer.BeginFrame(); renderer.SetCamera({-.15f,.08f,-.35f},{0,-.02f,-.04f});
    renderer.DrawHand(XMMatrixIdentity(),palette,false,{.72f,.51f,.35f,1},visual::Material::Skin);
    if(blocked) renderer.DrawMesh(visual::Mesh::Box,{-.3f,.49f,-.28f},{.3f,.12f,.3f},{.3f,.3f,.3f,1},visual::Material::Stone);
    XMFLOAT2 probe; Check(renderer.Project({0,-.02f,-.065f},probe),"Hand shadow probe");
    const auto path=blocked?L"hand-skin-occluded.png":L"hand-skin-lit.png";
    Check(renderer.Capture(path) && renderer.EndFrame(false),"Hand occlusion capture");
    const auto pixels=Pixels(path); const int cx=int(probe.x*960),cy=int(probe.y*720);
    Check(cx>4 && cx<956 && cy>4 && cy<716,"Hand shadow probe bounds");
    for(int y=cy-4;y<=cy+4;++y) for(int x=cx-4;x<=cx+4;++x) {
      const auto at=(y*960+x)*4; light[blocked]+=(pixels[at]+pixels[at+1]+pixels[at+2])/(81.f*3);
    }
  }
  std::cout<<"Hand receiver light / occlusion: "<<light[0]<<" / "<<light[1]<<'\n';
  Check(light[0]-light[1]>10,"Curved finger filtering must retain external shadows");
  for(unsigned reversed=0;reversed<2;++reversed) {
    renderer.BeginFrame(); renderer.SetCamera({0,1,3},{0,1,0});
    for(unsigned order=0;order<visual::maxHands;++order) {
      const unsigned i=reversed?visual::maxHands-order-1:order;
      const auto varied=BuildHandPose(pose,i%2,false,false,float(i)/63);
      renderer.DrawHand(XMMatrixTranslation((int(i%8)-4)*.25f,(i/8)*.25f,0),BuildHandRig(varied),varied.mirrored,{.7f,.5f,.3f,1},visual::Material::Skin);
    }
    Check(renderer.Capture(reversed?L"hand-skin-capacity-reversed.png":L"hand-skin-capacity.png") && renderer.EndFrame(false) && renderer.ValidateDebugMessages(),"Full hand palette capacity");
  }
  Check(Pixels(L"hand-skin-capacity.png")==Pixels(L"hand-skin-capacity-reversed.png"),"Hand palettes changed when submission order reversed");
  renderer.BeginFrame();
  for(unsigned i=0;i<=visual::maxHands;++i) renderer.DrawHand(XMMatrixIdentity(),palette,false,{1,1,1,1},visual::Material::Skin);
  Check(!renderer.EndFrame(false),"Hand palette overflow must fail without corrupting memory");
  renderer.BeginFrame(); renderer.DrawHand(XMMatrixIdentity(),palette,false,{1,1,1,1},visual::Material::Skin);
  Check(renderer.EndFrame(false) && renderer.ValidateDebugMessages(),"Hand palette must recover after overflow");
  CoUninitialize(); std::cout<<"PASS: connected hand topology, rig deformation, mirror/cast/grip, shadows and bounded GPU palettes\n";
}
