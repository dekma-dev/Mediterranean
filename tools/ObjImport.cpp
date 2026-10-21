#include "tools/ObjImport.h"
#include <charconv>
#include <cmath>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tuple>

namespace assets {
namespace {
using namespace DirectX;
[[noreturn]] void Fail(std::size_t line,const std::string& message) {
  throw std::runtime_error("OBJ line "+std::to_string(line)+": "+message);
}
float Number(std::string_view token,std::size_t line) {
  if(!token.empty() && token.front()=='+') {
    token.remove_prefix(1);
    if(!token.empty() && token.front()=='-') Fail(line,"expected a finite number");
  }
  float value=0;
  const auto result=std::from_chars(token.data(),token.data()+token.size(),value);
  if(token.empty() || result.ec!=std::errc{} || result.ptr!=token.data()+token.size() || !std::isfinite(value))
    Fail(line,"expected a finite number");
  return value;
}
std::int64_t Integer(std::string_view token,std::size_t line) {
  if(!token.empty() && token.front()=='+') {
    token.remove_prefix(1);
    if(!token.empty() && token.front()=='-') Fail(line,"invalid integer index/group");
  }
  std::int64_t value=0;
  const auto result=std::from_chars(token.data(),token.data()+token.size(),value);
  if(token.empty() || result.ec!=std::errc{} || result.ptr!=token.data()+token.size()) Fail(line,"invalid integer index/group");
  return value;
}
unsigned Index(std::string_view token,std::size_t count,std::size_t line) {
  const auto value=Integer(token,line);
  if(!value || value>std::int64_t(count) || value< -std::int64_t(count)) Fail(line,"index is zero, forward-referencing or outside its declared array");
  return unsigned(value>0?value-1:std::int64_t(count)+value);
}
XMFLOAT3 Normalize(XMFLOAT3 v,std::size_t line) {
  const double length=std::sqrt(double(v.x)*v.x+double(v.y)*v.y+double(v.z)*v.z);
  if(!std::isfinite(length) || length<1e-12) Fail(line,"zero or cancelling normal");
  return {float(v.x/length),float(v.y/length),float(v.z/length)};
}
struct Corner { unsigned position=0; int normal=-1; bool uv=false; };
struct Face { std::array<Corner,3> corners; std::uint64_t smooth=0; std::size_t line=0; XMFLOAT3 normal{}; };
Corner ReadCorner(std::string_view token,std::size_t positions,std::size_t uvs,std::size_t normals,std::size_t line) {
  Corner corner;
  const auto slash=token.find('/');
  corner.position=Index(token.substr(0,slash),positions,line);
  if(slash==token.npos) return corner;
  const auto second=token.find('/',slash+1);
  const auto uv=token.substr(slash+1,second==token.npos?token.npos:second-slash-1);
  if(!uv.empty()) { Index(uv,uvs,line); corner.uv=true; }
  if(second==token.npos) {
    if(uv.empty()) Fail(line,"missing texture index after slash");
  } else {
    corner.normal=int(Index(token.substr(second+1),normals,line));
  }
  return corner;
}
}
visual::Geometry ImportObj(std::string_view source,ObjLimits limits) {
  if(source.size()>limits.bytes) Fail(1,"source exceeds the byte budget");
  if(source.find('\0')!=source.npos) Fail(1,"NUL bytes are not supported");
  if(source.substr(0,3)=="\xef\xbb\xbf") source.remove_prefix(3);
  std::vector<XMFLOAT3> positions,normals;
  std::vector<XMFLOAT4> colors;
  std::vector<Face> faces;
  std::size_t uvCount=0,lineNumber=0;
  std::uint64_t smooth=0;
  std::istringstream input{std::string(source)}; std::string line;
  while(std::getline(input,line)) {
    ++lineNumber;
    if(line.size()>8192) Fail(lineNumber,"line exceeds 8192 bytes");
    line.erase(line.find('#')==line.npos?line.size():line.find('#'));
    std::istringstream words(line); std::string command,token;
    if(!(words>>command)) continue;
    std::vector<std::string> args;
    while(words>>token) args.push_back(token);
    if(command=="v" || command=="vn" || command=="vt") {
      const bool valid=command=="v"?(args.size()==3 || args.size()==6):command=="vn"?args.size()==3:(args.size()==2 || args.size()==3);
      if(!valid) Fail(lineNumber,"expected v xyz [rgb], vn xyz or vt uv [w]");
      float values[6]{};
      for(std::size_t i=0;i<args.size();++i) values[i]=Number(args[i],lineNumber);
      if(command=="v") {
        if(positions.size()>=limits.records) Fail(lineNumber,"position budget exceeded");
        for(unsigned i=0;i<3;++i) if(std::abs(values[i])>limits.coordinate) Fail(lineNumber,"position exceeds the coordinate budget");
        XMFLOAT4 color{1,1,1,1};
        if(args.size()==6) {
          for(unsigned i=3;i<6;++i) if(values[i]<0 || values[i]>1) Fail(lineNumber,"vertex colors must be in [0, 1]");
          color={values[3],values[4],values[5],1};
        }
        positions.push_back({values[0],values[1],values[2]}); colors.push_back(color);
      } else if(command=="vn") {
        if(normals.size()>=limits.records) Fail(lineNumber,"normal budget exceeded");
        normals.push_back(Normalize({values[0],values[1],values[2]},lineNumber));
      } else if(++uvCount>limits.records) Fail(lineNumber,"texture-coordinate budget exceeded");
    } else if(command=="f") {
      if(args.size()!=3) Fail(lineNumber,"only triangles are supported; enable Triangulated Mesh when exporting");
      if(faces.size()>=limits.triangles) Fail(lineNumber,"triangle budget exceeded");
      Face face; face.line=lineNumber; face.smooth=smooth;
      for(unsigned k=0;k<3;++k) face.corners[k]=ReadCorner(args[k],positions.size(),uvCount,normals.size(),lineNumber);
      for(unsigned k=1;k<3;++k)
        if((face.corners[k].normal>=0)!=(face.corners[0].normal>=0) || face.corners[k].uv!=face.corners[0].uv)
          Fail(lineNumber,"mixed normal/UV presence within one triangle");
      const auto a=XMLoadFloat3(&positions[face.corners[0].position]);
      const auto b=XMLoadFloat3(&positions[face.corners[1].position]);
      const auto c=XMLoadFloat3(&positions[face.corners[2].position]);
      const auto cross=XMVector3Cross(b-a,c-a);
      if(XMVectorGetX(XMVector3LengthSq(cross))<1e-16f) Fail(lineNumber,"degenerate triangle");
      XMStoreFloat3(&face.normal,cross); faces.push_back(face);
    } else if(command=="s") {
      if(args.size()!=1) Fail(lineNumber,"expected one smoothing group");
      if(args[0]=="off" || args[0]=="0") smooth=0;
      else if(args[0]=="on") smooth=1;
      else { const auto group=Integer(args[0],lineNumber); if(group<1) Fail(lineNumber,"smoothing group must be positive"); smooth=std::uint64_t(group); }
    } else if(command=="o" || command=="g") {
      // Names are metadata only; this slot embeds one combined mesh.
    } else if(command=="mtllib" || command=="usemtl") {
      Fail(lineNumber,"material files are unsupported; disable Materials in the exporter and use the engine material");
    } else Fail(lineNumber,"unsupported record '"+command+"'");
  }
  if(faces.empty()) Fail(lineNumber?lineNumber:1,"mesh has no triangles");
  using SmoothKey=std::pair<unsigned,std::uint64_t>;
  std::map<SmoothKey,XMFLOAT3> averaged;
  for(const auto& face:faces) if(face.smooth) for(const auto& corner:face.corners) {
    auto& sum=averaged[{corner.position,face.smooth}];
    XMStoreFloat3(&sum,XMLoadFloat3(&sum)+XMLoadFloat3(&face.normal));
  }
  // Keep explicit normal seams and flat faces separate while sharing smooth
  // corners. UV seams need no duplicate vertices in the procedural material path.
  using Key=std::tuple<unsigned,int,std::uint64_t,std::size_t>;
  std::map<Key,unsigned> vertices; visual::Geometry geometry;
  for(std::size_t f=0;f<faces.size();++f) {
    const auto& face=faces[f];
    for(const auto& corner:face.corners) {
      const Key key{corner.position,corner.normal,corner.normal>=0?0:face.smooth,corner.normal>=0 || face.smooth?0:f+1};
      auto found=vertices.find(key);
      if(found==vertices.end()) {
        if(geometry.vertices.size()>=limits.vertices) Fail(face.line,"render vertex budget exceeded");
        const auto normal=corner.normal>=0?normals[corner.normal]:Normalize(face.smooth?averaged.at({corner.position,face.smooth}):face.normal,face.line);
        const unsigned index=unsigned(geometry.vertices.size());
        geometry.vertices.push_back({positions[corner.position],normal,colors[corner.position]});
        found=vertices.emplace(key,index).first;
      }
      geometry.indices.push_back(found->second);
    }
    const auto start=geometry.indices.size()-3;
    auto normal=XMVectorZero();
    for(unsigned k=0;k<3;++k) normal+=XMLoadFloat3(&geometry.vertices[geometry.indices[start+k]].normal);
    if(XMVectorGetX(XMVector3Dot(normal,XMLoadFloat3(&face.normal)))<=0)
      Fail(face.line,"normals oppose triangle winding; recalculate outward normals before export");
  }
  return geometry;
}
}
