#include "engine/ClassArt.h"
#include <Windows.h>
#include <algorithm>
#include <cstring>
#include <cmath>
#include <stdexcept>
namespace classart {
namespace {
void Check(bool value,const char* reason){if(!value)throw std::runtime_error(reason);}
struct Reader {
 std::string_view bytes;std::size_t offset=0;
 template<class T>T Get(){Check(sizeof(T)<=bytes.size()-offset,"Truncated class art");T value;std::memcpy(&value,bytes.data()+offset,sizeof(T));offset+=sizeof(T);return value;}
};
}
Library Decode(std::string_view bytes){
 Check(bytes.size()<=16*1024*1024,"Class art byte budget exceeded");Reader r{bytes};
 Check(r.Get<unsigned>()==0x3154494b && r.Get<unsigned>()==1 && r.Get<unsigned>()==unsigned(Part::Count) && r.Get<unsigned>()==2,"Invalid class art header");
 Library result;std::size_t decodedBytes=0;
 for(auto& item:result)for(unsigned lod=0;lod<2;++lod){
  auto& g=item[lod];const unsigned vertices=r.Get<unsigned>(),indices=r.Get<unsigned>();
  Check(vertices>=3&&vertices<65536&&indices>=3&&indices<300000&&indices%3==0,"Invalid class art counts");
  Check(std::size_t(vertices)*sizeof(visual::Vertex)+std::size_t(indices)*4<=bytes.size()-r.offset,"Truncated class art mesh");
  g.vertices.reserve(vertices);g.indices.reserve(indices);
  std::vector<unsigned char> missingNormal(vertices);
  for(unsigned i=0;i<vertices;++i){
   const auto v=r.Get<visual::Vertex>();
   Check(std::isfinite(v.position.x)&&std::isfinite(v.position.y)&&std::isfinite(v.position.z)&&std::abs(v.position.x)<2&&std::abs(v.position.y)<3&&std::abs(v.position.z)<2,"Class art escaped attachment envelope");
   const float n=v.normal.x*v.normal.x+v.normal.y*v.normal.y+v.normal.z*v.normal.z;
   Check(std::isfinite(n)&&(n==0 || std::abs(n-1)<.001f),"Invalid class art normal");
   missingNormal[i]=n==0;
   for(float c:{v.color.x,v.color.y,v.color.z})Check(std::isfinite(c)&&c>=0&&c<=1,"Invalid class art color");
   Check(std::isfinite(v.color.w)&&v.color.w>=0&&v.color.w<unsigned(visual::Material::Count)&&std::floor(v.color.w)==v.color.w,"Invalid class art material");
   g.vertices.push_back(v);
  }
  for(unsigned i=0;i<indices;++i){const auto index=r.Get<unsigned>();Check(index<vertices,"Invalid class art index");g.indices.push_back(index);}
  // Oppositely wound faces can cancel Blender's shared vertex normal. Split
  // only these corners and reconstruct each face normal; retain all valid art.
  for(unsigned i=0;i<indices;i+=3){
   const auto ia=g.indices[i],ib=g.indices[i+1],ic=g.indices[i+2];
   if(!missingNormal[ia]&&!missingNormal[ib]&&!missingNormal[ic])continue;
   const auto a=g.vertices[ia].position,b=g.vertices[ib].position,c=g.vertices[ic].position;
   const DirectX::XMFLOAT3 u{b.x-a.x,b.y-a.y,b.z-a.z},v{c.x-a.x,c.y-a.y,c.z-a.z};
   DirectX::XMFLOAT3 normal{u.y*v.z-u.z*v.y,u.z*v.x-u.x*v.z,u.x*v.y-u.y*v.x};
   const float length=std::sqrt(normal.x*normal.x+normal.y*normal.y+normal.z*normal.z);
   Check(std::isfinite(length)&&length>1e-10f,"Degenerate class art face has no normal");
   normal.x/=length;normal.y/=length;normal.z/=length;
   for(unsigned corner=0;corner<3;++corner){
    auto& index=g.indices[i+corner];if(!missingNormal[index])continue;
    if(missingNormal[index]==1){g.vertices[index].normal=normal;missingNormal[index]=2;}
    else {auto vertex=g.vertices[index];vertex.normal=normal;
     Check(g.vertices.size()<65535,"Repaired class art exceeded vertex budget");
     index=unsigned(g.vertices.size());g.vertices.push_back(vertex);
    }
   }
  }
  Check(std::find(missingNormal.begin(),missingNormal.end(),1)==missingNormal.end(),"Unused class art vertex has no normal");
  decodedBytes+=g.vertices.size()*sizeof(visual::Vertex)+g.indices.size()*4;
  Check(decodedBytes<=16*1024*1024,"Repaired class art exceeded byte budget");
  if(lod)Check(indices<item[0].indices.size()*.65f,"Class art LOD did not reduce geometry");
 }
 Check(r.offset==bytes.size(),"Trailing class art data");return result;
}
const Library& Collection(){static const auto library=[] {
 const auto module=GetModuleHandleW(nullptr);const auto resource=FindResourceW(module,MAKEINTRESOURCEW(305),MAKEINTRESOURCEW(10));
 Check(resource!=nullptr,"Missing Blender class art");const auto handle=LoadResource(module,resource);const auto data=handle?LockResource(handle):nullptr;
 Check(data!=nullptr,"Cannot read Blender class art");return Decode({static_cast<const char*>(data),SizeofResource(module,resource)});
}();return library;}
visual::Mesh Mesh(Part part,unsigned lod){return visual::Mesh(unsigned(visual::Mesh::OldWarriorChest)+unsigned(part)*2+std::min(lod,1u));}
}
