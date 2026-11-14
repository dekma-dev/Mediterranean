#include "game/Game.h"
#include "game/BuildLayout.h"
#include "game/SpellArtwork.h"
#include "game/ClassEffects.h"
#include "engine/Renderer.h"
#include "platform/Win32Window.h"
#include <filesystem>
#include <set>
#include <iostream>
#include <stdexcept>
#include <cmath>
namespace {
using namespace rpg;
void Check(bool ok,const char* text) {if(!ok)throw std::runtime_error(text);}
void Tick(Simulation& s,float duration,int hz=60,std::vector<CombatVisualEvent>* events=nullptr) {
 while(duration>1e-6f) {const float dt=std::min(duration,1.f/hz);s.Update(dt,{});duration-=dt;
  if(events)for(std::size_t i=0;i<s.VisualEventCount();++i)events->push_back(s.VisualEvents()[i]);}
}
Simulation Hero(ClassId id) {
 Simulation s;Check(s.Load("MEDITERRANEAN 3\n12 -3.8 50 0 3 90\n5 1 1 0\n4\n0 0 0 0\n"
  "100 -4 7 0 0 1 3 0 0\n101 4 9 0 0 1 3 0 0\n102 0 12 0 0 1 3 0 0\n"),"Legacy practice fixture");
 auto save=s.Save();const auto at=save.find("PROFILE ");
 save.replace(at,save.find('\n',at)-at,"PROFILE "+std::to_string(int(id))+" \"Tester\" 4 0 3");
 Check(s.Load(save) && s.Level()>=4,"Chain veteran fixture");
 Command c;c.selectActor=210;s.Update(.001f,c);Check(s.TargetActor() && s.TargetActor()->id==210,"Practice target selection");return s;
}
std::vector<CombatVisualEvent> Cast(Simulation& s,int ability,int hz) {
 Command c;c.ability=ability;s.Update(.001f,c);Check(s.Attacking() && s.AttackAbility()==ability,"New ability rejected");
 std::vector<CombatVisualEvent> events;Tick(s,s.Spell(ability).recovery+.01f,hz,&events);Check(!s.Attacking(),"New ability clock disagrees with metadata");return events;
}
float ContactCount(const std::vector<CombatVisualEvent>& events) {
 // Contact identity and count matter here; damage is compared using the real
 // feedback records in the paired scenarios below.
 int hits=0;for(const auto& e:events)hits+=e.kind==CombatVisualKind::Hit || e.kind==CombatVisualKind::HeavyHit;
 return float(hits);
}
void Gameplay() {
 std::set<std::uint64_t> keys;
 for(int panel=0;panel<=int(ui::Panel::Quest);++panel)for(int sides=0;sides<4;++sides)for(int spell=-1;spell<15;++spell)for(bool confirm:{false,true}) {
   ui::Interface hud;hud.panel=ui::Panel(panel);hud.characterOpen=(sides&2)!=0;hud.inventoryOpen=(sides&1)!=0;hud.selectedSpell=spell;hud.confirmTalentReset=confirm;
   Check(keys.insert(hud.StateKey()).second,"New spell inspection aliases another UI state/hit layout");
 }
 CombatChain chain;chain.Contact(100,0,0);chain.Advance(.5f);chain.Contact(100,0,0);
 Check(chain.Steps(100)==1,"Repeating one button must not build a chain");
 chain.Contact(100,12,0);Check(chain.Steps(100)==2,"Alternating basics must build a chain");
 chain.Contact(101,0,0);Check(chain.Steps(101)==1 && chain.Steps(100)==0,"Chain leaked to another target");
 chain.Contact(101,12,0);chain.Contact(101,13,0);Check(chain.Contact(101,14,0)==3 && chain.Steps(101)==0,"Three-step payoff must consume exactly once");
 for(int hz:{30,60,144}) {
  chain.Clear();chain.Contact(100,0,0);chain.Advance(4.99f);chain.Advance(1.f/hz);
  chain.Contact(100,12,1.f/hz-.005f);Check(chain.Steps(100)==2,"Subframe chain expiration changed with refresh rate");
  for(auto id:{ClassId::Warrior,ClassId::Scout,ClassId::Mage}) {
   auto s=Hero(id);Check(ContactCount(Cast(s,0,hz))==1,"First basic contact count");
   Check(s.ChainSteps()==1,"First basic must start chain");
   Check(ContactCount(Cast(s,12,hz))==1 && s.ChainSteps()==2,"Second basic must produce one contact and two steps");
   if(id==ClassId::Mage) Check(s.TargetActor()->snared>0,"Ice needles lost their slow");
   if(id==ClassId::Warrior) Check(s.Barrier()>=6,"Warrior alternation lost its guard");
   Check(ContactCount(Cast(s,13,hz))==1 && s.ChainSteps()==3,"Tactical link must extend the chain");
   const auto saved=s.Save();Simulation restored;Check(restored.Load(saved) && restored.ChainSteps()==0 && restored.Cooldown(13)>0,"Loading must retain cooldowns without restoring a free combo");
   int payoffs=0;for(const auto& e:Cast(s,14,hz)) if(e.chain) {++payoffs;Check(e.chain==3 && e.actor==210,"Payoff identity/step snapshot");}
   Check(payoffs==1 && s.ChainSteps()==0,"Finisher must spend one chain on actual contact");
   if(id==ClassId::Warrior)Check(s.Barrier()>=36,"Three-step steel finisher ward");
   if(id==ClassId::Mage)Check(s.TargetActor()->snared==0 && s.TargetActor()->rooted==0,"Thermal rupture must consume frost");
   const auto current=s.Save();Check(restored.Load(current) && restored.Save()==current,"Version 23 cooldown round trip");
   auto old=current;old.replace(0,16,"MEDITERRANEAN 22");const auto record=old.find("CHAINS ");old.erase(record,old.find('\n',record)-record+1);
   Check(restored.Load(old) && restored.Cooldown(12)==0 && restored.Cooldown(13)==0 && restored.Cooldown(14)==0,"Version 22 migration");
   auto invalid=current;const auto key=invalid.find("CHAINS ");invalid.replace(key,invalid.find('\n',key)-key,"CHAINS nan 1 1");const auto unchanged=restored.Save();
   Check(!restored.Load(invalid) && restored.Save()==unchanged,"Malformed new cooldown record must reject atomically");
   auto miss=Hero(id);Cast(miss,0,hz);Command c;c.ability=12;miss.Update(.001f,c);
   auto& dummy=const_cast<Enemy&>(miss.Dummies()[0]);dummy.position.x+=20;Tick(miss,2,hz);
   Check(miss.ClassResource()==1 && miss.ChainSteps()==1,"A missed second basic must not grant reserve or chain");
   Tick(miss,5,hz);Check(miss.ChainSteps()==0,"Expired combo remained visible");
   // Test real damage, not only the visual payoff flag. Keep target control
   // states equal to isolate the two-step multiplier from frost/exposure.
   auto plain=Hero(id),linked=Hero(id);Cast(linked,0,hz);Cast(linked,12,hz);
   auto& target=const_cast<Enemy&>(linked.Dummies()[0]);target.snared=target.rooted=target.exposed=0;
   const auto finishDamage=[&](Simulation& hero) {
     Cast(hero,14,hz);int damage=0;
     for(const auto& f:hero.Feedback()) if(f.kind==FeedbackKind::Damage) damage=f.amount;
     return damage;
   };
   const int base=finishDamage(plain),bonus=finishDamage(linked);
   Check(std::abs(float(bonus-base)-linked.HeavyDamage()*.4f)<1.1f,"Two-step finisher did not increase actual damage by 40% heavy");
   auto oldPlain=Hero(id),oldLinked=Hero(id);
   // Exercise the Scout distance bonus and Mage healthy-target bonus: neither
   // may overwrite the independently earned chain contribution.
   if(id==ClassId::Scout) {
     const_cast<Enemy&>(oldPlain.Dummies()[0]).position.z+=5;
     const_cast<Enemy&>(oldLinked.Dummies()[0]).position.z+=5;
   }
   Cast(oldLinked,0,hz);Cast(oldLinked,12,hz);
   auto& oldTarget=const_cast<Enemy&>(oldLinked.Dummies()[0]);oldTarget.snared=oldTarget.rooted=oldTarget.exposed=0;
   const auto oldDamage=[&](Simulation& hero) {
     Command attack;attack.ability=1;hero.Update(.001f,attack);Check(hero.Attacking(),"Existing payoff rejected");
     int damage=0;float duration=hero.Spell(1).recovery+.01f;
     while(duration>1e-6f) {
       const float dt=std::min(duration,1.f/hz);hero.Update(dt,{});duration-=dt;
       for(std::size_t event=0;event<hero.VisualEventCount();++event)if(hero.VisualEvents()[event].kind==CombatVisualKind::HeavyHit)
         for(const auto& f:hero.Feedback())if(f.kind==FeedbackKind::Damage)damage=f.amount;
     }
     return damage;
   };
   const int oldBase=oldDamage(oldPlain),oldBonus=oldDamage(oldLinked);
   Check(std::abs(float(oldBonus-oldBase)-oldLinked.HeavyDamage()*.24f)<1.1f && oldLinked.ChainSteps()==0,
     "Existing heavy attack overwrote or failed to consume its chain bonus");
   auto cancel=Hero(id);c={};c.ability=12;cancel.Update(.001f,c);cancel.StopMovement();Tick(cancel,2,hz);
   Check(cancel.ClassResource()==0 && cancel.ChainSteps()==0,"Cancelled basic granted resources");
   if(id!=ClassId::Warrior) {
    auto moving=Hero(id);c={};c.ability=13;moving.Update(.001f,c);c={};c.movement={.1f,0};moving.Update(.05f,c);
    Check(moving.Attacking(),"Mobile setup unexpectedly cancelled on movement");
    auto finish=Hero(id);c={};c.ability=14;finish.Update(.001f,c);c={};c.movement={.1f,0};finish.Update(.05f,c);
    Check(finish.Attacking()==(id==ClassId::Scout),"Mage finisher/Scout mobile preparation rules diverged");
   }
  }
 }
 for(int n=0;n<int(ClassId::Count);++n) {
  Simulation fresh;fresh.CreateCharacter(ClassId(n),"Tester");
  Check(std::string(fresh.AbilityStatus(13))==(AdvancedClass(ClassId(n))?"REQUIRES LEVEL 3":"DISABLED"),"Level/class gate");
 }
}
void Native(Renderer& r,const std::filesystem::path& output) {
 std::set<std::pair<bool,int>> cells;
 for(int id=0;id<int(ClassId::Count);++id)for(int ability=0;ability<(AdvancedClass(ClassId(id))?15:8);++ability) {
  const auto cell=ui::SpellArtwork(ClassId(id),ability);const auto uv=cell.Uv();
  Check(cell.tile>=0 && cells.emplace(cell.combat,cell.tile).second && uv.x>=0 && uv.y>=0 && uv.z<=1 && uv.w<=1,"Spell atlas cells alias or leave bounds");
 }
 Check(cells.size()==109,"Missing available spell illustrations");
 for(auto size:{std::pair<int,int>{960,600},{1920,1080},{2560,1080}}) {
  Check(r.Resize(size.first,size.second),"Spell review resize");const ui::Layout layout(float(size.first),float(size.second));
  const auto bars=ui::HudTransform::At(float(size.first),float(size.second),640,710);
  for(auto id:{ClassId::Warrior,ClassId::Scout,ClassId::Mage}) {
   auto state=Hero(id);ui::Interface hud;const auto saved=state.Save();
   const auto draw=[&] {r.BeginFrame();hud.Render(r,state,.5f,false);Check(r.EndFrame(false),"Hotbar edit draw");};
   const auto point=[&](float x,float y,bool click,bool held,bool release,bool right=false) {return hud.Pointer(layout.x+x*layout.scale,layout.y+y*layout.scale,float(size.first),float(size.second),click,right,held,release);};
   const auto hotbar=[&](float x,float y,bool click,bool held,bool release,bool right=false){return point(bars.X(x),bars.Y(y),click,held,release,right);};
   std::set<int> defaults;for(int i=0;i<20;++i)if(hud.HotbarSpell(i,id)>=0)Check(defaults.insert(hud.HotbarSpell(i,id)).second,"New profile has duplicated bindings");
   draw();hotbar(375,677,false,false,false,true);Check(hud.HotbarSpell(0,id)==-1,"RMB must remove disabled/no-target spells too");
   draw();hotbar(432,677,true,true,false);point(640,350,false,true,false);draw();point(640,350,false,false,true);
   Check(hud.HotbarSpell(1,id)==-1,"World drop must unbind without casting");
   hud.AssignHotbar(0,0,id);draw();hotbar(375,677,true,true,false);point(700,350,false,true,false);hud.CancelDrag();point(700,350,false,false,true);
   Check(hud.HotbarSpell(0,id)==0,"Focus-loss cancellation must preserve binding");
   draw();hotbar(375,677,true,true,false);point(640,350,false,true,false);
   hotbar(375,677,false,true,false,true);hotbar(375,677,false,false,true);
   Check(hud.HotbarSpell(0,id)==0,"RMB drag cancellation must not remove the slot beneath it");
   hud.SetOnly(ui::Panel::Inventory);draw();hotbar(375,677,true,true,false);point(1100,460,false,true,false);draw();point(1100,460,false,false,true);
   Check(hud.HotbarSpell(0,id)==0,"Dropping on another window must preserve binding");
   hud.SetOnly(ui::Panel::Build);draw();const auto card=ui::buildlayout::Spell(11);
   point(card.x+8,card.y+10,true,true,false);hotbar(375,677,false,true,false);draw();hotbar(375,677,false,false,true);
   Check(hud.HotbarSpell(0,id)==12,"Spellbook must restore a new basic attack into an empty/replaced slot");
   const auto bindings=hud.SaveHotbars();ui::Interface restored;Check(restored.LoadHotbars(bindings) && restored.SaveHotbars()==bindings,"Removed/reassigned bindings must persist");
   Check(state.Save()==saved,"Hotbar editing must never change simulation or player save");
   hud.selectedSpell=14;r.BeginFrame();hud.Render(r,state,1,false);
   Check(r.Capture((output/("book-"+std::to_string(int(id))+"-"+std::to_string(size.first)+".png")).wstring()) && r.EndFrame(false),"New spellbook screenshot");
   hud.SetOnly(ui::Panel::None);Cast(state,0,60);Cast(state,12,60);r.BeginFrame();hud.Render(r,state,2,false);
   Check(r.Capture((output/("chain-"+std::to_string(int(id))+"-"+std::to_string(size.first)+".png")).wstring()) && r.EndFrame(false),"Chain HUD screenshot");
   Check(state.Level()==Hero(id).Level(),"Interface changed progression");
  }
 }
 Check(r.InterfaceBytes()<30*1024*1024,"Unique spell art exceeded 30 MiB total UI budget");
 Check(r.Resize(960,600),"Effects resize");
 for(auto id:{ClassId::Warrior,ClassId::Scout,ClassId::Mage})for(int ability:{12,13,14}) {
  CombatVisualEvent e;e.school=id;e.ability=ability;e.heavy=ability!=12;e.position={0,0};e.height=1.1f;e.source={0,-2};e.destination={0,0};e.flight=.25f;e.chain=ability==14?3:0;
  r.BeginFrame();r.SetCamera({2,2.5f,-5},{0,1,0});classfx::Projectile(r,e,{0,1.1f,-2},.15f);classfx::Impact(r,e,.13f);
  Check(r.EffectVertices()<1600,"New effects exceeded fixed geometry budget");
  Check(r.Capture((output/("effect-"+std::to_string(int(id))+"-"+std::to_string(ability)+".png")).wstring()) && r.EndFrame(false),"Spell effect screenshot");
 }
 Check(r.ValidateDebugMessages(),"Native spell review emitted DirectX warnings");
 Check(r.Resize(1920,1080),"World spell capture resize");
 for(auto id:{ClassId::Warrior,ClassId::Scout,ClassId::Mage}) {
   const auto fixture=Hero(id);Game game;game.Initialize();Check(game.RestoreSnapshot(fixture.Save()),"Native world fixture");
   game.SetCameraOrbit(.35f,.46f,6.8f);Command select;select.selectActor=210;game.ExecuteCommand(select,.01f);
   const auto tick=[&](float duration) {while(duration>1e-6f) {const auto dt=std::min(duration,1.f/60);game.ExecuteCommand({},dt);duration-=dt;}};
   for(int ability:{0,12}) {Command c;c.ability=ability;game.ExecuteCommand(c,.001f);tick(game.State().Spell(ability).recovery+.01f);}
   r.BeginFrame();game.Render(r);
   Check(r.Capture((output/("world-chain-"+std::to_string(int(id))+".png")).wstring()) && r.EndFrame(false),"World chain screenshot");
   Command finish;finish.ability=14;game.ExecuteCommand(finish,.001f);tick(game.State().Spell(14).contact*.45f);
   r.BeginFrame();game.Render(r);Check(r.Capture((output/("world-preparation-"+std::to_string(int(id))+".png")).wstring()) && r.EndFrame(false),"World preparation screenshot");
   tick(game.State().Spell(14).contact*.55f+.075f);
   r.BeginFrame();game.Render(r);Check(r.Capture((output/("world-impact-"+std::to_string(int(id))+".png")).wstring()) && r.EndFrame(false),"World impact screenshot");
 }
}
}
int main(int argc,char** argv) {try {
 Gameplay();Win32Window window;Check(window.Create(GetModuleHandleW(nullptr),960,600,L"Spell chain review",false),"Test window");
 Renderer renderer;Check(renderer.Initialize(window.GetHandle(),960,600),"Test renderer");
 const auto output=argc>1?std::filesystem::path(argv[1]):std::filesystem::path("spell-chain-review");std::filesystem::create_directories(output);
 Native(renderer,output);std::cout<<"PASS: nine abilities, target-bound chains, hit/cancel/save rules, unique art and hotbar editing.\n";
 }catch(const std::exception& e) {std::cerr<<e.what()<<'\n';return 1;} }
