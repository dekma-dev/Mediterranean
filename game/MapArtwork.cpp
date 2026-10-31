#include "game/MapArtwork.h"
#include "engine/Geometry.h"
#include <algorithm>
#include <cmath>
#include <utility>
namespace {
struct RGB { float r,g,b; };
struct Point { float x,y; };
constexpr unsigned size=512;
constexpr RGB paper{.88f,.80f,.63f},ink{.29f,.25f,.17f};
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
Point Map(float x,float z) { return {(x/rpg::WorldMap::surveyWidth+.5f)*size,(.5f-z/rpg::WorldMap::surveyDepth)*size}; }
bool Inside(const std::vector<Point>& points,float x,float y) {
 bool inside=false;
 for (std::size_t i=0,j=points.size()-1;i<points.size();j=i++) {
  const auto a=points[i],b=points[j];
  if ((a.y>y)!=(b.y>y) && x<(b.x-a.x)*(y-a.y)/(b.y-a.y)+a.x) inside=!inside;
 }
 return inside;
}
struct Survey {
 std::vector<std::uint8_t> pixels=std::vector<std::uint8_t>(size*size*4);
 void Put(int x,int y,RGB color,float opacity=1) {
  if (x<0 || y<0 || x>=int(size) || y>=int(size)) return;
  const auto offset=(std::size_t(y)*size+x)*4;
  color=Mix({pixels[offset]/255.f,pixels[offset+1]/255.f,pixels[offset+2]/255.f},color,std::clamp(opacity,0.f,1.f));
  pixels[offset]=static_cast<std::uint8_t>(std::clamp(color.r,0.f,1.f)*255);
  pixels[offset+1]=static_cast<std::uint8_t>(std::clamp(color.g,0.f,1.f)*255);
  pixels[offset+2]=static_cast<std::uint8_t>(std::clamp(color.b,0.f,1.f)*255);
  pixels[offset+3]=255;
 }
 // Soft ink edges and restrained pigment variation survive the minimap's mip levels.
 void Line(Point a,Point b,float width=1,RGB color=ink,float opacity=.85f) {
  const float dx=b.x-a.x,dy=b.y-a.y,length2=dx*dx+dy*dy;
  if (length2<.0001f) return;
  const int left=std::max(0,int(std::floor(std::min(a.x,b.x)-width-1)));
  const int right=std::min(int(size)-1,int(std::ceil(std::max(a.x,b.x)+width+1)));
  const int top=std::max(0,int(std::floor(std::min(a.y,b.y)-width-1)));
  const int bottom=std::min(int(size)-1,int(std::ceil(std::max(a.y,b.y)+width+1)));
  for (int y=top;y<=bottom;++y) for (int x=left;x<=right;++x) {
   const float px=x+.5f,py=y+.5f;
   const float t=std::clamp(((px-a.x)*dx+(py-a.y)*dy)/length2,0.f,1.f);
   const float distance=std::hypot(px-a.x-t*dx,py-a.y-t*dy);
   const float coverage=std::clamp(width*.5f+.65f-distance,0.f,1.f);
   if (coverage>0) Put(x,y,color,coverage*opacity*(.85f+.15f*Hash(x,y)));
  }
 }
 void Path(const std::vector<Point>& points,float width=1,float opacity=.85f,bool close=true) {
  for (std::size_t i=1;i<points.size();++i) Line(points[i-1],points[i],width,ink,opacity);
  if (close && points.size()>2) Line(points.back(),points.front(),width,ink,opacity);
 }
 void Shape(const std::vector<Point>& points,RGB wash,float opacity=.85f) {
  if (points.size()<3) return;
  float left=points[0].x,right=left,top=points[0].y,bottom=top;
  for (const auto p : points) { left=std::min(left,p.x); right=std::max(right,p.x); top=std::min(top,p.y); bottom=std::max(bottom,p.y); }
  for (int y=std::max(0,int(std::floor(top)));y<=std::min(int(size)-1,int(std::ceil(bottom)));++y)
   for (int x=std::max(0,int(std::floor(left)));x<=std::min(int(size)-1,int(std::ceil(right)));++x)
    if (Inside(points,x+.5f,y+.5f)) Put(x,y,wash,opacity*(.90f+.10f*Noise(x*.15f,y*.15f)));
  Path(points,.85f,.78f);
 }
 void Tree(Point p,float scale,float seed) {
  // A cartographer's scalloped crown and visible branchwork, drawn in elevation.
  Line({p.x-9*scale,p.y+12*scale},{p.x+11*scale,p.y+13*scale},.7f,ink,.24f);
  std::vector<Point> crown;
  for (int i=0;i<48;++i) {
   const float a=i*6.2831853f/48;
   const float radius=1+.12f*std::sin(a*7+seed)+.06f*std::sin(a*11+seed);
   crown.push_back({p.x+std::cos(a)*13*scale*radius,p.y-7*scale+std::sin(a)*16*scale*radius});
  }
  Shape(crown,{.55f,.61f,.39f},.42f);
  Line({p.x-1.7f*scale,p.y+13*scale},{p.x-.4f*scale,p.y-2*scale},.95f);
  Line({p.x+1.8f*scale,p.y+13*scale},{p.x+.8f*scale,p.y-3*scale},.7f);
  Line({p.x,p.y+2*scale},{p.x-6*scale,p.y-4*scale},.7f,ink,.65f);
  Line({p.x,p.y-2*scale},{p.x+6*scale,p.y-8*scale},.7f,ink,.65f);
  for (int i=0;i<4;++i) {
   const float x=p.x+(i-2)*4*scale,y=p.y-11*scale+std::sin(seed+i)*3*scale;
   Line({x,y},{x+3*scale,y-3*scale},.6f,ink,.38f);
  }
 }
 void Building(Point p,float w,float h,rpg::PropKind kind,rpg::BuildingRole role) {
  if (kind==rpg::PropKind::Ruin) {
   Shape({{p.x-w,p.y+h*.5f},{p.x-w,p.y-h},{p.x-w*.4f,p.y-h*.8f},{p.x-w*.3f,p.y+h*.1f},
     {p.x+w*.2f,p.y+h*.1f},{p.x+w*.4f,p.y-h*1.2f},{p.x+w,p.y-h},{p.x+w,p.y+h*.5f}}, {.70f,.68f,.55f});
   for (int i=0;i<5;++i) Line({p.x-w,p.y-h+i*h*.3f},{p.x-w*.5f,p.y-h+i*h*.3f},.65f,ink,.55f);
   Line({p.x-w*1.2f,p.y+h*.8f},{p.x+w*1.1f,p.y+h*.6f},.7f,ink,.5f);
   return;
  }
  const RGB roof=role==rpg::BuildingRole::WatchHall?RGB{.54f,.59f,.56f}:
    role==rpg::BuildingRole::Storehouse?RGB{.63f,.64f,.44f}:RGB{.69f,.46f,.31f};
  Shape({{p.x-w*.8f,p.y-h*.1f},{p.x+w*.8f,p.y-h*.1f},{p.x+w*.8f,p.y+h*.8f},{p.x-w*.8f,p.y+h*.8f}},
    {.84f,.74f,.53f});
  Shape({{p.x-w,p.y-h*.05f},{p.x-w*.42f,p.y-h},{p.x+w*.55f,p.y-h*.85f},{p.x+w,p.y+h*.05f}},roof,.65f);
  Line({p.x-w*.42f,p.y-h},{p.x,p.y+h*.02f},1);
  for (int i=1;i<5;++i) {
   const float t=i/5.f;
   Line({p.x-w*.42f+t*w*.97f,p.y-h+t*h*.15f},{p.x+t*w,p.y+h*.02f},.55f,ink,.52f);
  }
  Shape({{p.x-w*.4f,p.y+h*.8f},{p.x-w*.4f,p.y+h*.28f},{p.x-w*.15f,p.y+h*.25f},{p.x-w*.15f,p.y+h*.8f}},
    {.42f,.36f,.24f},.7f);
  Line({p.x+w*.35f,p.y+h*.25f},{p.x+w*.35f,p.y+h*.55f},2.f,ink,.65f);
  Line({p.x-w*.9f,p.y+h},{p.x+w*.95f,p.y+h*.95f},.65f,ink,.45f);
 }
};
}
std::vector<std::uint8_t> BuildMapArtwork(const rpg::WorldMap& world) {
 // Fixed-size ink-and-wash survey. Coast, routes and symbols use real map data.
 std::vector<Point> coast{Map(-74,-82),Map(74,-82),Map(74,82),Map(-74,82)};
 Survey survey;
 for (unsigned row=0;row<size;++row) for (unsigned column=0;column<size;++column) {
  const float x=((column+.5f)/size-.5f)*rpg::WorldMap::surveyWidth,z=(.5f-(row+.5f)/size)*rpg::WorldMap::surveyDepth;
  const float n=Noise(x*.24f,z*.24f),fine=Hash(int(column),int(row));
  // Match the atlas's paper gutters at the texture edges.
  const float edge=std::clamp(float(std::min({column,row,size-1-column,size-1-row}))/16,0.f,1.f);
  RGB color=Mix(paper,{.94f,.87f,.71f},edge*(.16f+n*.28f));
  color=Mix(color,{.51f,.42f,.28f},edge*(.018f+.025f*fine));
  if (Inside(coast,column+.5f,row+.5f)) {
   color=Mix(color,{.62f,.67f,.43f},.10f+n*.16f);
   const float height=rpg::WorldMap::Height({x,z});
   const float contour=std::abs(height*.5f-std::round(height*.5f));
   color=Mix(color,ink,(height>.4f && contour<.035f)?.20f:0.f);
   color=Mix(color,{.45f,.41f,.30f},std::min(.25f,rpg::terrain::Slope(x,z)*.38f));
   const auto surface=world.SurfaceAt({x,z});
   const float road=std::max(surface.dirt,surface.paving);
   color=Mix(color,{.94f,.86f,.66f},road*.78f);
   const float border=std::max(0.f,1-std::abs(road-.48f)/.20f);
   color=Mix(color,ink,border*(.21f+.08f*Noise(x*4,z*4)));
  }
  survey.Put(int(column),int(row),color);
 }
 // Uneven shore contours and short cliff hachures give a drawn coastline.
 for (float expansion : {1.025f,1.055f}) {
  std::vector<Point> contour;
  for (auto p : coast) contour.push_back({256+(p.x-256)*expansion,256+(p.y-256)*expansion});
  survey.Path(contour,.65f,expansion<1.04f?.35f:.18f);
 }
 survey.Path(coast,1.25f,.88f);
 for (std::size_t i=0;i<coast.size();++i) {
  const auto a=coast[i],b=coast[(i+1)%coast.size()];
  const float length=std::hypot(b.x-a.x,b.y-a.y);
  const int count=std::max(1,int(length/7));
  for (int j=0;j<count;++j) {
   const float t=(j+.35f)/count; const Point p{a.x+(b.x-a.x)*t,a.y+(b.y-a.y)*t};
   const float radial=std::hypot(p.x-256,p.y-256),reach=3+Hash(int(i),j)*4;
   survey.Line(p,{p.x+(p.x-256)/radial*reach,p.y+(p.y-256)/radial*reach},.6f,ink,.44f);
  }
 }
 // Sparse grass strokes, kept clear of the same roads and services as world foliage.
 for (const auto& cover : world.Cover()) {
  if (Hash(int(cover.position.x*19),int(cover.position.z*19))>.38f) continue;
  const auto p=Map(cover.position.x,cover.position.z);
  for (int i=-1;i<=1;++i) survey.Line({p.x+i*2,p.y+1},{p.x+i*3,p.y-2.5f+std::abs(i)},.6f,ink,.30f);
 }
 const auto dockA=Map(rpg::WorldMap::dockMin.x,rpg::WorldMap::dockMax.z);
 const auto dockB=Map(rpg::WorldMap::dockMax.x,rpg::WorldMap::dockMin.z);
 survey.Shape({dockA,{dockB.x,dockA.y},dockB,{dockA.x,dockB.y}}, {.69f,.56f,.35f},.42f);
 for (float y=dockA.y+4;y<dockB.y;y+=5) survey.Line({dockA.x+1,y},{dockB.x-1,y+.4f},.6f,ink,.55f);
 for (const auto& o : world.Obstacles()) {
  const auto p=Map(o.center.x,o.center.z);
  if (o.kind==rpg::PropKind::Tree) survey.Tree(p,1.f,Hash(int(p.x),int(p.y))*6);
  else if (o.kind==rpg::PropKind::Counter) {
   survey.Shape({{p.x-14,p.y+5},{p.x-11,p.y-5},{p.x+11,p.y-5},{p.x+14,p.y+5}}, {.60f,.64f,.42f},.65f);
   for (int i=-2;i<=2;++i) survey.Line({p.x+i*4,p.y-4},{p.x+i*5,p.y+4},.7f,ink,.6f);
   survey.Line({p.x-11,p.y+5},{p.x-11,p.y+11},1);
   survey.Line({p.x+11,p.y+5},{p.x+11,p.y+11},1);
  } else if(o.kind==rpg::PropKind::SurveyStone) {
   survey.Line({p.x-2,p.y+2},{p.x,p.y-3},1,ink,.8f);survey.Line({p.x,p.y-3},{p.x+2,p.y+2},1,ink,.8f);
  } else survey.Building(p,o.halfX*size/rpg::WorldMap::surveyWidth,o.halfZ*size/rpg::WorldMap::surveyDepth,o.kind,o.role);
 }
 // Wear is baked once into the existing atlas: no per-frame texture uploads.
 for(unsigned y=0;y<size;++y) for(unsigned x=0;x<size;++x) {
  const float edge=float(std::min({x,y,size-1-x,size-1-y}));
  const float waviness=3+Noise(x*.12f,y*.12f)*5;
  const float scorch=std::exp(-edge/12.f)*(.23f+.26f*Noise(x*.07f,y*.07f));
  survey.Put(int(x),int(y),{.30f,.18f,.075f},scorch);
  if(edge<waviness) survey.Put(int(x),int(y),{.23f,.16f,.08f},.55f);
  const float fold=std::min(std::abs(float(x)-size*.48f-std::sin(y*.025f)*1.8f),std::abs(float(y)-size*.56f));
  if(fold<2.7f) survey.Put(int(x),int(y),fold<.8f?RGB{.33f,.25f,.13f}:RGB{.98f,.88f,.65f},fold<.8f?.16f:.13f);
  const float stain=Noise(x*.018f+14,y*.018f-8);
  if(stain>.58f) survey.Put(int(x),int(y),{.48f,.29f,.11f},(stain-.58f)*.35f);
  if(Hash(int(x),int(y))>.984f) survey.Put(int(x),int(y),{.42f,.32f,.18f},.17f);
 }
 return std::move(survey.pixels);
}
