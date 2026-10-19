#include "game/Interface.h"
#include "game/CharacterView.h"
#include "game/MapArtwork.h"
#include "engine/Renderer.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <sstream>
#include <iomanip>
#include <utility>

namespace ui {
namespace {
using Color=DirectX::XMFLOAT4;
using Point=DirectX::XMFLOAT2;
template<unsigned Segments> const std::array<Point,Segments+1>& CirclePoints() {
 static const auto points=[] {
  std::array<Point,Segments+1> result{};
  for (unsigned i=0;i<=Segments;++i) {
   const float angle=i*6.2831853f/Segments;
   result[i]={std::cos(angle),std::sin(angle)};
  }
  return result;
 }();
 return points;
}
const Color ink{.97f,.93f,.80f,1},muted{.73f,.77f,.67f,1},gold{.86f,.67f,.30f,1};
const Color dark{.038f,.075f,.065f,.99f},border{.39f,.38f,.24f,1},blue{.23f,.46f,.63f,1};
std::string Seconds(float value) {
 std::ostringstream text;
 if (value<1) text<<std::fixed<<std::setprecision(1)<<std::ceil(std::max(0.f,value)*10)/10;
 else text<<int(std::ceil(value));
 return text.str();
}
struct Canvas {
 Renderer& r; Layout layout;
 explicit Canvas(Renderer& renderer):r(renderer),layout(float(r.Width()),float(r.Height())) {}
 Point P(float x,float y) const { return {(layout.x+x*layout.scale)/r.Width(),(layout.y+y*layout.scale)/r.Height()}; }
 void RectFill(Rect a,Color c) { const auto p=P(a.x,a.y); r.DrawHudRect(p.x,p.y,a.w*layout.scale/r.Width(),a.h*layout.scale/r.Height(),c); }
 void Gradient(Rect a,Color top,Color bottom) {
  for (int i=0;i<16;++i) {
   const float t=float(i)/15;
   RectFill({a.x,a.y+a.h*i/16,a.w,a.h/16+.1f},{top.x+(bottom.x-top.x)*t,top.y+(bottom.y-top.y)*t,top.z+(bottom.z-top.z)*t,top.w+(bottom.w-top.w)*t});
  }
 }
 void Frame(Rect a,Color fill=dark,Color line=border) {
  RectFill({a.x+3,a.y+4,a.w,a.h},{0,0,0,.40f}); RectFill(a,{.018f,.024f,.013f,1});
  RectFill({a.x+1,a.y+1,a.w-2,a.h-2},line);
  RectFill({a.x+3,a.y+3,a.w-6,a.h-6},fill);
  RectFill({a.x+2,a.y+2,a.w-4,1},{.72f,.62f,.32f,.6f});
  RectFill({a.x+3,a.y+a.h-3,a.w-6,1},{.015f,.025f,.015f,.95f});
 }
 void Text(float x,float y,const std::string& s,float size=18,Color c=ink) {
  const auto shadow=P(x+1,y+1); r.DrawText(shadow.x,shadow.y,s,{.01f,.015f,.008f,c.w*.85f},size*layout.scale/12);
  const auto p=P(x,y); r.DrawText(p.x,p.y,s,c,size*layout.scale/12);
 }
 float Width(const std::string& s,float size=18) const { return r.TextWidth(s,size*layout.scale/12)/layout.scale; }
 void Center(float x,float y,float width,const std::string& s,float size=18,Color c=ink) { Text(x+(width-Width(s,size))*.5f,y,s,size,c); }
 void Wrap(float x,float y,float width,const std::string& text,float size=16,Color c=muted,int maxLines=3) {
  std::istringstream paragraphs(text); std::string paragraph; int row=0;
  while (row<maxLines && std::getline(paragraphs,paragraph)) {
   std::istringstream words(paragraph); std::string word,line;
   while (words>>word) {
    const auto candidate=line.empty()?word:line+" "+word;
    if (Width(candidate,size)>width && !line.empty()) {
     Text(x,y+row*(size+5),line,size,c); line=word;
     if (++row>=maxLines) return;
    } else line=candidate;
   }
   if (!line.empty()) Text(x,y+row*(size+5),line,size,c);
   ++row;
  }
 }
 void Triangle(Point a,Point b,Point c,Color tint) { r.DrawHudTriangle(P(a.x,a.y),P(b.x,b.y),P(c.x,c.y),tint); }
 void Line(float x,float y,float tx,float ty,float width,Color tint) {
  const float length=std::hypot(tx-x,ty-y); if (length<.01f) return;
  const float dx=-(ty-y)*width*.5f/length,dy=(tx-x)*width*.5f/length;
  Triangle({x+dx,y+dy},{tx+dx,ty+dy},{tx-dx,ty-dy},tint);
  Triangle({x+dx,y+dy},{tx-dx,ty-dy},{x-dx,y-dy},tint);
 }
 void Oval(float x,float y,float rx,float ry,Color color) {
  const auto& points=CirclePoints<24>();
  for (int i=0;i<24;++i) {
   const auto a=points[i],b=points[i+1];
   Triangle({x,y},{x+a.x*rx,y+a.y*ry},{x+b.x*rx,y+b.y*ry},color);
  }
 }
 void Ring(float x,float y,float radius,Color tint,float width=1) {
  const auto& points=CirclePoints<32>();
  for (int i=0;i<32;++i) {
   const auto a=points[i],b=points[i+1];
   Line(x+a.x*radius,y+a.y*radius,x+b.x*radius,y+b.y*radius,width,tint);
  }
 }
 void Diamond(float x,float y,float rx,float ry,Color tint) {
  Triangle({x,y-ry},{x+rx,y},{x,y+ry},tint); Triangle({x,y-ry},{x,y+ry},{x-rx,y},tint);
 }
 void Scroll(float x,float y,float side=1,float scale=1) {
  static const auto curve=[] {
   std::array<DirectX::XMFLOAT3,22> result{};
   for (int i=1;i<=22;++i) {
    const float t=i/22.f,angle=t*7.2f;
    result[i-1]={std::sin(angle),1-std::cos(angle),(1-t)*14};
   }
   return result;
  }();
  Point previous{x,y};
  for (const auto& sample : curve) {
   const float rad=sample.z*scale;
   Point p{x+side*(sample.x*rad),y+sample.y*rad};
   Line(previous.x,previous.y,p.x,p.y,2*scale,{.22f,.30f,.14f,1});
   Line(previous.x,previous.y-1,p.x,p.y-1,.6f*scale,{.51f,.49f,.25f,.8f}); previous=p;
  }
 }
 void Window(Rect a) {
  Frame(a,dark,{.37f,.39f,.17f,1});
  Gradient({a.x+5,a.y+5,a.w-10,a.h-10},{.13f,.23f,.19f,1},{.035f,.065f,.059f,1});
  // Fine horizontal tool marks keep large panels from reading as flat plastic.
  for(float y=a.y+15;y<a.y+a.h-15;y+=7) {
   const float shade=.025f+.015f*std::sin(y*1.7f);
   RectFill({a.x+12,y,a.w-24,1},{.55f,.62f,.47f,shade});
  }
  // Embossed rails and leaf inlays give the frame depth without an art atlas.
  for (float side : {0.f,1.f}) {
   const float x=a.x+side*(a.w-9);
   RectFill({x+2,a.y+7,6,a.h-14},{.075f,.12f,.055f,1});
   Line(x+3,a.y+8,x+3,a.y+a.h-8,1,{.47f,.47f,.23f,1});
   Line(x+7,a.y+8,x+7,a.y+a.h-8,1,{.025f,.04f,.018f,1});
  }
  Gradient({a.x+8,a.y+3,a.w-16,6},{.36f,.40f,.20f,1},{.08f,.13f,.045f,1});
  Gradient({a.x+8,a.y+a.h-9,a.w-16,6},{.08f,.13f,.045f,1},{.32f,.36f,.17f,1});
  for (float y=a.y+35;y<a.y+a.h-25;y+=72) {
   for (float side : {0.f,1.f}) {
    const float x=a.x+12+side*(a.w-24),dir=side==0?1.f:-1.f;
    Scroll(x,y,dir,.55f);
    Triangle({x,y+14},{x+dir*5,y+9},{x+dir*4,y+18},{.28f,.36f,.16f,1});
    Line(x,y+14,x+dir*5,y+9,.8f,{.51f,.51f,.27f,1});
   }
  }
  RectFill({a.x+18,a.y+21,a.w-36,a.h-42},{.015f,.022f,.012f,.24f});
  for (float side : {0.f,1.f}) for (float bottom : {0.f,1.f}) {
   const float x=a.x+side*a.w,y=a.y+bottom*a.h;
   const float dx=side?-1.f:1.f,dy=bottom?-1.f:1.f;
   // Cast-metal corner brackets: overlapping leaves, incised veins and studs.
   for(int leaf=0;leaf<3;++leaf) {
    const float length=25.f-leaf*5,offset=leaf*5.f;
    Triangle({x+dx*3,y+dy*3},{x+dx*(length+8),y+dy*(offset+4)},
      {x+dx*(offset+5),y+dy*(length+7)},{.24f,.29f,.19f,1});
    Line(x+dx*5,y+dy*5,x+dx*(length+5),y+dy*(offset+4),1,{.66f,.58f,.32f,1});
    Line(x+dx*7,y+dy*7,x+dx*(offset+5),y+dy*(length+3),1,{.08f,.12f,.08f,1});
   }
   Oval(x,y,6,6,{.11f,.14f,.10f,1}); Oval(x,y,4,4,gold); Oval(x-1,y-1,1.5f,1.5f,{1,.88f,.52f,1});
  }
 }
 void Banner(Rect a,const std::string& label,float size=18) {
  const float cy=a.y+a.h*.5f;
  Diamond(a.x+11,cy,18,a.h*.5f,{.16f,.19f,.065f,1}); Diamond(a.x+a.w-11,cy,18,a.h*.5f,{.16f,.19f,.065f,1});
  Frame({a.x+10,a.y,a.w-20,a.h},{.22f,.16f,.055f,1},gold);
  Gradient({a.x+13,a.y+3,a.w-26,a.h-6},{.51f,.34f,.13f,1},{.15f,.115f,.055f,1});
  for(float x=a.x+19;x<a.x+a.w-19;x+=6) {
   Line(x,a.y+5,x+2,a.y+7,.6f,{.90f,.72f,.39f,.45f});
   Line(x,a.y+a.h-6,x+2,a.y+a.h-4,.6f,{.08f,.06f,.025f,.6f});
  }
  Line(a.x+15,a.y+4,a.x+a.w-15,a.y+4,1,{1,.81f,.40f,.75f});
  for (float x : {a.x+8,a.x+a.w-8}) { Diamond(x,cy,7,11,gold); Diamond(x,cy,4,7,{.23f,.29f,.08f,1}); }
  Center(a.x,a.y+(a.h-size)*.5f-2,a.w,label,size,{1,.9f,.51f,1});
 }
 void EmptySlot(Rect a) {
  Frame(a,{.085f,.09f,.04f,1},{.26f,.26f,.12f,1});
  Gradient({a.x+4,a.y+4,a.w-8,a.h-8},{.105f,.105f,.075f,1},{.047f,.055f,.043f,1});
  for(float side : {-1.f,1.f}) {
   const float x=a.x+a.w*.5f+side*a.w*.20f;
   Scroll(x,a.y+a.h*.27f,-side,a.w/90);
   Scroll(x,a.y+a.h*.62f,side,a.w/90);
  }
 }
};
void Icon(Canvas& c,Rect rect,Item item,float alpha=1) {
 const float x=rect.x+rect.w*.5f,y=rect.y+rect.h*.5f,s=rect.w/60;
 const Color metal{.72f,.77f,.77f,alpha},leather{.48f,.30f,.16f,alpha},cloth{.25f,.44f,.60f,alpha};
 if (item==Item::Starter || item==Item::Watchguard) {
  c.Line(x-12*s,y+17*s,x+15*s,y-18*s,10*s,{.07f,.12f,.15f,alpha});
  c.Line(x-12*s,y+17*s,x+13*s,y-15*s,7*s,item==Item::Watchguard?Color{.84f,.75f,.48f,alpha}:metal);
  c.Line(x-10*s,y+15*s,x+15*s,y-18*s,2*s,{.94f,.98f,1,alpha});
  c.Line(x-14*s,y+15*s,x+10*s,y-15*s,2*s,{.30f,.40f,.46f,alpha});
  c.Triangle({x+10*s,y-17*s},{x+20*s,y-24*s},{x+17*s,y-12*s},metal);
  c.Line(x-16*s,y+5*s,x-1*s,y+17*s,4*s,gold); c.Line(x-12*s,y+17*s,x-18*s,y+24*s,5*s,leather);
  c.Oval(x-18*s,y+24*s,4*s,3*s,gold);
  for(int i=0;i<3;++i) c.Line(x-(13+i*2)*s,y+(18+i*2)*s,x-(16+i*2)*s,y+(16+i*2)*s,.9f*s,metal);
 } else if (item==Item::Potion) {
  c.Oval(x,y+7*s,14*s,17*s,{.33f,.57f,.61f,alpha});
  c.Oval(x,y+11*s,11*s,12*s,{.67f,.13f,.16f,alpha});
  c.Oval(x-3*s,y+7*s,7*s,9*s,{.95f,.22f,.19f,alpha});
  c.Oval(x+3*s,y+18*s,7*s,3*s,{.34f,.035f,.09f,alpha});
  c.RectFill({x-6*s,y-17*s,12*s,16*s},{.54f,.72f,.72f,alpha});
  c.RectFill({x-8*s,y-21*s,16*s,6*s},leather); c.Line(x-6*s,y+2*s,x-7*s,y+13*s,2*s,{.92f,.86f,.72f,alpha});
 } else if (item==Item::Tunic || item==Item::Ward) {
  const Color tint=item==Item::Ward?metal:cloth;
  c.Triangle({x-14*s,y-18*s},{x+14*s,y-18*s},{x+17*s,y+19*s},tint);
  c.Triangle({x-14*s,y-18*s},{x+17*s,y+19*s},{x-17*s,y+19*s},tint);
  c.Line(x-15*s,y-14*s,x-22*s,y+1*s,11*s,tint); c.Line(x+15*s,y-14*s,x+22*s,y+1*s,11*s,tint);
  c.RectFill({x-17*s,y+13*s,34*s,4*s},leather); c.Oval(x,y-20*s,7*s,5*s,dark);
  for(float side : {-1.f,1.f}) {
   c.Line(x+side*11*s,y-8*s,x+side*13*s,y+11*s,3*s,{.11f,.22f,.29f,alpha});
   c.Line(x+side*7*s,y-12*s,x+side*8*s,y+9*s,1.3f*s,{.64f,.76f,.80f,alpha});
  }
  c.Line(x,y-14*s,x,y+12*s,2*s,gold); c.RectFill({x-3*s,y+12*s,6*s,5*s},gold);
  if (item==Item::Ward) { c.Oval(x-18*s,y-12*s,10*s,7*s,gold); c.Oval(x+18*s,y-12*s,10*s,7*s,gold); }
 } else if (item==Item::Helm) {
  c.Oval(x,y-5*s,20*s,20*s,metal);
  c.Oval(x-4*s,y-10*s,13*s,11*s,{.86f,.91f,.88f,alpha});
  c.RectFill({x-14*s,y-5*s,28*s,26*s},{.055f,.08f,.07f,alpha});
  c.Line(x-19*s,y-7*s,x+19*s,y-7*s,5*s,gold);
  c.Line(x-18*s,y-3*s,x-15*s,y+20*s,7*s,metal); c.Line(x+18*s,y-3*s,x+15*s,y+20*s,7*s,metal);
  c.Line(x,y-24*s,x,y-10*s,5*s,gold);
  c.Line(x,y-5*s,x,y+12*s,4*s,metal);
 } else if (item==Item::Shield) {
  c.Diamond(x,y,23*s,28*s,gold); c.Diamond(x,y-1*s,18*s,23*s,cloth);
  c.Line(x,y-20*s,x,y+19*s,3*s,metal); c.Line(x-14*s,y-5*s,x+14*s,y-5*s,3*s,metal);
  c.Oval(x,y-4*s,6*s,6*s,gold);
 } else if (item==Item::Shoulders) {
  for (float side : {-1.f,1.f}) {
   c.Oval(x+side*13*s,y-5*s,13*s,13*s,leather);
   for (int layer=0;layer<3;++layer)
    c.Line(x+side*4*s,y+layer*6*s,x+side*25*s,y+(layer*6-4)*s,4*s,metal);
   c.Oval(x+side*14*s,y-12*s,2*s,2*s,gold);
  }
 } else if (item==Item::Gloves) {
  for (float side : {-1.f,1.f}) {
   const float cx=x+side*13*s;
   c.RectFill({cx-7*s,y-20*s,14*s,10*s},gold);
   c.Oval(cx,y-2*s,9*s,13*s,leather);
   c.Line(cx+side*6*s,y-7*s,cx+side*11*s,y+4*s,5*s,leather);
   for (int finger=0;finger<3;++finger)
    c.Line(cx+(finger-1)*5*s,y+2*s,cx+(finger-1)*5*s,y+(18-finger*2)*s,4*s,leather);
  }
 } else if (item==Item::Belt) {
  c.RectFill({x-25*s,y-7*s,50*s,13*s},leather);
  c.RectFill({x-6*s,y-9*s,12*s,17*s},gold);
  c.RectFill({x-3*s,y-5*s,6*s,9*s},leather);
  for (float side : {-1.f,1.f}) {
   c.Oval(x+side*17*s,y+10*s,8*s,11*s,leather);
   c.Line(x+side*17*s-6*s,y+4*s,x+side*17*s+6*s,y+4*s,3*s,gold);
  }
 } else if (item==Item::Cloak) {
  c.Triangle({x,y-24*s},{x+24*s,y+22*s},{x-24*s,y+22*s},cloth);
  for (float side : {-1.f,1.f})
   c.Line(x+side*3*s,y-17*s,x+side*21*s,y+21*s,2*s,gold);
  c.Line(x-21*s,y+21*s,x+21*s,y+21*s,2*s,gold);
  c.Oval(x,y-15*s,4*s,3*s,gold);
 } else if (item==Item::Trousers) {
  c.RectFill({x-15*s,y-20*s,30*s,15*s},leather);
  c.Line(x-8*s,y-6*s,x-10*s,y+21*s,12*s,{.39f,.43f,.40f,alpha});
  c.Line(x+8*s,y-6*s,x+10*s,y+21*s,12*s,{.39f,.43f,.40f,alpha});
 } else {
  for (float side : {-1.f,1.f}) {
   c.RectFill({x+side*10*s-5*s,y-19*s,11*s,31*s},leather);
   c.Oval(x+side*10*s+3*s,y+17*s,11*s,6*s,leather);
   c.RectFill({x+side*10*s-6*s,y-10*s,13*s,3*s},gold);
  }
 }
}
void TalentIcon(Canvas& c,Rect rect,int index,float alpha=1) {
 if (index%3!=2) {
  Icon(c,rect,index%3==0?(index<3?Item::Starter:Item::Watchguard):Item::Shield,alpha);
  return;
 }
 const float x=rect.x+rect.w*.5f,y=rect.y+rect.h*.5f,s=std::min(rect.w,rect.h)/48;
 c.Oval(x,y,20*s,20*s,{.10f,.25f,.20f,alpha});
 c.Ring(x,y,18*s,{.40f,.71f,.58f,alpha},1.5f*s);
 c.RectFill({x-3*s,y-12*s,6*s,24*s},{.73f,.93f,.59f,alpha});
 c.RectFill({x-12*s,y-3*s,24*s,6*s},{.73f,.93f,.59f,alpha});
 if (index>=3) {
  c.Ring(x,y,23*s,{.88f,.76f,.45f,alpha},s);
  c.Triangle({x+19*s,y-16*s},{x+24*s,y-10*s},{x+16*s,y-10*s},{.88f,.76f,.45f,alpha});
 }
}
Point MapPoint(Rect a,rpg::Vec2 p) { const float span=46*a.w/a.h; return {a.x+(p.x+span*.5f)/span*a.w,a.y+(23-p.z)/46*a.h}; }
void Atlas(Canvas& c,const rpg::Simulation& sim,Rect a,bool compact,bool waypoint,rpg::Vec2 pin) {
 if (compact) {
  const auto p=c.P(a.x,a.y);
  c.r.DrawRoundMapArtwork(p.x,p.y,a.w*c.layout.scale/c.r.Width(),a.h*c.layout.scale/c.r.Height());
 } else {
 c.RectFill(a,{.88f,.80f,.63f,1});
 const float width=a.h*42/46;
 const auto top=c.P(a.x+(a.w-width)*.5f,a.y);
 c.r.DrawMapArtwork(top.x,top.y,width*c.layout.scale/c.r.Width(),a.h*c.layout.scale/c.r.Height());
 }
 const Color mapInk{.29f,.25f,.17f,1};
 const auto lettering=[&](float x,float y,const std::string& text,float size) {
  const auto p=c.P(x,y);
  c.r.DrawText(p.x,p.y,text,mapInk,size*c.layout.scale/12);
 };
 if (!compact) {
  // A paper sheet with ink rules and marginal annotations; world projection is unchanged.
  for (float inset : {4.f,7.f}) {
   const Color rule{.40f,.32f,.20f,inset==4?.65f:.30f};
   c.Line(a.x+inset,a.y+inset,a.x+a.w-inset,a.y+inset,.7f,rule);
   c.Line(a.x+inset,a.y+a.h-inset,a.x+a.w-inset,a.y+a.h-inset,.7f,rule);
   c.Line(a.x+inset,a.y+inset,a.x+inset,a.y+a.h-inset,.7f,rule);
   c.Line(a.x+a.w-inset,a.y+inset,a.x+a.w-inset,a.y+a.h-inset,.7f,rule);
  }
  lettering(a.x+15,a.y+a.h*.54f,"Astral dock",14);
  const auto dock=MapPoint(a,{(rpg::WorldMap::dockMin.x+rpg::WorldMap::dockMax.x)*.5f,
    (rpg::WorldMap::dockMin.z+rpg::WorldMap::dockMax.z)*.5f});
  c.Line(a.x+92,a.y+a.h*.54f+12,dock.x-9,dock.y+8,.65f,{.40f,.32f,.20f,.65f});
  lettering(a.x+a.w-94,a.y+a.h*.48f,"Training\ngrounds",14);
  const auto post=MapPoint(a,rpg::WorldMap::trainingPost);
  c.Line(post.x+12,post.y,a.x+a.w-103,a.y+a.h*.48f+17,.65f,{.40f,.32f,.20f,.65f});
  const std::pair<const char*,float> labels[]={{"BROKEN WATCH",.065f},{"DAWNWATCH VILLAGE",.865f}};
  for (const auto& label : labels) {
   const float width=c.Width(label.first,16),x=a.x+(a.w-width)*.5f,y=a.y+a.h*label.second;
   c.RectFill({x-7,y-2,width+14,23},{.90f,.83f,.66f,.92f});
   lettering(x,y,label.first,16);
   c.Line(x,y+20,x+width,y+20,.65f,{.40f,.32f,.20f,.45f});
  }
  const float x=a.x+20,y=a.y+a.h-34,span=5*a.h/46;
  c.Line(x,y,x+span,y,1,mapInk);
  for (float t : {0.f,.5f,1.f}) c.Line(x+span*t,y-3,x+span*t,y+3,.8f,mapInk);
  lettering(x,y+5,"0",10); lettering(x+span-14,y+5,"5 m",10);
 }
 const auto marker=[&](rpg::Vec2 position,Color tint,float radius) {
  const auto p=MapPoint(a,position); c.Oval(p.x,p.y,radius+1,radius+1,{.08f,.12f,.13f,1}); c.Oval(p.x,p.y,radius,radius,tint);
 };
 for (const auto& e : sim.Enemies()) if (e.health>0 || !e.looted)
  marker(e.position,e.health>0?Color{.72f,.20f,.15f,1}:gold,compact?2.f:4.f);
 marker(rpg::WorldMap::questGiver,{.98f,.77f,.25f,1},compact?3.f:5.f);
 marker(rpg::WorldMap::astralAnchor,{.59f,.38f,.84f,1},compact?3.f:5.f);
 marker(rpg::WorldMap::trainingPost,{.35f,.72f,.76f,1},compact?3.f:5.f);
 marker(rpg::WorldMap::quartermaster,{.95f,.57f,.22f,1},compact?3.f:5.f);
 if (sim.AnchorState()!=rpg::AnchorQuest::Complete) {
  const auto p=MapPoint(a,sim.ObjectivePosition());
  c.Ring(p.x,p.y,compact?5.f:10.f,mapInk,compact?2.5f:3.5f);
  c.Ring(p.x,p.y,compact?5.f:10.f,{.98f,.85f,.40f,1},compact?1.f:2.f);
 }
 if (waypoint) {
  const auto p=MapPoint(a,pin); c.Line(p.x,p.y,p.x,p.y-12,2,{.15f,.30f,.49f,1});
  c.Triangle({p.x,p.y-12},{p.x+9,p.y-9},{p.x,p.y-5},{.18f,.48f,.80f,1});
 }
 const auto p=MapPoint(a,sim.Player()); const float angle=sim.Facing(),size=compact?5.f:8.f;
 const auto arrow=[&](float x,float y) -> Point { return {p.x+std::cos(angle)*x-std::sin(angle)*y,p.y+std::sin(angle)*x+std::cos(angle)*y}; };
 c.Oval(p.x,p.y,size+2,size+2,{.07f,.13f,.18f,.85f});
 c.Triangle(arrow(0,-size),arrow(-size*.65f,size*.7f),arrow(size*.65f,size*.7f),{.68f,.87f,1,1});
 if (!compact) {
  const float x=a.x+a.w-52,y=a.y+66;
  c.Ring(x,y,19,{.40f,.32f,.20f,.65f},.65f);
  for (int i=0;i<8;++i) {
   const float angle=i*3.14159265f/4,radius=i%2?15.f:29.f;
   const Point tip{x+std::sin(angle)*radius,y-std::cos(angle)*radius};
   const Point left{x+std::sin(angle-.35f)*6,y-std::cos(angle-.35f)*6};
   const Point right{x+std::sin(angle+.35f)*6,y-std::cos(angle+.35f)*6};
   c.Triangle({x,y},left,tip,{.35f,.28f,.17f,.9f});
   c.Triangle({x,y},tip,right,{.71f,.59f,.38f,1});
   c.Line(left.x,left.y,tip.x,tip.y,.65f,mapInk);
   c.Line(right.x,right.y,tip.x,tip.y,.65f,mapInk);
  }
  lettering(x-5,y-49,"N",15); lettering(x+34,y-8,"E",11);
  lettering(x-4,y+31,"S",11); lettering(x-43,y-8,"W",11);
 }
}
}

void Interface::Render(Renderer& renderer,const rpg::Simulation& s,float time,bool debug) {
 Canvas c(renderer); regions_.clear();
 renderedState_=StateKey(); renderedWidth_=float(renderer.Width()); renderedHeight_=float(renderer.Height());
 if (!renderer.HasMapArtwork() && !renderer.SetMapArtwork(BuildMapArtwork(s.Map()),512)) return;
 const auto button=[&](Rect rect,const std::string& label,Action action,int value=0,bool enabled=true) {
  const bool hover=rect.Contains(mouseX_,mouseY_);
  c.Frame(rect,enabled && hover?Color{.26f,.31f,.12f,1}:Color{.10f,.16f,.075f,1},hover && enabled?gold:border);
  c.Gradient({rect.x+4,rect.y+4,rect.w-8,rect.h-8},enabled?Color{.23f,.28f,.12f,1}:Color{.12f,.14f,.08f,1},{.065f,.10f,.045f,1});
  c.Center(rect.x,rect.y+(rect.h-15)*.5f-2,rect.w,label,15,enabled?ink:muted);
  regions_.push_back({rect,{action,value},enabled});
 };
 const auto slot=[&](Rect rect,Item item,bool selectedSlot=false,int bag=-1,int gear=-1) {
  if (item==Item::None) {
   c.EmptySlot(rect);
   // Empty bag cells clear inspection but remain valid drag destinations.
   regions_.push_back({rect,{Action::SelectItem,int(Item::None)},bag>=0,bag,gear});
   regions_.back().event.inventory.source=bag;
   return;
  }
  const bool rare=item==Item::Ward || item==Item::Watchguard || item==Item::Helm || item==Item::Shield;
  const bool hover=rect.Contains(mouseX_,mouseY_);
  c.Frame(rect,{.03f,.065f,.065f,1},hover || selectedSlot?gold:rare?Color{.35f,.64f,.31f,1}:Color{.44f,.43f,.25f,1});
  c.Gradient({rect.x+4,rect.y+4,rect.w-8,rect.h-8},rare?Color{.14f,.31f,.20f,1}:Color{.14f,.22f,.25f,1},{.025f,.045f,.06f,1});
  for (int i=0;i<5;++i) c.Line(rect.x+rect.w*.5f,rect.y+rect.h*.5f,rect.x+5+i*(rect.w-10)/4,rect.y+5,1,{.52f,.60f,.39f,.15f});
  Icon(c,{rect.x+3,rect.y+3,rect.w-6,rect.h-6},item,dragging_ && item==dragItem_?.35f:1.f);
  if (Equipped(s,item)) c.Diamond(rect.x+rect.w-5,rect.y+5,3,3,{.66f,.92f,.39f,1});
  if (item==Item::Potion) c.Text(rect.x+rect.w-15,rect.y+rect.h-16,std::to_string(bag>=0?s.Items().Bag()[bag].count:s.Potions()),12,{1,.94f,.57f,1});
  regions_.push_back({rect,{Action::SelectItem,static_cast<int>(item)},true,bag,gear});
  regions_.back().event.inventory.source=bag;
 };
 const auto blockWindow=[&](Rect rect) { regions_.push_back({rect,{},false}); c.Window(rect); };
 const auto close=[&](Rect rect,Panel which) { button(rect,"X",Action::Close,int(which)); };

 // Carved unit frame, circular survey and compact combat dock.
 c.Window({18,23,276,72});
 c.Oval(49,58,27,29,{.04f,.07f,.025f,1}); c.Ring(49,58,26,gold,2); c.Ring(49,58,21,border,2);
 c.Diamond(49,58,17,21,{.18f,.28f,.13f,1}); c.Center(25,42,48,std::to_string(s.Level()),25,gold);
 c.Banner({77,18,208,27},"Frontier guard",16);
 c.RectFill({84,53,194,19},{.025f,.045f,.02f,1});
 c.Gradient({86,55,190*s.Health()/s.MaxHealth(),15},{.42f,.62f,.20f,1},{.12f,.29f,.085f,1});
 c.Center(84,52,194,std::to_string(int(s.Health()))+" / "+std::to_string(int(s.MaxHealth())),13);
 c.Text(87,77,"Dawnwatch expedition",12,muted);
 if (s.TargetActor()) {
  const auto& target=*s.TargetActor(); c.Window({475,26,278,62}); c.Banner({487,20,254,26},target.Name(),16);
  c.RectFill({491,52,246,14},{.12f,.035f,.02f,1});
  c.Gradient({493,54,242*target.health/target.MaxHealth(),10},{.74f,.30f,.13f,1},{.35f,.06f,.03f,1});
  c.Center(491,50,246,std::to_string(int(std::ceil(target.health)))+" / "+std::to_string(int(target.MaxHealth())),12,ink);
  const auto distance=std::to_string(int(std::ceil(std::hypot(s.Player().x-target.position.x,s.Player().z-target.position.z))));
  c.Center(479,69,270,target.windup>0?"Strike in "+Seconds(target.windup)+"s - evade!":distance+" m  /  Selected target",12,target.windup>0?gold:muted);
  if (target.windup>0) {
   c.RectFill({491,84,246,3},{.12f,.035f,.02f,1});
   c.RectFill({491,84,246*std::clamp(1-target.windup/target.WindupDuration(),0.f,1.f),3},{1,.57f,.16f,1});
  }
 }
 c.Oval(1170,94,81,81,{.025f,.04f,.02f,1});
 Atlas(c,s,{1096,20,148,148},true,hasWaypoint,waypoint);
 c.Ring(1170,94,77,{.30f,.33f,.10f,1},5); c.Ring(1170,94,80,gold,1.5f); c.Ring(1170,94,74,gold,1);
 for (int i=0;i<16;++i) {
  const float a=i*6.2831853f/16;
  c.Line(1170+std::sin(a)*75,94+std::cos(a)*75,1170+std::sin(a)*79,94+std::cos(a)*79,1,gold);
 }
 c.Diamond(1170,17,6,9,{.39f,.51f,.16f,1}); c.Text(1164,23,"N",12);
 c.Banner({1057,3,209,25},"Dawnwatch",16);
 c.Banner({1118,158,104,23},"Atlas  [M]",12);
 regions_.push_back({{1090,13,160,168},{Action::Map},true});
 if (!inventoryOpen || panel!=Panel::None) {
  c.Frame({1001,193,265,hasWaypoint?183.f:158.f},{.025f,.045f,.025f,.82f},border);
  c.Text(1013,202,"EXPEDITION",14,gold); c.Line(1010,226,1260,226,1,border);
  c.Wrap(1013,236,244,s.Objective(),15,ink,4);
  const auto objective=s.ObjectivePosition();
  if (s.AnchorState()!=rpg::AnchorQuest::Complete)
   c.Text(1013,332,std::to_string(int(std::ceil(std::hypot(s.Player().x-objective.x,s.Player().z-objective.z))))+" m to objective",13,gold);
  if (hasWaypoint) c.Text(1013,355,"Waypoint  "+std::to_string(int(std::ceil(std::hypot(s.Player().x-waypoint.x,s.Player().z-waypoint.z))))+" m",13,{.56f,.76f,.88f,1});
 }
 if (s.Sparring() || s.SparringReportTime()>0) {
  c.Window({470,105,506,81});
  c.Text(485,113,s.Sparring()?"SPARRING  /  "+std::to_string(int(std::ceil(s.SparringSeconds())))+" seconds":s.SparringResult(),17,gold);
  c.Text(485,141,"Dealt "+std::to_string(s.SparringDamageDealt())+"  /  Taken "+std::to_string(s.SparringDamageTaken())+"  /  No XP or loot",13);
  c.Text(485,164,s.Sparring()?"E stops. Nonlethal. Potions are consumed normally.":"Health and consumables are not refunded. E at the post starts again.",11,muted);
 }
 c.Gradient({0,704,1280,16},{.13f,.20f,.085f,1},{.035f,.06f,.03f,1}); c.Line(0,704,1280,704,2,border);
 for (float x=14;x<1280;x+=32) c.Scroll(x,706,1,.5f);
 const float xp=s.Level()==3?1.f:float(s.Experience()-s.LevelStartExperience())/(s.NextLevelExperience()-s.LevelStartExperience());
 c.Frame({439,645,403,7}); c.RectFill({442,648,397*xp,2},{.55f,.63f,.29f,1});
 c.Window({437,655,406,55}); c.Scroll(420,676,-1,1.5f); c.Scroll(860,676,1,1.5f);
 for (int i=0;i<9;++i) {
  const float x=447+i*43.f; const Rect rect{x,662,39,40};
  if (i>=5) { c.EmptySlot(rect); continue; }
  c.Frame(rect,{.08f,.12f,.055f,1},rect.Contains(mouseX_,mouseY_)?gold:border);
  c.Gradient({x+4,666,31,32},i==3?Color{.16f,.36f,.16f,1}:Color{.25f,.19f,.10f,1},{.035f,.06f,.03f,1});
  if (i==3) {
   c.Oval(x+20,682,13,13,{.20f,.39f,.17f,1});
   c.RectFill({x+17,671,6,22},{.76f,.91f,.43f,1}); c.RectFill({x+9,679,22,6},{.76f,.91f,.43f,1});
  } else {
   Icon(c,{x+2,663,35,35},i<2?Item::Starter:i==4?Item::Potion:Item::Boots);
   if (i==1) c.Line(x+6,679,x+31,690,2,{.94f,.41f,.12f,1});
  }
  const bool ready=panel!=Panel::Merchant && (i<4?std::string(s.AbilityStatus(i))=="READY":CanUse(s,Item::Potion));
  const float cooldown=i<4?s.Cooldown(i):s.PotionCooldown();
  if (!ready) c.RectFill({x+3,665,33,34},{.02f,.025f,.01f,.36f});
  if (cooldown>0) {
   const float fraction=std::clamp(cooldown/(i<4?s.AbilityCooldown(i):3.f),0.f,1.f);
   c.RectFill({x+3,665,33,34*fraction},{.015f,.025f,.04f,.65f});
   c.Center(x,675,39,Seconds(cooldown),cooldown<1?15.f:19.f,ink);
   c.RectFill({x+4,698,31*(1-fraction),2},blue);
  } else c.RectFill({x+4,698,31,2},ready?Color{.51f,.78f,.37f,1}:Color{.58f,.25f,.13f,1});
  if (i==4) { c.RectFill({x+23,688,14,12},{.025f,.035f,.02f,.9f}); c.Center(x+22,686,16,std::to_string(s.Potions()),11,ink); }
  c.Text(x+3,662,i<4?std::to_string(i+1):"Q",12,{1,.96f,.73f,1});
  regions_.push_back({rect,{i<4?Action::Ability:Action::UseItem,i<4?i:static_cast<int>(Item::Potion)},ready});
 }
 const std::pair<Action,const char*> shortcuts[]={{Action::Build,"N"},{Action::Character,"C"},{Action::Inventory,"I"},{Action::Map,"M"},{Action::Journal,"J"},{Action::Menu,"Esc"}};
 for (int i=0;i<6;++i) button({1027+i*40.f,671,37,32},shortcuts[i].second,shortcuts[i].first);
 if (s.TalentPoints()>0) {
  c.Oval(1059,670,9,9,{.35f,.20f,.06f,1}); c.Ring(1059,670,9,gold);
  c.Center(1050,663,18,std::to_string(s.TalentPoints()),12,ink);
  regions_.push_back({{1050,661,18,18},{Action::Build},true});
 }
 c.Text(22,674,std::to_string(s.Gold())+" gold",16,gold);
 c.Text(22,695,"WASD  Move     RMB  Orbit     F11  Display",11,muted);
 if (!characterOpen) c.Wrap(22,578,405,s.Message(),14,ink,3);
 if (panel==Panel::None && s.Health()>0) {
  const auto prompt=s.InteractionPrompt();
  if (!prompt.empty()) { const float w=std::min(510.f,c.Width(prompt,14)+30); c.Frame({640-w/2,609,w,27}); c.Center(640-w/2,614,w,prompt,14,gold); }
  if (s.Channeling()) { c.RectFill({470,640,340,3},border); c.RectFill({470,640,340*s.ChannelProgress(),3},blue); }
 }

 // Independent side windows: no world dimming, keyboard movement remains live.
 if (characterOpen) {
  blockWindow({12,116,438,500});
  c.Banner({30,106,265,31},"Frontier guard",19); close({420,121,24,24},Panel::Character);
  c.Center(30,148,265,"Level "+std::to_string(s.Level())+"  /  Dawnwatch guard",13);
  c.Frame({65,176,188,329},{.025f,.045f,.025f,1});
  for (int i=0;i<16;++i) {
   const float x=69+i*11.f;
   c.Gradient({x,180,11,280},{.12f+.045f*std::sin(i*1.7f),.028f,.022f,1},{.024f,.043f,.023f,1});
  }
  for (int row=0;row<4;++row) for (int column=0;column<5;++column)
   c.Frame({69+column*35.f,462+row*9.f,34,8},{.13f,.18f,.105f,1},{.17f,.22f,.12f,1});
  c.Oval(159,474,63,12,{.01f,.02f,.01f,.7f});
  const auto p=c.P(70,181);
  if (renderer.BeginPortrait(p.x,p.y,178*c.layout.scale/renderer.Width(),316*c.layout.scale/renderer.Height())) {
   DrawCharacter(renderer,{0,0},0,previewYaw+std::sin(time*.35f)*.05f,{.22f,.37f,.54f,1},0,0,CharacterOutfit::From(s.Items()),animation::Idle(time));
   renderer.EndPortrait();
  }
  using G=rpg::GearSlot;
  const G left[]={G::Chest,G::Legs,G::Feet,G::Shoulders,G::Back};
  const G right[]={G::Weapon,G::Offhand,G::Head,G::Hands,G::Waist};
  const char* leftLabels[]={"Chest","Legs","Feet","Shoulders","Back"};
  const char* rightLabels[]={"Weapon","Off hand","Head","Hands","Waist"};
  for (int row=0;row<5;++row) {
   const float y=190+row*64.f;
   slot({23,y,38,40},s.Items().Worn(left[row]),false,-1,int(left[row]));
   slot({258,y,38,40},s.Items().Worn(right[row]),false,-1,int(right[row]));
   c.Center(15,y+44,54,leftLabels[row],10,muted);
   c.Center(251,y+44,52,rightLabels[row],10,muted);
  }
  c.Banner({69,514,180,26},s.WardArmorEquipped()?"Astral ward":"Frontier outfit",14);
  c.Text(28,552,"Drag gear to your bag. Right-click to remove.",11,muted);
  button({26,579,34,27},"<",Action::RotatePreview,-1);
  button({64,579,63,27},"Front",Action::RotatePreview,0);
  button({131,579,34,27},">",Action::RotatePreview,1);
  button({174,579,117,27},inventoryOpen?"Close bag [I]":"Open bag [I]",Action::Inventory);
  c.Frame({304,152,132,449},{.035f,.075f,.07f,1});
  c.Center(304,163,132,"VITALITY",12,gold);
  c.Center(310,186,120,std::to_string(int(s.Health()))+" / "+std::to_string(int(s.MaxHealth())),16);
  c.Frame({314,211,112,12});
  c.Gradient({317,214,106*std::clamp(s.Health()/s.MaxHealth(),0.f,1.f),6},{.56f,.27f,.20f,1},{.28f,.08f,.07f,1});
  c.Center(304,239,132,"COMBAT",12,gold);
  const char* names[]={"Armor","Strike","Heavy strike"};
  const int values[]={int(s.DamageReduction()),int(s.StrikeDamage()),int(s.HeavyDamage())};
  for (int i=0;i<3;++i) {
   const float y=261+i*34.f;
   c.Gradient({312,y,116,30},{.12f,.19f,.16f,1},{.055f,.10f,.09f,1});
   c.Text(318,y+8,names[i],11,muted);
   const auto value=std::to_string(values[i]); c.Text(420-c.Width(value,15),y+6,value,15,ink);
  }
  c.Line(314,374,426,374,1,border);
  c.Center(304,387,132,"PROGRESSION",12,gold);
  c.Center(310,409,120,"Level "+std::to_string(s.Level()),19);
  const float xp=s.Level()==3?1.f:std::clamp(float(s.Experience()-s.LevelStartExperience())/
    float(s.NextLevelExperience()-s.LevelStartExperience()),0.f,1.f);
  c.Frame({314,438,112,10}); c.Gradient({317,441,106*xp,4},blue,{.13f,.25f,.42f,1});
  c.Center(310,457,120,s.Level()==3?"Level cap reached":std::to_string(s.NextLevelExperience()-s.Experience())+" XP to next level",10,muted);
  c.Line(314,484,426,484,1,border);
  c.Text(316,501,"Gold",12,muted);
  const auto wealth=std::to_string(s.Gold()); c.Text(422-c.Width(wealth,14),500,wealth,14,gold);
  c.Text(316,528,"Potions",12,muted);
  const auto potions=std::to_string(s.Potions()); c.Text(422-c.Width(potions,14),527,potions,14,ink);
  int wornCount=0; for(int i=0;i<rpg::Inventory::gearCount;++i) wornCount+=s.Items().Worn(rpg::GearSlot(i))!=Item::None;
  c.Center(310,572,120,std::to_string(wornCount)+" / "+std::to_string(rpg::Inventory::gearCount)+" slots equipped",10,muted);
 }
 if (inventoryOpen) {
  blockWindow({994,196,276,452}); c.Banner({1023,186,218,31},"Bag",19); close({1239,201,24,24},Panel::Inventory);
  c.Text(1013,231,"BELONGINGS",11,gold);
  button({1183,223,69,25},"Sort",Action::InventoryChange);
  regions_.back().event.inventory.action=rpg::InventoryAction::Sort;
  if (!Owned(s,selected)) selected=Item::None;
  if (selectedBag<0 || selectedBag>=rpg::Inventory::capacity || s.Items().Bag()[selectedBag].item!=selected)
    selectedBag=s.Items().Find(selected);
  for (int i=0;i<24;++i) {
   const Rect rect{1010+float(i%6)*40,255+float(i/6)*42,37,39};
   const auto item=s.Items().Bag()[i].item;
   slot(rect,item,selectedBag==i,i);
  }
  c.Frame({1008,430,248,27},{.035f,.065f,.06f,1}); c.Text(1017,436,std::to_string(24-s.Items().UsedSlots())+" free slots",12,muted);
  c.Oval(1189,443,5,5,gold); c.Text(1200,434,std::to_string(s.Gold()),14,gold);
  const auto& info=Describe(selected);
  c.Frame({1008,464,248,104},{.025f,.055f,.05f,1});
  if(selected!=Item::None) Icon(c,{1017,473,30,30},selected);
  c.Text(1056,475,selected==Item::None?"Select an item":info.name,13,selected==Item::Watchguard || selected==Item::Ward?Color{.63f,.85f,.43f,1}:ink);
  c.Text(1056,494,selected==Item::None?"Inspect your belongings":info.type,10,muted);
  std::string detail=info.description;
  const auto delta=[](float value) { return (value>=0?std::string("+"):std::string())+std::to_string(int(value)); };
  if (info.slot==rpg::GearSlot::Weapon) {
   detail="Strike "+std::to_string(int(info.strike+s.StrikeBonus()))+"  /  Heavy "+std::to_string(int(info.heavy+s.HeavyBonus()))+" damage.\n";
   const auto& current=Describe(s.Items().Worn(info.slot));
   detail+=Equipped(s,selected)?"Equipped / remove to fight unarmed":"Vs worn: "+delta(info.strike-current.strike)+" strike / "+delta(info.heavy-current.heavy)+" heavy";
  } else if (info.slot!=rpg::GearSlot::None) {
   detail="Damage reduction: "+std::to_string(int(info.armor))+".\n";
   detail+=Equipped(s,selected)?"Equipped / right-click to remove":"Compared to worn: "+delta(info.armor-Describe(s.Items().Worn(info.slot)).armor)+" armor";
  }
  if(selected==Item::None) detail="Click a slot to inspect. Drag gear to equip or right-click to use.";
  c.Wrap(1018,518,230,detail,11,ink,3);
  const int count=selectedBag>=0?s.Items().Bag()[selectedBag].count:0;
  const bool split=count>1 && s.Items().UsedSlots()<rpg::Inventory::capacity && s.Health()>0;
  if (panel==Panel::Merchant) {
   const rpg::TradeCommand sale{rpg::TradeAction::Sell,selected,selectedBag};
   button({1011,574,132,27},Equipped(s,selected)?"Unequip to sell":selectedBag<0?"Select bag item":"Sell one "+std::to_string(rpg::SellPrice(selected))+" g",
     Action::Sell,int(selected),std::string(s.TradeStatus(sale))=="Available");
   regions_.back().event.inventory.source=selectedBag;
  }
  if (selected==Item::Potion) {
   button({panel==Panel::Merchant?1148.f:1011.f,574,panel==Panel::Merchant?102.f:239.f,27},"Split half",Action::InventoryChange,0,split);
   regions_.back().event.inventory={rpg::InventoryAction::Split,selectedBag,-1,selected,count/2};
  }
  const bool usable=CanUse(s,selected);
  std::string label=selected==Item::Potion?(s.Health()<=0?"Defeated":s.Health()>=s.MaxHealth()?"Full health":s.PotionCooldown()>0?"Cooling down":"Drink draught"):
   selected==Item::None?"Select an item":Equipped(s,selected)?"Unequip":"Equip";
  button({1011,606,239,29},label,Action::UseItem,int(selected),usable);
  regions_.back().event.inventory.source=selectedBag;
 }

 if (panel==Panel::Merchant) {
  // Side windows remain interactive while the service window captures movement.
  blockWindow({470,126,506,522}); c.Banner({487,116,472,35},"Dawnwatch quartermaster",21);
  close({939,158,24,24},Panel::Merchant);
  c.Text(489,168,"FIELD SUPPLIES",13,gold); c.Text(755,168,std::to_string(s.Gold())+" gold",14,gold);
  for (int i=0;i<int(rpg::shopOffers.size());++i) {
   const auto& offer=rpg::shopOffers[i]; const auto& info=Describe(offer.item); const float y=196+i*70.f;
   c.Frame({487,y,471,66},{.065f,.11f,.06f,1}); c.Frame({496,y+10,43,46},{.11f,.20f,.14f,1},gold);
   Icon(c,{500,y+14,35,38},offer.item);
   c.Text(550,y+6,info.name,16,ink);
   c.Text(550,y+28,"Stock "+std::to_string(s.ShopStock(i))+" / "+std::to_string(offer.stock)+"   |   "+info.type,11,muted);
   const bool available=std::string(s.PurchaseStatus(i))=="Available";
   c.Text(550,y+45,available?(offer.item==Item::Potion?"Restores 40 health.":"Protection +"+std::to_string(int(info.armor))+" while equipped."):s.PurchaseStatus(i),12,available?ink:gold);
   button({854,y+12,92,30},"Buy "+std::to_string(offer.price)+" g",Action::Purchase,i,available);
  }
  c.Text(490,418,"BUYBACK",13,gold); c.Text(590,418,"Click an item to reclaim one at its sale price",12,muted);
  for (int i=0;i<int(Item::Count);++i) {
   const auto item=Item(i); const int count=s.BuybackCount(item);
   const Rect rect{490+(i%9)*51.f,447+(i/9)*66.f,43,45};
   if (count==0) { c.EmptySlot(rect); continue; }
   const bool available=std::string(s.TradeStatus({rpg::TradeAction::Buyback,item}))=="Available";
   c.Frame(rect,{.08f,.16f,.11f,1},rect.Contains(mouseX_,mouseY_)?gold:border);
   Icon(c,{rect.x+4,rect.y+4,35,37},item,available?1.f:.5f);
   if (count>1) c.Text(rect.x+28,rect.y+28,std::to_string(count),12,gold);
   c.Center(rect.x,rect.y+50,rect.w,std::to_string(rpg::SellPrice(item))+" g",12,gold);
   regions_.push_back({rect,{Action::Buyback,i},available});
  }
  c.Wrap(490,582,465,merchantNotice.empty()?"Unequip gear before selling. Buyback is saved until reclaimed or R resets the expedition.":merchantNotice,12,ink,2);
  c.Text(490,624,"E or X closes trade. C opens your equipment.",12,muted);
 } else if (panel!=Panel::None) {
  c.RectFill({0,0,1280,720},{0,0,0,panel==Panel::Menu?.16f:.30f}); regions_.clear();
  if (panel==Panel::Menu) {
   c.Window({476,148,328,438}); c.Banner({490,137,300,34},"MEDITERRANEAN",20);
   c.Center(476,177,328,"Dawnwatch expedition",12,muted);
   button({496,197,288,34},"Return to game",Action::Close);
   button({496,241,139,32},"Character",Action::Character); button({645,241,139,32},"Inventory",Action::Inventory);
   button({496,281,139,32},"Atlas",Action::Map); button({645,281,139,32},"Journal",Action::Journal);
   button({496,327,288,32},fullscreen?"Fullscreen  /  F11":"Windowed  /  F11",Action::Fullscreen);
   button({496,369,139,32},"Save game",Action::Save); button({645,369,139,32},"Load game",Action::Load);
   c.Line(500,418,780,418,1,border);
   c.Text(500,433,"Music  "+std::to_string(musicVolume)+"%",13,ink);
   button({712,427,30,25},"-",Action::MusicVolume,-10,musicVolume>0);
   button({752,427,30,25},"+",Action::MusicVolume,10,musicVolume<100);
   c.Text(500,467,"Effects  "+std::to_string(effectsVolume)+"%",13,ink);
   button({712,461,30,25},"-",Action::EffectsVolume,-10,effectsVolume>0);
   button({752,461,30,25},"+",Action::EffectsVolume,10,effectsVolume<100);
   button({496,506,288,32},"Exit to desktop",Action::Quit);
   c.Center(476,546,328,audioAvailable?"World remains live  /  Esc to return":"Audio unavailable",12,muted);
   if (!menuNotice.empty()) c.Center(476,563,328,menuNotice,11,gold);
   return;
  }
  if (panel==Panel::Build) {
   selectedTalent=std::clamp(selectedTalent,0,int(rpg::Talent::Count)-1);
   c.Window({96,52,1088,595}); c.Banner({123,43,1003,39},"Champion build",23);
   close({1146,61,27,27},Panel::Build);
   c.Text(124,104,"Level "+std::to_string(s.Level())+"  /  Dawnwatch guard",18,ink);
   c.Frame({850,102,306,32},{.07f,.16f,.13f,1},s.TalentPoints()>0?gold:border);
   c.Center(850,109,306,std::to_string(s.TalentPoints())+" talent point"+(s.TalentPoints()==1?"":"s")+" available",15,gold);
   c.Frame({284,149,392,51},{.09f,.18f,.16f,1},gold);
   c.Center(284,157,392,"LEVEL 1  /  FOUNDATIONS",15,gold);
   c.Center(284,178,392,"Strike  /  Heavy strike  /  Dash  /  Mend",12,ink);
   c.Line(480,200,480,223,3,border); c.Line(248,223,712,223,3,border);
   const char* branches[]={"ARMS","GUARD","SPIRIT"};
   const Color branchColors[]={{.83f,.54f,.31f,1},{.40f,.64f,.83f,1},{.45f,.77f,.57f,1}};
   for (int branch=0;branch<3;++branch) {
    const float x=248+branch*232.f;
    c.Line(x,223,x,274,3,border);
    c.RectFill({x-76,235,152,25},dark); c.Center(x-76,239,152,branches[branch],16,branchColors[branch]);
    const bool connected=s.HasTalent(rpg::Talent(branch));
    c.Line(x,377,x,445,4,{.025f,.045f,.04f,1});
    c.Line(x,377,x,445,2,connected?branchColors[branch]:border);
    c.Oval(x,410,16,16,dark); c.Ring(x,410,16,connected?branchColors[branch]:border);
    c.Center(x-15,401,30,"3",14,connected?ink:muted);
   }
   for (int i=0;i<int(rpg::Talent::Count);++i) {
    const auto talent=rpg::Talent(i); const auto& info=rpg::talents[i];
    const bool learned=s.HasTalent(talent),available=std::string(s.TalentStatus(talent))=="Ready to learn";
    const Rect rect{143+(i%3)*232.f,274+(i/3)*171.f,210,103};
    const bool hovered=rect.Contains(mouseX_,mouseY_),selectedNode=i==selectedTalent;
    const Color accent=learned?Color{.57f,.83f,.45f,1}:available?gold:border;
    c.Frame(rect,selectedNode?Color{.12f,.23f,.20f,1}:Color{.035f,.085f,.075f,1},selectedNode || hovered?gold:accent);
    c.RectFill({rect.x+4,rect.y+4,3,rect.h-8},accent);
    TalentIcon(c,{rect.x+13,rect.y+10,34,38},i,learned || available?1.f:.45f);
    c.Wrap(rect.x+57,rect.y+11,143,info.name,16,learned || available?ink:muted,2);
    c.Wrap(rect.x+12,rect.y+53,186,info.effect,12,muted,2);
    const char* state=learned?"LEARNED":available?"AVAILABLE / 1 POINT":
      s.Level()<info.level?"LOCKED":info.prerequisite!=rpg::Talent::Count && !s.HasTalent(info.prerequisite)?"PREREQUISITE":"UNAVAILABLE";
    c.Text(rect.x+12,rect.y+84,"LEVEL "+std::to_string(info.level)+" / "+state,10,accent);
    regions_.push_back({rect,{Action::SelectTalent,i},true});
   }
   c.Frame({850,150,306,454},{.035f,.08f,.075f,1});
   const auto talent=rpg::Talent(selectedTalent); const auto& info=rpg::talents[selectedTalent];
   const bool learned=s.HasTalent(talent);
   const std::string status=s.TalentStatus(talent);
   TalentIcon(c,{874,166,48,52},selectedTalent);
   c.Text(937,175,branches[selectedTalent%3],14,branchColors[selectedTalent%3]);
   c.Text(937,199,"Rank "+std::to_string(selectedTalent/3+1),12,muted);
   c.Wrap(874,232,258,info.name,20,gold,2);
   c.Wrap(874,288,258,info.effect,15,ink,2);
   c.Wrap(874,341,258,info.description,14,muted,4);
   c.Line(874,423,1132,423,1,border);
   c.Text(874,436,"Requires level "+std::to_string(info.level),13,ink);
   c.Wrap(874,460,258,info.prerequisite==rpg::Talent::Count?"Prerequisite: Foundations":
     std::string("Prerequisite: ")+rpg::talents[int(info.prerequisite)].name,12,muted,2);
   c.Wrap(874,493,258,status,13,learned?Color{.57f,.83f,.45f,1}:gold,2);
   button({874,546,258,37},learned?"Learned":"Learn / 1 point",Action::LearnTalent,selectedTalent,status=="Ready to learn");
   c.Text(148,566,"One point at level 2 and level 3. Specialize or mix branches.",13,ink);
   c.Frame({148,589,664,8}); c.RectFill({151,592,658*xp,2},gold);
   c.Text(148,607,s.Level()==3?"Level cap reached  /  2 points earned":
     std::to_string(s.Experience())+" XP  /  "+std::to_string(s.NextLevelExperience())+" for next level",12,muted);
   c.Text(124,628,"N or X closes. Choices are saved with F5. The world remains active.",12,muted);
   return;
  }
  const bool map=panel==Panel::Map;
  const Rect frame=map?Rect{130,44,1020,604}:Rect{180,70,920,572};
  c.Window(frame); c.Banner({frame.x+27,frame.y-9,frame.w-85,39},map?"Atlas of Dawnwatch":"Expedition journal",23);
  close({frame.x+frame.w-38,frame.y+9,27,27},panel);
  if (map) {
   const Rect mapRect{155,126,645,480}; c.Frame({151,122,653,488}); Atlas(c,s,mapRect,false,hasWaypoint,waypoint);
   regions_.push_back({mapRect,{Action::PlaceWaypoint},true});
   c.Text(833,134,"DAWNWATCH ISLAND",18,gold); c.Text(833,166,"Hand-drawn island survey",14,muted);
   c.Wrap(833,207,286,s.Objective(),17,ink,4);
   const std::pair<Color,const char*> legend[]={{{.68f,.87f,1,1},"Your position and facing"},{gold,"Keeper / quest objective"},{{.72f,.20f,.15f,1},"Living raider"},{{.59f,.38f,.84f,1},"Astral anchor"},{{.35f,.72f,.76f,1},"Sparring post"},{{.95f,.57f,.22f,1},"Quartermaster"}};
   for (int i=0;i<6;++i) { c.Oval(842,320+i*27.f,4,4,legend[i].first); c.Text(858,309+i*27.f,legend[i].second,15,muted); }
   c.Wrap(833,486,286,"Click traversable ground to place a waypoint. Right-click to clear it.",15,ink,3);
   if (hasWaypoint) button({833,562,286,39},"Clear waypoint",Action::ClearWaypoint);
   c.Text(156,619,"Roads and markers follow the playable map. Island: 34 x 38 m.",13,muted);
  } else {
   c.Frame({208,158,397,339}); c.Text(229,177,"01   Patrol the frontier",22);
   c.Text(229,216,s.QuestState()==rpg::Quest::Complete?"COMPLETED":"FRONTIER PATROL",13,gold);
   c.Wrap(229,257,348,"Speak to the gold beacon keeper. Defeat one raider north of the village, then return to claim your reward.",17,ink,5);
   c.Text(229,390,"REWARD",13,gold); c.Text(229,419,"25 gold  /  60 XP\nWatchguard blade",17);
   c.Frame({622,158,450,339}); c.Text(643,177,"02   Secure the astral anchor",22);
   c.Text(643,216,s.AnchorState()==rpg::AnchorQuest::Complete?"COMPLETED":s.AnchorState()==rpg::AnchorQuest::Locked?"UNLOCKS AFTER THE PATROL":"ASTRAL ANCHOR",13,gold);
   c.Wrap(643,257,402,"Clear all three raiders, stabilize the northern crystal for three seconds, then return to the keeper.",17,ink,5);
   c.Text(643,390,"REWARD",13,gold); c.Text(643,419,"50 gold  /  120 XP\nAstral ward armor",17);
   button({734,545,338,43},"Show objective on the atlas",Action::Map);
   c.Text(209,557,"Raiders defeated: "+std::to_string(s.DefeatedCount())+" / 3",17,muted);
   c.Text(209,606,"The world remains active while reading the journal.",14,muted);
  }
 }
 // Reverse hit order matches pointer dispatch: covered cells never leak tooltips.
 for (auto it=regions_.rbegin();!dragging_ && it!=regions_.rend();++it) if (it->rect.Contains(mouseX_,mouseY_)) {
  if (it->event.action==Action::SelectItem || it->event.action==Action::Purchase) {
   const bool purchase=it->event.action==Action::Purchase;
   const Item item=purchase?rpg::shopOffers[it->event.value].item:static_cast<Item>(it->event.value);
   if (!rpg::ValidItem(item)) break;
   const auto& info=Describe(item); const auto summary=SummarizeItem(s,item);
   const float x=std::clamp(mouseX_>640?mouseX_-329:mouseX_+21,8.f,963.f),y=std::clamp(mouseY_-42,8.f,466.f);
   c.Window({x,y,309,246}); Icon(c,{x+13,y+12,34,38},item);
   c.Text(x+57,y+11,info.name,16,gold); c.Text(x+57,y+37,info.type,11,muted);
   c.RectFill({x+14,y+62,281,1},border);
   c.Text(x+14,y+73,summary.stats,14,ink);
   const Color comparison=summary.difference>0?Color{.48f,.86f,.37f,1}:summary.difference<0?Color{1,.43f,.32f,1}:muted;
   c.Wrap(x+14,y+99,281,summary.comparison,13,comparison,2);
   c.Wrap(x+14,y+142,281,info.description,12,muted,3);
   const std::string quantity=it->bag>=0?"Stack "+std::to_string(s.Items().Bag()[it->bag].count):"Owned "+std::to_string(s.Items().Count(item));
   c.Text(x+14,y+198,purchase?"Price "+std::to_string(rpg::shopOffers[it->event.value].price)+" gold":quantity+"   |   Sell "+std::to_string(rpg::SellPrice(item))+" gold each",12,gold);
   const std::string action=purchase?(it->enabled?"Click Buy to purchase one":s.PurchaseStatus(it->event.value)):
     s.Health()<=0?"Recover before using equipment/items":item==Item::Potion?(s.Health()>=s.MaxHealth()?"Health is full":s.PotionCooldown()>0?"Potion is cooling down":"Right-click: drink  /  Drag: move"):
     Equipped(s,item)?"Right-click: remove  /  Drag: move":"Right-click: equip  /  Drag: move";
   c.Text(x+14,y+221,action,12,ink);
  } else if (it->event.action==Action::Buyback) {
   const auto item=Item(it->event.value); const float x=std::clamp(mouseX_-140,475.f,659.f);
   c.Window({x,312,309,116}); c.Text(x+14,322,Describe(item).name,17,gold);
   c.Text(x+14,351,"Held: "+std::to_string(s.BuybackCount(item))+"  /  "+std::to_string(rpg::SellPrice(item))+" gold each",13,ink);
   const auto status=s.TradeStatus({rpg::TradeAction::Buyback,item});
   c.Text(x+14,390,std::string(status)=="Available"?"Click to reclaim one into your bag":status,12,muted);
  } else if (panel!=Panel::Merchant && (it->event.action==Action::Ability || it->event.action==Action::UseItem) && it->rect.y>650) {
   const bool potion=it->event.action==Action::UseItem; const int ability=it->event.value;
   static const char* names[]={"Strike","Heavy strike","Dash","Mend"};
   const std::string detail=potion?"Restores up to 40 health.":ability==0?std::to_string(int(s.StrikeDamage()))+" melee damage.":
    ability==1?std::to_string(int(s.HeavyDamage()))+" damage. Interrupts a windup.":ability==2?"Rush to your selected enemy within 5 m. Stops at melee distance or obstacles.":"Restores up to "+std::to_string(int(s.MendHealing()))+" health.";
   const std::string status=potion?(s.Health()<=0?"DEFEATED":s.Potions()==0?"NO POTIONS":s.Health()>=s.MaxHealth()?"FULL HEALTH":s.PotionCooldown()>0?"COOLDOWN":"READY"):s.AbilityStatus(ability);
   const float x=std::clamp(it->rect.x-30,460.f,680.f);
   c.Window({x,470,300,149}); c.Text(x+14,480,potion?"Healing draught":names[ability],19,gold);
   c.Wrap(x+14,512,272,detail,14,ink,2);
   const float remaining=potion?s.PotionCooldown():s.Cooldown(ability);
   c.Text(x+14,554,"Cooldown: "+Seconds(potion?3.f:s.AbilityCooldown(ability))+"s"+
     (potion?"  /  Stock: "+std::to_string(s.Potions()):ability<2?"  /  Range: 2.5 m":ability==2?"  /  Range: 5 m":""),12,muted);
   c.Line(x+14,576,x+286,576,1,border);
   c.Text(x+14,589,status+(remaining>0?"  "+Seconds(remaining)+"s":""),12,status=="READY"?Color{.57f,.84f,.40f,1}:Color{1,.64f,.40f,1});
  }
  break;
 }
 if (dragging_) {
  for (auto it=regions_.rbegin();it!=regions_.rend();++it) if (it->rect.Contains(mouseX_,mouseY_)) {
   bool compatible=it->bag>=0 || (dragBag_>=0 && it->gear==int(Describe(dragItem_).slot));
   if (it->bag>=0 && dragGear_>=0) {
    const auto replacement=s.Items().Bag()[it->bag].item;
    compatible=replacement==Item::None || int(Describe(replacement).slot)==dragGear_;
   }
   if (it->bag>=0 || it->gear>=0) c.Frame(it->rect,{.1f,.2f,.1f,.15f},compatible?Color{.5f,.9f,.3f,1}:Color{.9f,.2f,.1f,1});
   break;
  }
  c.Frame({mouseX_-21,mouseY_-21,42,42},{.04f,.08f,.04f,.8f},gold);
  Icon(c,{mouseX_-19,mouseY_-19,38,38},dragItem_);
 }
 if (noticeTime>0 && panel==Panel::None) {
  c.Frame({465,111,510,58},{.035f,.06f,.025f,.95f});
  c.Wrap(477,123,486,inventoryNotice,14,ink,2);
 }
 if (s.Health()<=0 && panel==Panel::None) {
  c.Window({460,285,510,146}); c.Banner({481,276,468,37},"Return to Dawnwatch",23);
  c.Center(460,336,510,"Press Enter to recover with your progress intact.",17);
  c.Center(460,374,510,"Quests, equipment and collected loot are retained.",14,muted);
 }
 if (debug) {
  c.Frame({24,102,360,58}); c.Text(35,110,"Draws "+std::to_string(renderer.LastDrawCalls())+"   GPU "+std::to_string(renderer.GpuMilliseconds()).substr(0,5)+" ms",15);
  c.Text(35,133,"Meshes "+std::to_string(renderer.MeshBytes()/1024)+" KiB   F2 HUD   F3 metrics",12,muted);
 }
}
}
