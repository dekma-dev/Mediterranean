#include "tools/GltfRigImport.h"
#include <algorithm>
#include <array>
#include <charconv>
#include <cmath>
#include <cstring>
#include <map>
#include <stdexcept>

namespace assets {
namespace {
[[noreturn]] void Fail(const std::string& message) {throw std::runtime_error("GLB rig: "+message);}
void Need(bool ok,const std::string& message) {if(!ok) Fail(message);}
struct Json {
  enum Kind {Object,Array,String,Number,Literal} kind=Literal;
  std::map<std::string,Json> members;std::vector<Json> values;
  std::string text;double number=0;
};
class Reader {
  std::string_view source;std::size_t pos=0,nodes=0;
  void White() {while(pos<source.size() && (source[pos]==' ' || source[pos]=='\r' || source[pos]=='\n' || source[pos]=='\t')) ++pos;}
  char Take() {Need(pos<source.size(),"truncated JSON at byte "+std::to_string(pos));return source[pos++];}
  std::string String() {
    Need(Take()=='"',"expected JSON string");std::string text;
    while(true) {
      const unsigned char ch=static_cast<unsigned char>(Take());if(ch=='"') return text;
      Need(ch>=32 && ch<127,"only ASCII strings supported in this import subset");
      if(ch=='\\') {
        const char escaped=Take();Need(escaped=='"' || escaped=='\\' || escaped=='/',"unsupported string escape");text+=escaped;
      } else text+=char(ch);
      Need(text.size()<=1024,"JSON string budget exceeded");
    }
  }
  Json Value(unsigned depth) {
    Need(depth<=24 && ++nodes<=8192,"JSON nesting/value budget exceeded");White();Json j;
    Need(pos<source.size(),"missing JSON value");const char c=source[pos];
    if(c=='{' || c=='[') {
      ++pos;j.kind=c=='{'?Json::Object:Json::Array;White();
      const char close=c=='{'?'}':']';
      if(pos<source.size() && source[pos]==close) {++pos;return j;}
      while(true) {
        White();
        if(c=='{') {
          const auto name=String();White();Need(Take()==':',"expected colon");
          auto value=Value(depth+1);Need(j.members.emplace(name,std::move(value)).second,"duplicate JSON key: "+name);
        } else j.values.push_back(Value(depth+1));
        White();const char separator=Take();if(separator==close) return j;
        Need(separator==',',"expected JSON separator");
      }
    }
    if(c=='"') {j.kind=Json::String;j.text=String();return j;}
    const auto start=pos;
    while(pos<source.size() && source[pos]!=',' && source[pos]!=']' && source[pos]!='}' && source[pos]!=' ' && source[pos]!='\n' && source[pos]!='\r' && source[pos]!='\t') ++pos;
    const auto token=source.substr(start,pos-start);
    if(token=="true" || token=="false" || token=="null") return j;
    // Validate JSON number grammar before conversion (from_chars alone accepts
    // some non-JSON forms). No leading plus, leading zeros or incomplete exponent.
    std::size_t n=0;if(n<token.size() && token[n]=='-') ++n;
    Need(n<token.size(),"invalid JSON number");
    if(token[n]=='0') ++n;
    else {Need(token[n]>='1' && token[n]<='9',"invalid JSON number");while(n<token.size() && token[n]>='0' && token[n]<='9') ++n;}
    if(n<token.size() && token[n]=='.') {const auto begin=++n;while(n<token.size() && token[n]>='0' && token[n]<='9') ++n;Need(n>begin,"invalid JSON fraction");}
    if(n<token.size() && (token[n]=='e' || token[n]=='E')) {++n;if(n<token.size() && (token[n]=='+' || token[n]=='-')) ++n;const auto begin=n;while(n<token.size() && token[n]>='0' && token[n]<='9') ++n;Need(n>begin,"invalid JSON exponent");}
    Need(n==token.size(),"invalid JSON number suffix");
    const auto parsed=std::from_chars(token.data(),token.data()+token.size(),j.number);
    Need(parsed.ec==std::errc{} && parsed.ptr==token.data()+token.size() && std::isfinite(j.number),"invalid finite JSON number");j.kind=Json::Number;return j;
  }
public:
  explicit Reader(std::string_view text):source(text) {}
  Json Read() {auto value=Value(0);White();Need(pos==source.size(),"trailing JSON data");return value;}
};
const Json* Optional(const Json& j,const char* key) {
  Need(j.kind==Json::Object,"expected object");const auto it=j.members.find(key);return it==j.members.end()?nullptr:&it->second;
}
const Json& Field(const Json& j,const char* key) {const auto* v=Optional(j,key);Need(v!=nullptr,std::string("missing field: ")+key);return *v;}
void Fields(const Json& j,std::initializer_list<const char*> allowed) {
  Need(j.kind==Json::Object,"expected object");
  for(const auto& field:j.members) Need(std::any_of(allowed.begin(),allowed.end(),[&](const char* name){return field.first==name;}),"unsupported field: "+field.first);
}
const std::vector<Json>& Array(const Json& j) {Need(j.kind==Json::Array,"expected array");return j.values;}
std::string Text(const Json& j) {Need(j.kind==Json::String,"expected string");return j.text;}
std::size_t Integer(const Json& j) {Need(j.kind==Json::Number && j.number>=0 && j.number<=4*1024*1024 && std::floor(j.number)==j.number,"invalid bounded integer");return std::size_t(j.number);}
std::size_t Offset(const Json& j) {const auto* v=Optional(j,"byteOffset");return v?Integer(*v):0;}
void Floats(const Json& j,float* dest,std::size_t count) {
  const auto& a=Array(j);Need(a.size()==count,"wrong transform component count");
  for(std::size_t i=0;i<count;++i) {Need(a[i].kind==Json::Number && std::abs(a[i].number)<=1e8,"invalid transform component");dest[i]=float(a[i].number);}
}
std::uint32_t U32(std::string_view bytes,std::size_t offset) {
  Need(offset<=bytes.size() && bytes.size()-offset>=4,"truncated GLB integer");
  const auto* p=reinterpret_cast<const unsigned char*>(bytes.data()+offset);
  return std::uint32_t(p[0])|(std::uint32_t(p[1])<<8)|(std::uint32_t(p[2])<<16)|(std::uint32_t(p[3])<<24);
}
}
RigAsset ImportGlbAsset(std::string_view bytes) {
  Need(bytes.size()>=20 && bytes.size()<=4*1024*1024,"file budget is 20 bytes..4 MiB");
  Need(U32(bytes,0)==0x46546c67 && U32(bytes,4)==2 && U32(bytes,8)==bytes.size(),"invalid GLB header/version/length");
  std::string_view json,binary;std::size_t at=12;unsigned chunks=0;
  while(at<bytes.size()) {
    const auto length=U32(bytes,at),type=U32(bytes,at+4);at+=8;
    Need(length%4==0 && length<=bytes.size()-at,"invalid chunk bounds/alignment");
    if(chunks==0) {Need(type==0x4e4f534a && length<=1024*1024,"first chunk must be bounded JSON");json=bytes.substr(at,length);}
    else {Need(chunks==1 && type==0x004e4942,"unsupported or repeated chunk");binary=bytes.substr(at,length);}
    at+=length;++chunks;
  }
  const auto doc=Reader(json).Read();Fields(doc,{"asset","nodes","skins","buffers","bufferViews","accessors","animations","meshes"});
  const auto& asset=Field(doc,"asset");Fields(asset,{"version","generator"});Need(Text(Field(asset,"version"))=="2.0","only glTF 2.0 supported");
  if(const auto* generator=Optional(asset,"generator")) (void)Text(*generator);
  const auto& nodes=Array(Field(doc,"nodes"));const auto& skins=Array(Field(doc,"skins"));
  Need(!nodes.empty() && nodes.size()<=65 && skins.size()==1,"expected at most 65 nodes and exactly one skin");
  const auto& skin=skins[0];Fields(skin,{"name","joints","skeleton","inverseBindMatrices"});
  if(const auto* name=Optional(skin,"name")) (void)Text(*name);
  const auto& ids=Array(Field(skin,"joints"));Need(!ids.empty() && ids.size()<=64,"joint budget is 1..64");
  const bool hasMesh=Optional(doc,"meshes")!=nullptr;
  Need(nodes.size()==ids.size()+(hasMesh?1:0),"expected joints and optionally one identity mesh node");
  std::vector<int> remap(nodes.size(),-1);
  for(std::size_t i=0;i<ids.size();++i) {const auto id=Integer(ids[i]);Need(id<nodes.size() && remap[id]<0,"invalid or duplicate skin joint");remap[id]=int(i);}
  std::vector<animation::Joint> joints(ids.size());std::vector<int> parents(nodes.size(),-1);
  for(std::size_t id=0;id<nodes.size();++id) {
    const auto& n=nodes[id];if(remap[id]>=0) Fields(n,{"name","children","translation","rotation","scale"});
    if(remap[id]<0) {
      Fields(n,{"name","mesh","skin"});
      if(const auto* name=Optional(n,"name")) (void)Text(*name);
      Need(Integer(Field(n,"mesh"))==0 && Integer(Field(n,"skin"))==0,"mesh node must reference mesh/skin zero");
      continue;
    }
    auto& j=joints[remap[id]];const auto* name=Optional(n,"name");j.name=name?Text(*name):"joint_"+std::to_string(id);
    if(const auto* v=Optional(n,"translation")) {float t[3];Floats(*v,t,3);j.local.translation={t[0],t[1],-t[2]};}
    if(const auto* v=Optional(n,"rotation")) {float q[4];Floats(*v,q,4);j.local.rotation={-q[0],-q[1],q[2],q[3]};}
    if(const auto* v=Optional(n,"scale")) {float s[3];Floats(*v,s,3);j.local.scale={s[0],s[1],s[2]};}
    if(const auto* children=Optional(n,"children")) for(const auto& child:Array(*children)) {
      const auto c=Integer(child);Need(c<nodes.size() && c!=id && remap[c]>=0 && parents[c]<0,"invalid or multiply-parented node");parents[c]=int(id);
    }
  }
  for(std::size_t id=0;id<nodes.size();++id) if(remap[id]>=0) joints[remap[id]].parent=parents[id]<0?-1:remap[parents[id]];
  if(const auto* root=Optional(skin,"skeleton")) {const auto id=Integer(*root);Need(id<nodes.size() && remap[id]>=0 && parents[id]<0,"skeleton must identify the root");}
  std::vector<DirectX::XMFLOAT4X4> binds(ids.size());
  for(auto& m:binds) DirectX::XMStoreFloat4x4(&m,DirectX::XMMatrixIdentity());
  struct Accessor {std::string type;std::size_t count=0,components=0,componentType=0;std::vector<float> values;bool bounds=false;};
  std::vector<Accessor> data;
  if(Optional(doc,"buffers")) {
    const auto& buffers=Array(Field(doc,"buffers"));Need(buffers.size()==1,"exactly one embedded buffer required");
    Fields(buffers[0],{"byteLength"});const auto declared=Integer(Field(buffers[0],"byteLength"));
    Need(declared<=binary.size() && binary.size()-declared<=3,"invalid embedded buffer length");
    const auto& accessors=Array(Field(doc,"accessors"));const auto& views=Array(Field(doc,"bufferViews"));
    Need(!accessors.empty() && accessors.size()<=256 && !views.empty() && views.size()<=256,"accessor/view budget exceeded");
    for(const auto& view:views) {
      Fields(view,{"buffer","byteOffset","byteLength"});Need(Integer(Field(view,"buffer"))==0,"invalid buffer index");
      const auto offset=Offset(view),length=Integer(Field(view,"byteLength"));
      Need(offset%4==0 && length>0 && offset<=declared && length<=declared-offset,"buffer view exceeds embedded buffer");
    }
    std::size_t floats=0;
    for(const auto& accessor:accessors) {
      Fields(accessor,{"bufferView","byteOffset","componentType","count","type","min","max"});
      const auto componentType=Integer(Field(accessor,"componentType"));
      Need(componentType==5126 || componentType==5123,"only FLOAT/UNSIGNED_SHORT accessors supported");
      const std::size_t componentBytes=componentType==5126?4:2;
      Accessor a;a.componentType=componentType;a.type=Text(Field(accessor,"type"));a.count=Integer(Field(accessor,"count"));
      a.components=a.type=="SCALAR"?1:a.type=="VEC3"?3:a.type=="VEC4"?4:a.type=="MAT4"?16:0;
      Need(a.components && a.count>0 && a.count<=65536,"unsupported accessor shape/count");
      Need(componentType==5126 || a.type=="SCALAR" || a.type=="VEC4","UNSIGNED_SHORT supports indices/joints only");
      floats+=a.count*a.components;Need(floats<=262144,"decoded accessor budget exceeded");
      const auto v=Integer(Field(accessor,"bufferView"));Need(v<views.size(),"invalid buffer view index");
      const auto& view=views[v];const auto start=Offset(view),length=Integer(Field(view,"byteLength")),relative=Offset(accessor);
      Need(relative%4==0 && relative<=length && a.count*a.components*componentBytes<=length-relative,"accessor alignment/range invalid");
      a.values.resize(a.count*a.components);
      for(std::size_t i=0;i<a.values.size();++i) {
        const auto offset=start+relative+i*componentBytes;
        if(componentType==5126) {const auto bits=U32(binary,offset);std::memcpy(&a.values[i],&bits,4);}
        else a.values[i]=float(static_cast<unsigned char>(binary[offset]) | (unsigned(static_cast<unsigned char>(binary[offset+1]))<<8));
        Need(std::isfinite(a.values[i]),"non-finite accessor value");
      }
      for(const char* label:{"min","max"}) if(const auto* bound=Optional(accessor,label)) {
        const auto& entries=Array(*bound);Need(entries.size()==a.components,"invalid accessor bound size");
        for(std::size_t c=0;c<a.components;++c) {
          float actual=a.values[c];for(std::size_t k=c+a.components;k<a.values.size();k+=a.components)
            actual=label[1]=='i'?std::min(actual,a.values[k]):std::max(actual,a.values[k]);
          Need(entries[c].kind==Json::Number && float(entries[c].number)==actual,"accessor bound does not match data");
        }
      }
      a.bounds=Optional(accessor,"min") && Optional(accessor,"max");data.push_back(std::move(a));
    }
  } else {
    Need(binary.empty() && !Optional(doc,"bufferViews") && !Optional(doc,"accessors"),"missing embedded buffer");
  }
  const auto get=[&](const Json& index,const char* type,std::size_t componentType=5126)->const Accessor& {
    const auto id=Integer(index);Need(id<data.size() && data[id].type==type && data[id].componentType==componentType,"invalid accessor index/type");return data[id];
  };
  if(const auto* index=Optional(skin,"inverseBindMatrices")) {
    const auto& a=get(*index,"MAT4");Need(a.count==ids.size(),"bind count must match joint count");
    for(std::size_t i=0;i<a.count;++i) for(unsigned r=0;r<4;++r) for(unsigned c=0;c<4;++c)
      binds[i].m[r][c]=a.values[i*16+r*4+c]*((r==2)!=(c==2)?-1.f:1.f);
  }
  try {
    RigAsset result{animation::Skeleton(joints,std::move(binds)),{}, {}};
    if(hasMesh) {
      const auto& meshes=Array(Field(doc,"meshes"));Need(meshes.size()==1,"exactly one mesh supported");
      Fields(meshes[0],{"name","primitives"});
      if(const auto* name=Optional(meshes[0],"name")) (void)Text(*name);
      const auto& primitives=Array(Field(meshes[0],"primitives"));Need(primitives.size()==1,"exactly one mesh primitive supported");
      const auto& primitive=primitives[0];Fields(primitive,{"attributes","indices","mode"});
      if(const auto* mode=Optional(primitive,"mode")) Need(Integer(*mode)==4,"only TRIANGLES supported");
      const auto& attributes=Field(primitive,"attributes");Fields(attributes,{"POSITION","NORMAL","JOINTS_0","WEIGHTS_0"});
      const auto& positions=get(Field(attributes,"POSITION"),"VEC3");
      const auto& normals=get(Field(attributes,"NORMAL"),"VEC3");
      const auto& jointIds=get(Field(attributes,"JOINTS_0"),"VEC4",5123);
      const auto& weights=get(Field(attributes,"WEIGHTS_0"),"VEC4");
      const auto& indices=get(Field(primitive,"indices"),"SCALAR",5123);
      Need(positions.bounds && positions.count>=3 && positions.count<=16384,"position bounds/vertex budget invalid");
      Need(normals.count==positions.count && jointIds.count==positions.count && weights.count==positions.count,"mesh attribute counts differ");
      Need(indices.count%3==0,"triangle index count must be divisible by three");
      auto& mesh=result.mesh;mesh.vertices.resize(positions.count);
      for(std::size_t v=0;v<positions.count;++v) {
        auto& vertex=mesh.vertices[v];
        vertex.position={positions.values[v*3],positions.values[v*3+1],-positions.values[v*3+2]};
        vertex.normal={normals.values[v*3],normals.values[v*3+1],-normals.values[v*3+2]};
        for(unsigned c=0;c<4;++c) {
          vertex.joints[c]=std::uint16_t(jointIds.values[v*4+c]);
          vertex.weights[c]=weights.values[v*4+c];
        }
      }
      mesh.indices.reserve(indices.count);
      for(std::size_t i=0;i<indices.count;i+=3) {
        for(unsigned c:{0u,2u,1u}) {
          const auto index=std::uint16_t(indices.values[i+c]);
          Need(index<mesh.vertices.size(),"mesh index exceeds vertex count");mesh.indices.push_back(index);
        }
        const auto a=mesh.indices[i],b=mesh.indices[i+1],c=mesh.indices[i+2];
        Need(a!=b && b!=c && a!=c,"repeated triangle vertex");
        using namespace DirectX;
        const auto origin=XMLoadFloat3(&mesh.vertices[a].position);
        const auto cross=XMVector3Cross(XMVectorSubtract(XMLoadFloat3(&mesh.vertices[b].position),origin),
          XMVectorSubtract(XMLoadFloat3(&mesh.vertices[c].position),origin));
        Need(XMVectorGetX(XMVector3LengthSq(cross))>1e-16f,"degenerate bind triangle");
      }
      // Validate weights, normals and skin-space deformation before publication.
      (void)DeformMesh(mesh,result.skeleton.RestPose());
    }
    if(const auto* animations=Optional(doc,"animations")) {
      const auto& list=Array(*animations);Need(!list.empty() && list.size()<=16,"animation budget is 1..16");
      std::size_t totalKeys=0;
      for(const auto& source:list) {
        Fields(source,{"name","channels","samplers"});NamedClip named;
        const auto* name=Optional(source,"name");named.name=name?Text(*name):"clip_"+std::to_string(result.clips.size());
        Need(!named.name.empty() && named.name.size()<=64,"invalid clip name");
        for(const auto& previous:result.clips) Need(previous.name!=named.name,"duplicate clip name");
        const auto& samplers=Array(Field(source,"samplers"));const auto& channels=Array(Field(source,"channels"));
        Need(!samplers.empty() && samplers.size()<=192 && !channels.empty() && channels.size()<=192,"channel/sampler budget exceeded");
        struct Channel {const Accessor* times=nullptr;const Accessor* values=nullptr;};
        std::vector<std::array<Channel,3>> tracks(ids.size());std::vector<bool> used(samplers.size());
        for(const auto& channel:channels) {
          Fields(channel,{"sampler","target"});const auto s=Integer(Field(channel,"sampler"));Need(s<samplers.size(),"invalid sampler index");used[s]=true;
          const auto& sampler=samplers[s];Fields(sampler,{"input","output","interpolation"});
          if(const auto* interpolation=Optional(sampler,"interpolation")) Need(Text(*interpolation)=="LINEAR","only LINEAR interpolation supported");
          const auto& target=Field(channel,"target");Fields(target,{"node","path"});
          const auto node=Integer(Field(target,"node"));Need(node<nodes.size() && remap[node]>=0,"invalid animation target joint");
          const auto path=Text(Field(target,"path"));const int component=path=="translation"?0:path=="rotation"?1:path=="scale"?2:-1;
          Need(component>=0,"unsupported animation target path");auto& track=tracks[remap[node]][component];Need(!track.times,"duplicate joint/path channel");
          track.times=&get(Field(sampler,"input"),"SCALAR");track.values=&get(Field(sampler,"output"),component==1?"VEC4":"VEC3");
          Need(track.times->bounds && track.times->count==track.values->count,"time bounds/output count mismatch");
          float previous=-1;
          for(float t:track.times->values) {Need(t>=0 && t>previous && t<=600,"unordered/out-of-range animation time");previous=t;}
          named.clip.duration=std::max(named.clip.duration,previous);
          // Validate every source value before conversion/interpolation.
          for(std::size_t k=0;k<track.values->count;++k) {
            const auto* v=&track.values->values[k*track.values->components];
            if(component==1) {double norm=0;for(int c=0;c<4;++c) norm+=double(v[c])*v[c];Need(std::abs(norm-1)<.001,"animation quaternion must be normalized");}
            else for(int c=0;c<3;++c) Need(component==0?std::abs(v[c])<=1000:(v[c]>=.01f && v[c]<=100),"animation transform exceeds budget");
          }
        }
        Need(std::all_of(used.begin(),used.end(),[](bool x){return x;}),"unused animation sampler unsupported");
        Need(named.clip.duration>0,"zero-duration animation unsupported");named.clip.tracks.resize(ids.size());
        for(std::size_t joint=0;joint<tracks.size();++joint) {
          std::vector<float> times;
          for(const auto& channel:tracks[joint]) if(channel.times) times.insert(times.end(),channel.times->values.begin(),channel.times->values.end());
          if(times.empty()) continue;
          times.push_back(0);times.push_back(named.clip.duration);std::sort(times.begin(),times.end());times.erase(std::unique(times.begin(),times.end()),times.end());
          totalKeys+=times.size();Need(totalKeys<=65536,"converted clip key budget exceeded");
          for(float time:times) {
            animation::Transform value=joints[joint].local;
            for(unsigned c=0;c<3;++c) {
              const auto& channel=tracks[joint][c];if(!channel.times) continue;
              const auto& ts=channel.times->values;const auto end=std::upper_bound(ts.begin(),ts.end(),time);
              const auto right=end==ts.end()?ts.size()-1:std::size_t(end-ts.begin());
              const auto left=end==ts.begin()?0:right-(end!=ts.end());
              const float f=left==right?0:(time-ts[left])/(ts[right]-ts[left]);
              const auto stride=channel.values->components;const auto* a=&channel.values->values[left*stride];const auto* b=&channel.values->values[right*stride];
              if(c==1) {
                const auto qa=DirectX::XMQuaternionNormalize(DirectX::XMVectorSet(-a[0],-a[1],a[2],a[3]));
                const auto qb=DirectX::XMQuaternionNormalize(DirectX::XMVectorSet(-b[0],-b[1],b[2],b[3]));
                DirectX::XMStoreFloat4(&value.rotation,DirectX::XMQuaternionNormalize(DirectX::XMQuaternionSlerp(qa,qb,f)));
              } else {
                DirectX::XMFLOAT3 v{a[0]+(b[0]-a[0])*f,a[1]+(b[1]-a[1])*f,a[2]+(b[2]-a[2])*f};
                if(c==0) {v.z=-v.z;value.translation=v;} else value.scale=v;
              }
            }
            named.clip.tracks[joint].push_back({time,value});
          }
        }
        result.skeleton.ValidateClip(named.clip);result.clips.push_back(std::move(named));
      }
    }
    return result;
  }
  catch(const std::runtime_error& error) {
    if(std::string_view(error.what()).substr(0,8)=="GLB rig:") throw;
    Fail(error.what());
  }
}
animation::Skeleton ImportGlbRig(std::string_view bytes) {
  auto asset=ImportGlbAsset(bytes);Need(asset.clips.empty() && asset.mesh.vertices.empty(),"mesh/animated asset requires ImportGlbAsset");return std::move(asset.skeleton);
}
std::vector<DeformedVertex> DeformMesh(const SkinMesh& mesh,const animation::Pose& pose) {
  using namespace DirectX;
  Need(!mesh.vertices.empty() && mesh.vertices.size()<=16384,"invalid deformation vertex count");
  Need(!pose.skin.empty() && pose.skin.size()<=64 && pose.normal.size()==pose.skin.size(),"invalid deformation palette");
  for(const auto* palette:{&pose.skin,&pose.normal}) for(const auto& matrix:*palette)
    for(const auto& row:matrix.m) for(float v:row) Need(std::isfinite(v) && std::abs(v)<=1e8f,"invalid deformation matrix");
  for(const auto& matrix:pose.skin) {
    Need(std::abs(matrix._14)<1e-6f && std::abs(matrix._24)<1e-6f && std::abs(matrix._34)<1e-6f && std::abs(matrix._44-1)<1e-6f,
      "skin matrix must be affine");
    const float determinant=XMVectorGetX(XMMatrixDeterminant(XMLoadFloat4x4(&matrix)));
    Need(std::isfinite(determinant) && determinant>1e-12f,"singular or reflected skin matrix");
  }
  std::vector<DeformedVertex> output;output.reserve(mesh.vertices.size());
  for(const auto& vertex:mesh.vertices) {
    for(float v:{vertex.position.x,vertex.position.y,vertex.position.z}) Need(std::isfinite(v) && std::abs(v)<=1000,"invalid mesh position");
    const auto normal=XMLoadFloat3(&vertex.normal);
    const float length=XMVectorGetX(XMVector3LengthSq(normal));
    Need(std::isfinite(length) && std::abs(length-1)<.001f,"mesh normal must be normalized");
    float sum=0;
    for(unsigned c=0;c<4;++c) {
      Need(vertex.joints[c]<pose.skin.size(),"mesh joint exceeds skin palette");
      Need(std::isfinite(vertex.weights[c]) && vertex.weights[c]>=0 && vertex.weights[c]<=1,"invalid skin weight");
      sum+=vertex.weights[c];
    }
    Need(std::abs(sum-1)<.001f,"skin weights must sum to one");
    auto p=XMVectorZero(),n=XMVectorZero();
    for(unsigned c=0;c<4;++c) {
      const auto joint=vertex.joints[c];const float weight=vertex.weights[c]/sum;
      // Affine skinning must not perform a perspective divide per influence.
      p=XMVectorAdd(p,XMVectorScale(XMVector3Transform(XMLoadFloat3(&vertex.position),XMLoadFloat4x4(&pose.skin[joint])),weight));
      n=XMVectorAdd(n,XMVectorScale(XMVector3TransformNormal(normal,XMLoadFloat4x4(&pose.normal[joint])),weight));
    }
    const float norm=XMVectorGetX(XMVector3LengthSq(n));
    Need(std::isfinite(norm) && norm>1e-20f,"collapsed deformed normal");
    DeformedVertex deformed;XMStoreFloat3(&deformed.position,p);XMStoreFloat3(&deformed.normal,XMVector3Normalize(n));
    for(float v:{deformed.position.x,deformed.position.y,deformed.position.z}) Need(std::isfinite(v) && std::abs(v)<=1e8f,"deformed vertex exceeds budget");
    output.push_back(deformed);
  }
  return output;
}
}
