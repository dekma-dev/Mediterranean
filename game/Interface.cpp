#include "game/Interface.h"
#include "game/CharacterView.h"
#include "game/CharacterSelection.h"
#include "game/MapArtwork.h"
#include "game/UiSkin.h"
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
const Color ink{.97f,.93f,.80f,1},muted{.78f,.74f,.65f,1},gold{.86f,.67f,.30f,1};
const Color dark{.055f,.044f,.032f,.99f},border{.39f,.34f,.24f,1},blue{.23f,.46f,.63f,1};
std::string Seconds(float value) {
 std::ostringstream text;
 if (value<1) text<<std::fixed<<std::setprecision(1)<<std::ceil(std::max(0.f,value)*10)/10;
 else text<<int(std::ceil(value));
 return text.str();
}
std::string Number(float value) {
 std::ostringstream text; text<<std::setprecision(4)<<value; return text.str();
}
std::string SpellEffect(const rpg::Simulation& s,int ability) {
 if (ability==0) return Number(s.StrikeDamage())+" base damage";
 if (ability==1) return Number(s.HeavyDamage())+" base damage"+
   (s.HasTalent(rpg::Talent::Executioner)?" / "+Number(s.HeavyDamage()*1.25f)+" at half target HP":"");
 if (ability==2) return s.RetreatMobility()?Number(s.CombatStyleInfo().retreatDistance)+" m backward escape / no target required":"5 m acquisition / 1.5 m stopping distance";
 return "Up to "+Number(s.MendHealingNow())+" healing"+
   (s.HasTalent(rpg::Talent::Mercy)?" / "+Number(s.MendHealing()+20)+" at low HP":"")+
   (rpg::ClassRecoveryBarrier(s.Class())>0?" / "+Number(rpg::ClassRecoveryBarrier(s.Class()))+" ward":"");
}
std::string TalentEffect(const rpg::Simulation& s,int index) {
 const auto info=s.TalentDefinition(index);
 if(info.strike || info.heavy) return "+"+Number(info.strike)+" basic / +"+Number(info.heavy)+" heavy damage";
 if(info.health) return "+"+Number(info.health)+" maximum health";
 if(info.armor) return "+"+Number(info.armor)+" damage reduction";
 if(info.mend) return "+"+Number(info.mend)+" self healing";
 if(info.mendCooldown) return "Healing cooldown: -"+Number(info.mendCooldown)+"s";
 if(info.dashCooldown) return "Dash cooldown: -"+Number(info.dashCooldown)+"s";
 return index==6?"Heavy: +25% finishing damage":"Healing: +20 at low health";
}
Color IconUv(int tile) {
 // Authored atlas cell edges, inset three source pixels to exclude separators.
 // The last painted row is taller; explicit UVs keep every object intact.
 constexpr float xx[]={0,252,500,750,1005,1254};
 constexpr float yy[]={0,249,485,730,962,1254};
 const int col=tile%5,row=tile/5;
 return {(xx[col]+3)/1254,(yy[row]+3)/1254,(xx[col+1]-3)/1254,(yy[row+1]-3)/1254};
}
struct Canvas {
 Renderer& r; Layout layout;
 explicit Canvas(Renderer& renderer):r(renderer),layout(float(r.Width()),float(r.Height())) {}
 Point P(float x,float y) const { return {(layout.x+x*layout.scale)/r.Width(),(layout.y+y*layout.scale)/r.Height()}; }
 void RectFill(Rect a,Color c) { const auto p=P(a.x,a.y); r.DrawHudRect(p.x,p.y,a.w*layout.scale/r.Width(),a.h*layout.scale/r.Height(),c); }
 void Gradient(Rect a,Color top,Color bottom) {
  const auto p=P(a.x,a.y);
  r.DrawHudGradient(p.x,p.y,a.w*layout.scale/r.Width(),a.h*layout.scale/r.Height(),top,bottom);
 }
 void Art(Rect a,Renderer::UiArt art=Renderer::UiArt::Frame,Color uv={0,0,1,1},Color tint={1,1,1,1}) {
  const auto p=P(a.x,a.y);r.DrawUiArt(art,p.x,p.y,a.w*layout.scale/r.Width(),a.h*layout.scale/r.Height(),uv,tint);
 }
 void Skin(Rect a,float corner=24,Color tint={1,1,1,1},bool center=true) {
  skin::Panel(a.x,a.y,a.w,a.h,corner,[&](float x,float y,float w,float h,float u,float v,float uu,float vv) {
   Art({x,y,w,h},Renderer::UiArt::Frame,{u,v,uu,vv},tint);
  },center);
 }
 void Frame(Rect a,Color fill=dark,Color line=border) {
  RectFill({a.x+2,a.y+3,a.w,a.h},{0,0,0,.5f});
  const float corner=std::min(18.f,a.h*.28f);Skin(a,corner);
  RectFill({a.x+5,a.y+5,a.w-10,a.h-10},{fill.x,fill.y,fill.z,.28f});
  if (line.y>.5f) {
   Line(a.x+5,a.y+2,a.x+a.w-5,a.y+2,1,line);
   Line(a.x+5,a.y+a.h-3,a.x+a.w-5,a.y+a.h-3,1,{line.x,line.y,line.z,.6f});
  }
 }
 void Button(Rect a,bool enabled=true,bool highlight=false,bool primary=false) {
  Skin(a,std::min(16.f,a.h*.32f),enabled?Color{1,1,1,1}:Color{.48f,.48f,.48f,1});
  const float edge=std::min(6.f,a.h*.14f);
  const Color tint=primary?Color{.46f,.025f,.035f,highlight?.78f:.65f}:Color{.19f,.22f,.19f,highlight?.62f:.24f};
  RectFill({a.x+edge,a.y+edge,a.w-2*edge,a.h-2*edge},enabled?tint:Color{0,0,0,.3f});
  if(highlight && enabled) {
   Line(a.x+7,a.y+4,a.x+a.w-7,a.y+4,1,{1,.82f,.45f,1});
   Line(a.x+7,a.y+a.h-5,a.x+a.w-7,a.y+a.h-5,1,{1,.82f,.45f,.6f});
  }
 }
 void Text(float x,float y,const std::string& s,float size=18,Color c=ink) {
  if (c.x+c.y+c.z>1) {
   const auto shadow=P(x+1,y+1); r.DrawText(shadow.x,shadow.y,s,{.01f,.015f,.008f,c.w*.85f},size*layout.scale/12);
  }
  const auto p=P(x,y); r.DrawText(p.x,p.y,s,c,size*layout.scale/12);
 }
 float Width(const std::string& s,float size=18) const { return r.TextWidth(s,size*layout.scale/12)/layout.scale; }
 void FitText(float x,float y,float width,const std::string& s,float size=18,Color c=ink) {
  size*=std::min(1.f,width/std::max(1.f,Width(s,size)));Text(x,y,s,size,c);
 }
 void Center(float x,float y,float width,const std::string& s,float size=18,Color c=ink) {
  size*=std::min(1.f,width/std::max(1.f,Width(s,size)));Text(x+(width-Width(s,size))*.5f,y,s,size,c);
 }
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
 std::vector<std::string> Lines(float width,const std::string& text,float size) const {
  std::vector<std::string> result;
  std::istringstream paragraphs(text); std::string paragraph;
  while (std::getline(paragraphs,paragraph)) {
   std::istringstream words(paragraph); std::string word,line;
   while (words>>word) {
    const auto candidate=line.empty()?word:line+" "+word;
    if (Width(candidate,size)>width && !line.empty()) { result.push_back(line); line.clear(); }
    // Keep unusually long authored words inside the dialogue frame as well.
    if (Width(word,size)>width) {
     for (char ch : word) {
      if (!line.empty() && Width(line+ch,size)>width) { result.push_back(line); line.clear(); }
      line+=ch;
     }
    } else line+=line.empty()?word:" "+word;
   }
   result.push_back(line);
  }
  return result;
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
  RectFill({a.x+4,a.y+6,a.w,a.h},{0,0,0,.55f});
  Skin(a,28);
  RectFill({a.x+11,a.y+11,a.w-22,a.h-22},{.006f,.005f,.004f,.24f});
  // Restrained inset stitching and metal corner studs leave the content clear.
  for(float side:{0.f,1.f}) {
   const float x=a.x+8+side*(a.w-16);
   for(float y=a.y+34;y<a.y+a.h-28;y+=24)
    Line(x,y,x,y+4,1,{.52f,.40f,.23f,.65f});
   for(float end:{0.f,1.f}) {
    const float y=a.y+9+end*(a.h-18);
    RectFill({x-2,y-2,4,4},{.67f,.51f,.27f,1});
    RectFill({x-1,y-1,2,1},{.91f,.75f,.44f,1});
   }
  }
 }
 void Banner(Rect a,const std::string& label,float size=18) {
  Skin(a,std::min(20.f,a.h*.48f));
  RectFill({a.x+12,a.y+5,a.w-24,a.h-10},{.25f,.10f,.035f,.45f});
  Center(a.x+14,a.y+(a.h-size)*.5f-2,a.w-28,label,size,{1,.88f,.61f,1});
 }
 void EmptySlot(Rect a) {
  Skin(a,10,{.46f,.44f,.38f,1});
  RectFill({a.x+4,a.y+4,a.w-8,a.h-8},{.005f,.008f,.006f,.38f});
 }
 void Alcove(Rect a) {
  Skin(a,18);
  Art({a.x+6,a.y+6,a.w-12,a.h-12},Renderer::UiArt::Hall,{.32f,0,.67f,1},{.66f,.69f,.72f,1});
  Oval(a.x+a.w*.5f,a.y+a.h-28,a.w*.28f,9,{.005f,.008f,.01f,.6f});
 }
 void Parchment(Rect a) {
  Frame(a,{.66f,.52f,.32f,1},{.30f,.21f,.11f,1});
  Gradient({a.x+4,a.y+4,a.w-8,a.h-8},{.83f,.74f,.54f,1},{.68f,.55f,.35f,1});
  for (int i=0;i<90;++i) {
   const float x=a.x+7+std::fmod(i*71.71f,a.w-14),y=a.y+7+std::fmod(i*43.31f,a.h-14);
   Line(x,y,std::min(a.x+a.w-7,x+3+i%11),y,.6f,{.30f,.20f,.10f,.13f});
  }
  for (int i=0;i<6;++i) {
   RectFill({a.x+4+i,a.y+4,1,a.h-8},{.24f,.15f,.065f,.12f});
   RectFill({a.x+a.w-5-i,a.y+4,1,a.h-8},{.24f,.15f,.065f,.10f});
  }
 }
 // Painted armor crest with a live level/threat medallion.
 void Portrait(float x,float y,bool enemy,int level) {
  Skin({x-29,y-29,58,59},17);
  Art({x-23,y-23,46,45},Renderer::UiArt::Icons,IconUv(16),enemy?Color{.88f,.59f,.48f,1}:Color{1,1,1,1});
  Oval(x,y+25,12,10,{.07f,.055f,.035f,1}); Ring(x,y+25,10,gold);
  Center(x-11,y+17,22,enemy?"!":std::to_string(level),12);
 }
 void CooldownSweep(Rect a,float fraction) {
  const float end=std::clamp(fraction,0.f,1.f)*6.2831853f;
  const Point center{a.x+a.w*.5f,a.y+a.h*.5f};
  const auto edge=[&](float angle) {
   float x=std::sin(angle),y=-std::cos(angle),m=std::max(std::abs(x),std::abs(y));
   return Point{center.x+x/m*a.w*.5f,center.y+y/m*a.h*.5f};
  };
  for (float from=0;from<end;from+=.09817477f)
   Triangle(center,edge(from),edge(std::min(end,from+.09817477f)),{.006f,.012f,.025f,.70f});
 }
};
std::string ItemTypeLabel(const rpg::ItemInfo& info) {
 return info.slot==rpg::GearSlot::Weapon?std::string(info.type)+(info.id==Item::Watchguard?" / Uncommon":" / Common"):info.type;
}
void PaintedIcon(Canvas& c,Rect rect,int tile,float alpha=1) {
 if(tile<0 || tile>=25) return;
 c.Art(rect,Renderer::UiArt::Icons,IconUv(tile),{1,1,1,alpha});
}
void Icon(Canvas& c,Rect rect,Item item,float alpha=1,rpg::ClassId id=rpg::ClassId::Warrior) {
 if(item==Item::None) return;
 int tile=-1;
 if(item==Item::Starter || item==Item::Watchguard) tile=int(rpg::WeaponFor(id).form);
 else if(item>=Item::Potion && item<=Item::Cloak) tile=11+int(item)-int(Item::Potion);
 PaintedIcon(c,rect,tile,alpha);
}
void ClassSigil(Canvas& c,Rect rect,rpg::ClassId id,float alpha=1) {
 PaintedIcon(c,rect,int(rpg::WeaponFor(id).form),alpha);
}
void AbilityArtwork(Canvas& c,Rect rect,int ability,rpg::ClassId id=rpg::ClassId::Warrior) {
 if(ability==2) PaintedIcon(c,rect,22);
 else if(ability==3) PaintedIcon(c,rect,23);
 else if(ability>=4) PaintedIcon(c,rect,11);
 else if(ability==1 && (id==rpg::ClassId::Warrior || id==rpg::ClassId::Paladin || id==rpg::ClassId::Demonologist)) PaintedIcon(c,rect,24);
 else {
  PaintedIcon(c,rect,int(rpg::WeaponFor(id).form));
  if(ability==1) {
   c.Line(rect.x+3,rect.y+4,rect.x+rect.w-3,rect.y+4,2,gold);
   c.Line(rect.x+3,rect.y+8,rect.x+rect.w-3,rect.y+8,1,gold);
  }
 }
}
void TalentIcon(Canvas& c,Rect rect,int index,float alpha=1,rpg::ClassId id=rpg::ClassId::Warrior) {
 if(id!=rpg::ClassId::Warrior && index%3==0) {ClassSigil(c,rect,id,alpha);return;}
 if (index==int(rpg::Talent::Pursuit)) {
  AbilityArtwork(c,rect,2); return;
 }
 if (index%3!=2) {
  Icon(c,rect,index%3==0?(index<3?Item::Starter:Item::Watchguard):Item::Shield,alpha);
  return;
 }
 PaintedIcon(c,rect,23,alpha);
 if(index>=3) c.Line(rect.x+4,rect.y+rect.h-3,rect.x+rect.w-4,rect.y+rect.h-3,2,{.88f,.76f,.45f,alpha});
}
Point MapPoint(Rect a,rpg::Vec2 p) { const float depth=rpg::WorldMap::surveyDepth,span=depth*a.w/a.h;
 return {a.x+(p.x+span*.5f)/span*a.w,a.y+(.5f-p.z/depth)*a.h}; }
