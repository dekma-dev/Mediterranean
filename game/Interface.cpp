#include "game/Interface.h"
#include "game/CharacterView.h"
#include "game/MapArtwork.h"
#include "engine/Renderer.h"
#include <algorithm>
#include <cmath>
#include <sstream>

namespace ui {
namespace {
using Color=DirectX::XMFLOAT4;
using Point=DirectX::XMFLOAT2;
const Color ink{.94f,.89f,.69f,1},muted{.66f,.69f,.50f,1},gold{.79f,.62f,.25f,1};
const Color dark{.055f,.09f,.055f,.98f},border{.30f,.30f,.13f,1},blue{.23f,.46f,.63f,1};
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
  for (int i=0;i<24;++i) {
   const float a=i*6.2831853f/24,b=(i+1)*6.2831853f/24;
   Triangle({x,y},{x+std::cos(a)*rx,y+std::sin(a)*ry},{x+std::cos(b)*rx,y+std::sin(b)*ry},color);
  }
 }
 void Ring(float x,float y,float radius,Color tint,float width=1) {
  for (int i=0;i<32;++i) {
   const float a=i*6.2831853f/32,b=(i+1)*6.2831853f/32;
   Line(x+std::cos(a)*radius,y+std::sin(a)*radius,x+std::cos(b)*radius,y+std::sin(b)*radius,width,tint);
  }
 }
 void Diamond(float x,float y,float rx,float ry,Color tint) {
  Triangle({x,y-ry},{x+rx,y},{x,y+ry},tint); Triangle({x,y-ry},{x,y+ry},{x-rx,y},tint);
 }
 void Scroll(float x,float y,float side=1,float scale=1) {
  Point previous{x,y};
  for (int i=1;i<=22;++i) {
   const float t=i/22.f,angle=t*7.2f,rad=(1-t)*14*scale;
   Point p{x+side*(std::sin(angle)*rad),y+(1-std::cos(angle))*rad};
   Line(previous.x,previous.y,p.x,p.y,2*scale,{.22f,.30f,.14f,1});
   Line(previous.x,previous.y-1,p.x,p.y-1,.6f*scale,{.51f,.49f,.25f,.8f}); previous=p;
  }
 }
 void Window(Rect a) {
  Frame(a,dark,{.37f,.39f,.17f,1});
  Gradient({a.x+5,a.y+5,a.w-10,a.h-10},{.13f,.20f,.11f,.99f},{.045f,.075f,.045f,.99f});
  for (float y=a.y+30;y<a.y+a.h-25;y+=31) {
   Scroll(a.x+12,y,1,.55f); Scroll(a.x+a.w-12,y,-1,.55f);
  }
  RectFill({a.x+18,a.y+21,a.w-36,a.h-42},{.015f,.022f,.012f,.24f});
  for (float side : {0.f,1.f}) for (float bottom : {0.f,1.f}) {
   const float x=a.x+side*a.w,y=a.y+bottom*a.h;
   Diamond(x,y,8,8,{.12f,.17f,.07f,1}); Diamond(x,y,5,5,gold); Diamond(x,y,2,3,{.30f,.39f,.14f,1});
  }
 }
 void Banner(Rect a,const std::string& label,float size=18) {
  const float cy=a.y+a.h*.5f;
  Diamond(a.x+11,cy,18,a.h*.5f,{.16f,.19f,.065f,1}); Diamond(a.x+a.w-11,cy,18,a.h*.5f,{.16f,.19f,.065f,1});
  Frame({a.x+10,a.y,a.w-20,a.h},{.22f,.16f,.055f,1},gold);
  Gradient({a.x+13,a.y+3,a.w-26,a.h-6},{.48f,.31f,.085f,1},{.14f,.13f,.035f,1});
  for (float x : {a.x+8,a.x+a.w-8}) { Diamond(x,cy,7,11,gold); Diamond(x,cy,4,7,{.23f,.29f,.08f,1}); }
  Center(a.x,a.y+(a.h-size)*.5f-2,a.w,label,size,{1,.9f,.51f,1});
 }
 void EmptySlot(Rect a) {
  Frame(a,{.085f,.09f,.04f,1},{.26f,.26f,.12f,1});
  Scroll(a.x+a.w*.5f,a.y+6,1,a.w/46);
  Diamond(a.x+a.w*.5f,a.y+a.h*.68f,5,7,{.19f,.21f,.10f,1});
 }
};
void Icon(Canvas& c,Rect rect,Item item,float alpha=1) {
 const float x=rect.x+rect.w*.5f,y=rect.y+rect.h*.5f,s=rect.w/60;
 const Color metal{.72f,.77f,.77f,alpha},leather{.48f,.30f,.16f,alpha},cloth{.25f,.44f,.60f,alpha};
 if (item==Item::Starter || item==Item::Watchguard) {
  c.Line(x-12*s,y+17*s,x+13*s,y-15*s,7*s,item==Item::Watchguard?Color{.84f,.75f,.48f,alpha}:metal);
  c.Triangle({x+10*s,y-17*s},{x+20*s,y-24*s},{x+17*s,y-12*s},metal);
  c.Line(x-16*s,y+5*s,x-1*s,y+17*s,4*s,gold); c.Line(x-12*s,y+17*s,x-18*s,y+24*s,5*s,leather);
 } else if (item==Item::Potion) {
  c.Oval(x,y+7*s,14*s,17*s,{.33f,.57f,.61f,alpha});
  c.Oval(x,y+11*s,11*s,12*s,{.67f,.13f,.16f,alpha});
  c.RectFill({x-6*s,y-17*s,12*s,16*s},{.54f,.72f,.72f,alpha});
  c.RectFill({x-8*s,y-21*s,16*s,6*s},leather); c.Line(x-6*s,y+2*s,x-7*s,y+13*s,2*s,{.92f,.86f,.72f,alpha});
 } else if (item==Item::Tunic || item==Item::Ward) {
  const Color tint=item==Item::Ward?metal:cloth;
  c.Triangle({x-14*s,y-18*s},{x+14*s,y-18*s},{x+17*s,y+19*s},tint);
  c.Triangle({x-14*s,y-18*s},{x+17*s,y+19*s},{x-17*s,y+19*s},tint);
  c.Line(x-15*s,y-14*s,x-22*s,y+1*s,11*s,tint); c.Line(x+15*s,y-14*s,x+22*s,y+1*s,11*s,tint);
  c.RectFill({x-17*s,y+13*s,34*s,4*s},leather); c.Oval(x,y-20*s,7*s,5*s,dark);
  if (item==Item::Ward) { c.Oval(x-18*s,y-12*s,10*s,7*s,gold); c.Oval(x+18*s,y-12*s,10*s,7*s,gold); }
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
Point MapPoint(Rect a,rpg::Vec2 p) { const float span=46*a.w/a.h; return {a.x+(p.x+span*.5f)/span*a.w,a.y+(23-p.z)/46*a.h}; }
void Atlas(Canvas& c,const rpg::Simulation& sim,Rect a,bool compact,bool waypoint,rpg::Vec2 pin) {
 if (compact) {
  const auto p=c.P(a.x,a.y);
  c.r.DrawRoundMapArtwork(p.x,p.y,a.w*c.layout.scale/c.r.Width(),a.h*c.layout.scale/c.r.Height());
 } else {
 c.RectFill(a,{.16f,.26f,.30f,1});
 const float width=a.h*42/46;
 const auto top=c.P(a.x+(a.w-width)*.5f,a.y);
 c.r.DrawMapArtwork(top.x,top.y,width*c.layout.scale/c.r.Width(),a.h*c.layout.scale/c.r.Height());
 }
 static const auto terrain=visual::BuildMesh(visual::Mesh::Terrain);
 if (!compact) for (float scale : {.98f,1.f}) for (std::size_t i=1;i<terrain.vertices.size();++i) {
  const auto p=terrain.vertices[i].position,q=terrain.vertices[i+1<terrain.vertices.size()?i+1:1].position;
  const auto p2=MapPoint(a,{p.x*scale,p.z*scale}),q2=MapPoint(a,{q.x*scale,q.z*scale});
  c.Line(p2.x,p2.y,q2.x,q2.y,scale==1?1.7f:1.f,{.74f,.67f,.44f,scale==1?.85f:.23f});
 }
 if (!compact) {
  const Color label{.95f,.89f,.69f,1};
  c.Text(a.x+a.w*.12f,a.y+a.h*.54f,"Astral dock",14,label);
  c.Text(a.x+a.w*.71f,a.y+a.h*.49f,"Training grounds",14,label);
  c.Center(a.x,a.y+a.h*.12f,a.w,"BROKEN WATCH",16,label);
  c.Center(a.x,a.y+a.h*.87f,a.w,"DAWNWATCH VILLAGE",16,label);
 }
 const auto marker=[&](rpg::Vec2 position,Color tint,float radius) {
  const auto p=MapPoint(a,position); c.Oval(p.x,p.y,radius+1,radius+1,{.08f,.12f,.13f,1}); c.Oval(p.x,p.y,radius,radius,tint);
 };
 for (const auto& e : sim.Enemies()) if (e.health>0 || !e.looted)
  marker(e.position,e.health>0?Color{.72f,.20f,.15f,1}:gold,compact?2.f:4.f);
 marker(rpg::WorldMap::questGiver,{.98f,.77f,.25f,1},compact?3.f:5.f);
 marker(rpg::WorldMap::astralAnchor,{.59f,.38f,.84f,1},compact?3.f:5.f);
 if (sim.AnchorState()!=rpg::AnchorQuest::Complete) {
  const auto p=MapPoint(a,sim.ObjectivePosition()); c.Ring(p.x,p.y,compact?5.f:10.f,{.98f,.85f,.40f,1},compact?1.f:2.f);
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
  const float x=a.x+a.w-27,y=a.y+28;
  c.Line(x,y-13,x,y+13,1,{.29f,.32f,.25f,1}); c.Line(x-13,y,x+13,y,1,{.29f,.32f,.25f,1});
  c.Triangle({x,y-17},{x-4,y+5},{x+4,y+5},{.26f,.29f,.21f,1}); c.Text(x-5,y-39,"N",15,{.21f,.26f,.21f,1});
 }
}
}

void Interface::Render(Renderer& renderer,const rpg::Simulation& s,float time,bool debug) {
 Canvas c(renderer); regions_.clear();
 if (!renderer.HasMapArtwork() && !renderer.SetMapArtwork(BuildMapArtwork(s.Map()),512)) return;
 const auto button=[&](Rect rect,const std::string& label,Action action,int value=0,bool enabled=true) {
  const bool hover=rect.Contains(mouseX_,mouseY_);
  c.Frame(rect,enabled && hover?Color{.20f,.24f,.24f,.98f}:Color{.07f,.095f,.11f,.98f},enabled && hover?gold:border);
  c.Center(rect.x,rect.y+(rect.h-20)*.5f-2,rect.w,label,17,enabled?ink:muted);
  regions_.push_back({rect,{action,value},enabled});
 };
 const auto slot=[&](Rect rect,Item item,bool present,bool selectedSlot=false) {
  const bool hover=rect.Contains(mouseX_,mouseY_);
  c.Frame(rect,{.08f,.105f,.12f,1},selectedSlot || hover?gold:Equipped(s,item) && present?Color{.28f,.48f,.43f,1}:border);
  if (present) {
   Icon(c,{rect.x+5,rect.y+4,rect.w-10,rect.h-10},item);
   if (Equipped(s,item)) c.RectFill({rect.x+3,rect.y+rect.h-5,rect.w-6,2},{.36f,.65f,.51f,1});
   if (item==Item::Potion) c.Text(rect.x+rect.w-22,rect.y+rect.h-23,std::to_string(s.Potions()),17);
   regions_.push_back({rect,{Action::SelectItem,static_cast<int>(item)},true});
  }
 };
 const auto itemDetails=[&](Rect rect,Item item) {
  const auto& info=Describe(item);
  Icon(c,{rect.x,rect.y,62,62},item);
  c.Text(rect.x+77,rect.y+2,info.name,22,item==Item::Ward || item==Item::Watchguard?Color{.52f,.76f,.54f,1}:ink);
  c.Text(rect.x+77,rect.y+32,info.type,15,muted);
  c.RectFill({rect.x,rect.y+76,rect.w,1},border);
  c.Text(rect.x,rect.y+94,info.description,16,ink);
  std::string stats;
  if (item==Item::Starter || item==Item::Watchguard) {
   const bool reward=item==Item::Watchguard;
   stats="Strike damage   "+std::to_string((reward?27:20)+3*(s.Level()-1))+"\nHeavy damage   "+std::to_string((reward?45:35)+5*(s.Level()-1));
  } else if (item==Item::Ward) stats="Incoming damage reduced by 2\nAutomatically equipped quest reward";
  else if (item==Item::Potion) stats="In bag   "+std::to_string(s.Potions())+"\nCooldown   3 seconds";
  else stats=item==Item::Tunic && s.HasWardArmor()?"Worn beneath ward armor\nNo additional combat bonuses":"Starting outfit\nNo additional combat bonuses";
  c.Text(rect.x,rect.y+166,stats,17,gold);
  const bool usable=CanUse(s,item);
  std::string label=item==Item::Potion?"Drink draught":Equipped(s,item)?"Equipped":"Equip item";
  if (item!=Item::Potion && item!=Item::Starter && item!=Item::Watchguard) label="Part of your outfit";
  button({rect.x,rect.y+250,rect.w,42},label,Action::UseItem,static_cast<int>(item),usable);
  if (item==Item::Potion && !usable) c.Wrap(rect.x,rect.y+306,rect.w,s.Health()<=0?"Return to Dawnwatch before using items.":s.Health()>=s.MaxHealth()?"You are already at full health.":"Potion is cooling down.",15,muted);
 };
 // Persistent HUD leaves the centre of the world available for movement and combat.
 c.Frame({24,24,280,96}); c.Oval(57,67,23,29,{.20f,.32f,.37f,1});
 c.Text(43,51,std::to_string(s.Level()),24,gold);
 c.Text(94,33,"Frontier guard",20); c.Text(94,58,"Dawnwatch expedition",13,muted);
 c.RectFill({94,83,191,13},{.13f,.18f,.17f,1}); c.RectFill({94,83,191*s.Health()/s.MaxHealth(),13},{.29f,.53f,.37f,1});
 c.Center(94,79,191,std::to_string(int(s.Health()))+" / "+std::to_string(int(s.MaxHealth())),13);
 const float xp=s.Level()==3?1.f:float(s.Experience()-s.LevelStartExperience())/(s.NextLevelExperience()-s.LevelStartExperience());
 c.RectFill({94,103,191,3},{.20f,.20f,.27f,1}); c.RectFill({94,103,191*xp,3},{.57f,.49f,.69f,1});
 if (s.Target()>=0) {
  const auto& target=s.Enemies()[s.Target()]; c.Frame({453,24,310,78});
  c.Text(469,33,target.Name(),20); c.Text(469,60,target.windup>0?"Preparing attack - move or interrupt":"Selected target",13,target.windup>0?gold:muted);
  c.RectFill({469,84,278,6},{.20f,.12f,.12f,1}); c.RectFill({469,84,278*target.health/target.MaxHealth(),6},{.64f,.24f,.20f,1});
 }
 c.Frame({1062,24,194,202}); Atlas(c,s,{1070,49,178,163},true,hasWaypoint,waypoint);
 c.Text(1073,26,"Dawnwatch",16,gold); c.Text(1230,26,"N",15);
 regions_.push_back({{1062,24,194,202},{Action::Map},true});
 c.RectFill({990,236,266,181},{.025f,.04f,.05f,.74f});
 c.Text(1001,246,"EXPEDITION",14,gold); c.RectFill({1001,270,255,1},border);
 c.Wrap(1001,280,248,s.Objective(),16,ink,4);
 const auto objective=s.ObjectivePosition();
 if (s.AnchorState()!=rpg::AnchorQuest::Complete)
  c.Text(1001,370,std::to_string(int(std::ceil(std::hypot(s.Player().x-objective.x,s.Player().z-objective.z))))+" m to objective",14,muted);
 if (hasWaypoint) c.Text(1001,396,"Waypoint  "+std::to_string(int(std::ceil(std::hypot(s.Player().x-waypoint.x,s.Player().z-waypoint.z))))+" m",14,{.56f,.76f,.88f,1});
 c.Frame({404,614,414,86});
 for (int i=0;i<5;++i) {
  const float x=414+i*80.f; Rect rect{x,624,66,58};
  c.Frame(rect,{.10f,.14f,.17f,1},rect.Contains(mouseX_,mouseY_)?gold:border);
  if (i==3) {
   c.Oval(x+33,652,19,19,{.16f,.34f,.27f,1});
   c.RectFill({x+29,638,8,28},{.62f,.79f,.55f,1}); c.RectFill({x+19,648,28,8},{.62f,.79f,.55f,1});
  } else {
   Icon(c,{x+11,625,43,45},i<2?Item::Starter:i==4?Item::Potion:Item::Boots);
   if (i==1) { c.Line(x+14,647,x+48,661,3,gold); c.Line(x+16,642,x+19,653,3,gold); }
  }
  const bool ready=i<4?std::string(s.AbilityStatus(i))=="READY":CanUse(s,Item::Potion);
  if (!ready) c.RectFill({rect.x+1,rect.y+1,rect.w-2,rect.h-2},{.02f,.025f,.03f,.52f});
  const float cooldown=i<4?s.Cooldown(i):s.PotionCooldown();
  if (cooldown>0) c.Center(x,636,66,std::to_string(int(std::ceil(cooldown))),22);
  c.Text(x+4,625,i<4?std::to_string(i+1):"Q",13,gold);
  static const char* names[]={"Strike","Heavy","Dash","Mend","Potion"}; c.Center(x,682,66,names[i],12,muted);
  regions_.push_back({rect,{i<4?Action::Ability:Action::UseItem,i<4?i:static_cast<int>(Item::Potion)},ready});
 }
 button({974,658,52,38},"C",Action::Character); button({1031,658,52,38},"I",Action::Inventory);
 button({1088,658,52,38},"M",Action::Map); button({1145,658,52,38},"J",Action::Journal);
 button({1202,658,54,38},"Esc",Action::Menu);
 c.Text(26,664,std::to_string(s.Gold())+" gold",18,gold);
 c.Text(26,690,"WASD move   RMB orbit   F11 display",12,muted);
 c.Wrap(26,553,840,s.Message(),15,ink,2);
 if (panel==Panel::None && s.Health()>0) {
  const auto prompt=s.InteractionPrompt();
  if (!prompt.empty()) { const float w=std::min(680.f,c.Width(prompt,18)+34); c.Frame({640-w/2,577,w,30}); c.Center(640-w/2,579,w,prompt,18,gold); }
  if (s.Channeling()) { c.RectFill({470,607,340,4},border); c.RectFill({470,607,340*s.ChannelProgress(),4},blue); }
 }
 if (panel!=Panel::None) {
  c.RectFill({0,0,1280,720},{0,0,0,.38f});
  // Only the active panel receives pointer events; HUD buttons cannot click through it.
  regions_.clear();
  if (panel==Panel::Menu) {
   c.Frame({432,82,416,560}); c.Center(432,108,416,"MEDITERRANEAN",30,gold);
   c.Center(432,149,416,"Dawnwatch expedition",17,muted);
   button({474,197,332,44},"Return to game",Action::Close);
   button({474,251,161,42},"Character",Action::Character); button({645,251,161,42},"Inventory",Action::Inventory);
   button({474,303,161,42},"Atlas",Action::Map); button({645,303,161,42},"Journal",Action::Journal);
   button({474,355,332,42},fullscreen?"Display: Fullscreen  /  F11":"Display: Windowed  /  F11",Action::Fullscreen);
   button({474,407,161,42},"Save game",Action::Save); button({645,407,161,42},"Load game",Action::Load);
   button({474,469,332,42},"Exit to desktop",Action::Quit);
   c.Center(432,545,416,"The world continues while this menu is open.",15,gold);
   c.Center(432,571,416,"Esc returns to the game. Progress saves with F5.",14,muted);
   if (!menuNotice.empty()) c.Center(432,605,416,menuNotice,14,gold);
   return;
  }
  const bool map=panel==Panel::Map;
  const Rect frame=map?Rect{130,44,1020,604}:Rect{180,70,920,572};
  c.Frame(frame);
  const char* title=panel==Panel::Inventory?"EXPEDITION BAG":panel==Panel::Character?"FRONTIER GUARD":panel==Panel::Map?"ATLAS OF DAWNWATCH":"EXPEDITION JOURNAL";
  c.Text(frame.x+26,frame.y+18,title,25,gold); button({frame.x+frame.w-53,frame.y+16,32,32},"X",Action::Close);
  c.RectFill({frame.x+22,frame.y+61,frame.w-44,1},border);
  if (panel==Panel::Inventory) {
   c.Text(209,148,"Belongings",19); c.Text(209,177,"Select an item to inspect it.",14,muted);
   std::vector<Item> items;
   for (Item item : {Item::Starter,Item::Watchguard,Item::Potion,Item::Tunic,Item::Trousers,Item::Boots,Item::Ward}) if (Owned(s,item)) items.push_back(item);
   if (!Owned(s,selected)) selected=Item::Starter;
   for (int i=0;i<24;++i) {
    Rect rect{209+float(i%6)*65,210+float(i/6)*70,57,60};
    if (i<int(items.size())) slot(rect,items[i],true,selected==items[i]); else c.Frame(rect,{.055f,.075f,.09f,1});
   }
   c.Text(209,505,std::to_string(items.size())+" / 24 slots",15,muted); c.Text(209,539,std::to_string(s.Gold())+" gold",22,gold);
   c.RectFill({622,153,1,412},border); itemDetails({654,164,410,350},selected);
   c.Text(209,600,"Equipped gear has a green edge. Consumables show the stack count.",15,muted);
  } else if (panel==Panel::Character) {
   c.Text(210,148,"Level "+std::to_string(s.Level())+"  /  Dawnwatch guard",18);
   c.Frame({292,186,282,370},{.065f,.095f,.12f,1});
   // Render the same dressed model used in the world, with the actual equipped weapon/armor.
   const auto p=c.P(306,193);
   if (renderer.BeginPortrait(p.x,p.y,254*c.layout.scale/renderer.Width(),352*c.layout.scale/renderer.Height())) {
    DrawCharacter(renderer,{0,0},0,std::sin(time*.35f)*.12f,{.22f,.37f,.54f,1},0,0,s.HasWardArmor(),s.RewardWeaponEquipped());
    renderer.EndPortrait();
   }
   slot({211,200,64,68},s.HasWardArmor()?Item::Ward:Item::Tunic,true);
   c.Text(217,272,"Chest",13,muted);
   slot({211,320,64,68},Item::Trousers,true); c.Text(220,392,"Legs",13,muted);
   slot({211,440,64,68},Item::Boots,true); c.Text(219,512,"Feet",13,muted);
   slot({591,200,64,68},s.RewardWeaponEquipped()?Item::Watchguard:Item::Starter,true); c.Text(590,272,"Main hand",13,muted);
   c.Frame({591,320,64,68},{.05f,.07f,.08f,1}); c.Center(591,338,64,"--",22,muted); c.Text(592,392,"Off hand",13,muted);
   c.Frame({591,440,64,68},{.05f,.07f,.08f,1}); c.Center(591,458,64,"--",22,muted); c.Text(607,512,"Head",13,muted);
   c.RectFill({685,157,1,405},border);
   c.Text(714,160,"ATTRIBUTES",15,gold);
   const std::pair<std::string,std::string> stats[]={
    {"Health",std::to_string(int(s.Health()))+" / "+std::to_string(int(s.MaxHealth()))},
    {"Experience",std::to_string(s.Experience())},
    {"Strike damage",std::to_string(int(s.StrikeDamage()))},
    {"Heavy damage",std::to_string(int(s.HeavyDamage()))},
    {"Damage reduction",std::to_string(int(s.DamageReduction()))}};
   for (int i=0;i<5;++i) { c.Text(714,202+i*44.f,stats[i].first,17,muted); c.Text(957,202+i*44.f,stats[i].second,17); }
   c.Wrap(714,448,326,"Equipment slots show your current outfit. Click a filled slot to inspect its item in the bag.",16,muted,4);
   button({714,552,325,42},"Open inventory",Action::Inventory);
   c.Text(211,600,"Ward armor and the watchguard blade appear on the character when equipped.",15,muted);
  } else if (panel==Panel::Map) {
   const Rect mapRect{155,126,645,480}; c.Frame({151,122,653,488}); Atlas(c,s,mapRect,false,hasWaypoint,waypoint);
   regions_.push_back({mapRect,{Action::PlaceWaypoint},true});
   c.Text(833,134,"DAWNWATCH ISLAND",19,gold); c.Text(833,166,"North-up local survey",15,muted);
   c.Wrap(833,207,286,s.Objective(),18,ink,4);
   const std::pair<Color,const char*> legend[]={{{.68f,.87f,1,1},"Your position and facing"},{gold,"Keeper / quest objective"},{{.72f,.20f,.15f,1},"Living raider"},{{.59f,.38f,.84f,1},"Astral anchor"}};
   for (int i=0;i<4;++i) { c.Oval(842,333+i*33.f,4,4,legend[i].first); c.Text(858,322+i*33.f,legend[i].second,16,muted); }
   c.Wrap(833,478,286,"Click traversable ground to place a waypoint. Right-click to clear it.",16,ink,3);
   if (hasWaypoint) button({833,562,286,39},"Clear waypoint",Action::ClearWaypoint);
   c.Text(156,619,"Roads, buildings and markers follow the playable map. Scale: island 34 x 38 m.",14,muted);
  } else {
   c.Frame({208,158,397,339}); c.Text(229,177,"01   Patrol the frontier",23);
   c.Text(229,216,s.QuestState()==rpg::Quest::Complete?"COMPLETED":"FRONTIER PATROL",14,gold);
   c.Wrap(229,257,348,"Speak to the gold beacon keeper. Defeat one raider north of the village, then return to claim your reward.",18,ink,5);
   c.Text(229,390,"REWARD",14,gold); c.Text(229,419,"25 gold  /  60 XP\nWatchguard blade",18);
   c.Frame({622,158,450,339}); c.Text(643,177,"02   Secure the astral anchor",23);
   c.Text(643,216,s.AnchorState()==rpg::AnchorQuest::Complete?"COMPLETED":s.AnchorState()==rpg::AnchorQuest::Locked?"UNLOCKS AFTER THE PATROL":"ASTRAL ANCHOR",14,gold);
   c.Wrap(643,257,402,"Clear all three raiders, stabilize the northern crystal for three seconds, then return to the keeper.",18,ink,5);
   c.Text(643,390,"REWARD",14,gold); c.Text(643,419,"50 gold  /  120 XP\nAstral ward armor",18);
   button({734,545,338,43},"Show objective on the atlas",Action::Map);
   c.Text(209,557,"Raiders defeated: "+std::to_string(s.DefeatedCount())+" / 3",18,muted);
   c.Text(209,606,"The world remains active while reading the journal.",15,muted);
  }
  // Item hover information is drawn last and kept inside the design canvas.
  for (const auto& region : regions_) if (region.event.action==Action::SelectItem && region.rect.Contains(mouseX_,mouseY_)) {
   const Item item=static_cast<Item>(region.event.value); const auto& info=Describe(item);
   const float x=std::clamp(mouseX_+17,8.f,954.f),y=std::clamp(mouseY_+20,8.f,596.f);
   c.Frame({x,y,318,112},{.025f,.035f,.044f,.99f},gold); c.Text(x+13,y+9,info.name,20);
   c.Text(x+13,y+39,info.type,14,muted); c.Text(x+13,y+70,Equipped(s,item)?"Equipped  /  Click to inspect":"Click to inspect",15,gold);
   break;
  }
 }
 if (panel==Panel::None) {
  for (const auto& region : regions_) if (region.rect.Contains(mouseX_,mouseY_) &&
    (region.event.action==Action::Ability || region.event.action==Action::UseItem)) {
   const int ability=region.event.value;
   const bool potion=region.event.action==Action::UseItem;
   static const char* names[]={"Strike","Heavy strike","Dash","Mend"};
   const std::string name=potion?"Healing draught":names[ability];
   const std::string detail=potion?"Restores up to 40 health.":ability==0?std::to_string(int(s.StrikeDamage()))+" melee damage.":
     ability==1?std::to_string(int(s.HeavyDamage()))+" damage. Interrupts a windup.":ability==2?"Dash in your facing direction.":"Restores up to 35 health.";
   const std::string status=potion?(s.Health()<=0?"DEFEATED":s.Potions()==0?"NO POTIONS":s.Health()>=s.MaxHealth()?"FULL HEALTH":s.PotionCooldown()>0?"COOLDOWN":"READY"):s.AbilityStatus(ability);
   const float x=std::clamp(region.rect.x-70,10.f,970.f);
   c.Frame({x,483,300,116},{.025f,.035f,.044f,.99f},gold);
   c.Text(x+14,492,name,21); c.Wrap(x+14,526,272,detail,16,muted,2); c.Text(x+14,571,status,13,gold);
   break;
  }
 }
 if (s.Health()<=0 && panel==Panel::None) {
  c.Frame({380,256,520,174}); c.Center(380,282,520,"Return to Dawnwatch",27,gold);
  c.Center(380,328,520,"Press Enter to recover with your progress intact.",18);
  c.Center(380,364,520,"Quests, equipment and collected loot are retained.",15,muted);
 }
 if (debug) {
  c.Frame({24,134,360,61}); c.Text(35,143,"Draws "+std::to_string(renderer.LastDrawCalls())+"   GPU "+std::to_string(renderer.GpuMilliseconds()).substr(0,5)+" ms",16);
  c.Text(35,166,"Meshes "+std::to_string(renderer.MeshBytes()/1024)+" KiB   F2 HUD   F3 metrics",13,muted);
 }
}
}


