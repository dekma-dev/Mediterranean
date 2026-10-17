#include "game/MapArtwork.h"
#include "engine/Geometry.h"
#include <algorithm>
#include <cmath>
namespace {
struct RGB { float r,g,b; };
float Hash(int x,int y) {
 std::uint32_t value=std::uint32_t(x)*374761393u+std::uint32_t(y)*668265263u;
 value=(value^(value>>13))*1274126177u; return float(value^(value>>16))/4294967295.f;
}
float Noise(float x,float y) {
 const int ix=int(std::floor(x)),iy=int(std::floor(y));
 float u=x-ix,v=y-iy; u=u*u*(3-2*u); v=v*v*(3-2*v);
 return (Hash(ix,iy)*(1-u)+Hash(ix+1,iy)*u)*(1-v)+(Hash(ix,iy+1)*(1-u)+Hash(ix+1,iy+1)*u)*v;
}
RGB Mix(RGB a,RGB b,float t) { return {a.r+(b.r-a.r)*t,a.g+(b.g-a.g)*t,a.b+(b.b-a.b)*t}; }
float Fraction(float value) { return value-std::floor(value); }
}
std::vector<std::uint8_t> BuildMapArtwork(const rpg::WorldMap& world) {
 // A small illustrated survey generated from the playable coastline and props.
 // Fine grain, roof tiles and canopy shading are authored here, never downloaded.
 const auto terrain=visual::BuildMesh(visual::Mesh::Terrain);
 std::vector<std::uint8_t> pixels(512*512*4);
 for (unsigned row=0;row<512;++row) for (unsigned column=0;column<512;++column) {
  const float x=(column+.5f)/512*42-21,z=23-(row+.5f)/512*46;
  bool land=false;
  for (std::size_t i=1,j=terrain.vertices.size()-1;i<terrain.vertices.size();j=i++) {
   const auto a=terrain.vertices[i].position,b=terrain.vertices[j].position;
   if ((a.z>z)!=(b.z>z) && x<(b.x-a.x)*(z-a.z)/(b.z-a.z)+a.x) land=!land;
  }
  const float n=Noise(x*.55f,z*.55f),fine=Noise(x*15,z*15);
  RGB color{.15f+n*.018f,.25f+n*.026f,.285f+n*.025f};
  if (land) {
   color=Mix({.36f,.43f,.26f},{.60f,.59f,.37f},n);
   const float road=std::min(std::abs(x)-1.35f,std::min(std::abs(z+11)-1.2f,std::abs(z)-.9f));
   const float blend=std::clamp(.5f-road*5+(n-.5f)*.5f,0.f,1.f);
   color=Mix(color,{.68f+fine*.08f,.59f+fine*.065f,.39f+fine*.04f},blend);
   for (const auto& o : world.Obstacles()) {
    const float dx=x-o.center.x,dz=z-o.center.z;
    if (o.kind==rpg::PropKind::Tree) {
     if (std::hypot(dx-.7f,dz-.6f)<1.7f) color=Mix(color,{.19f,.25f,.18f},.30f);
     for (int branch=0;branch<3;++branch) {
      const float angle=branch*2.1f+o.center.z,cx=dx-std::sin(angle)*.65f,cz=dz-std::cos(angle)*.65f;
      const float distance=std::hypot(cx,cz);
      if (distance<1.02f+(Noise(x*3,z*3)-.5f)*.2f) {
       const float light=std::clamp(.55f-.22f*cx+.15f*cz+(1-distance)*.2f,0.f,1.f);
       color=Mix({.18f,.28f,.17f},{.40f,.49f,.24f},light); color=Mix(color,{.58f,.58f,.32f},fine*.13f);
      }
     }
    } else {
     if (std::abs(dx-1.f)<o.halfX+.35f && std::abs(dz-.8f)<o.halfZ+.35f) color=Mix(color,{.18f,.23f,.17f},.45f);
     if (std::abs(dx)<o.halfX+.3f && std::abs(dz)<o.halfZ+.3f) {
      if (o.kind==rpg::PropKind::Counter) color=Mix({.34f,.23f,.12f},{.60f,.44f,.22f},fine);
      else if (o.kind==rpg::PropKind::Ruin) color=Mix({.36f,.37f,.32f},{.56f,.54f,.43f},fine);
      else {
       const float tx=Fraction((dx+o.halfX)*4+std::floor(dz*5)*.5f),tz=Fraction(dz*5);
       const float tile=tx<.07f || tz<.09f?.62f:1.f;
       const float slope=dx<0?1.1f:.77f;
       color={.48f*slope*tile,.29f*slope*tile,.17f*slope*tile};
       if (std::abs(dx)<.07f) color={.67f,.49f,.28f};
      }
     }
    }
   }
   if (x>-15.25f && x<-9.75f && z>-3.2f && z<2.6f) {
    const float plank=Fraction(z/.45f)<.08f?.55f:1.f;
    color={.46f*plank,.34f*plank,.21f*plank};
   }
   color=Mix(color,{fine,fine*.94f,fine*.75f},.045f);
  }
  const auto offset=(static_cast<std::size_t>(row)*512+column)*4;
  pixels[offset]=static_cast<std::uint8_t>(std::clamp(color.r,0.f,1.f)*255);
  pixels[offset+1]=static_cast<std::uint8_t>(std::clamp(color.g,0.f,1.f)*255);
  pixels[offset+2]=static_cast<std::uint8_t>(std::clamp(color.b,0.f,1.f)*255); pixels[offset+3]=255;
 }
 return pixels;
}
