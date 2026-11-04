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
 std::string garment;for(unsigned v:{0x32545747u,2u,12u,2u})Add(garment,v);
 for(unsigned part:{0u,1u,7u,8u,14u,15u,21u,22u,23u,24u,25u,26u}) {
  Add(garment,part);
  for(unsigned lod=0;lod<2;++lod) {
   const unsigned count=unsigned(art[part][lod].vertices.size());Add(garment,count);
   const auto influence=part>=21&&part<=23?visual::SkinInfluence{{0,2,4,3},{.4f,.2f,.3f,.1f}}:
     visual::SkinInfluence{{0,1,6,7},{.2f,.4f,.3f,.1f}};
   for(unsigned v=0;v<count;++v)Add(garment,influence);
  }
 }
 const auto weights=classart::DecodeGarmentWeights(garment,art);
 Check(weights[0][0].size()==6 && weights[23][1].size()==3 && weights[2][0].empty(),"Garment part/LOD weights mismatch");
 const auto rejectGarment=[&](const std::string& bad){bool rejected=false;try{classart::DecodeGarmentWeights(bad,art);}catch(const std::exception&){rejected=true;}Check(rejected,"Malformed garment weights accepted");};
 for(unsigned cut:{0u,15u,20u,unsigned(garment.size()-1)})rejectGarment(garment.substr(0,cut));
 for(unsigned part:{2u,27u,0xffffffffu}) {
  auto bad=garment;std::memcpy(bad.data()+16,&part,4);rejectGarment(bad);
 }
 auto duplicate=garment;const unsigned firstPart=0;std::memcpy(duplicate.data()+316,&firstPart,4);rejectGarment(duplicate);
 auto badCount=garment;const unsigned count=100000;std::memcpy(badCount.data()+20,&count,4);rejectGarment(badCount);
 auto badBone=garment;const unsigned bone=10;std::memcpy(badBone.data()+24,&bone,4);rejectGarment(badBone);
 for(float value:{-.1f,1.1f,std::numeric_limits<float>::quiet_NaN(),.1f}) {
  auto bad=garment;std::memcpy(bad.data()+40,&value,4);rejectGarment(bad);
 }
 rejectGarment(garment+"x");
 std::cout<<"PASS: garment LOD/count, bone, finite normalization and truncation checks\n";
 std::cout<<"PASS: opposed-face normals repaired, nonunit/NaN/degenerate data rejected\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
