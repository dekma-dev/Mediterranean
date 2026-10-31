#include "engine/PlayerBody.h"
#include <Windows.h>
#include <cmath>
#include <cstring>
#include <stdexcept>

namespace playerbody {
namespace {
void Check(bool ok,const char* message) {if(!ok)throw std::runtime_error(message);}
struct Reader {
 std::string_view bytes;std::size_t offset=0;
 template<class T>T Get() {
  Check(sizeof(T)<=bytes.size()-offset,"Truncated player body archive");
  T value;std::memcpy(&value,bytes.data()+offset,sizeof(value));offset+=sizeof(value);return value;
 }
};
}
Library Decode(std::string_view bytes) {
 using namespace DirectX;
 Check(bytes.size()<=1024*1024,"Player body archive exceeds its byte budget");Reader r{bytes};
 Check(r.Get<std::uint32_t>()==0x314b5350 && r.Get<std::uint32_t>()==1 &&
  r.Get<std::uint32_t>()==visual::bodyBones && r.Get<std::uint32_t>()==visual::bodyLodCount,"Invalid player body header");
 for(const auto& bone:visual::BodyBindPose()) for(const auto expected:{bone.from,bone.to}) {
  const auto actual=r.Get<XMFLOAT3>();
  Check(std::isfinite(actual.x)&&std::isfinite(actual.y)&&std::isfinite(actual.z)&&
   std::abs(actual.x-expected.x)<1e-5f && std::abs(actual.y-expected.y)<1e-5f && std::abs(actual.z-expected.z)<1e-5f,
   "Player body was exported for a different bind pose");
 }
 Library result;
 for(unsigned lod=0;lod<result.size();++lod) {
  auto& body=result[lod];const auto vertices=r.Get<std::uint32_t>(),indices=r.Get<std::uint32_t>();
  Check(vertices>=3 && vertices<=16384 && indices>=3 && indices<=65535 && indices%3==0,"Invalid player body counts");
  Check(std::size_t(vertices)*60+std::size_t(indices)*4<=bytes.size()-r.offset,"Truncated player body mesh");
  if(lod)Check(indices<result[lod-1].mesh.indices.size()*.65,"Player body LOD must reduce topology");
  body.mesh.vertices.reserve(vertices);body.influences.reserve(vertices);body.mesh.indices.reserve(indices);
  for(unsigned i=0;i<vertices;++i) {
   visual::Vertex v;v.position=r.Get<XMFLOAT3>();v.normal=r.Get<XMFLOAT3>();
   const auto region=r.Get<std::uint32_t>();Check(region<visual::bodyRegions,"Invalid player body region");
   v.color={1,1,1,float(region)};visual::SkinInfluence skin;
   for(auto& bone:skin.bones)bone=r.Get<std::uint32_t>();skin.weights=r.Get<XMFLOAT4>();
   Check(std::abs(v.position.x)<.9f && v.position.y>.1f && v.position.y<1.64f && std::abs(v.position.z)<.4f,
    "Player anatomy exceeds the fitted body envelope");
   body.mesh.vertices.push_back(v);body.influences.push_back(skin);
  }
  for(unsigned i=0;i<indices;++i)body.mesh.indices.push_back(r.Get<std::uint32_t>());
  visual::ValidateBodyGeometry(body);
 }
 Check(r.offset==bytes.size(),"Trailing player body data");return result;
}
std::string_view EmbeddedBytes() {
 const auto module=GetModuleHandleW(nullptr);
 const auto resource=FindResourceW(module,MAKEINTRESOURCEW(304),MAKEINTRESOURCEW(10));
 Check(resource!=nullptr,"Missing anatomical player body");
 const auto handle=LoadResource(module,resource);const auto data=handle?LockResource(handle):nullptr;
 Check(data!=nullptr,"Cannot load anatomical player body");
 return {static_cast<const char*>(data),SizeofResource(module,resource)};
}
const Library& Collection() {static const auto bodies=Decode(EmbeddedBytes());return bodies;}
}
