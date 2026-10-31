#include "game/Game.h"
#include "game/CharacterView.h"
#include "game/HandPose.h"
#include "game/WeaponVisual.h"
#include "game/ClassClothing.h"
#include "game/ArmorPose.h"
#include "engine/Renderer.h"
#include "platform/Win32Window.h"
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <set>
#include <map>

namespace {
using namespace rpg;
using namespace DirectX;
void Check(bool ok,const char* text) {if(!ok) {std::cerr<<text<<'\n';std::exit(1);}}
float Distance(XMFLOAT3 a,XMFLOAT3 b) {return XMVectorGetX(XMVector3Length(XMLoadFloat3(&a)-XMLoadFloat3(&b)));}
std::string Fixture(ClassId id,bool veteran=false) {
  Simulation s;
  Check(s.Load(veteran?
    "MEDITERRANEAN 3\n3 -10 50 0 3 90\n5 1 1 0\n4\n0 0 0 0\n100 -4 7 0 0 1 3 0 0\n101 4 9 0 0 1 3 0 0\n102 0 12 0 0 1 3 0 0\n":
    "MEDITERRANEAN 3\n-4 5.2 50 0 0 0\n2 0 0 0\n0\n0 0 0 0\n100 -4 7 80 0 0 0 0 0\n101 4 9 80 0 0 0 0 0\n102 0 12 120 0 0 0 0 0\n"),"Legacy equipment fixture");
  auto save=s.Save();const auto p=save.find("PROFILE ");
  save.replace(p,save.find('\n',p)-p,"PROFILE "+std::to_string(int(id))+" \"Tester\" 0 0 0");return save;
}
void CatalogAndPersistence() {
  std::set<std::string> names;std::set<WeaponForm> forms;
  for(int i=0;i<int(ClassId::Count);++i) {
    const auto id=ClassId(i);forms.insert(WeaponFor(id).form);
    Simulation s;Check(s.Load(Fixture(id,true)) && s.CanTrade(),"Class veteran equipment migration");
    for(auto tier:{Item::Starter,Item::Watchguard}) {
      const auto contextual=s.ItemDefinition(tier);const auto& durable=DescribeItem(tier);
      Check(contextual.id==durable.id && contextual.slot==durable.slot && contextual.stackLimit==1 &&
        contextual.strike==durable.strike && contextual.heavy==durable.heavy && contextual.armor==durable.armor,
        "Class appearance changed durable weapon stats/slot");
      names.insert(contextual.name);
      Command equip;equip.inventory={InventoryAction::Equip,s.Items().Find(tier),int(GearSlot::Weapon),tier};
      if(s.Items().Worn(GearSlot::Weapon)!=tier) s.Update(.01f,equip);
      Check(s.Items().Worn(GearSlot::Weapon)==tier,"Class weapon could not be equipped");
      const auto outfit=CharacterOutfit::From(s.Items(),id);
      Check(outfit.classId==id && outfit.weapon && outfit.rewardWeapon==(tier==Item::Watchguard),"World/portrait outfit lost class/tier");
      Command remove;remove.inventory={InventoryAction::Unequip,int(GearSlot::Weapon),-1,tier};s.Update(.01f,remove);
      Check(!CharacterOutfit::From(s.Items(),id).weapon,"Unequipped class weapon remained visible");
      const int gold=s.Gold();Command sell;sell.trade={TradeAction::Sell,tier,s.Items().Find(tier)};s.Update(.01f,sell);
      Check(s.Gold()==gold+SellPrice(tier) && s.Items().Count(tier)==0,"Class weapon sell accounting");
      Check(s.Message().find(contextual.name)!=std::string::npos,"Trade message lost class weapon name");
      Command reclaim;reclaim.trade={TradeAction::Buyback,tier,-1};s.Update(.01f,reclaim);
      Check(s.Gold()==gold && s.Items().Count(tier)==1,"Class weapon buyback accounting");
      Simulation copy;Check(copy.Load(s.Save()) && copy.Save()==s.Save() && copy.Class()==id,"Equipment save round-trip changed durable data");
    }
  }
  Check(names.size()==22 && forms.size()==11,"Every class/tier needs its own identity");
  Check(WeaponFor(ClassId::Count).form==WeaponForm::Blade,"Invalid class fallback escaped catalog");
}
void LaunchContracts() {
  Check(Distance(CombatEffects::SpellTracePoint({0,3,0},{2,4},.5f,0),{1,2.125f,2})<.000001f,
    "Physical traces must follow their muzzle-to-target line without a magic arc");
  for(auto id:{ClassId::Scout,ClassId::Engineer}) for(int hz:{30,60,144,1000}) {
    Simulation s;Check(s.Load(Fixture(id)),"Launcher fixture");
    Command remove;remove.inventory={InventoryAction::Unequip,int(GearSlot::Weapon),-1,Item::Starter};s.Update(.001f,remove);
    Command fire;fire.ability=0;fire.nextTarget=true;s.Update(.001f,fire);
    Check(!s.Attacking() && s.Cooldown(0)==0 && std::string(s.AbilityStatus(0))=="NO WEAPON","Unarmed launcher fired/spent cooldown");
    Command equip;equip.inventory={InventoryAction::Equip,s.Items().Find(Item::Starter),int(GearSlot::Weapon),Item::Starter};s.Update(.001f,equip);
    fire.nextTarget=false;s.Update(.001f,fire);Check(s.Attacking(),"Re-equipped launcher did not fire");
    s.Update(.001f,remove);Check(!s.Attacking(),"Removing launcher during preparation retained a phantom shot");
    Check(s.Load(Fixture(id)),"Reload launcher");fire.nextTarget=true;s.Update(.001f,fire);
    int launches=0,hits=0;float time=0;
    while(time<.4f-1e-6f) {
      const float dt=std::min(1.f/hz,.4f-time);s.Update(dt,{});time+=dt;
      for(std::size_t n=0;n<s.VisualEventCount();++n) {
        launches+=s.VisualEvents()[n].kind==CombatVisualKind::SpellCast;
        hits+=s.VisualEvents()[n].kind==CombatVisualKind::Hit;
      }
    }
    Check(launches==1 && hits==1,"Physical release/contact differed with refresh rate");
    Check(s.Load(Fixture(id)),"Reload launched-shot fixture");s.Update(.001f,fire);
    for(int n=0;n<10;++n) s.Update(.01f,{});
    s.Update(.001f,remove);Check(s.Attacking(),"Removing launcher recalled an already released projectile");
    for(int n=0;n<10;++n) s.Update(.01f,{});
    Check(s.Enemies()[0].health<80,"Released projectile lost its committed damage");
  }
}
void ShapesAndSockets() {
  float minimumSkin=1;
  const auto pose=BuildCharacterPose(0,0,false);
  const auto hand=BuildHandPose(pose,1,true,false,0);const auto palette=BuildHandRig(hand);
  for(int i=1;i<int(ClassId::Count);++i) {
    const auto form=WeaponFor(ClassId(i)).form;const bool focused=oldstyle::Focused(ClassId(i));
    const auto mesh=visual::BuildMesh(focused?oldstyle::Mesh(ClassId(i),5):WeaponMesh(form));
    std::cout<<WeaponFor(ClassId(i)).starter<<": "<<mesh.indices.size()/3<<" triangles\n";
    Check(!mesh.vertices.empty() && mesh.indices.size()/3<(focused?16000u:1300u),"Weapon mesh missing/too expensive");
    // All grips share the same conservative .025 m clearance envelope.
    for(const auto& v:mesh.vertices) if(std::abs(v.position.y)<.085f && std::abs(v.position.z)<.04f && !(focused&&form==WeaponForm::Handbow&&v.color.w==6))
      Check(std::hypot(v.position.x,v.position.z)<=.0256f,"Fitted handle exceeds the skinned hand's grip envelope");
    for(float t:{0.f,.04f,.09f,.15f,.21f,.30f,.50f,.76f}) for(bool shield:{false,true}) for(int layer=0;layer<4;++layer) {
      animation::Sample sample;sample.action=animation::SampleAction(ClassId(i),animation::ActionClip::Heavy,t);
      if(!animation::RangedClass(ClassId(i))) {sample.attackSeconds=t;sample.heavyAttack=true;}
      if(layer==1) {sample.air=.8f;sample.rise=.5f;}
      if(layer==2) sample.landing=.65f;
      if(layer==3) {sample.dash=.75f;sample.dashBrake=.4f;sample.dashPhase=.4f;}
      const auto posed=BuildCharacterPose(0,.35f,shield,sample,form);const auto frame=WeaponFrame(posed);
      Check(std::abs(XMVectorGetX(XMMatrixDeterminant(frame))-1)<.00001f,"Weapon frame changed scale/handedness");
      for(const auto& v:mesh.vertices) {
        XMFLOAT3 point;XMStoreFloat3(&point,XMVector3TransformCoord(XMLoadFloat3(&v.position),frame));
        Check(point.y>.04f && Distance(point,posed.arms[1].end)<1.1f,"Equipped weapon penetrated floor or escaped grip bounds");
      }
      if(PhysicalLauncher(form)) {
        const XMFLOAT3 local{0,form==WeaponForm::Pistol?.16f:.035f,form==WeaponForm::Pistol?.40f:.78f};XMFLOAT3 muzzle;
        XMStoreFloat3(&muzzle,XMVector3TransformCoord(XMLoadFloat3(&local),frame));
        Check(Distance(muzzle,WeaponReleasePoint(posed,form))<.000001f,"Physical shot detached from its visible muzzle");
        Check(Distance(muzzle,SpellReleasePoint(posed,shield))>.20f,"Physical shot originated at the casting palm");
      }
    }
  }
  // Sample triangle interiors, including the coarse far hand, against the
  // shared grip cylinder instead of checking only the finger joint chain.
  for(unsigned level=0;level<3;++level) {
    const auto& skin=visual::CharacterHand(level,hand.mirrored);std::vector<XMFLOAT3> points;
    for(std::size_t n=0;n<skin.mesh.vertices.size();++n)
      points.push_back(visual::SkinHandVertex(skin.mesh.vertices[n],skin.influences[n],palette).position);
    for(std::size_t n=0;n<skin.mesh.indices.size();n+=3) {
      const auto a=XMLoadFloat3(&points[skin.mesh.indices[n]]),b=XMLoadFloat3(&points[skin.mesh.indices[n+1]]),c=XMLoadFloat3(&points[skin.mesh.indices[n+2]]);
      for(int u=0;u<=6;++u) for(int v=0;v<=6-u;++v) {
        XMFLOAT3 p;XMStoreFloat3(&p,(a*float(6-u-v)+b*float(u)+c*float(v))/6);
        if(std::abs(p.x)<.085f) minimumSkin=std::min(minimumSkin,std::hypot(p.y,p.z)-.0256f);
      }
    }
  }
  Check(minimumSkin>=.0002f,"Class weapon handle intersects a skinned palm/finger");
  std::cout<<"Minimum class grip clearance: "<<minimumSkin<<" m\n";
}
void Captures() {
  const auto root=std::filesystem::current_path()/"class-equipment-check";std::filesystem::create_directories(root);
  Win32Window window;Check(window.Create(GetModuleHandleW(nullptr),1280,720,L"Class equipment checks",false),"Equipment window");
  Renderer renderer;Check(renderer.Initialize(window.GetHandle(),1280,720),"Equipment renderer");
  for(int i=0;i<int(ClassId::Count);++i) {
    const auto id=ClassId(i);Simulation hero;Check(hero.CreateCharacter(id,"Tester"),"Capture hero");
    const auto path=root/(i?"hero-"+std::to_string(i)+".save":"patrol.save");
    {std::ofstream out(path);out<<hero.Save();Check(bool(out),"Capture save fixture");}
    for(bool action:{false,true}) {
      animation::Sample sample;if(action) {
        sample.action=animation::SampleAction(id,animation::ActionClip::Heavy,combat::StrokeStart(true));
        if(!animation::RangedClass(id)) {sample.attackSeconds=combat::Contact(true);sample.heavyAttack=true;}
      }
      const auto& color=hero.ClassInfo().color;
      renderer.BeginFrame();renderer.SetCamera({2.2f,1.65f,3.5f},{0,1.05f,0});
      DrawCharacter(renderer,{0,0},0,0,{color[0],color[1],color[2],1},0,0,CharacterOutfit::From(hero.Items(),id),sample,1);
      Check(renderer.Capture((root/(std::to_string(i)+(action?"-action.png":"-idle.png"))).wstring()) && renderer.EndFrame(false),"Equipment world capture");
    }
    for(int variant=0;variant<4;++variant) {
      auto outfit=CharacterOutfit::From(hero.Items(),id);
      animation::Sample sample;
      if(variant==0) outfit.appearance.dye=2;
      if(variant==1) {outfit.tunic=false;outfit.shoulders=false;outfit.belt=false;}
      if(variant==2) {sample.air=.8f;sample.rise=.6f;}
      if(variant==3) {sample.dash=.8f;sample.dashPhase=.4f;sample.dashBrake=.3f;}
      const auto& color=hero.ClassInfo().color;
      renderer.BeginFrame();renderer.SetCamera({2.2f,1.65f,3.5f},{0,1.05f,0});
      DrawCharacter(renderer,{0,0},0,0,{color[0],color[1],color[2],1},0,.3f,outfit,sample,1);
      Check(renderer.Capture((root/(std::to_string(i)+"-clothing-"+std::to_string(variant)+".png")).wstring()) &&
        renderer.EndFrame(false),"Clothing dye/removal/air/dash capture");
    }
    Game game;game.Initialize();Check(game.RestoreSnapshot(Fixture(id,true)),"Equipment UI fixture");
    game.OpenPanel(ui::Panel::Character);game.ShowPanel(ui::Panel::Inventory);renderer.BeginFrame();game.Render(renderer);
    Check(renderer.Capture((root/(std::to_string(i)+"-inventory.png")).wstring()) && renderer.EndFrame(false),"Class inventory/portrait capture");
    Check(game.RestoreSnapshot(Fixture(id)),"Class HUD fixture");game.OpenPanel(ui::Panel::None);
    Command basic;basic.nextTarget=true;basic.ability=0;game.ExecuteCommand(basic,.001f);
    for(int tick=0;tick<30;++tick) game.ExecuteCommand({},.01f);
    renderer.BeginFrame();game.Render(renderer);
    Check(renderer.Capture((root/(std::to_string(i)+"-hud.png")).wstring()) && renderer.EndFrame(false),"Class resource HUD capture");
    if(PhysicalLauncher(WeaponFor(id).form)) {
      Check(game.RestoreSnapshot(Fixture(id)),"Combat capture fixture");game.OpenPanel(ui::Panel::None);
      game.SetCameraOrbit(1.4f,.25f,4.2f);
      Command attack;attack.nextTarget=true;attack.ability=1;game.ExecuteCommand(attack,.01f);
      for(int tick=0;tick<23;++tick) game.ExecuteCommand({},.01f);
      renderer.BeginFrame();game.Render(renderer);
      Check(renderer.Capture((root/(std::to_string(i)+"-shot.png")).wstring()) && renderer.EndFrame(false),"Weapon release combat capture");
    }
    game.ChooseCharacter(root/"patrol.save");
    Input input;Input::Frame frame;frame.focused=true;frame.width=1280;frame.height=720;
    const ui::Layout layout(1280,720);const auto rect=ui::CharacterSelection::Bounds(ui::CharacterSelection::Control(int(ui::CharacterSelection::Control::Class0)+i));
    frame.client={LONG(layout.x+(rect.x+rect.w/2)*layout.scale),LONG(layout.y+(rect.y+rect.h/2)*layout.scale)};
    input.Update(frame);game.Update(.016,input,0);frame.down[VK_LBUTTON]=true;input.Update(frame);game.Update(.016,input,0);
    frame.down[VK_LBUTTON]=false;input.Update(frame);game.Update(.016,input,0);
    Check(game.Selection().Hero().Class()==id,"Roster capture selected wrong class");
    renderer.BeginFrame();game.Render(renderer);
    Check(renderer.Capture((root/(std::to_string(i)+"-roster.png")).wstring()) && renderer.EndFrame(false),"Class roster capture");
  }
  Check(renderer.ValidateDebugMessages(),"Class weapon D3D debug errors");
  std::cout<<"Immutable GPU mesh bytes: "<<renderer.MeshBytes()<<'\n';
}
void ClothingContracts() {
  std::set<std::string> silhouettes;
  for(int i=0;i<int(ClassId::Count);++i) {
    Simulation hero;Check(hero.CreateCharacter(ClassId(i),"Tester"),"Clothing hero");
    auto outfit=CharacterOutfit::From(hero.Items(),ClassId(i));
    const auto all=clothing::Build(outfit);
    unsigned cuirasses=0;
    for(unsigned n=0;n<all.count;++n) cuirasses+=all.parts[n].mesh==visual::Mesh::OldWarriorChest;
    Check(cuirasses==(ClassId(i)==ClassId::Warrior?1u:0u),"Guard harness must belong only to the equipped Warrior chest");
    Check(all.count>0 && all.count<48,"Class fittings missing or over budget");
    std::string signature;
    for(unsigned n=0;n<all.count;++n) {
      const auto& p=all.parts[n];
      Check((visual::IsClassArt(p.mesh)?p.position.y>=0:p.position.y>.65f) && p.position.y<1.7f && std::abs(p.position.x)<.5f &&
        p.scale.x>0 && p.scale.y>0 && p.scale.z>0,"Clothing escaped torso envelope");
      signature+=std::to_string(int(p.mesh))+":"+std::to_string(p.position.x)+":"+
        std::to_string(p.position.y)+":"+std::to_string(p.scale.x)+":"+std::to_string(p.scale.y)+";";
    }
    silhouettes.insert(signature);
    outfit.tunic=false;outfit.shoulders=false;outfit.belt=false;
    Check(clothing::Build(outfit).count==0,"Unequipped fittings remain visible");
    const unsigned shoulderParts=oldstyle::Focused(ClassId(i))?2u:8u;
      outfit.shoulders=true;Check(clothing::Build(outfit).count==shoulderParts,"Shoulder ownership depends on chest equipment");
    outfit.appearance.dye=2;const auto dyed=clothing::Build(outfit);
    Check(dyed.count==shoulderParts,"Dye changed garment topology");
  }
  Check(silhouettes.size()==std::size_t(ClassId::Count),"Class clothing shares identical geometry");
}
void ShoulderArticulation() {
  CharacterPose pose{};
  for(unsigned arm=0;arm<2;++arm) {
    const float side=arm==0?1.f:-1.f;
    pose.arms[arm].root={side*.305f,1.43f,0};
    pose.arms[arm].joint={side*.305f,1.15f,0};
    XMFLOAT3 origin;
    XMStoreFloat3(&origin,XMVector3TransformCoord(XMVectorZero(),GuardianShoulderTransform(pose,arm)));
    Check(Distance(origin,{side*.34f,1.48f,0})<.00001f,"Resting shoulder lost authored socket");
  }
  XMFLOAT3 previous{};
  for(int n=0;n<=180;++n) {
    const float angle=n*XM_PI/180;
    for(unsigned arm=0;arm<2;++arm) {
      const float side=arm==0?1.f:-1.f;
      pose.arms[arm].joint={side*(.305f+.28f*std::sin(angle)),1.43f-.28f*std::cos(angle),0};
    }
    XMFLOAT3 left,right,tip;
    XMStoreFloat3(&left,XMVector3TransformCoord(XMVectorZero(),GuardianShoulderTransform(pose,0)));
    XMStoreFloat3(&right,XMVector3TransformCoord(XMVectorZero(),GuardianShoulderTransform(pose,1)));
    Check(Distance(left,{-right.x,right.y,right.z})<.00001f,"Shoulder hinges are not mirrored");
    XMStoreFloat3(&tip,XMVector3TransformCoord(XMVectorSet(.15f,0,0,1),GuardianShoulderTransform(pose,0)));
    Check(std::isfinite(tip.x) && tip.y>1.4f && tip.y<1.7f,"Shoulder escaped bounded hinge");
    if(n) Check(Distance(tip,previous)<.01f,"Shoulder hinge snaps during arm elevation");
    previous=tip;
  }
  const auto before=GuardianShoulderTransform(pose,0);
  pose.upperPitch=.35f;pose.upperRoll=-.2f;pose.upperYaw=.7f;pose.bodyLift=.12f;pose.bodyShift=.04f;
  for(auto& arm:pose.arms) {arm.root=pose.UpperPoint(arm.root);arm.joint=pose.UpperPoint(arm.joint);}
  XMFLOAT3 local,actual;
  XMStoreFloat3(&local,XMVector3TransformCoord(XMVectorSet(.1f,.02f,0,1),before));
  XMStoreFloat3(&actual,XMVector3TransformCoord(XMVectorSet(.1f,.02f,0,1),GuardianShoulderTransform(pose,0)));
  Check(Distance(actual,pose.UpperPoint(local))<.00001f,"Torso movement applied twice to shoulder articulation");
}
}
int main() {
  CatalogAndPersistence();LaunchContracts();ShapesAndSockets();ClothingContracts();ShoulderArticulation();Captures();
  std::cout<<"PASS: class equipment identity, migration/trade, timed launcher release, fitted geometry, grips, clothing ownership and 101 GPU views\n";
}
