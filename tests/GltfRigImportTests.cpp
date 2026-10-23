#include "tools/GltfRigImport.h"
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <cmath>

void Require(bool ok,const char* message) {if(!ok) {std::cerr<<message<<'\n';std::exit(1);}}
void Put(std::string& s,std::size_t at,std::uint32_t v) {for(unsigned i=0;i<4;++i) s[at+i]=char((v>>(8*i))&255);}
std::string Glb(std::string json) {
  while(json.size()%4) json+=' ';std::string result(20,'\0');
  Put(result,0,0x46546c67);Put(result,4,2);Put(result,8,std::uint32_t(20+json.size()));
  Put(result,12,std::uint32_t(json.size()));Put(result,16,0x4e4f534a);return result+json;
}
void Reject(const std::string& data) {
  try {assets::ImportGlbAsset(data);} catch(const std::runtime_error& e) {
    Require(std::string(e.what()).find("GLB rig:")==0,"Import errors must identify the asset format");return;
  }
  Require(false,"Malformed or unsupported asset was accepted");
}
std::string AlterJson(const std::string& glb,const std::string& from,const std::string& to) {
  std::uint32_t length=0;for(unsigned i=0;i<4;++i) length|=std::uint32_t(static_cast<unsigned char>(glb[12+i]))<<(8*i);
  auto json=glb.substr(20,length);const auto at=json.find(from);Require(at!=std::string::npos,"Mutation fixture field missing");
  json.replace(at,from.size(),to);auto result=Glb(json);result+=glb.substr(20+length);Put(result,8,std::uint32_t(result.size()));return result;
}
int main() {
  std::ifstream input(std::string(RPG_RIG_FIXTURE),std::ios::binary);std::ostringstream bytes;bytes<<input.rdbuf();
  Require(bool(input),"Missing original GLB fixture");const auto source=bytes.str();
  const auto rig=assets::ImportGlbRig(source);Require(rig.Size()==2,"Fixture joint count");const auto pose=rig.RestPose();
  Reject(AlterJson(source,"\"generator\":\"Mediterranean original hinge fixture\"","\"generator\":12"));
  Reject(AlterJson(source,"\"joints\":[1,0]","\"name\":{},\"joints\":[1,0]"));
  for(const auto& m:pose.skin) for(int r=0;r<4;++r) for(int c=0;c<4;++c)
    Require(std::abs(m.m[r][c]-(r==c?1.f:0.f))<.0001f,"Explicit inverse binds must preserve the original mesh");
  Require(std::abs(pose.global[0]._42-1)<.0001f && std::abs(pose.global[0]._43+2)<.0001f,"Joint remapping and Z reflection must agree");
  const std::string simple=R"({"asset":{"version":"2.0"},"nodes":[{"translation":[0,1,2]}],"skins":[{"joints":[0]}]})";
  const auto identityBind=assets::ImportGlbRig(Glb(simple)).RestPose();
  Require(identityBind.skin[0]._42==1 && identityBind.skin[0]._43==-2,"Omitted glTF binds mean identity, not computed rest inverses");
  const auto rotation=assets::ImportGlbRig(Glb(R"({"asset":{"version":"2.0"},"nodes":[{"rotation":[0,0.70710678,0,0.70710678]}],"skins":[{"joints":[0]}]})")).RestPose();
  DirectX::XMFLOAT3 rotated;
  DirectX::XMStoreFloat3(&rotated,DirectX::XMVector3TransformCoord(DirectX::XMVectorSet(1,0,0,1),DirectX::XMLoadFloat4x4(&rotation.skin[0])));
  Require(std::abs(rotated.z-1)<.0001f,"Quaternion handedness conversion must agree with reflected positions");
  for(std::size_t n:{std::size_t(0),std::size_t(12),source.size()-1}) Reject(source.substr(0,n));
  auto bad=source;Put(bad,8,0);Reject(bad);
  bad=source;Put(bad,12,0xfffffffc);Reject(bad);
  bad=source;Put(bad,4,1);Reject(bad);
  bad=source;Put(bad,16,0);Reject(bad);
  bad=source;Put(bad,bad.size()-4,0x7fc00000);Reject(bad);
  Reject(AlterJson(source,"\"buffer\":0","\"buffer\":0,\"byteStride\":64"));
  Reject(AlterJson(source,"\"bufferView\":0","\"bufferView\":0,\"byteOffset\":1"));
  Reject(AlterJson(source,"\"byteLength\":128","\"byteLength\":127"));
  Reject(AlterJson(source,"\"componentType\":5126","\"componentType\":5123"));
  Reject(AlterJson(source,"\"byteLength\":128","\"byteLength\":128,\"uri\":\"outside.bin\""));
  Reject(AlterJson(source,"\"count\":2","\"count\":2,\"sparse\":{}"));
  Reject(AlterJson(source,"\"joints\":[1,0]","\"joints\":[1,1]"));
  Reject(Glb(std::string(25,'[')+"0"+std::string(25,']')));
  for(const std::string extra:{",\"meshes\":[]",",\"animations\":[]",",\"extensionsRequired\":[\"unknown\"]",",\"asset\":{}"}) {
    auto json=simple;json.insert(json.size()-1,extra);Reject(Glb(json));
  }
  for(const char* parents:{R"([{"children":[1]},{"children":[0]}])",R"([{"children":[1,1]},{}])",R"([{},{}])"})
    Reject(Glb(std::string(R"({"asset":{"version":"2.0"},"nodes":)")+parents+R"(,"skins":[{"joints":[0,1]}]})"));
  Reject(Glb(R"({"asset":{"version":"2.0"},"nodes":[{"matrix":[]}],"skins":[{"joints":[0]}]})"));
  Reject(Glb(R"({"asset":{"version":"2.0"},"nodes":[{"translation":[00,0,0]}],"skins":[{"joints":[0]}]})"));
  Reject(Glb(R"({"asset":{"version":"2.0"},"nodes":[{"name":"\u0041"}],"skins":[{"joints":[0]}]})"));
  Reject(Glb(simple+"garbage"));
  Reject(std::string(4*1024*1024+1,'x'));
  // Truncate the accessor's declared range while retaining a valid GLB envelope.
  bad=source;const auto where=bad.find("\"count\":2");Require(where!=std::string::npos,"Fixture accessor marker");bad[where+8]='3';Reject(bad);
  std::ifstream motionFile(RPG_MOTION_FIXTURE,std::ios::binary);std::ostringstream motionBytes;motionBytes<<motionFile.rdbuf();
  Require(bool(motionFile),"Missing animated fixture");const auto motion=motionBytes.str();
  const auto asset=assets::ImportGlbAsset(motion);
  Require(asset.clips.size()==1 && asset.clips[0].name=="hinge_motion","Imported named clip");
  const auto& clip=asset.clips[0].clip;
  Require(clip.duration==1 && clip.tracks[0].size()==5 && clip.tracks[1].empty(),"Different timelines merge at all source keys; untargeted joint remains at rest");
  for(const auto sample: {std::pair<double,float>{0,0},{.125,0},{.5,1},{.875,2},{1,2}}) {
    const auto pose=asset.skeleton.Sample(clip,sample.first,false);
    Require(std::abs(pose.global[0]._41-sample.second)<1e-5f && std::abs(pose.global[0]._43-(-2-sample.second))<1e-5f,"Translation endpoints hold and Z converts consistently");
  }
  const auto half=asset.skeleton.Sample(clip,.5,false);
  Require(std::abs(half.global[0]._11-std::sqrt(2.f))<1e-4f && std::abs(half.global[0]._13-std::sqrt(2.f))<1e-4f && std::abs(half.global[0]._22-3)<1e-4f,"Imported rotation uses slerp and independent scale channel");
  const auto loop=asset.skeleton.Sample(clip,1,true);
  Require(std::abs(loop.global[0]._41)<1e-5f,"Imported clips preserve loop boundary behavior");
  bool staticRejected=false;try {assets::ImportGlbRig(motion);} catch(const std::runtime_error&) {staticRejected=true;}
  Require(staticRejected,"Static-only API must not silently discard animation");
  Reject(AlterJson(motion,"\"LINEAR\"","\"CUBICSPLINE\""));
  Reject(AlterJson(motion,"\"LINEAR\"","\"STEP\""));
  Reject(AlterJson(motion,"\"path\":\"translation\"","\"path\":\"rotation\""));
  Reject(AlterJson(motion,"\"path\":\"rotation\"","\"path\":\"weights\""));
  Reject(AlterJson(motion,"\"input\":1","\"input\":250"));
  Reject(AlterJson(motion,"\"min\":[0.0]","\"min\":[-1.0]"));
  Reject(AlterJson(motion,"\"output\":2","\"output\":4"));
  std::uint32_t jsonLength=0;for(unsigned i=0;i<4;++i) jsonLength|=std::uint32_t(static_cast<unsigned char>(motion[12+i]))<<(8*i);
  auto invalidMotion=motion;Put(invalidMotion,28+jsonLength+132,0);Reject(invalidMotion); // repeated time
  invalidMotion=motion;Put(invalidMotion,28+jsonLength+148,0);Reject(invalidMotion); // zero first rotation quaternion
  invalidMotion=motion;Put(invalidMotion,28+jsonLength+176,0x7fc00000);Reject(invalidMotion); // NaN translation
  std::cout<<"GLB rig import, coordinate conversion and malformed-input checks passed\n";
}