void Atlas(Canvas& c,const rpg::Simulation& sim,Rect a,bool compact,bool waypoint,rpg::Vec2 pin) {
 if (compact) {
  const auto p=c.P(a.x,a.y);
  c.r.DrawRoundMapArtwork(p.x,p.y,a.w*c.layout.scale/c.r.Width(),a.h*c.layout.scale/c.r.Height());
 } else {
 c.RectFill(a,{.88f,.80f,.63f,1});
 const float width=a.h*rpg::WorldMap::surveyWidth/rpg::WorldMap::surveyDepth;
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
  const std::pair<const char*,float> labels[]={{"SUNDERED BASTION",.06f},{"DAWNWATCH VILLAGE",.74f}};
  for (const auto& label : labels) {
   const float width=c.Width(label.first,16),x=a.x+(a.w-width)*.5f,y=a.y+a.h*label.second;
   c.RectFill({x-7,y-2,width+14,23},{.90f,.83f,.66f,.92f});
   lettering(x,y,label.first,16);
   c.Line(x,y+20,x+width,y+20,.65f,{.40f,.32f,.20f,.45f});
  }
  const float x=a.x+20,y=a.y+a.h-34,span=5*a.h/rpg::WorldMap::surveyDepth;
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
 for(const auto& npc:rpg::WorldMap::residents) {
  marker(npc.position,{.85f,.67f,.27f,1},compact?3.f:5.f);
  if(!compact) {const auto p=MapPoint(a,npc.position);lettering(p.x-36,p.y+9,npc.name,11);}
 }
 if(sim.SurveyActive()) for(unsigned i=0;i<rpg::WorldMap::surveySites.size();++i)
  marker(rpg::WorldMap::surveySites[i],sim.SurveyMask()&(1u<<i)?Color{.4f,.58f,.34f,1}:Color{.35f,.69f,.88f,1},compact?2.f:4.f);
 if (sim.AnchorState()!=rpg::AnchorQuest::Complete || sim.FrontierCompleted()<rpg::frontierQuestCount) {
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
   const float bearing=i*3.14159265f/4,radius=i%2?15.f:29.f;
   const Point tip{x+std::sin(bearing)*radius,y-std::cos(bearing)*radius};
   const Point left{x+std::sin(bearing-.35f)*6,y-std::cos(bearing-.35f)*6};
   const Point right{x+std::sin(bearing+.35f)*6,y-std::cos(bearing+.35f)*6};
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

void RenderLoading(Renderer& renderer,LoadingStage stage) {
 Canvas c(renderer);
 renderer.DrawHudRect(0,0,1,1,{.018f,.014f,.01f,1});
 c.Art({0,0,1280,720},Renderer::UiArt::Hall);
 c.Gradient({0,0,1280,300},{.01f,.008f,.006f,.92f},{.01f,.008f,.006f,0});
 c.Gradient({0,390,1280,330},{.01f,.008f,.006f,0},{.01f,.008f,.006f,.96f});
 c.Center(0,58,1280,"MEDITERRANEAN",40,{1,.85f,.54f,1});
 c.Center(0,115,1280,"T H E   D A W N W A T C H   C H R O N I C L E",13,ink);
 c.Window({270,466,740,186});
 c.Banner({390,449,500,38},"Beyond the weathered gates",22);
 const char* titles[]={"Preparing the island","Reading your chronicles","Tuning the instruments","Entering Dawnwatch"};
 const char* details[]={"Laying roads, raising the watch and preparing the journey.",
  "Gathering your heroes and their saved journeys.","Preparing the music and sounds of the frontier.",
  "Preparing the landscape and your first view of the world."};
 const unsigned index=std::min(unsigned(stage),3u);
 c.Center(290,503,700,titles[index],21,ink);
 c.Center(290,540,700,details[index],14,muted);
 for(unsigned i=0;i<4;++i) {
  const float x=412+115.f*i;
  if(i<3) c.Line(x+15,591,x+100,591,1,border);
  c.Frame({x,584,15,15},i<=index?Color{.48f,.31f,.10f,1}:dark,i==index?gold:border);
  if(i<=index) c.RectFill({x+5,589,5,5},i==index?ink:gold);
 }
 c.Center(290,620,700,"WASD  Travel     M  Atlas     J  Journal     Esc  Menu",12,gold);
 c.Center(0,681,1280,"Your chronicle awaits.",13,muted);
}

void RenderCharacterSelection(Renderer& renderer,const CharacterSelection& selection) {
 Canvas c(renderer);using C=CharacterSelection::Control;using Page=CharacterSelection::Page;using Status=CharacterSelection::Status;
 const auto page=selection.Screen();const bool roster=page==Page::Roster,customize=page==Page::Customize;
 const bool valid=!roster || selection.State()!=Status::Unreadable,saved=selection.State()==Status::Saved;
 const auto& hero=selection.Hero();const auto& info=hero.ClassInfo();
 renderer.DrawHudRect(0,0,1,1,{.017f,.016f,.014f,1});
 c.Art({0,0,1280,720},Renderer::UiArt::Hall);
 c.Gradient({0,0,1280,132},{.015f,.012f,.01f,.9f},{.015f,.012f,.01f,0});
 c.Gradient({0,628,1280,92},{.015f,.012f,.01f,0},{.015f,.012f,.01f,.94f});
 c.Text(66,35,"MEDITERRANEAN",34,{1,.85f,.54f,1});
 c.Text(68,80,"T H E   D A W N W A T C H   C H R O N I C L E",11,ink);
 c.Text(842,46,roster?"YOUR CHARACTERS":customize?"GIVE YOUR HERO AN IDENTITY":"CHOOSE YOUR CALLING",customize?17.f:20.f,ink);
 c.Text(842,79,roster?"Dawnwatch  /  Local campaign":customize?"CREATE  /  II. Appearance & name":"CREATE  /  I. Class & talents",13,gold);
 c.Window({64,144,222,480});c.Banner({74,133,202,36},roster?"The company":"The eleven callings",16);
 c.Window({814,140,400,510});c.Banner({852,129,324,36},roster?"Hero's chronicle":customize?"A name for the ages":"Choose a path",19);
 if(valid) {
  const auto p=c.P(326,112);
  if(renderer.BeginPortrait(p.x,p.y,464*c.layout.scale/renderer.Width(),490*c.layout.scale/renderer.Height(),selection.PreviewZoom())) {
   auto pose=animation::Idle(selection.Time());
   if(selection.PreviewAction()>=0) {
    pose.action=animation::SampleAction(hero.Class(),animation::ActionClip::Heavy,selection.PreviewAction());
    if(!animation::RangedClass(hero.Class())) {pose.attackSeconds=selection.PreviewAction();pose.heavyAttack=true;}
   }
   DrawCharacter(renderer,{0,0},0,selection.Yaw(),{info.color[0],info.color[1],info.color[2],1},0,0,
     CharacterOutfit::From(hero.Items(),hero.Class(),hero.Look()),pose);
   renderer.EndPortrait();
  }
 } else c.Center(326,353,464,"Character unavailable",22,muted);
 c.Center(326,565,464,roster && !saved?"An unclaimed calling":hero.Nickname(),22,ink);
 c.Center(438,613,144,"Rotate character",13,muted);
 const auto button=[&](C control,const char* label) {
  const auto a=CharacterSelection::Bounds(control);
  const bool enabled=selection.Enabled(control),focused=selection.Focus()==control,hover=selection.Hover()==control;
  const bool primary=control==C::Enter || control==C::Next || control==C::Confirm || control==C::Create;
  c.Button(a,enabled,focused || hover,primary);
  c.Center(a.x,a.y+(a.h-18)*.5f-1,a.w,label,primary?19.f:15.f,enabled?ink:muted);
  if(focused && enabled) c.Line(a.x+8,a.y+a.h-7,a.x+a.w-8,a.y+a.h-7,1,gold);
 };
 button(C::Left,"<");button(C::Right,">");
 button(C::Closeup,selection.Closeup()?"Full figure":"Face close-up");
 button(C::Pose,"Class action");button(C::ResetView,"Reset view");
 if(!roster) {
  for(int step=0;step<2;++step) {
   const Rect tab{330+232.f*step,104,224,30};
   c.Frame(tab,dark,(customize?1:0)==step?gold:border);
   c.Center(tab.x,tab.y+7,tab.w,step?"II  /  Identity":"I  /  Calling",13,(customize?1:0)==step?ink:muted);
  }
 }
 if(roster) {
  const auto card=CharacterSelection::Bounds(C::Hero);c.Frame(card,dark,valid?gold:border);
  if(valid) {
   ClassSigil(c,{857,207,52,52},hero.Class());
   const std::string name=saved?hero.Nickname():info.name;
   const float nameSize=std::min(22.f,22.f*248.f/std::max(1.f,c.Width(name,22)));
   c.Text(925,202,name,nameSize,ink);
   c.Text(925,240,saved?"Level "+std::to_string(hero.Level()):"Empty class slot",17,gold);
   c.Text(925,270,saved?hero.ClassName():"Create a hero to begin",14,muted);
   c.Text(842,334,saved?"SAVED JOURNEY":"A NEW CHRONICLE",12,gold);
   c.Wrap(842,361,350,saved?"Continue this hero's expedition, equipment, talents and quest progress.":
     "Create a character: choose a class, customize your appearance and give your hero a name.",16,ink,3);
   c.Wrap(842,430,350,selection.Changed()?"Save changed. Review the hero before entering.":info.role,13,gold,2);
  } else {
   c.Text(861,211,"Unable to read this character",19,ink);c.Text(861,252,"The saved file has been kept.",15,muted);
   c.Wrap(842,348,350,"Restore this class's save in the saves folder, then select Refresh. You can create a hero in another empty class slot.",16,ink,5);
  }
  button(C::Create,"Create character");button(C::Enter,"Enter World");button(C::Refresh,"Refresh [R]");button(C::Quit,"Exit game");
 } else {
  ClassSigil(c,{842,181,46,46},hero.Class());c.Text(900,182,info.name,22,ink);c.Text(900,213,info.role,12,gold);
  if(!customize) {
   c.Wrap(842,240,350,std::string(hero.CombatStyleInfo().resource)+" / "+hero.CombatStyleInfo().payoff,14,ink,3);
   c.Text(842,292,"STARTING ARTS",11,gold);
   for(int i=0;i<4;++i) {
    const Rect r{842+180.f*(i%2),313+54.f*(i/2),170,47};
    c.Frame(r,dark,border);AbilityArtwork(c,{r.x+5,r.y+6,34,34},i,hero.Class());
    c.Wrap(r.x+46,r.y+8,118,hero.Spell(i).name,13,ink,2);
   }
   c.Text(842,429,"TALENT PATHS  /  N IN GAME",11,gold);
   c.Wrap(842,451,350,std::string(info.branches[0])+"  /  "+info.branches[1]+"  /  "+info.branches[2],14,ink,2);
   c.Text(842,495,"Health "+std::to_string(int(hero.MaxHealth()))+"  /  Range "+Number(info.range)+" m",13,muted);
   button(C::Next,selection.ClassSlot(int(selection.SelectedClass())).status==Status::New?"Appearance & name":"Class slot occupied");
  } else {
   c.Text(842,241,"HERO'S NAME",11,gold);const auto a=CharacterSelection::Bounds(C::Name);
   c.Frame(a,dark,selection.Focus()==C::Name?gold:border);
   std::string name=selection.DraftName().empty()?"Enter a name":selection.DraftName();
   if(selection.Focus()==C::Name && std::fmod(selection.Time(),1.f)<.55f) name+="|";
   const float nameSize=std::min(17.f,17.f*(a.w-20)/std::max(1.f,c.Width(name,17)));
   c.Text(a.x+10,a.y+10,name,nameSize,selection.DraftName().empty()?muted:ink);
   c.Text(842,308,"1-20 letters; spaces, apostrophe, hyphen. Ctrl+A clears.",10,muted);
   const auto& look=hero.Look();
   const auto choice=[&](float y,const char* title,const auto& palette,int selected,C first,C left,C right) {
    c.Text(842,y-18,title,11,gold);c.Text(1010,y-18,palette[selected].name,11,ink);
    for(int index=0;index<rpg::Appearance::count;++index) {
     const auto control=C(int(first)+index);const auto a=CharacterSelection::Bounds(control);const auto& swatch=palette[index];
     c.Button(a,true,selection.Hover()==control || selection.Focus()==control,index==selected);
     c.RectFill({a.x+7,a.y+6,a.w-14,a.h-12},swatch.r==0?Color{info.color[0],info.color[1],info.color[2],1}:Color{swatch.r,swatch.g,swatch.b,1});
     if(index==selected) c.Line(a.x+6,a.y+a.h-3,a.x+a.w-6,a.y+a.h-3,2,gold);
    }
    button(left,"<");button(right,">");
   };
   choice(348,"COMPLEXION",rpg::skinTones,look.skin,C::Skin0,C::SkinLeft,C::SkinRight);
   choice(406,"HAIR COLOR",rpg::hairColors,look.hair,C::Hair0,C::HairLeft,C::HairRight);
   choice(464,"TUNIC DYE",rpg::tunicDyes,look.dye,C::Dye0,C::DyeLeft,C::DyeRight);
   button(C::Confirm,"Create character");
  }
  button(C::Back,"< Back");button(C::Cancel,"Cancel");
 }
 for(int i=0;i<int(rpg::ClassId::Count);++i) {
  const auto control=C(int(C::Class0)+i);const auto a=CharacterSelection::Bounds(control);const bool selected=int(selection.SelectedClass())==i;
  const auto& slot=selection.ClassSlot(i);
  c.Button(a,selection.Enabled(control),selected || selection.Hover()==control || selection.Focus()==control,selected);
  ClassSigil(c,{a.x+4,a.y+4,25,25},rpg::ClassId(i));
  if(roster && slot.status==Status::Saved) {
   const float size=std::min(12.f,12.f*139.f/std::max(1.f,c.Width(slot.name,12)));
   c.Text(a.x+34,a.y+3,slot.name,size,ink);
   c.Text(a.x+34,a.y+18,std::string(rpg::classes[i].name)+"  /  Lv. "+std::to_string(slot.level),10,gold);
  } else {
   c.Text(a.x+34,a.y+3,rpg::classes[i].name,13,selected?ink:muted);
   c.Text(a.x+34,a.y+18,slot.status==Status::New?"Empty slot":slot.status==Status::Saved?"Already recruited":"Save unavailable",9,muted);
  }
 }
 c.Text(84,594,customize?"Back to change your class":"One hero per class",11,muted);
 if(!selection.Notice().empty()) c.Wrap(842,650,350,selection.Notice(),12,gold,3);
 c.Text(72,699,"Tab  Select     Enter  Confirm     Arrow keys  Rotate     F11  Display",11,muted);
}

void Interface::Render(Renderer& renderer,const rpg::Simulation& s,float time,bool debug) {
 Canvas c(renderer); regions_.clear();
 renderedState_=StateKey(); renderedWidth_=float(renderer.Width()); renderedHeight_=float(renderer.Height());
 if (!renderer.HasMapArtwork() && !renderer.SetMapArtwork(BuildMapArtwork(s.Map()),512)) return;
 const auto button=[&](Rect rect,const std::string& label,Action action,int value=0,bool enabled=true) {
  const bool hover=rect.Contains(mouseX_,mouseY_);
  c.Button(rect,enabled,hover);
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
  Icon(c,{rect.x+3,rect.y+3,rect.w-6,rect.h-6},item,dragging_ && item==dragItem_?.35f:1.f,s.Class());
  for(float dx : {4.f,rect.w-5}) {
   c.Oval(rect.x+dx,rect.y+4,1.4f,1.4f,gold);
   c.Line(rect.x+dx,rect.y+rect.h-8,rect.x+dx,rect.y+rect.h-4,.8f,border);
  }
  if (Equipped(s,item)) c.Diamond(rect.x+rect.w-5,rect.y+5,3,3,{.66f,.92f,.39f,1});
  if (item==Item::Potion) c.Text(rect.x+rect.w-15,rect.y+rect.h-16,std::to_string(bag>=0?s.Items().Bag()[bag].count:s.Potions()),12,{1,.94f,.57f,1});
  regions_.push_back({rect,{Action::SelectItem,static_cast<int>(item)},true,bag,gear});
  regions_.back().event.inventory.source=bag;
 };
 const auto blockWindow=[&](Rect rect) { regions_.push_back({rect,{},false}); c.Window(rect); };
 const auto close=[&](Rect rect,Panel which) { button(rect,"X",Action::Close,int(which)); };

 // Combat frames sit beside the action; side windows move them to the top.
 const bool combatLayout=!HasPanels();
 const float ux=combatLayout?330.f:18.f,uy=combatLayout?518.f:23.f;
 regions_.push_back({{ux-5,uy,285,78},{},false});
 c.Frame({ux+27,uy+8,229,66},{.035f,.075f,.065f,.94f},{.39f,.46f,.28f,1});
 c.Banner({ux+49,uy,208,26},s.Nickname(),15);
 c.Portrait(ux+29,uy+36,false,s.Level());
 c.Frame({ux+58,uy+30,189,20},{.035f,.075f,.035f,1});
 c.Gradient({ux+61,uy+33,183*std::clamp(s.Health()/s.MaxHealth(),0.f,1.f),14},
   {.49f,.76f,.22f,1},{.12f,.31f,.075f,1});
 c.Center(ux+60,uy+31,185,std::to_string(int(s.Health()))+" / "+std::to_string(int(s.MaxHealth()))+
   (s.Barrier()>0?"  [+"+Number(s.Barrier())+"]":""),12);
 c.Text(ux+62,uy+55,"Lv. "+std::to_string(s.Level())+"  /  "+s.ClassName(),11,muted);
 // Each discipline has its own crest, resource name and instrument: melee
 // seals, physical ammunition chambers, or arcane gems in a curved register.
 const auto& style=s.CombatStyleInfo();const auto& tint=s.ClassInfo().color;
 const Color classAccent{tint[0],tint[1],tint[2],1};
 const float rx=combatLayout?610.f:328.f;
 regions_.push_back({{rx,uy+4,132,64},{},false});
 c.Frame({rx,uy+4,132,64},{.045f,.045f,.04f,.97f},classAccent);
 c.Skin({rx+3,uy+7,126,58},7,{.32f,.31f,.29f,1});
 ClassSigil(c,{rx+5,uy+8,22,22},s.Class());
 c.Center(rx+28,uy+8,99,style.resource,13,ink);
 for(int i=0;i<style.capacity;++i) {
  const float px=rx+66+(i-(style.capacity-1)*.5f)*23,py=uy+37;
  const Color fill=i<s.ClassResource()?classAccent:Color{.12f,.13f,.12f,1};
  if(!s.RetreatMobility()) {
   c.Diamond(px,py,9,10,gold);c.Diamond(px,py,6,7,fill);
  } else if(rpg::PhysicalLauncher(rpg::WeaponFor(s.Class()).form)) {
   c.Frame({px-7,py-9,14,18},fill,gold);c.Line(px-3,py-6,px-3,py+5,1,ink);
  } else {
   const float arc=(i-(style.capacity-1)*.5f)*(i-(style.capacity-1)*.5f)*2;
   c.Oval(px,py-arc,7,8,fill);c.Ring(px,py-arc,8,gold);c.Oval(px-2,py-3-arc,1.5f,2,ink);
  }
 }
 c.Center(rx+3,uy+51,126,s.EmpoweredAttack()?"EMPOWERED CAST":s.EmpoweredReady()?"2  /  EMPOWERED":
   s.Barrier()>0?"Ward "+Number(s.Barrier()):"Build with basic hits",10,s.EmpoweredReady()?gold:muted);
 if (s.TargetActor()) {
  const auto& target=*s.TargetActor();
  const float tx=combatLayout?750.f:475.f,ty=combatLayout?518.f:26.f;
  regions_.push_back({{tx,ty,286,78},{},false});
  c.Frame({tx,ty+8,244,66},{.085f,.045f,.035f,.94f},{.46f,.36f,.20f,1});
  c.Banner({tx+2,ty,226,26},target.Name(),14); c.Portrait(tx+253,ty+36,true,target.kind==rpg::EnemyKind::Captain?3:1);
  c.Frame({tx+9,ty+30,223,20},{.10f,.025f,.015f,1});
  c.Gradient({tx+12,ty+33,217*std::clamp(target.health/target.MaxHealth(),0.f,1.f),14},
    {.76f,.35f,.16f,1},{.35f,.075f,.04f,1});
  c.Center(tx+10,ty+31,221,std::to_string(int(std::ceil(target.health)))+" / "+std::to_string(int(target.MaxHealth())),12);
  const auto distance=std::to_string(int(std::ceil(std::hypot(s.Player().x-target.position.x,s.Player().z-target.position.z))));
  const std::string condition=target.stunned>0?"Stunned  "+Seconds(target.stunned)+"s":
    target.rooted>0?"Rooted  "+Seconds(target.rooted)+"s":target.snared>0?"Snared  "+Seconds(target.snared)+"s":
    target.exposed>0?"Exposed +25%  "+Seconds(target.exposed)+"s":"";
  c.Center(tx+5,ty+54,228,target.windup>0?"Incoming strike  "+Seconds(target.windup)+"s":
    !condition.empty()?condition:distance+" m  /  Selected target",11,target.windup>0?gold:muted);
  if (target.windup>0) {
   c.RectFill({tx+10,ty+70,220,3},{.12f,.035f,.02f,1});
   c.RectFill({tx+10,ty+70,220*std::clamp(1-target.windup/target.WindupDuration(),0.f,1.f),3},{1,.57f,.16f,1});
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
 c.Banner({1057,3,209,25},s.Map().AreaName(s.Player()),13);
 c.Banner({1118,158,104,23},"Atlas  [M]",12);
 regions_.push_back({{1090,13,160,168},{Action::Map},true});
 if (!inventoryOpen || panel!=Panel::None) {
  regions_.push_back({{1001,193,265,hasWaypoint?183.f:158.f},{},false});
  c.Frame({1001,193,265,hasWaypoint?183.f:158.f},{.025f,.045f,.025f,.82f},border);
  c.Text(1013,202,"EXPEDITION",14,gold); c.Line(1010,226,1260,226,1,border);
  c.Wrap(1013,236,244,s.Objective(),15,ink,4);
  const auto objective=s.ObjectivePosition();
  if (s.AnchorState()!=rpg::AnchorQuest::Complete || s.FrontierCompleted()<rpg::frontierQuestCount)
   c.Text(1013,332,std::to_string(int(std::ceil(std::hypot(s.Player().x-objective.x,s.Player().z-objective.z))))+" m to objective",13,gold);
  if (hasWaypoint) c.Text(1013,355,"Waypoint  "+std::to_string(int(std::ceil(std::hypot(s.Player().x-waypoint.x,s.Player().z-waypoint.z))))+" m",13,{.56f,.76f,.88f,1});
 }
 if (s.Sparring() || s.SparringReportTime()>0) {
  regions_.push_back({{470,105,506,81},{},false});
  c.Window({470,105,506,81});
  c.Text(485,113,s.Sparring()?"SPARRING  /  "+std::to_string(int(std::ceil(s.SparringSeconds())))+" seconds":s.SparringResult(),17,gold);
  c.Text(485,141,"Dealt "+std::to_string(s.SparringDamageDealt())+"  /  Taken "+std::to_string(s.SparringDamageTaken())+"  /  No XP or loot",13);
  c.Text(485,164,s.Sparring()?"E stops. Nonlethal. Potions are consumed normally.":"Health and consumables are not refunded. E at the post starts again.",11,muted);
 }
 c.Skin({0,704,1280,16},7,{.65f,.64f,.57f,1});
 const float xp=s.Level()==rpg::levelCap?1.f:float(s.Experience()-s.LevelStartExperience())/(s.NextLevelExperience()-s.LevelStartExperience());
 c.Frame({439,645,403,7}); c.RectFill({442,648,397*xp,2},{.55f,.63f,.29f,1});
 c.Window({437,655,406,55}); c.Scroll(420,676,-1,1.5f); c.Scroll(860,676,1,1.5f);
 for (int i=0;i<5;++i) {
  const float x=447+i*43.f; const Rect rect{x,662,39,40};
  c.Frame(rect,{.08f,.12f,.055f,1},i==1 && s.EmpoweredReady()?classAccent:rect.Contains(mouseX_,mouseY_)?gold:border);
  AbilityArtwork(c,{x+3,665,33,34},i,s.Class());
  const bool ready=panel!=Panel::Merchant && (i<4?std::string(s.AbilityStatus(i))=="READY":CanUse(s,Item::Potion));
  const float cooldown=i<4?s.Cooldown(i):s.PotionCooldown();
  if (!ready) c.RectFill({x+3,665,33,34},{.02f,.025f,.01f,.36f});
  if (cooldown>0) {
   const float fraction=std::clamp(cooldown/(i<4?s.AbilityCooldown(i):3.f),0.f,1.f);
   c.CooldownSweep({x+3,665,33,34},fraction);
   c.Center(x,675,39,Seconds(cooldown),cooldown<1?15.f:19.f,ink);
   c.RectFill({x+4,698,31*(1-fraction),2},blue);
  } else c.RectFill({x+4,698,31,2},ready?Color{.51f,.78f,.37f,1}:Color{.58f,.25f,.13f,1});
  if (i==4) { c.RectFill({x+23,688,14,12},{.025f,.035f,.02f,.9f}); c.Center(x+22,686,16,std::to_string(s.Potions()),11,ink); }
  c.Text(x+3,662,i<4?std::to_string(i+1):"Q",12,{1,.96f,.73f,1});
  regions_.push_back({rect,{i<4?Action::Ability:Action::UseItem,i<4?i:static_cast<int>(Item::Potion)},ready});
 }
 c.Frame({665,662,168,40},{.035f,.07f,.065f,1});
 c.Center(668,666,162,s.RetreatMobility()?rpg::PhysicalLauncher(rpg::WeaponFor(s.Class()).form)?"MARKSMAN / "+Number(s.Spell(0).range)+" m":"SPELLCASTER / "+Number(s.Spell(0).range)+" m":"MELEE / "+Number(s.Spell(0).range)+" m",12,classAccent);
 c.Center(668,684,162,s.Attacking()?(rpg::PhysicalLauncher(rpg::WeaponFor(s.Class()).form)?"Shot in progress":s.ClassInfo().range>3?"Cast in progress":"Attack in progress"):
   s.TargetActor()?"Tab changes target":"Tab selects a target",10,muted);
 // A second row contains real navigation tools, leaving the primary bindings intact.
 if (panel==Panel::None) {
  const char* labels[]={"C Gear","I Bag","N Build","M Atlas","J Journal","Esc"};
  const Action actions[]={Action::Character,Action::Inventory,Action::Build,Action::Map,Action::Journal,Action::Menu};
  for(int i=0;i<6;++i) button({447+i*65.f,617,62,23},labels[i],actions[i]);
  if (s.Attacking()) {
   const float contact=combat::Contact(s.HeavyAttack());
   const char* phase=s.AttackSeconds()<combat::StrokeStart(s.HeavyAttack())?"Wind-up":
     s.AttackSeconds()<contact?(s.ClassInfo().range>3?"Release":"Swing"):"Recovery";
   c.Frame({505,588,270,23},{.035f,.065f,.065f,.97f});
   c.Gradient({508,591,264*s.AttackProgress(),17},{.27f,.48f,.49f,1},{.10f,.22f,.26f,1});
   c.Center(508,591,264,std::string(s.Spell(s.HeavyAttack()?1:0).name)+" / "+phase,12);
  }
 }
 if (s.TalentPoints()>0) {
  c.Oval(631,615,9,9,{.35f,.20f,.06f,1}); c.Ring(631,615,9,gold);
  c.Center(622,608,18,std::to_string(s.TalentPoints()),12,ink);
  regions_.push_back({{622,606,18,18},{Action::Build},true});
 }
 c.Text(22,674,std::to_string(s.Gold())+" gold",16,gold);
 c.Text(22,695,"WASD  Move     RMB  Orbit     F11  Display",11,muted);
 if (!characterOpen && panel!=Panel::Quest) c.Wrap(22,614,385,s.Message(),14,ink,3);
 if (panel==Panel::None && s.Health()>0) {
  const auto prompt=s.InteractionPrompt();
  if (!prompt.empty()) { const float w=std::min(510.f,c.Width(prompt,14)+30); c.Frame({640-w/2,485,w,27}); c.Center(640-w/2,490,w,prompt,14,gold); }
  if (s.Channeling()) { c.RectFill({470,640,340,3},border); c.RectFill({470,640,340*s.ChannelProgress(),3},blue); }
 }

 // Independent side windows: no world dimming, keyboard movement remains live.
 if (characterOpen) {
  blockWindow({12,116,438,500});
  c.Banner({30,106,265,31},s.Nickname(),19); close({420,121,24,24},Panel::Character);
  c.Frame({65,146,188,23},{.16f,.29f,.26f,1},{.42f,.47f,.32f,1});
  c.Center(65,151,188,"Level "+std::to_string(s.Level())+"  /  "+s.ClassName(),12);
  c.Alcove({65,176,188,329});
  const auto p=c.P(70,181);
  if (renderer.BeginPortrait(p.x,p.y,178*c.layout.scale/renderer.Width(),316*c.layout.scale/renderer.Height())) {
   DrawCharacter(renderer,{0,0},0,previewYaw+std::sin(time*.35f)*.05f,
     {s.ClassInfo().color[0],s.ClassInfo().color[1],s.ClassInfo().color[2],1},0,0,CharacterOutfit::From(s.Items(),s.Class(),s.Look()),animation::Idle(time));
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
  c.Frame({304,152,132,449},{.07f,.12f,.10f,1});
  c.Gradient({309,157,122,75},{.28f,.20f,.09f,.5f},{.08f,.05f,.025f,.3f});
  c.Center(304,163,132,"VITALITY",12,gold);
  c.Center(310,186,120,std::to_string(int(s.Health()))+" / "+std::to_string(int(s.MaxHealth())),16);
  c.Frame({314,211,112,12});
  c.Gradient({317,214,106*std::clamp(s.Health()/s.MaxHealth(),0.f,1.f),6},{.56f,.27f,.20f,1},{.28f,.08f,.07f,1});
  c.Center(304,239,132,"COMBAT",12,gold);
  c.Frame({312,260,116,49},{.13f,.20f,.14f,1});
  Icon(c,{316,265,32,36},Item::Shield);
  c.Text(354,266,"Armor",12,muted); c.Text(354,282,std::to_string(int(s.DamageReduction())),20,ink);
  for (int i=0;i<2;++i) {
   const float x=312+i*60.f;
   c.Frame({x,314,56,80},{.14f,.20f,.14f,1},{.36f,.39f,.24f,1});
   c.Center(x,319,56,i?"Heavy":"Basic",11,gold);
   Icon(c,{x+13,334,30,30},i?Item::Watchguard:Item::Starter,1,s.Class());
   c.Center(x,368,56,std::to_string(int(i?s.HeavyDamage():s.StrikeDamage())),21,ink);
  }
  c.Center(304,407,132,"PROGRESSION",11,gold);
  const float characterXp=s.Level()==rpg::levelCap?1.f:std::clamp(float(s.Experience()-s.LevelStartExperience())/
    float(s.NextLevelExperience()-s.LevelStartExperience()),0.f,1.f);
  c.Center(310,426,120,"Level "+std::to_string(s.Level()),19);
  c.Frame({314,453,112,10}); c.Gradient({317,456,106*characterXp,4},blue,{.13f,.25f,.42f,1});
  c.Center(310,469,120,s.Level()==rpg::levelCap?"Level cap reached":std::to_string(s.NextLevelExperience()-s.Experience())+" XP to next level",10,muted);
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
  const auto info=s.ItemDefinition(selected);
  c.Frame({1008,464,248,104},{.065f,.105f,.09f,1});
  c.Gradient({1012,468,240,40},{.28f,.20f,.09f,.5f},{.08f,.05f,.025f,.3f});
  if(selected!=Item::None) Icon(c,{1017,473,30,30},selected,1,s.Class());
  c.FitText(1056,475,190,selected==Item::None?"Select an item":info.name,13,selected==Item::Watchguard || selected==Item::Ward?Color{.63f,.85f,.43f,1}:ink);
  c.Text(1056,494,selected==Item::None?"Inspect your belongings":ItemTypeLabel(info),10,muted);
  c.Line(1017,510,1247,510,1,{.37f,.40f,.24f,1});
  std::string detail=info.description;
  const auto delta=[](float value) { return (value>=0?std::string("+"):std::string())+std::to_string(int(value)); };
  if (info.slot==rpg::GearSlot::Weapon) {
   detail="Basic "+std::to_string(int(info.strike+s.StrikeBonus()+s.ClassInfo().strike))+"  /  Heavy "+std::to_string(int(info.heavy+s.HeavyBonus()+s.ClassInfo().heavy))+" damage.\n";
   const auto& current=Describe(s.Items().Worn(info.slot));
   detail+=Equipped(s,selected)?(rpg::PhysicalLauncher(rpg::WeaponFor(s.Class()).form)?"Equipped / required to fire":"Equipped / remove to fight unarmed"):"Vs worn: "+delta(info.strike-current.strike)+" strike / "+delta(info.heavy-current.heavy)+" heavy";
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
   const auto& offer=rpg::shopOffers[i]; const auto info=s.ItemDefinition(offer.item); const float y=196+i*70.f;
   c.Frame({487,y,471,66},{.065f,.11f,.06f,1}); c.Frame({496,y+10,43,46},{.11f,.20f,.14f,1},gold);
   Icon(c,{500,y+14,35,38},offer.item,1,s.Class());
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
   Icon(c,{rect.x+4,rect.y+4,35,37},item,available?1.f:.5f,s.Class());
   if (count>1) c.Text(rect.x+28,rect.y+28,std::to_string(count),12,gold);
   c.Center(rect.x,rect.y+50,rect.w,std::to_string(rpg::SellPrice(item))+" g",12,gold);
   regions_.push_back({rect,{Action::Buyback,i},available});
  }
  c.Wrap(490,582,465,merchantNotice.empty()?"Unequip gear before selling. Buyback is saved until reclaimed or R resets the expedition.":merchantNotice,12,ink,2);
  c.Text(490,624,"E or X closes trade. C opens your equipment.",12,muted);
 } else if (panel==Panel::Quest) {
  // A left-side conversation keeps the world visible, but captures gameplay input.
  regions_.clear(); blockWindow({16,110,450,526});
  close({429,121,24,24},Panel::Quest);
  const auto id=s.QuestDialogue(); const auto stage=s.DialogueStage();
  const auto& quest=rpg::DescribeQuest(id);
  const int textKey=int(id)*4+int(stage);
  if (questTextKey_!=textKey) { questPage=0; questTextKey_=textKey; }
  c.Text(40,130,quest.speaker,14,gold);
  const float titleSize=std::min(21.f,21.f*400/std::max(1.f,c.Width(quest.title,21)));
  c.Text(40,164,quest.title,titleSize,ink);
  const char* heading=stage==rpg::QuestStage::Offer?"QUEST OFFER":stage==rpg::QuestStage::Progress?"IN PROGRESS":
    stage==rpg::QuestStage::TurnIn?"READY TO COMPLETE":"QUEST COMPLETED";
  c.Parchment({32,195,416,174});
  const Color manuscript{.22f,.145f,.075f,1};
  c.Text(40,202,heading,11,manuscript);
  const auto lines=c.Lines(400,rpg::QuestStory(id,stage),16);
  constexpr int rows=7;
  const int pages=std::max(1,(int(lines.size())+rows-1)/rows);
  questPage=std::clamp(questPage,0,pages-1);
  for (int row=0;row<rows && questPage*rows+row<int(lines.size());++row)
    c.Text(40,219+row*20.f,lines[questPage*rows+row],16,manuscript);
  if (pages>1) {
   button({40,374,73,25},"Previous",Action::QuestPage,-1,questPage>0);
   c.Center(130,380,220,"Page "+std::to_string(questPage+1)+" / "+std::to_string(pages),12,muted);
   button({367,374,73,25},"Next",Action::QuestPage,1,questPage+1<pages);
  }
  c.Frame({34,408,414,66},{.045f,.085f,.07f,1});
  const int required=id==rpg::QuestId::Patrol?1:3;
  const bool survey=id==rpg::QuestId::Watch9;
  const int progress=stage==rpg::QuestStage::Offer && id>=rpg::QuestId::Watch1?0:stage==rpg::QuestStage::Complete?required:
    survey?int(bool(s.SurveyMask()&1))+int(bool(s.SurveyMask()&2))+int(bool(s.SurveyMask()&4)):std::min(required,s.DefeatedCount());
  c.Text(46,416,std::string("OBJECTIVES  /  ")+(survey?"Bearings ":"Enemies ")+std::to_string(progress)+" / "+std::to_string(required)+
    (id==rpg::QuestId::Anchor && s.AnchorState()>=rpg::AnchorQuest::Return?"  /  Anchor stable":""),12,gold);
  c.Wrap(46,436,390,quest.objective,14,ink,2);
  c.Frame({34,484,414,70},{.07f,.12f,.08f,1},gold);
  if (quest.item!=rpg::Item::None && rpg::ValidItem(quest.item)) {
   Icon(c,{46,496,42,45},quest.item,1,s.Class());
   c.Text(100,492,std::string(stage==rpg::QuestStage::Complete?"RECEIVED  /  ":"REWARD  /  ")+
     std::to_string(quest.gold)+" gold  /  "+std::to_string(quest.experience)+" XP",13,gold);
   c.Text(100,512,s.ItemDefinition(quest.item).name,16,ink);
   const auto& item=Describe(quest.item);
   const auto stats=item.slot==rpg::GearSlot::Weapon?
     "Base strike "+std::to_string(int(item.strike))+" / Heavy "+std::to_string(int(item.heavy))+"  -  In your bag":
     "Protection "+std::to_string(int(item.armor))+"  -  Equipped on completion";
   c.Text(100,534,stats,11,muted);
  } else { c.Text(48,499,"FIELD EXPERIENCE",13,gold); c.Text(48,523,std::to_string(quest.experience)+" XP / class progression",18,ink); }
  const auto action=s.QuestPrimaryAction();
  const std::string status=s.QuestConfirmStatus();
  c.Wrap(40,561,400,status=="READY"?"Read the assignment and rewards before confirming.":status,13,muted,1);
  if (action!=rpg::QuestAction::None) {
   button({38,590,160,34},stage==rpg::QuestStage::Offer?"Decline":"Later",Action::Close,int(Panel::Quest));
   button({242,590,202,34},stage==rpg::QuestStage::Offer?"Accept quest":"Complete quest",
     Action::QuestConfirm,int(action),status=="READY");
  } else button({242,590,202,34},"Close",Action::Close,int(Panel::Quest));
 } else if (panel!=Panel::None) {
  c.RectFill({0,0,1280,720},{0,0,0,panel==Panel::Menu?.16f:.30f}); regions_.clear();
  if (panel==Panel::Menu) {
   c.Window({476,148,328,492}); c.Banner({490,137,300,34},"MEDITERRANEAN",20);
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
   button({496,506,288,32},"Save & choose character",Action::CharacterSelection);
   button({496,546,288,32},"Exit to desktop",Action::Quit);
   c.Center(476,586,328,audioAvailable?"World remains live  /  Esc to return":"Audio unavailable",12,muted);
   if (!menuNotice.empty()) c.Wrap(496,603,288,menuNotice,11,gold,2);
   return;
  }
  if (panel==Panel::Build) {
   selectedTalent=std::clamp(selectedTalent,0,int(rpg::Talent::Count)-1);
   c.Window({96,52,1088,595}); c.Banner({123,43,1003,39},"Champion build",23);
   close({1146,61,27,27},Panel::Build);
   c.Text(124,104,"Level "+std::to_string(s.Level())+"  /  "+s.ClassName(),18,ink);
   c.Frame({850,102,306,32},{.07f,.16f,.13f,1},s.TalentPoints()>0?gold:border);
   c.Center(850,109,306,std::to_string(s.TalentPoints())+" talent point"+(s.TalentPoints()==1?"":"s")+" available",15,gold);
   button({594,102,218,32},loadoutsPage?"Talents & spellbook":"Saved builds",Action::BuildPage);
   if (loadoutsPage) {
    const auto name=[&s](int mask) -> std::string {
     if (mask<0) return "Empty slot";
     if (!mask) return "Unspent talents";
     for (int i=0;i<int(rpg::buildPresets.size());++i) if (unsigned(mask)==rpg::buildPresets[i].mask) return rpg::ClassPresetName(s.Class(),i);
     if(rpg::TalentCount(unsigned(mask))>2) return std::string(s.ClassInfo().name)+" / "+std::to_string(rpg::TalentCount(unsigned(mask)))+" talents";
     std::string result;
     for (int i=0;i<int(rpg::Talent::Count);++i) if (rpg::HasTalent(unsigned(mask),rpg::Talent(i))) {
      if (!result.empty()) result+=" + ";
      result+=s.TalentDefinition(i).name;
     }
     return result;
    };
    const char* slotNames[]={"LOADOUT I","LOADOUT II","LOADOUT III"};
    for (int i=0;i<3;++i) {
     const int mask=s.Loadout(i);
     const Rect card{124,156+i*94.f,332,82};
     c.Frame(card,selectedLoadout==i?Color{.10f,.19f,.17f,1}:dark,selectedLoadout==i?gold:border);
     c.Text(card.x+14,card.y+10,std::string(slotNames[i])+(mask>=0 && unsigned(mask)==s.BuildMask()?" / ACTIVE":""),13,gold);
     c.Wrap(card.x+14,card.y+33,207,name(mask),13,mask<0?muted:ink,2);
     if (mask>=0) {
      int shown=0;
      for (int t=0;t<int(rpg::Talent::Count) && shown<2;++t) if (rpg::HasTalent(unsigned(mask),rpg::Talent(t)))
       TalentIcon(c,{card.x+237+(shown++)*39.f,card.y+34,28,31},t,1,s.Class());
     }
     regions_.push_back({card,{Action::SelectLoadout,i},true});
    }
    button({124,448,332,32},"Preview my active build",Action::SelectBuildPreset,-1);
    c.Text(124,482,"SPECIALIZATION PRESETS / LEVEL 3",11,gold);
    for (int i=0;i<int(rpg::buildPresets.size());++i)
     button({124+(i%2)*172.f,502+(i/2)*34.f,160,29},rpg::ClassPresetName(s.Class(),i),Action::SelectBuildPreset,i);
    c.Text(124,608,"Select a slot, then store the preview.",12,muted);
    c.Frame({488,150,668,454},{.035f,.08f,.075f,1});
    c.Text(512,166,"BUILD PREVIEW",12,gold);
    c.Text(512,188,name(previewBuild),19,ink);
    const unsigned mask=previewBuild<0?s.BuildMask():unsigned(previewBuild);
    const int cost=rpg::TalentCount(mask),earned=s.EarnedPoints();
    c.Text(512,220,cost>earned?"Requires "+std::to_string(cost)+" points / "+std::to_string(earned)+" earned":
      std::to_string(cost)+" of "+std::to_string(earned)+" earned points / "+std::to_string(earned-cost)+" unspent",12,muted);
    c.Text(796,249,"CURRENT",11,muted); c.Text(925,249,"PREVIEW",11,gold); c.Text(1060,249,"CHANGE",11,muted);
    const auto live=s.PreviewBuild(s.BuildMask()),proposed=s.PreviewBuild(mask);
    const char* labels[]={"Maximum health","Basic damage","Heavy damage","Damage reduction","Self healing","Dash cooldown","Healing cooldown"};
    const float current[]={live.health,live.strike,live.heavy,live.armor,live.mend,live.dashCooldown,live.mendCooldown};
    const float next[]={proposed.health,proposed.strike,proposed.heavy,proposed.armor,proposed.mend,proposed.dashCooldown,proposed.mendCooldown};
    for (int i=0;i<7;++i) {
     const float y=273+i*24.f,delta=next[i]-current[i]; const std::string unit=i>=5?"s":"";
     if (i%2==0) c.RectFill({505,y-3,632,24},{.075f,.125f,.11f,1});
     c.Text(512,y,labels[i],13,ink); c.Text(810,y,Number(current[i])+unit,13,muted);
     c.Text(940,y,Number(next[i])+unit,13,gold);
     c.Text(1071,y,delta==0?"--":std::string(delta>0?"+":"")+Number(delta)+unit,13,
       delta==0?muted:((i>=5?delta<0:delta>0)?Color{.57f,.83f,.45f,1}:Color{.94f,.55f,.40f,1}));
    }
    const std::string conditional=rpg::HasTalent(mask,rpg::Talent::Executioner)?
      std::string(s.TalentDefinition(6).name)+": Heavy deals "+Number(proposed.heavy*1.25f)+" at or below half enemy health.":
      rpg::HasTalent(mask,rpg::Talent::Mercy)?std::string(s.TalentDefinition(8).name)+": Healing restores up to "+Number(proposed.mend+20)+" at or below 35% health.":
      "Base spell values shown with your current level and worn equipment.";
    c.Wrap(512,449,620,conditional,13,ink,2);
    const bool confirming=pendingBuild.action!=rpg::BuildAction::None;
    if (confirming) {
     const bool applying=pendingBuild.action==rpg::BuildAction::Apply;
     const std::string status=s.LoadoutStatus(pendingBuild);
     c.Text(512,487,applying?"Apply this build?":s.Loadout(pendingBuild.slot)>=0?"Replace the saved build in this slot?":"Store this build in the selected slot?",16,gold);
     c.Wrap(512,511,620,applying?"Health cannot increase; excess health is clamped. Active cooldowns keep counting down.":
       "Only this saved slot changes. Your active talents stay as they are. Press F5 afterward to save to disk.",12,muted,2);
     button({512,552,300,32},applying?"Confirm apply":"Confirm store",Action::ConfirmBuild,0,
       status==(applying?"Ready to apply":"Ready to store"));
     button({832,552,300,32},"Cancel",Action::CancelBuild);
     if (status!=(applying?"Ready to apply":"Ready to store")) c.Text(512,586,status,11,gold);
    } else {
     const std::string apply=s.LoadoutStatus({rpg::BuildAction::Apply,previewBuild,selectedLoadout});
     const std::string store=s.LoadoutStatus({rpg::BuildAction::Store,previewBuild,selectedLoadout});
     c.Wrap(512,500,300,apply,12,muted,2); c.Wrap(832,500,300,store,12,muted,2);
     button({512,552,300,32},"Apply preview",Action::RequestBuildApply,0,apply=="Ready to apply");
     button({832,552,300,32},"Store in loadout "+std::to_string(selectedLoadout+1),Action::RequestBuildStore,0,store=="Ready to store");
    }
    c.Wrap(488,612,668,s.Message(),11,muted,2);
    return;
   }
   c.Text(148,136,"SPELLBOOK / select an ability to inspect its current power and upgrades",11,muted);
   for(int i=0;i<4;++i) {
    const Rect spellSlot{148+i*170.f,157,154,52};
    c.Frame(spellSlot,selectedSpell==i?Color{.12f,.23f,.20f,1}:dark,selectedSpell==i?gold:border);
    AbilityArtwork(c,{spellSlot.x+6,spellSlot.y+6,39,39},i,s.Class());
    c.Text(spellSlot.x+52,spellSlot.y+10,s.Spell(i).name,14,ink);
    c.Text(spellSlot.x+52,spellSlot.y+30,std::string("Key ")+s.Spell(i).key+" / "+Number(s.AbilityCooldown(i))+"s",11,muted);
    regions_.push_back({spellSlot,{Action::SelectSpell,i},true});
   }
   const auto& branches=s.ClassInfo().branches;
   const Color branchColors[]={{.83f,.54f,.31f,1},{.40f,.64f,.83f,1},{.45f,.77f,.57f,1}};
   for (int branch=0;branch<3;++branch) {
    const float x=248+branch*232.f;
    c.RectFill({x-76,222,152,25},dark); c.Center(x-76,226,152,branches[branch],16,branchColors[branch]);
    const bool connected=s.HasTalent(rpg::Talent(branch));
    const Color wire=connected?branchColors[branch]:border;
    // Both level-three choices connect to the foundation, not to each other.
    c.Line(x,346,x,353,2,wire); c.Line(x,353,x-111,353,2,wire);
    c.Line(x-111,353,x-111,512,2,wire);
    c.Line(x-111,406,x-105,406,2,wire); c.Line(x-111,512,x-105,512,2,wire);
    c.Center(x-45,452,90,"DISCIPLINES",10,muted);
   }
   for (int i=0;i<int(rpg::Talent::Count);++i) {
    const auto talent=rpg::Talent(i); const auto& info=s.TalentDefinition(i);
    const bool learned=s.HasTalent(talent),available=std::string(s.TalentStatus(talent))=="Ready to learn";
    const Rect rect{143+(i%3)*232.f,254+(i/3)*106.f,210,92};
    const bool hovered=rect.Contains(mouseX_,mouseY_),selectedNode=selectedSpell<0 && !confirmTalentReset && i==selectedTalent;
    const Color accent=learned?Color{.57f,.83f,.45f,1}:available?gold:border;
    c.Frame(rect,selectedNode?Color{.12f,.23f,.20f,1}:Color{.035f,.085f,.075f,1},selectedNode || hovered?gold:accent);
    c.RectFill({rect.x+4,rect.y+4,3,rect.h-8},accent);
    TalentIcon(c,{rect.x+11,rect.y+8,30,33},i,learned || available?1.f:.45f,s.Class());
    c.Wrap(rect.x+50,rect.y+10,150,info.name,15,learned || available?ink:muted,2);
    c.Wrap(rect.x+12,rect.y+46,186,TalentEffect(s,i),12,muted,2);
    const char* state=learned?"LEARNED":available?"AVAILABLE / 1 POINT":
      s.Level()<info.level?"LOCKED":info.prerequisite!=rpg::Talent::Count && !s.HasTalent(info.prerequisite)?"PREREQUISITE":"UNAVAILABLE";
    c.Text(rect.x+12,rect.y+76,"LEVEL "+std::to_string(info.level)+" / "+state,10,accent);
    regions_.push_back({rect,{Action::SelectTalent,i},true});
   }
   c.Frame({850,150,306,454},{.035f,.08f,.075f,1});
   if (confirmTalentReset) {
    c.Wrap(874,173,258,"Reset your talents?",22,gold,2);
    c.Wrap(874,245,258,"Refund all spent points for free. Your spells return to their foundation power.",15,ink,4);
    c.Wrap(874,344,258,"Health will not be restored. Losing Vitality lowers your maximum and clamps excess health. Active cooldowns keep counting down.",14,muted,6);
    const std::string status=s.TalentResetStatus();
    c.Wrap(874,478,258,status,13,gold,2);
    button({874,524,258,32},"Confirm reset",Action::ResetTalents,0,status=="Ready to reset");
    button({874,563,258,28},"Keep current build",Action::CancelTalentReset);
   } else if (rpg::ValidSpell(selectedSpell)) {
    const auto& spell=s.Spell(selectedSpell);
    AbilityArtwork(c,{874,166,48,48},selectedSpell,s.Class());
    c.Text(937,175,"FOUNDATION SPELL",13,gold);
    c.Text(937,198,std::string("Key ")+spell.key+" / learned at level 1",11,muted);
    c.Text(874,231,spell.name,22,ink);
    c.Wrap(874,264,258,SpellEffect(s,selectedSpell),14,gold,2);
    c.Wrap(874,311,258,selectedSpell==1?std::string(style.resource)+": "+std::to_string(style.capacity)+
      " basic hits fill the reserve. "+style.payoff:spell.description,13,muted,5);
    c.Text(874,413,"Cooldown "+Number(s.AbilityCooldown(selectedSpell))+"s / "+
      (spell.range>0?Number(spell.range)+" m":"self"),13,ink);
    c.Text(874,435,selectedSpell<2?"Contact "+Number(spell.contact)+"s / recovery "+Number(spell.recovery)+"s":
      selectedSpell==2?Number(s.RetreatMobility()?style.retreatDistance:3.5f)+" m travel / "+Number(s.MobilityDuration())+"s burst":"Instant / no mana cost",12,muted);
    c.Text(874,457,selectedSpell==1 && style.steadyHeavy?"Stand still until release":spell.targeting,12,muted);
    c.Text(874,483,"RELATED TALENTS",11,gold);
    for(int i=0;i<3;++i) if (rpg::ValidTalent(spell.upgrades[i])) {
     const auto talent=spell.upgrades[i];
     button({874,503+i*30.f,258,26},std::string(s.HasTalent(talent)?"* ":"")+s.TalentDefinition(int(talent)).name,
       Action::SelectTalent,int(talent));
    }
   } else {
   const auto talent=rpg::Talent(selectedTalent); const auto& info=s.TalentDefinition(selectedTalent);
   const bool learned=s.HasTalent(talent);
   const std::string status=s.TalentStatus(talent);
   TalentIcon(c,{874,166,48,52},selectedTalent,1,s.Class());
   c.Text(937,175,branches[selectedTalent%3],14,branchColors[selectedTalent%3]);
   c.Text(937,199,selectedTalent<3?"Foundation talent":selectedTalent<6?"Specialization / option A":"Specialization / option B",11,muted);
   c.Wrap(874,232,258,info.name,20,gold,2);
   c.Wrap(874,288,258,TalentEffect(s,selectedTalent),15,ink,2);
   c.Wrap(874,341,258,info.description,14,muted,4);
   c.Line(874,423,1132,423,1,border);
   c.Text(874,436,"Requires level "+std::to_string(info.level),13,ink);
   c.Wrap(874,460,258,info.prerequisite==rpg::Talent::Count?"Prerequisite: Foundations":
     std::string("Prerequisite: ")+s.TalentDefinition(int(info.prerequisite)).name,12,muted,2);
   c.Wrap(874,493,258,status,13,learned?Color{.57f,.83f,.45f,1}:gold,2);
   button({874,546,258,37},learned?"Learned":"Learn / 1 point",Action::LearnTalent,selectedTalent,status=="Ready to learn");
   }
   c.Text(148,568,"Points at levels 2, 3, 5, 7 and 9. Five choices by level 10.",12,ink);
   c.Frame({148,589,480,8}); c.RectFill({151,592,474*xp,2},gold);
   button({666,590,146,28},"Reset talents",Action::ConfirmTalentReset,0,
     std::string(s.TalentResetStatus())=="Ready to reset" && !confirmTalentReset);
   c.Text(148,607,s.Level()==rpg::levelCap?"Level cap reached  /  "+std::to_string(s.EarnedPoints())+" points earned":
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
   c.Text(156,619,"Roads and markers follow the playable map. Island: 58 x 62 m.",13,muted);
  } else {
   c.Parchment({208,151,397,384});
   for(int i=0;i<rpg::frontierQuestCount+2;++i) {
    const auto id=rpg::QuestId(int(rpg::QuestId::Patrol)+i);const auto& quest=rpg::DescribeQuest(id);
    const bool done=i==0?s.QuestState()==rpg::Quest::Complete:i==1?s.AnchorState()==rpg::AnchorQuest::Complete:i-2<s.FrontierCompleted();
    c.FitText(227,160+i*30.f,359,std::to_string(i+1)+". "+quest.title,13,{.22f,.15f,.09f,1});
    c.Text(227,175+i*30.f,done?"Completed":id>s.CurrentQuest()?"Locked":id==s.CurrentQuest()?"Current assignment":"Campaign assignment",9,{.32f,.22f,.13f,1});
   }
   const auto id=s.CurrentQuest();
   const auto& quest=rpg::DescribeQuest(id);
   c.Frame({622,158,450,377});c.Wrap(643,177,402,quest.title,23,ink,2);
   c.Wrap(643,243,402,quest.offer,16,muted,8);
   c.Text(643,475,"REWARD / "+std::to_string(quest.experience)+" XP",15,gold);
   button({734,545,338,43},"Show objective on the atlas",Action::Map);
   c.Wrap(209,552,397,s.Objective(),14,muted,2);
   c.Text(209,606,"The world remains active while reading the journal.",14,muted);
  }
 }
 // Reverse hit order matches pointer dispatch: covered cells never leak tooltips.
 for (auto it=regions_.rbegin();!dragging_ && it!=regions_.rend();++it) if (it->rect.Contains(mouseX_,mouseY_)) {
  if (it->event.action==Action::SelectItem || it->event.action==Action::Purchase) {
   const bool purchase=it->event.action==Action::Purchase;
   const Item item=purchase?rpg::shopOffers[it->event.value].item:static_cast<Item>(it->event.value);
   if (!rpg::ValidItem(item)) break;
   const auto info=s.ItemDefinition(item); const auto summary=SummarizeItem(s,item);
   const float x=std::clamp(mouseX_>640?mouseX_-329:mouseX_+21,8.f,963.f),y=std::clamp(mouseY_-42,8.f,466.f);
   c.Window({x,y,309,246}); Icon(c,{x+13,y+12,34,38},item,1,s.Class());
   c.FitText(x+57,y+11,238,info.name,16,gold); c.FitText(x+57,y+37,238,ItemTypeLabel(info),11,muted);
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
   c.FitText(x+14,y+221,281,action,12,ink);
  } else if (it->event.action==Action::Buyback) {
   const auto item=Item(it->event.value); const float x=std::clamp(mouseX_-140,475.f,659.f);
   c.Window({x,312,309,116}); c.FitText(x+14,322,281,s.ItemDefinition(item).name,17,gold);
   c.Text(x+14,351,"Held: "+std::to_string(s.BuybackCount(item))+"  /  "+std::to_string(rpg::SellPrice(item))+" gold each",13,ink);
   const auto status=s.TradeStatus({rpg::TradeAction::Buyback,item});
   c.Text(x+14,390,std::string(status)=="Available"?"Click to reclaim one into your bag":status,12,muted);
  } else if (panel!=Panel::Merchant && (it->event.action==Action::Ability || it->event.action==Action::UseItem) && it->rect.y>650) {
   const bool potion=it->event.action==Action::UseItem; const int ability=it->event.value;
   const std::string detail=potion?"Restores up to 40 health.":SpellEffect(s,ability);
   const std::string status=potion?(s.Health()<=0?"DEFEATED":s.Potions()==0?"NO POTIONS":s.Health()>=s.MaxHealth()?"FULL HEALTH":s.PotionCooldown()>0?"COOLDOWN":"READY"):s.AbilityStatus(ability);
   const float x=std::clamp(it->rect.x-30,460.f,680.f);
   c.Window({x,470,300,149}); c.FitText(x+14,480,272,potion?"Healing draught":s.Spell(ability).name,19,gold);
   c.Wrap(x+14,512,272,detail,14,ink,2);
   const float remaining=potion?s.PotionCooldown():s.Cooldown(ability);
   c.Text(x+14,554,"Cooldown: "+Number(potion?3.f:s.AbilityCooldown(ability))+"s"+
     (potion?"  /  Stock: "+std::to_string(s.Potions()):ability<3?"  /  "+std::string(ability==2 && s.RetreatMobility()?"Travel: ":"Range: ")+Number(s.Spell(ability).range)+" m":""),12,muted);
   c.Line(x+14,576,x+286,576,1,border);
   c.FitText(x+14,589,272,status+(remaining>0?"  "+Seconds(remaining)+"s":""),12,status=="READY"?Color{.57f,.84f,.40f,1}:Color{1,.64f,.40f,1});
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
   if (it->bag>=0 || it->gear>=0) c.Skin(it->rect,10,compatible?Color{.5f,.9f,.3f,1}:Color{.9f,.2f,.1f,1},false);
   break;
  }
  c.Frame({mouseX_-21,mouseY_-21,42,42},{.04f,.08f,.04f,.8f},gold);
  Icon(c,{mouseX_-19,mouseY_-19,38,38},dragItem_,1,s.Class());
 }
 if (noticeTime>0 && panel==Panel::None) {
  regions_.push_back({{465,111,510,58},{},false});
  c.Frame({465,111,510,58},{.035f,.06f,.025f,.95f});
  c.Wrap(477,123,486,inventoryNotice,14,ink,2);
 }
 if (s.Health()<=0 && panel==Panel::None) {
  c.Window({460,285,510,146}); c.Banner({481,276,468,37},"Return to Dawnwatch",23);
  c.Center(460,336,510,"Press Enter to recover with your progress intact.",17);
  c.Center(460,374,510,"Quests, equipment and collected loot are retained.",14,muted);
 }
 if (debug) {
  c.Frame({24,102,360,94}); c.Text(35,110,"Draws "+std::to_string(renderer.LastDrawCalls())+"   GPU "+std::to_string(renderer.GpuMilliseconds()).substr(0,5)+" ms",15);
  c.Text(35,133,"Meshes "+std::to_string(renderer.MeshBytes()/1024)+" KiB   F2 HUD   F3 metrics",12,muted);
  const auto& detail=renderer.LastBodyLodCounts();
  c.Text(35,151,"Body detail: "+std::to_string(detail[0])+" near / "+std::to_string(detail[1])+" mid / "+std::to_string(detail[2])+" far",12,muted);
  const auto& parts=renderer.LastAttachmentLodCounts();
  c.Text(35,169,"Parts: "+std::to_string(parts[0])+" simplified / "+std::to_string(parts[1])+" omitted",12,muted);
 }
}
}
