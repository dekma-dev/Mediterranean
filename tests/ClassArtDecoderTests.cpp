#include "engine/ClassArt.h"
#include <cmath>
#include <cstring>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
namespace {
void Check(bool ok,const char* reason){if(!ok)throw std::runtime_error(reason);}
template<class T>void Add(std::string& bytes,const T& value){bytes.append(reinterpret_cast<const char*>(&value),sizeof value);}
std::string Fixture(){
 std::string bytes;for(unsigned v:{0x3154494bu,1u,unsigned(classart::Part::Count),2u})Add(bytes,v);
 for(unsigned part=0;part<unsigned(classart::Part::Count);++part)for(unsigned lod=0;lod<2;++lod){
  Add(bytes,3u);Add(bytes,lod?3u:6u);
  for(auto p:{DirectX::XMFLOAT3{0,0,0},DirectX::XMFLOAT3{.1f,0,0},DirectX::XMFLOAT3{0,.1f,0}}){
   visual::Vertex vertex{p,{0,0,lod?1.f:0.f},{1,1,1,0}};Add(bytes,vertex);
  }
  for(unsigned i:{0u,1u,2u})Add(bytes,i);
  if(!lod)for(unsigned i:{2u,1u,0u})Add(bytes,i);
 }
 return bytes;
}
void Reject(const std::string& data){bool rejected=false;try{classart::Decode(data);}catch(const std::exception&){rejected=true;}Check(rejected,"Malformed class normal accepted");}
}
int main()try{
 const auto data=Fixture();const auto art=classart::Decode(data);
 for(const auto& part:art){
  const auto& mesh=part[0];Check(mesh.vertices.size()==6,"Opposite faces did not split shared zero normals");
  for(unsigned i=0;i<6;++i)Check(std::abs(mesh.vertices[mesh.indices[i]].normal.z-(i<3?1.f:-1.f))<.0001f,"Face normal has the wrong orientation");
 }
 for(float n:{std::numeric_limits<float>::quiet_NaN(),2.f,.01f}){
  auto bad=data;std::memcpy(bad.data()+24+12,&n,sizeof n);Reject(bad);
 }
 auto degenerate=data;std::memcpy(degenerate.data()+24+40,degenerate.data()+24,3*sizeof(float));Reject(degenerate);
 std::cout<<"PASS: opposed-face normals repaired, nonunit/NaN/degenerate data rejected\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
