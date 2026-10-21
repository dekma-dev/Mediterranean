#include "tools/ObjImport.h"
#include "game/CharacterView.h"
#include "engine/Renderer.h"
#include "platform/Win32Window.h"
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <stdexcept>
#include <tuple>

namespace {
using namespace DirectX;
void Check(bool ok,const char* message) {if(!ok) {std::cerr<<message<<'\n';std::exit(1);}}
const std::string triangle="v 0 0 0\nv 1 0 0\nv 0 1 0\n";
float Distance(XMFLOAT3 a,XMFLOAT3 b) {return XMVectorGetX(XMVector3Length(XMLoadFloat3(&a)-XMLoadFloat3(&b)));}
void Reject(const std::string& source,const char* reason,assets::ObjLimits limits={}) {
  try {assets::ImportObj(source,limits);Check(false,"Invalid OBJ was accepted");}
  catch(const std::runtime_error& error) {
    if(std::string(error.what()).find(reason)==std::string::npos) {std::cerr<<error.what()<<'\n';Check(false,"OBJ diagnostic lost its cause");}
    Check(std::string(error.what()).find("OBJ line ")==0,"OBJ diagnostic lost its line number");
  }
}
void Parsing() {
  const auto basic=assets::ImportObj(triangle+"f 1 2 3\n");
  Check(basic.vertices.size()==3 && basic.indices.size()==3,"Basic triangle import");
  for(const auto& v:basic.vertices) Check(Distance(v.normal,{0,0,1})<1e-6f,"Missing normal generation");
  const auto relative=assets::ImportObj("\xef\xbb\xbf# BOM/CRLF/negative indices\r\n"+triangle+"f -3 -2 -1 # comment\n");
  Check(relative.indices==basic.indices,"Relative OBJ indices changed winding");
  const auto explicitNormals=assets::ImportObj(triangle+"vn 0 0 4\nf +1//1 2//-1 3//1\n");
  Check(Distance(explicitNormals.vertices[0].normal,{0,0,1})<1e-6f,"Explicit normals must be normalized");
  const auto uv=assets::ImportObj(triangle+"vt 0 0\nvt 1 0\nvt 0 1 0\nvn 0 0 1\nf 1/1/1 2/2/1 3/3/1\n");
  Check(uv.indices==basic.indices,"OBJ UV references must be validated without changing procedural material geometry");
  const auto uvOnly=assets::ImportObj(triangle+"vt 0 0\nf 1/1 2/-1 3/1\n");
  Check(uvOnly.vertices.size()==3,"UV-only corner syntax");
  const auto colored=assets::ImportObj("v 0 0 0 .2 .4 .6\nv 1 0 0\nv 0 1 0\nf 1 2 3\n");
  Check(colored.vertices[0].color.y==.4f && colored.vertices[1].color.x==1,"Vertex colors/default white");
  const std::string wedge=triangle+"v 0 0 1\n";
  const auto smooth=assets::ImportObj(wedge+"o Shape\ng Edge\ns 7\nf 1 2 3\nf 1 4 2\n");
  Check(smooth.vertices.size()==4 && Distance(smooth.vertices[0].normal,{0,std::sqrt(.5f),std::sqrt(.5f)})<1e-6f,"Smoothing groups must share and average only their own vertices");
  const auto flat=assets::ImportObj(wedge+"s off\nf 1 2 3\nf 1 4 2\n");
  const auto seam=assets::ImportObj(wedge+"s 1\nf 1 2 3\ns 2\nf 1 4 2\n");
  Check(flat.vertices.size()==6 && seam.vertices.size()==6,"Flat faces and smoothing seams must retain hard edges");
  const auto split=assets::ImportObj(wedge+"vn 0 0 1\nvn 0 1 0\ns on\nf 1//1 2//1 3//1\nf 1//2 4//2 2//2\n");
  Check(split.vertices.size()==6,"Explicit normals must override smoothing groups and preserve seams");
  for(const auto& face:{"f 0 2 3","f 1 2 4","f -4 -2 -1","f 9223372036854775808 2 3","f 1x 2 3","f 1 2 3 1","f 1 2","f 1/ 2/ 3/","f 1// 2// 3//","f 1/1 2/1 3/1"})
    Reject(triangle+face,"OBJ line 4:");
  Reject(triangle+"vn 0 0 1\nf 1//1 2 3","mixed normal");
  Reject(triangle+"vt 0 0\nf 1/1 2 3","mixed normal/UV");
  Reject(triangle+"vn 0 0 -1\nf 1//1 2//1 3//1","normals oppose");
  Reject("vn 0 0 0","zero or cancelling normal");
  Reject("v nan 0 0","finite number"); Reject("v 1e99 0 0","finite number");
  Reject("v +-0.2 0 0","finite number"); Reject(triangle+"f +-3 -2 -1","invalid integer");
  Reject("v 1001 0 0","coordinate budget"); Reject("v 0 0 0 -1 1 1","vertex colors");
  Reject("v 0 0 0 1","expected v");
  Reject("v 0 0 0\nv 1 0 0\nv 2 0 0\nf 1 2 3","degenerate");
  Reject(triangle+"f 1 1 2","degenerate");
  Reject(triangle+"s -1","smoothing group"); Reject(triangle+"s 1 2","smoothing group");
  Reject("mtllib ../missing.mtl","material files"); Reject("usemtl Steel","material files");
  Reject("l 1 2","unsupported record"); Reject("# no faces","no triangles");
  Reject(std::string("v 0\0 0",7),"NUL"); Reject(std::string(8193,' '),"line exceeds");
  assets::ObjLimits limits;
  limits.bytes=5; Reject(triangle,"byte budget",limits); limits={};
  limits.records=2; Reject(triangle,"position budget",limits); limits={};
  limits.triangles=0; Reject(triangle+"f 1 2 3","triangle budget",limits); limits={};
  limits.vertices=2; Reject(triangle+"f 1 2 3","render vertex budget",limits);
}
void EmbeddedAsset() {
  std::ifstream file(RPG_BLADE_SOURCE,std::ios::binary);
  Check(bool(file),"Authored blade source is missing");
  const std::string source{std::istreambuf_iterator<char>(file),std::istreambuf_iterator<char>()};
  const auto imported=assets::ImportObj(source),embedded=visual::BuildMesh(visual::Mesh::Blade);
  Check(imported.indices==embedded.indices && imported.vertices.size()==embedded.vertices.size(),"The game must embed the current OBJ source");
  std::map<std::tuple<float,float,float>,unsigned> welded;
  std::vector<unsigned> positions;
  float low=1,high=-1;
  for(std::size_t i=0;i<embedded.vertices.size();++i) {
    const auto& a=imported.vertices[i];const auto& b=embedded.vertices[i];
    Check(Distance(a.position,b.position)==0 && Distance(a.normal,b.normal)==0 &&
      a.color.x==b.color.x && a.color.y==b.color.y && a.color.z==b.color.z && a.color.w==b.color.w,"Generated floats must round-trip exactly");
    low=std::min(low,b.position.y);high=std::max(high,b.position.y);
    Check(std::abs(b.position.x)<=.5f && std::abs(b.position.z)<=.5f,"Authored blade escaped its normalized equipment slot");
    const auto key=std::make_tuple(b.position.x,b.position.y,b.position.z);
    const auto found=welded.emplace(key,unsigned(welded.size()));positions.push_back(found.first->second);
  }
  Check(low==-.5f && high==.5f,"Blade root/tip must retain the established attachment and effect path");
  std::map<std::pair<unsigned,unsigned>,std::pair<unsigned,int>> edges;
  double volume=0;
  for(std::size_t t=0;t<embedded.indices.size();t+=3) {
    const auto a=XMLoadFloat3(&embedded.vertices[embedded.indices[t]].position);
    const auto b=XMLoadFloat3(&embedded.vertices[embedded.indices[t+1]].position);
    const auto c=XMLoadFloat3(&embedded.vertices[embedded.indices[t+2]].position);
    volume+=XMVectorGetX(XMVector3Dot(a,XMVector3Cross(b,c)))/6;
    for(unsigned k=0;k<3;++k) {
      const auto x=positions[embedded.indices[t+k]],y=positions[embedded.indices[t+(k+1)%3]];
      auto& edge=edges[std::minmax(x,y)];++edge.first;edge.second+=x<y?1:-1;
    }
  }
  for(const auto& edge:edges) Check(edge.second.first==2 && edge.second.second==0,"Blade has an open seam or reversed face");
  Check(volume>0 && embedded.indices.size()/3<=256,"Blade winding/budget");
  std::cout<<"Embedded blade: "<<embedded.vertices.size()<<" vertices, "<<embedded.indices.size()/3<<" triangles\n";
}
}
int main() {
  Parsing(); EmbeddedAsset();
  Check(SUCCEEDED(CoInitializeEx(nullptr,COINIT_MULTITHREADED)),"Asset COM");
  Win32Window window;Check(window.Create(GetModuleHandle(nullptr),960,720,L"Authored asset checks",false),"Hidden asset window");
  Renderer renderer;Check(renderer.Initialize(window.GetHandle(),960,720),"Asset renderer");
  for(unsigned view=0;view<4;++view) {
    renderer.BeginFrame();
    const XMFLOAT3 eye=view<2?(view==0?XMFLOAT3{1,2.2f,5.5f}:XMFLOAT3{5,2.2f,2}):XMFLOAT3{.8f,2,4};
    renderer.SetCamera(eye,view<2?XMFLOAT3{0,1.5f,0}:XMFLOAT3{0,1.3f,0});
    renderer.DrawMesh(visual::Mesh::Box,{0,-.05f,0},{5,.1f,5},{.5f,.5f,.5f,1},visual::Material::Stone);
    if(view<2) renderer.DrawMesh(visual::Mesh::Blade,{0,1.5f,0},{1.1f,2.8f,.8f},{.75f,.8f,.86f,1},visual::Material::Metal);
    else {
      animation::Sample motion;motion.attackSeconds=view==3?.36f:-1;motion.heavyAttack=view==3;
      DrawCharacter(renderer,{0,0},0,0,{.2f,.35f,.5f,1},0,0,{},motion,1);
    }
    Check(renderer.Capture(L"authored-blade-"+std::to_wstring(view)+L".png") && renderer.EndFrame(false) && renderer.ValidateDebugMessages(),"Authored mesh color/shadow render");
  }
  CoUninitialize();std::cout<<"PASS: OBJ syntax, seams, normals, budgets, embedded equality, blade anchors and render checks\n";
}
