#include "game/Game.h"
#include "game/CharacterPose.h"
#include <cstdlib>
#include <iostream>
#include <limits>

namespace {
void Require(bool value,const char* message) {
  if (!value) { std::cerr<<message<<'\n'; std::exit(1); }
}
bool Finite(DirectX::XMFLOAT3 p) { return std::isfinite(p.x) && std::isfinite(p.y) && std::isfinite(p.z); }
float Distance(DirectX::XMFLOAT3 a,DirectX::XMFLOAT3 b) {
  return std::sqrt((a.x-b.x)*(a.x-b.x)+(a.y-b.y)*(a.y-b.y)+(a.z-b.z)*(a.z-b.z));
}
void TransitionRegressions() {
  bool passed=true;
  const auto check=[&](bool value,const char* message) {
    if (!value) { std::cerr<<"Transition: "<<message<<'\n'; passed=false; }
  };
  const char* duel="MEDITERRANEAN 3\n-4 5.8 100 0 0 0\n2 0 0 0\n0\n0 0 0 0\n"
    "100 -4 7 80 0 0 1 0 3.14159265\n101 4 9 80 0 0 0 0 0\n102 0 12 120 0 0 0 0 0\n";
  Game game; Require(game.RestoreSnapshot(duel),"Variable-step recoil fixture must load.");
  rpg::Command strike; strike.nextTarget=true; strike.ability=0;
  game.ExecuteCommand(strike,.01f);
  check(game.State().Enemies()[0].damagedThisUpdate,"A successful strike must publish an impact event.");
  game.ExecuteCommand({},.01f);
  check(!game.State().Enemies()[0].damagedThisUpdate,"An impact event must not repeat on the next update.");
  const float beforeHit=game.EnemyAnimation(0).hit,health=game.State().Enemies()[0].health;
  strike.nextTarget=false; strike.ability=1; game.ExecuteCommand(strike,.05f);
  Require(beforeHit>0 && game.State().Enemies()[0].health<health,"Second-hit fixture must damage a living enemy.");
  check(game.State().Enemies()[0].damagedThisUpdate,"A second strike must publish its own impact event.");
  rpg::Simulation restored;
  const auto hitSave=game.State().Save();
  Require(restored.Load(hitSave),"Impact-time save must load.");
  check(restored.Save()==hitSave && !restored.Enemies()[0].damagedThisUpdate && !restored.DamagedThisUpdate(),
    "Transient impact events must not enter the save format or replay on load.");
  check(std::abs(game.EnemyAnimation(0).hit-beforeHit)<.0001f,
    "A hit on a longer frame must restart recoil while preserving its current amplitude.");
  game.ExecuteCommand({},.05f);
  check(game.EnemyAnimation(0).hit>.98f,"The second impact must reach its own recoil peak.");
  game.ExecuteCommand(strike,.01f);
  check(!game.State().Enemies()[0].damagedThisUpdate,"A rejected cooldown command must not emit a damage event.");

  const char* fatal="MEDITERRANEAN 3\n-4 5.8 5 0 0 0\n2 0 0 0\n0\n0 0 0 0\n"
    "100 -4 7 80 0 0 1 0.08 3.14159265\n101 4 9 80 0 0 0 0 0\n102 0 12 120 0 0 0 0 0\n";
  Require(game.RestoreSnapshot(fatal),"Fatal counterattack fixture must load.");
  strike.nextTarget=true; strike.ability=0; game.ExecuteCommand(strike,.01f);
  for (int i=0;i<3;++i) game.ExecuteCommand({},.05f);
  Require(game.State().Health()==0 && game.State().Enemies()[0].hitFlash>0,
    "Fatal counterattack must leave a living enemy with an unfinished damage flash.");
  for (int i=0;i<40;++i) game.ExecuteCommand({},.05f);
  check(game.EnemyAnimation(0).hit==0,"A frozen damage flash after player defeat must not replay recoil forever.");
  check(game.State().Enemies()[0].hitFlash==0,"Enemy damage tint must fade after player defeat.");
  check(game.EnemyPose(0).attack<.0001f,
    "A living enemy's finishing strike must settle after player defeat, not freeze at full extension.");
  const auto defeatedSave=game.State().Save();
  Require(game.RestoreSnapshot(defeatedSave),"Defeated encounter must reload.");
  check(game.EnemyPose(0).attack==0,
    "Loading a defeated encounter must not restore a frozen enemy strike pose.");
  check(!game.State().DamagedThisUpdate() && !game.State().Enemies()[0].damagedThisUpdate,
    "Defeat updates must clear old impact events.");

  const float expected=animation::Gesture(.8f-.6f,.09f,.42f);
  for (int hz : {30,60,144,0}) {
    game.Initialize(); rpg::Command jump; jump.jump=true;
    double elapsed=0; int steps=0,contacts=0;
    while (elapsed<.8-1e-8) {
      const double varied[]={.05,.004,.011,.027};
      const float dt=static_cast<float>(std::min(hz?1.0/hz:varied[steps++%4],.8-elapsed));
      game.ExecuteCommand(elapsed==0?jump:rpg::Command{},dt); elapsed+=dt;
      if (game.State().LandingSpeed()>0) {
        ++contacts;
        check(std::abs(elapsed-game.State().LandingElapsed()-.6)<.00001 &&
          std::abs(game.State().LandingSpeed()-7.2f)<.0001f,
          "Ground contact must report the actual impact time and speed.");
        const auto landingSave=game.State().Save();
        Require(restored.Load(landingSave),"Landing-time save must load.");
        check(restored.Save()==landingSave && restored.LandingSpeed()==0 && restored.LandingElapsed()==0,
          "Loading must discard the last frame's landing event.");
      }
    }
    check(contacts==1,"Each jump must emit exactly one landing event, including uneven steps.");
    const float actual=game.PlayerAnimation().landing;
    std::cout<<"Landing at 0.8 s / "<<hz<<" Hz: "<<actual<<" (expected "<<expected<<")\n";
    check(std::abs(actual-expected)<.001f,"Landing timing and impact strength must not depend on frame rate.");
  }
  Require(passed,"Animation transition regressions failed.");
}
}
int main() {
  TransitionRegressions();
  for(int hz : {30,60,144}) {
    Game caster;
    Require(caster.RestoreSnapshot("MEDITERRANEAN 3\n3 -10 50 0 0 0\n2 0 0 0\n0\n0 0 0 0\n"
      "100 -4 7 80 0 0 0 0 0\n101 4 9 80 0 0 0 0 0\n102 0 12 120 0 0 0 0 0\n"),"Cast fixture must load");
    rpg::Command mend; mend.ability=3; caster.ExecuteCommand(mend,.01f);
    Require(caster.State().Health()==85,"Spell pose must not delay authoritative healing");
    float elapsed=0;
    while(elapsed<.36f-1e-6f) {
      const float dt=std::min(1.f/hz,.36f-elapsed); caster.ExecuteCommand(mend,dt); elapsed+=dt;
    }
    Require(std::abs(caster.PlayerAnimation().cast-animation::Gesture(.36f,.18f,.72f))<.0001f,
      "Cast recovery must agree across frame rates; rejected cooldown input must not restart it");
    const auto pose=BuildCharacterPose(0,0,false,caster.PlayerAnimation());
    Require(pose.arms[0].end.y>1.2f,"Accepted Mend must raise the casting hand");
    const auto save=caster.State().Save();
    Require(caster.RestoreSnapshot(save) && caster.PlayerAnimation().cast==0,"Loading must clear the casting gesture");
    for(int i=0;i<hz;++i) caster.ExecuteCommand({},1.f/hz);
    Require(caster.PlayerAnimation().cast==0,"An inactive spell must not replay its gesture");
  }
  Require(animation::Gesture(.0001f,.09f,.42f)/.0001f<.04f &&
    animation::Gesture(.4199f,.09f,.42f)/.0001f<.01f,
    "Landing compression must start and end without an abrupt pose velocity");
  const auto neutral=BuildCharacterPose(0,0,false),tinyStep=BuildCharacterPose(0,.0001f,false);
  Require((tinyStep.legs[1].end.y-neutral.legs[1].end.y)/.0001f<.001f,
    "Foot lift must begin with zero slope at the grounded half of the stride.");
  animation::State lowAir,highAir;
  lowAir.airBlend=highAir.airBlend=1;
  lowAir.Update(.001f,.2499f,0,0,true,false,0); highAir.Update(.001f,.25f,0,0,true,false,0);
  Require((highAir.pose.air-lowAir.pose.air)/.0001f<.1f,
    "Air pose must join its full-height plateau without a sharp velocity corner.");
  // Fixed-length joints, planted idle/landing ankles and attached weapon safety
  // across extreme combinations, including simultaneous attack and impact.
  for (float air : {0.f,.5f,1.f}) for (float rise : {-1.f,0.f,1.f})
    for (float landing : {0.f,1.f}) for (float hit : {0.f,1.f}) for (float breath : {-1.f,1.f})
    for (float gait : {-1.f,0.f,1.f}) for (float swing : {0.f,.5f,1.f}) for (bool shield : {false,true})
    for (float lean : {-1.f,0.f,1.f}) for(float cast : {0.f,1.f}) {
    animation::Sample sample{breath,breath,air,rise,landing,hit,1}; sample.lean=lean; sample.cast=cast;
    const auto pose=BuildCharacterPose(swing,gait,shield,sample);
    for (int side=0;side<2;++side) {
      const auto& arm=pose.arms[side]; const auto& leg=pose.legs[side];
      Require(Finite(arm.root) && Finite(arm.joint) && Finite(arm.end) && Finite(leg.joint) && Finite(leg.end),"Animation produced a non-finite joint.");
      Require(std::abs(Distance(arm.root,arm.joint)-CharacterPose::upperArm)<.0001f &&
        std::abs(Distance(arm.joint,arm.end)-CharacterPose::forearm)<.0001f &&
        std::abs(Distance(leg.root,leg.joint)-CharacterPose::thigh)<.0001f &&
        std::abs(Distance(leg.joint,leg.end)-CharacterPose::shin)<.0001f,"Animation stretched a limb.");
      Require(leg.end.y>=.1699f,"Animation pushed a foot below the standing floor.");
      if (air==0 && gait==0) Require(std::abs(leg.end.y-.17f)<.0001f,"Idle/landing ankle must stay grounded while the knees bend.");
    }
    Require(std::abs(Distance(pose.weaponDirection,{0,0,0})-1)<.0001f,"Weapon orientation lost normalization.");
    Require(pose.arms[1].end.y+pose.weaponDirection.y*.96f>.08f,"Animation drove the sword tip through the floor.");
    const auto shoulder=pose.UpperPoint({.305f,1.43f,0});
    Require(Distance(shoulder,pose.arms[1].root)<.0001f,"Torso transform detached the arm root.");
  }
  for (float invalid : {std::numeric_limits<float>::quiet_NaN(),std::numeric_limits<float>::infinity(),-100.f,100.f}) {
    const auto pose=BuildCharacterPose(invalid,invalid,true,{invalid,invalid,invalid,invalid,invalid,invalid,invalid});
    Require(Finite(pose.arms[0].end) && Finite(pose.legs[1].end) && Finite(pose.UpperPoint({0,1.76f,0})),"Invalid animation channels must sanitize safely.");
  }
  // Full dash and braking poses retain fixed limbs and a clear weapon tip.
  for (float phase : {0.f,.1f,.22f,.45f,.68f,.9f,1.f}) for (bool shield : {false,true})
    for (float air : {0.f,1.f}) for (float impact : {0.f,1.f}) for (float swing : {0.f,1.f}) {
    animation::Sample sample; sample.dash=motion::DashSpeed(phase);
    sample.dashBrake=animation::Gesture(phase,.68f,1); sample.dashImpact=impact; sample.air=air; sample.cast=1;
    const auto pose=BuildCharacterPose(swing,0,shield,sample);
    for (int side=0;side<2;++side) {
      const auto& leg=pose.legs[side]; const auto& arm=pose.arms[side];
      Require(Finite(leg.end) && Finite(arm.end) && leg.end.y>=.1699f,"Burst poses must remain finite and above the floor");
      Require(std::abs(Distance(leg.root,leg.joint)-CharacterPose::thigh)<.0001f &&
        std::abs(Distance(leg.joint,leg.end)-CharacterPose::shin)<.0001f &&
        std::abs(Distance(arm.root,arm.joint)-CharacterPose::upperArm)<.0001f &&
        std::abs(Distance(arm.joint,arm.end)-CharacterPose::forearm)<.0001f,"Dash poses must not stretch limbs");
    }
    Require(pose.WeaponPoint(.96f).y>.08f,"The forward lean must keep the blade out of the floor");
  }
  // Breathing and the half-frequency sway must not snap at a phase wrap.
  for (float time : {motion::tau/1.8f,motion::tau*2/1.8f,100.f}) {
    const auto before=animation::Idle(time-.0001f,1.7f),after=animation::Idle(time+.0001f,1.7f);
    Require(std::abs(before.breath-after.breath)<.001f && std::abs(before.sway-after.sway)<.001f,"Idle animation snaps at a phase boundary.");
  }
  animation::Sample idleReference;
  for (int hz : {30,60,144}) {
    animation::State state;
    for (int i=0;i<hz*8;++i) state.Update(1.f/hz,0,0,0,true,0,0);
    if (hz!=30) Require(std::abs(state.pose.breath-idleReference.breath)<.0001f &&
      std::abs(state.pose.sway-idleReference.sway)<.0001f,"Idle rate depends on frame rate.");
    idleReference=state.pose;
    Game game; game.Initialize(); rpg::Command jump; jump.jump=true;
    bool rising=false,falling=false,landed=false; float peakLanding=0;
    for (int i=0;i<hz*2;++i) {
      game.ExecuteCommand(i==0?jump:rpg::Command{},1.f/hz);
      const auto sample=game.PlayerAnimation();
      if (game.State().Height()>.3f && game.State().VerticalSpeed()>1) rising=rising || sample.air>.1f;
      if (game.State().Height()>.3f && game.State().VerticalSpeed()<-1) falling=falling || sample.rise<0;
      if (i>hz/2 && game.State().Height()==0) {
        landed=true; Require(sample.air==0,"Grounded character retained floating jump feet.");
        peakLanding=std::max(peakLanding,sample.landing);
      }
    }
    Require(rising && falling && landed && peakLanding>.9f && peakLanding<=1,"Jump must traverse rise, fall and bounded landing compression.");
    Require(game.PlayerAnimation().landing==0 && game.PlayerAnimation().air==0,"Landing must settle completely.");
    const auto save=game.State().Save();
    game.ExecuteCommand(jump,.05f); Require(game.PlayerAnimation().air>0,"Takeoff must start the air pose.");
    Require(game.RestoreSnapshot(save) && game.PlayerAnimation().air==0 && game.PlayerAnimation().landing==0 &&
      game.PlayerAnimation().hit==0 && game.State().Save()==save,"Load must reset animation without modifying durable progress.");
    game.ExecuteCommand(jump,.05f); rpg::Command restart; restart.restart=true; game.ExecuteCommand(restart,.05f);
    Require(game.PlayerAnimation().air==0 && game.PlayerAnimation().travel==0,"Restart must discard the previous pose.");
  }
  animation::State hit;
  for(int hz : {30,60,144}) {
    Game weighted; weighted.Initialize(); rpg::Command run; run.movement={0,1};
    weighted.ExecuteCommand(run,1.f/hz);
    Require(weighted.PlayerAnimation().lean>0,"Acceleration must lean into forward travel");
    for(int i=0;i<hz;++i) weighted.ExecuteCommand(run,1.f/hz);
    for(int i=0;i<hz/10;++i) weighted.ExecuteCommand({},1.f/hz);
    Require(weighted.PlayerAnimation().lean<0,"Braking must counterbalance forward momentum");
    const auto snapshot=weighted.State().Save();
    Require(weighted.RestoreSnapshot(snapshot) && weighted.PlayerAnimation().lean==0,"Loading must reset inertial lean");
    rpg::Command jump; jump.jump=true; float peakHeight=0,peakCamera=0;
    for(int i=0;i<hz*2;++i) {
      weighted.ExecuteCommand(i==0?jump:rpg::Command{},1.f/hz);
      peakHeight=std::max(peakHeight,weighted.State().Height());
      peakCamera=std::max(peakCamera,weighted.CameraHeight());
    }
    Require(peakHeight>1 && peakCamera>1.6f && peakCamera<1.8f,
      "Jump camera must retain some vertical tracking without following the full jump arc");
    Require(std::abs(weighted.CameraHeight()-1.4f)<.0001f,"Jump camera must settle back to its ground target");
  }
  hit.Update(.01f,0,0,0,true,true,0);
  for (int i=0;i<5;++i) hit.Update(.01f,0,0,0,true,false,0);
  Require(hit.pose.hit>.98f,"A hit must produce a bounded recoil peak.");
  for (int i=0;i<100;++i) hit.Update(.01f,0,0,0,true,0,0);
  Require(hit.pose.hit==0,"Old hit flash must not keep retriggering recoil.");
  hit.Update(.01f,0,0,0,true,true,0); hit.Update(.05f,0,0,0,true,false,0);
  Require(hit.pose.hit>.98f,"A new hit must retrigger after recovery.");
  const float recoil=hit.pose.hit;
  hit.Update(.01f,0,0,0,true,true,0);
  Require(hit.pose.hit>=recoil,"Refreshing damage must not snap active recoil back to neutral.");
  hit.Update(.01f,0,0,0,true,true,0);
  Require(hit.hitAge==0 && hit.pose.hit>=recoil,"Consecutive damage events must register as fresh impacts.");
  const auto unchanged=hit.pose;
  for (float dt : {0.f,-1.f,std::numeric_limits<float>::quiet_NaN()}) hit.Update(dt,1,6,1,true,1,1);
  Require(hit.pose.hit==unchanged.hit && hit.pose.air==unchanged.air,"Invalid elapsed time must not advance animation.");
  hit.Update(.01f,0,0,0,false,1,1);
  Require(hit.pose.hit==0 && hit.pose.breath==0 && hit.pose.landing==0,"Defeat must clear reactions and breathing.");
  animation::State repeatJump;
  repeatJump.Update(.01f,0,0,0,true,false,0,7.2f); repeatJump.Update(.05f,0,0,0,true,false,0);
  repeatJump.Update(.04f,0,0,0,true,false,0);
  Require(repeatJump.pose.landing>.8f,"Landing fixture must compress.");
  repeatJump.Update(.01f,0,7.2f,0,true,0,0);
  Require(repeatJump.pose.landing==0,"A takeoff exactly at floor contact must clear the previous landing.");
  const char* encounter="MEDITERRANEAN 3\n-4 5.8 100 0 0 0\n2 0 0 0\n0\n0 0 0 0\n"
    "100 -4 7 80 0 0 1 0 3.14159265\n101 4 9 80 0 0 0 0 0\n102 0 12 120 0 0 0 0 0\n";
  Game game; rpg::Simulation reference;
  Require(game.RestoreSnapshot(encounter) && reference.Load(encounter),"Animation encounter fixture must load.");
  bool enemyRecoil=false,playerRecoil=false;
  for (int i=0;i<180;++i) {
    rpg::Command command; if (i==0) command.nextTarget=true; if (i==1) command.ability=0;
    const float previousHealth=game.State().Health();
    game.ExecuteCommand(command,1.f/60); reference.Update(1.f/60,command);
    Require(game.State().DamagedThisUpdate()==(game.State().Health()<previousHealth),
      "Player damage events must match actual hits, not the duration of the flash.");
    Require(game.State().Save()==reference.Save(),"Animation must not change authoritative combat or persistent state.");
    Require(game.EnemyPose(0).facing==game.State().Enemies()[0].facing,"Recoil must not rotate the committed attack cone.");
    enemyRecoil=enemyRecoil || game.EnemyAnimation(0).hit>.5f;
    playerRecoil=playerRecoil || game.PlayerAnimation().hit>.5f;
  }
  Require(enemyRecoil,"Real enemy damage must reach the animation layer.");
  Require(playerRecoil,"Real player damage must reach the animation layer.");
  std::cout<<"PASS: animation layers, fixed joints, floor/weapon clearance, phase continuity, jump/landing, recoil and reset lifecycle.\n";
}
