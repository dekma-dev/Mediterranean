#include "game/Game.h"
#include "game/SpellVfx.h"
#include "game/CastEffects.h"
#include "game/CharacterView.h"
#include "engine/Audio.h"
#include "engine/Renderer.h"
#include "platform/Win32Window.h"
#include <wincodec.h>
#include <filesystem>
#include <iostream>
#include <stdexcept>

namespace {
using namespace rpg;using namespace DirectX;using K=CombatVisualKind;using Pixels=std::vector<unsigned char>;
void Check(bool value,const char* what){if(!value)throw std::runtime_error(what);}
Pixels Capture(Renderer& r,const std::filesystem::path& path){
 Check(r.Capture(path.wstring()) && r.EndFrame(false),"Capture");
 Microsoft::WRL::ComPtr<IWICImagingFactory> f;Microsoft::WRL::ComPtr<IWICBitmapDecoder> d;
 Microsoft::WRL::ComPtr<IWICBitmapFrameDecode> frame;Microsoft::WRL::ComPtr<IWICFormatConverter> c;
 Check(SUCCEEDED(CoCreateInstance(CLSID_WICImagingFactory,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&f))),"WIC");
 Check(SUCCEEDED(f->CreateDecoderFromFilename(path.c_str(),nullptr,GENERIC_READ,WICDecodeMetadataCacheOnLoad,&d)) && SUCCEEDED(d->GetFrame(0,&frame)) && SUCCEEDED(f->CreateFormatConverter(&c)) &&
  SUCCEEDED(c->Initialize(frame.Get(),GUID_WICPixelFormat32bppRGBA,WICBitmapDitherTypeNone,nullptr,0,WICBitmapPaletteTypeCustom)),"Decode");
 Pixels p(size_t(r.Width())*r.Height()*4);Check(SUCCEEDED(c->CopyPixels(nullptr,r.Width()*4,UINT(p.size()),p.data())),"Copy");return p;
}
double Difference(const Pixels& a,const Pixels& b){Check(a.size()==b.size(),"Image dimensions");double sum=0;for(size_t i=0;i<a.size();++i)sum+=std::abs(int(a[i])-int(b[i]));return sum/a.size();}
unsigned VisiblePixels(const Pixels& a,const Pixels& b){Check(a.size()==b.size(),"Image dimensions");unsigned count=0;for(size_t i=0;i<a.size();i+=4)count+=std::max({std::abs(int(a[i])-int(b[i])),std::abs(int(a[i+1])-int(b[i+1])),std::abs(int(a[i+2])-int(b[i+2]))})>8;return count;}
float Distance(XMFLOAT3 a,XMFLOAT3 b){return XMVectorGetX(XMVector3Length(XMLoadFloat3(&a)-XMLoadFloat3(&b)));}
Simulation Hero(ClassId id){
 Simulation s;Check(s.Load("MEDITERRANEAN 3\n12 -3.8 50 0 3 90\n5 1 1 0\n4\n0 0 0 0\n100 -4 7 0 0 1 3 0 0\n101 4 9 0 0 1 3 0 0\n102 0 12 0 0 1 3 0 0\n"),"Legacy fixture");
 auto save=s.Save();const auto at=save.find("PROFILE ");save.replace(at,save.find('\n',at)-at,"PROFILE "+std::to_string(int(id))+" \"Tester\" 7 0 3");Check(s.Load(save) && s.Level()==10,"Level-ten fixture");return s;
}
void Motion(){
 const float contact=playerclips::Collection()[unsigned(playerclips::Clip::Slash)].contact;
 Check(std::abs(animation::ReturnCutPhase(contact,contact)-contact)<.00001f,"Backhand moved contact key");
 float prior=1;
 for(int n=0;n<=1000;++n){const float p=animation::ReturnCutPhase(n/1000.f,contact);Check(p>=-.00001f && p<=prior+.00001f,"Backhand time reversed twice");prior=p;}
 const auto raw=[&](float p,bool reverse){return playerclips::Sample(playerclips::Clip::Slash,reverse?animation::ReturnCutPhase(p,contact):p).weapon;};
 const auto velocity=[&](bool reverse){auto a=raw(contact-.001f,reverse),b=raw(contact+.001f,reverse);return XMFLOAT3{b.x-a.x,b.y-a.y,b.z-a.z};};
 const auto a=velocity(false),b=velocity(true);
 Check(a.x*b.x+a.y*b.y+a.z*b.z<-.000001f,"Backhand still crosses in the normal direction");
 for(int style=0;style<4;++style)for(int ability:{0,1,5,6,8,9,11,12,13,14})for(bool cancel:{false,true}){
  const bool heavy=ability!=0 && ability!=12,bow=style==1,staff=style>=2;
  animation::AuthoredWeaponController control;CharacterPose previous;float speed=0,shift=0;
  for(int n=0;n<1800;++n){const float t=n*.001f,end=cancel?.13f:combat::Duration(heavy);
   control.Update(.001f,t<end?t:-1,heavy,staff,true,1,bow,ability,style);
   animation::Sample s;s.authoredGait=true;s.authoredEquipment=style==3?playerclips::Equipment::None:bow?playerclips::Equipment::Bow:staff?playerclips::Equipment::Staff:playerclips::Equipment::Sword;
   s.authoredStrike=control.frame;s.authoredStrikePhase=control.phase;s.authoredStrikeWeight=control.weight;s.authoredStrikeHeavy=control.heavy;s.authoredAccent=control.accent;
   s.feetActive=true;s.feet[0].x=-.18f;s.feet[1].x=.18f;
   const auto pose=BuildCharacterPose(0,0,false,s,style==3?WeaponForm::Scepter:bow?WeaponForm::Handbow:staff?WeaponForm::AstralStaff:WeaponForm::Blade);
   for(int side=0;side<2;++side){Check(Distance(pose.legs[side].end,{side?.18f:-.18f,.17f,0})<.0001f,"Ability accent moved a planted foot");
    if(n)speed=std::max(speed,Distance(pose.arms[side].end,previous.arms[side].end)*1000);}
   shift=std::max(shift,std::abs(control.accent.pitch)+std::abs(control.accent.yaw));previous=pose;
  }
  Check(speed<13 && shift>.0001f && control.weight==0,"Ability choreography discontinuity or lost settling");
 }
 for(int hz:{30,60,144,1000}){
  animation::AuthoredWeaponController c;CharacterPose previous;float worst=0;
  for(int n=0;n<hz*4;++n){const float time=float(n)/hz,cycle=std::fmod(time,.58f);const int ability=int(time/.58f)%2?12:0;
   c.Update(1.f/hz,cycle<combat::Duration(false)?cycle:-1,false,false,true,1,false,ability,0);
   animation::Sample s;s.authoredGait=true;s.authoredEquipment=playerclips::Equipment::Sword;s.authoredStrike=c.frame;
   s.authoredStrikeWeight=c.weight;s.authoredStrikePhase=c.phase;s.authoredAccent=c.accent;
   const auto pose=BuildCharacterPose(0,0,false,s,WeaponForm::Blade);
   if(n)worst=std::max(worst,Distance(pose.arms[1].end,previous.arms[1].end)*hz);
   previous=pose;
  }
  Check(worst<13,"Alternating normal/backhand attacks snapped at recovery");
 }
}
void Native(Renderer& r,const std::filesystem::path& out){
 const float ground=WorldMap::Height({});
 const auto begin=[&]{r.BeginFrame();r.SetCamera({5,ground+5,-7},{0,ground+1,0});r.DrawMesh(visual::Mesh::Terrain,{},{1,1,1},{.25f,.24f,.23f,1},visual::Material::Stone);};
 for(auto school:{ClassId::Mage,ClassId::Priest})for(int ability:school==ClassId::Mage?std::vector<int>{8,9,11,14}:std::vector<int>{5,6}){
  CombatVisualEvent e;e.kind=K::HeavyHit;e.school=school;e.ability=ability;e.heavy=true;e.signature=ability==6;e.height=ground+1.2f;e.source={0,-3};e.sourceHeight=WorldMap::Height(e.source);
  Pixels primary;
  for(int hz:{30,60,144}){CombatEffects fx;fx.Emit(e);float elapsed=0;while(elapsed<.24f-1e-6f){const float dt=std::min(1.f/hz,.24f-elapsed);fx.Advance(dt);elapsed+=dt;}
   begin();fx.Render(r);Check(r.EffectVertices()>0 && r.EffectVertices()<1800,"Signature contact budget");
   auto p=Capture(r,out/("contact-"+std::to_string(int(school))+"-"+std::to_string(ability)+"-"+std::to_string(hz)+".png"));
   if(hz==30)primary=p;else Check(Difference(primary,p)<.04,"Signature changed with refresh rate");
   for(int n=0;n<20;++n)fx.Advance(.05f);begin();fx.Render(r);Check(r.EffectVertices()==0 && r.EndFrame(false),"Signature never expired");}
  e.primary=false;begin();spellvfx::Contact(r,e,.24f);auto secondary=Capture(r,out/("secondary-"+std::to_string(int(school))+"-"+std::to_string(ability)+".png"));
  std::cout<<"Contact "<<int(school)<<'/'<<ability<<" primary difference "<<Difference(primary,secondary)<<'\n';
  // A narrow returning beam changes fewer pixels than a ground explosion.
  // Require visible local contrast, not increased whole-screen exposure.
  Check(VisiblePixels(primary,secondary)>200,"Secondary victim repeated the main spectacle");
 }
 CombatVisualEvent meteor;meteor.school=ClassId::Mage;meteor.ability=9;meteor.destination={0,0};meteor.destinationHeight=.65f;meteor.flight=.5f;meteor.kind=K::SpellCast;meteor.heavy=true;
 const XMFLOAT3 start{0,WorldMap::Height({0,-3})+1,-3};const auto end=spellvfx::ProjectilePosition(meteor,start,1);
 Check(Distance(end,{0,WorldMap::Height({})+.65f,0})<.0001f,"Meteor missed accepted contact height");
 Check(spellvfx::ProjectilePosition(meteor,start,.2f).y>spellvfx::ProjectilePosition(meteor,start,.8f).y+2,"Meteor does not descend");
 for(float phase:{.1f,.45f,.8f,.98f}){begin();spellvfx::Flight(r,meteor,start,phase*.5f);Capture(r,out/("meteor-flight-"+std::to_string(int(phase*100))+".png"));}
 meteor.school=ClassId::Scout;meteor.ability=8;begin();spellvfx::Flight(r,meteor,start,.22f);const auto fanVertices=r.EffectVertices();Capture(r,out/"scout-fan.png");
 meteor.ability=0;begin();spellvfx::Flight(r,meteor,start,.22f);Check(fanVertices>r.EffectVertices()*3 && r.EndFrame(false),"Fan still renders a single arrow");
}
void Live(Renderer& r,const std::filesystem::path& out){
 Check(r.Resize(1600,900),"World resolution");
 for(auto id:{ClassId::Warrior,ClassId::Scout,ClassId::Mage,ClassId::Priest})for(int ability:id==ClassId::Mage?std::vector<int>{8,9,11,14}:id==ClassId::Priest?std::vector<int>{5,6}:id==ClassId::Warrior?std::vector<int>{6,12,14}:std::vector<int>{8,9,11}){
  Game g;g.Initialize();Check(g.RestoreSnapshot(Hero(id).Save()),"World fixture");g.SetCameraOrbit(.95f,.42f,9.f);
  Command c;c.selectActor=210;g.ExecuteCommand(c,.001f);
  // Earn reserve through real accepted contacts on the immortal practice dummy.
  for(int attempt=0;attempt<8 && !g.State().EmpoweredReady();++attempt) {c={};c.ability=0;g.ExecuteCommand(c,.001f);for(int n=0;n<240;++n)g.ExecuteCommand({},.01f);}
  Check(g.State().EmpoweredReady(),"Could not earn class reserve");
  auto& enemies=const_cast<std::vector<Enemy>&>(g.State().Enemies());
  const auto centre=g.State().TargetActor()->position;
  for(size_t i=0;i<2;++i){auto& e=enemies[i];e.position=e.home={centre.x+(i?.8f:-.8f),centre.z+.5f};e.health=e.MaxHealth();e.looted=false;e.state=EnemyState::Idle;e.attackCooldown=100;}
  std::cout<<"World "<<int(id)<<'/'<<ability<<'\n';
  c={};c.ability=ability;g.ExecuteCommand(c,.001f);Check(g.State().Attacking() && g.State().AttackAbility()==ability,"Signature rejected");
  int primaries=0,secondaries=0;bool contact=false,release=false,motion=false;
  for(int n=0;n<1000 && !contact;++n){g.ExecuteCommand({},.005f);for(size_t i=0;i<g.State().VisualEventCount();++i){const auto& e=g.State().VisualEvents()[i];
    if(e.kind==K::SpellCast || e.kind==K::HeavyStrike || e.kind==K::Strike)release=true;
    if(e.kind==K::Hit || e.kind==K::HeavyHit){contact=true;if(e.primary)++primaries;else ++secondaries;}}
   motion|=g.PlayerAnimation().authoredStrikeWeight>.1f && std::abs(g.PlayerAnimation().authoredAccent.pitch)+std::abs(g.PlayerAnimation().authoredAccent.yaw)>.001f;
   if(release && !contact && n%12==0){r.BeginFrame();g.Render(r);Capture(r,out/("world-flight-"+std::to_string(int(id))+"-"+std::to_string(ability)+".png"));}}
  if(!contact || !release || primaries!=1 || !motion)std::cerr<<"contact="<<contact<<" release="<<release<<" primary="<<primaries<<" motion="<<motion<<'\n';
  Check(contact && release && primaries==1 && motion,"Action lost its single primary contact/release or body motion");
  if((id==ClassId::Mage && (ability==8 || ability==9 || ability==11)) || (id==ClassId::Scout && ability==8))Check(secondaries>0,"Area fixture missed secondary contacts");
  sound::Events audio;audio.Update(g.State());unsigned grand=0;for(unsigned i=0;i<audio.count;++i)grand+=audio.cues[i]==sound::Cue::MeteorImpact || audio.cues[i]==sound::Cue::FirestormImpact || audio.cues[i]==sound::Cue::HolySanctuary;
  if((id==ClassId::Mage && (ability==9 || ability==11)) || (id==ClassId::Priest && ability==6))Check(grand==1,"Area cast duplicated its large acoustic impact");
  for(int n=0;n<24;++n)g.ExecuteCommand({},.01f);
  const auto save=g.State().Save();r.BeginFrame();g.Render(r);const auto image=Capture(r,out/("world-contact-"+std::to_string(int(id))+"-"+std::to_string(ability)+".png"));
  r.BeginFrame();g.Render(r);Check(image==Capture(r,out/"repeat.png") && save==g.State().Save(),"Rendering mutated choreography");
  // Find an actually clear orbit through the yard, rather than labelling a
  // camera pushed into foliage as a distant readability capture.
  float clearance=0,viewYaw=0,viewPitch=0;
  for(float pitch:{.20f,.40f,.65f,.85f,1.05f})for(int n=0;n<32;++n){const float yaw=n*6.2831853f/32;
   const auto d=g.State().CameraDistance(g.State().Player(),g.State().WorldHeight()+1.4f,{-std::sin(yaw)*std::cos(pitch),-std::cos(yaw)*std::cos(pitch)},std::sin(pitch),10.f);
   if(d>clearance){clearance=d;viewYaw=yaw;viewPitch=pitch;}}
  Check(clearance>=5,"World fixture lacks a clear review orbit");
  g.SetCameraOrbit(viewYaw,viewPitch,10.f);r.BeginFrame();g.Render(r);
  Capture(r,out/("world-wide-"+std::to_string(int(id))+"-"+std::to_string(ability)+".png"));
  Check(g.RestoreSnapshot(Hero(id).Save()) && g.Effects().BurstCount()==0 && g.PlayerAnimation().authoredStrikeWeight==0,"Load retained signature motion/effects");
 }
}
}
int main(){try{
 Check(SUCCEEDED(CoInitializeEx(nullptr,COINIT_MULTITHREADED)),"COM");Motion();
 const std::filesystem::path out="signature-review";std::filesystem::create_directories(out);
 Win32Window w;Check(w.Create(GetModuleHandleW(nullptr),960,600,L"Signature checks",false),"Window");Renderer r;Check(r.Initialize(w.GetHandle(),960,600),"Renderer");
 Native(r,out);Live(r,out);Check(r.ValidateDebugMessages(),"D3D diagnostics");
 std::cout<<"PASS: ability choreography, anchored feet, distinct native signatures, meteor endpoint, fan arrows, one primary area event/audio, actual four-class combat and load isolation.\n";
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
