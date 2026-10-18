#include "game/Game.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace {
void Require(bool value,const char* message) {
  if (!value) { std::cerr<<message<<'\n'; std::exit(1); }
}
float Distance(rpg::Vec2 a,rpg::Vec2 b) { return std::hypot(a.x-b.x,a.z-b.z); }
const char* pursuit="MEDITERRANEAN 3\n-4 3 100 0 0 0\n2 0 0 0\n0\n0 0 0 0\n"
  "100 -4 7 80 1 0 1 0 3.14159265\n101 4 9 80 0 0 0 0 0\n102 0 12 120 0 0 0 0 0\n";
void SmoothPoseTransitions() {
  bool passed=true;
  const auto check=[&](bool value,const char* message) {
    if (!value) { std::cerr<<"Smoothness: "<<message<<'\n'; passed=false; }
  };
  motion::Actor actor;
  actor.Update(1.5f,0,true,true,1,.001f);
  check(actor.facing>0 && actor.facing<.002f,"Turning must accelerate from rest instead of starting at full angular speed.");
  check(actor.attack>0 && actor.attack<.01f,"Attack presentation must ease into motion without delaying the combat event.");
  actor.Reset(0);
  for (int i=0;i<20;++i) actor.Update(0,4.f/120,true,true,0,1.f/120);
  constexpr float dt=.0001f;
  const float before=actor.gait; actor.Update(0,4*dt,true,true,0,dt);
  const float during=actor.gait; actor.Update(0,0,true,true,0,dt);
  const float beforeVelocity=(during-before)/dt,afterVelocity=(actor.gait-during)/dt;
  std::cout<<"Gait stop velocity change: "<<std::abs(afterVelocity-beforeVelocity)<<'\n';
  check(std::abs(afterVelocity-beforeVelocity)<.3f,"Stopping must preserve pose velocity while physical travel stops immediately.");
  float reference=0;
  for (int hz : {30,60,144}) {
    actor.Reset(3.05f); float previous=std::abs(motion::AngleDelta(actor.facing,-2.1f));
    for (int i=0;i<hz/2;++i) {
      actor.Update(-2.1f,0,true,true,0,1.f/hz);
      const float error=std::abs(motion::AngleDelta(actor.facing,-2.1f));
      check(error<=previous+.000001f,"A stationary turn target must settle without oscillation."); previous=error;
    }
    check(previous<.001f,"Turning must remain responsive and settle within half a second.");
    if (hz!=30) check(std::abs(motion::AngleDelta(actor.facing,reference))<.00001f,"Turn response must agree across update rates.");
    reference=actor.facing;
  }
  actor.Reset(0);
  for (int i=0;i<15;++i) actor.Update(1.5f,4.f/120,true,true,1,1.f/120);
  const auto beforeReverse=actor;
  actor.Update(-1.5f,7*dt,true,true,0,dt);
  check(std::abs(actor.turnVelocity-beforeReverse.turnVelocity)<.3f &&
    std::abs(actor.gaitVelocity-beforeReverse.gaitVelocity)<.5f &&
    std::abs(actor.attackVelocity-beforeReverse.attackVelocity)<1,
    "Retargeting turn, sprint and attack must preserve continuous pose velocities.");
  for (int i=0;i<480;++i) {
    const float speed=i%120<60?7.f:0;
    actor.Update(i%80<40?-1.5f:1.5f,speed/120,true,true,i%60<30?1.f:0.f,1.f/120);
    check(std::isfinite(actor.facing) && std::abs(actor.gait)<=1.00001f &&
      actor.attack>=-.00001f && actor.attack<=1.00001f,
      "Repeated reversals must keep turns finite and limb channels bounded.");
  }
  actor.Update(-.7f,0,true,true,0,.01f,true);
  check(actor.facing==-.7f && actor.turnVelocity==0,"Committed enemy facing must override visual turn momentum.");
  actor.Reset(.2f);
  check(actor.gaitVelocity==0 && actor.attackVelocity==0 && actor.turnVelocity==0,
    "Reset must discard pose velocity, not just pose position.");
  Require(passed,"Smooth pose transition checks failed.");
}
void TransitionRegressions() {
  bool passed=true;
  const auto check=[&](bool value,const char* message) {
    if (!value) { std::cerr<<"Transition: "<<message<<'\n'; passed=false; }
  };
  float reference=0;
  for (int hz : {30,60,144}) {
    rpg::Simulation simulation; rpg::Command jump; jump.jump=true;
    simulation.Update(1.f/hz,jump);
    for (int i=1;i<hz/3;++i) simulation.Update(1.f/hz,{});
    check(std::abs(simulation.Height()-1.f)<.0001f,"Jump apex must be one unit at every update rate");
    motion::Actor actor;
    for (int i=0;i<hz;++i) actor.Update(0,4.f/hz,true,true,0,1.f/hz);
    if (hz!=30) check(std::abs(actor.gait-reference)<.0001f,"Walking pose must not change amplitude/phase with frame rate");
    reference=actor.gait;
  }
  Game game; game.Initialize(); Input input; Input::Frame frame;
  frame.focused=true; frame.client={600,300}; frame.screen=frame.client;
  const auto step=[&]() { input.Update(frame); game.Update(1.0/60,input,0); };
  step(); frame.down[VK_RBUTTON]=true; step();
  const auto before=game.State().Player(); frame.screen.x+=160; frame.down['W']=true; step();
  const auto after=game.State().Player();
  check(std::abs(motion::AngleDelta(std::atan2(after.x-before.x,after.z-before.z),game.CameraYaw()))<.0001f,
    "W must follow the displayed camera, not its future target angle");
  game.CancelUiGesture(); const float cancelledYaw=game.CameraYaw();
  frame.down['W']=false; frame.screen.x+=100; step();
  check(game.CameraYaw()==cancelledYaw,"Cancelled orbit must not retain hidden rotation after focus/display changes");
  frame.down[VK_RBUTTON]=false; step(); frame.down[VK_RBUTTON]=true; step();
  frame.screen.x+=80; step(); const float menuYaw=game.CameraYaw(); game.OpenPanel(ui::Panel::Menu);
  step(); check(game.CameraYaw()==menuYaw,"Opening the menu must cancel pending orbit without snapping the view");
  game.OpenPanel(ui::Panel::None); frame.screen.x+=90; step();
  check(game.CameraYaw()==menuYaw,"Closing a panel while holding RMB must not resume its old orbit");

  motion::CameraBoom boom; boom.Reset(10); boom.Update(10,2,1.f/60);
  float peak=2;
  for (int i=0;i<20;++i) { boom.Update(10,i%2?2.f:10.f,1.f/60); peak=std::max(peak,boom.distance); }
  check(peak==2,"Alternating clear/blocked samples must not pump the camera at an obstacle edge");
  boom.Update(1,10,1.f/60);
  check(boom.distance<2 && boom.distance>1,"Recovery delay must not swallow a request to zoom inward");
  reference=0;
  for (int hz : {30,60,144}) {
    boom.Reset(10); boom.Update(10,2,1.f/hz);
    for (int i=0;i<hz/2;++i) boom.Update(10,10,1.f/hz);
    check(boom.distance>9.7f && boom.distance<9.9f,"Camera must recover after sustained clearance");
    if (hz!=30) check(std::abs(boom.distance-reference)<.001f,"Obstruction recovery timing must not depend on frame rate");
    reference=boom.distance;
  }

  rpg::Simulation pursuitGame; Require(pursuitGame.Load(pursuit),"Moving-target fixture must load");
  pursuitGame.Update(.01f,{});
  rpg::Command move; move.movement={1,0}; move.sprint=true;
  for (int i=0;i<12;++i) pursuitGame.Update(1.f/60,move);
  const auto& follower=pursuitGame.Enemies()[0]; const auto player=pursuitGame.Player();
  check(follower.position.x>-3.995f && std::abs(motion::AngleDelta(follower.facing,
    std::atan2(player.x-follower.position.x,player.z-follower.position.z)))<.015f,
    "Unobstructed pursuit must follow current movement without waiting for the path timer");

  const char* tree="MEDITERRANEAN 3\n-14.76 10 100 0 0 0\n2 0 0 0\n0\n0 0 0 0\n"
    "100 -13.19 10 80 0 0 1 0 -1.5707963\n101 4 9 80 0 0 0 0 0\n102 0 12 120 0 0 0 0 0\n";
  rpg::Simulation blocked; Require(blocked.Load(tree),"Close, occluded pursuit fixture must load");
  bool attacked=false; float travelled=0;
  for (int i=0;i<360 && !attacked;++i) {
    blocked.Update(1.f/60,{});
    const auto& enemy=blocked.Enemies()[0]; travelled+=enemy.movementDistance;
    Require(blocked.CanStand(enemy.position,.45f),"Close pursuit must keep trunk clearance");
    attacked=enemy.windup>0;
  }
  check(travelled>.5f && attacked,"An enemy inside melee distance but behind a trunk must walk around it, not stall");
  Require(passed,"Motion transition regressions failed");
}
}
int main() {
  SmoothPoseTransitions();
  TransitionRegressions();
  // Compare actual domain displacement, not just the interpolation helper.
  float reference=0;
  for (int hz : {30,60,144}) {
    rpg::Simulation simulation;
    rpg::Command move; move.movement={0,1};
    const auto start=simulation.Player();
    simulation.Update(1.f/hz,move);
    Require(simulation.MovementDistance()>0 && simulation.MovementDistance()<4.f/hz,
      "First movement sample must respond immediately below full speed");
    for (int i=1;i<hz;++i) simulation.Update(1.f/hz,move);
    const float travel=Distance(start,simulation.Player());
    Require(travel>3.8f && travel<4,"Acceleration must preserve nearly all the first second's travel");
    if (reference) Require(std::abs(reference-travel)<.001f,"Acceleration must agree at 30/60/144 Hz");
    reference=travel;
    const auto stopped=simulation.Player(); simulation.Update(1.f/hz,{});
    Require(Distance(stopped,simulation.Player())==0 && simulation.MovementDistance()==0,
      "Releasing input must not cause drift into attacks or through a menu");
    move.sprint=true; simulation.Update(1.f/hz,move);
    Require(simulation.MovementDistance()<7.f/hz,"Sprint must accelerate after stopping");
  }
  rpg::Simulation straight,diagonal;
  rpg::Command move; move.movement={0,1}; straight.Update(.05f,move);
  move.movement={1,1}; diagonal.Update(.05f,move);
  Require(std::abs(straight.MovementDistance()-diagonal.MovementDistance())<.00001f,
    "Diagonal input must not bypass the speed ramp");
  reference=0;
  for (int hz : {30,60,144}) {
    rpg::Simulation simulation; Require(simulation.Load(pursuit),"Pursuit fixture must load at each update rate");
    const auto start=simulation.Enemies()[0].position;
    for (int i=0;i<hz/2;++i) {
      simulation.Update(1.f/hz,{});
      Require(simulation.Enemies()[0].movementDistance>0,"An aligned pursuing enemy must not insert idle path frames");
    }
    const float travel=Distance(start,simulation.Enemies()[0].position);
    Require(travel>.9f && travel<1.1f,"Enemy acceleration must respond within the first half second");
    if (reference) Require(std::abs(reference-travel)<.001f,"Enemy acceleration must agree at 30/60/144 Hz");
    reference=travel;
  }

  Game game; game.Initialize(); move={}; move.movement={1,0};
  game.ExecuteCommand(move,1.f/60);
  Require(game.PlayerPose().facing>0 && game.PlayerPose().facing<game.State().Facing(),
    "Player presentation must turn gradually while the movement command responds immediately");
  for (int i=0;i<7;++i) game.ExecuteCommand(move,1.f/60);
  const auto standing=game.State().Player(); const float gait=game.PlayerPose().gait;
  Require(std::abs(gait)>.01f,"Walking must actually animate the player");
  game.ExecuteCommand({},1.f/60);
  Require(Distance(standing,game.State().Player())==0 && std::abs(game.PlayerPose().gait-gait)<.25f &&
    std::abs(game.PlayerPose().gait)>0,"Stopping must settle the pose without sliding the player or snapping the feet");
  for (int i=0;i<30;++i) game.ExecuteCommand({},1.f/60);
  Require(std::abs(game.PlayerPose().gait)<.0001f,"Idle pose must settle without walking in place");
  const float phase=game.PlayerPose().phase;
  rpg::Command dash; dash.ability=2; game.ExecuteCommand(dash);
  Require(game.State().MovementDistance()==0 && game.PlayerPose().phase==phase,
    "Dash must not count teleport distance as several walking steps");
  const auto save=game.State().Save();
  game.ExecuteCommand(move,.05f);
  Require(game.RestoreSnapshot(save) && game.PlayerPose().gait==0 && game.PlayerPose().attack==0 &&
    game.PlayerPose().facing==game.State().Facing(),"Loading must reset motion without changing durable save state");
  Require(game.State().Save()==save,"Presentation reset must not alter durable gameplay state");
  game.ExecuteCommand(move,.05f); rpg::Command reset; reset.restart=true; game.ExecuteCommand(reset);
  Require(game.PlayerPose().gait==0 && game.PlayerPose().phase==0 && game.State().MovementDistance()==0,
    "Restart must clear all locomotion history");
  const auto unchanged=game.State().Save(); const auto pose=game.PlayerPose();
  for (float dt : {0.f,-1.f,std::numeric_limits<float>::quiet_NaN()}) game.ExecuteCommand(move,dt);
  Require(game.State().Save()==unchanged && game.PlayerPose().phase==pose.phase,
    "Invalid or zero elapsed time must not advance smoothing");
  Input sampled; Input::Frame frame; frame.focused=true; frame.client={600,300};
  sampled.Update(frame); frame.down['W']=true;
  for (int i=0;i<12;++i) { sampled.Update(frame); game.Update(1.0/60,sampled,0); }
  const auto beforeMenu=game.State().Player(); game.OpenPanel(ui::Panel::Menu);
  sampled.Update(frame); game.Update(1.0/60,sampled,0);
  Require(Distance(beforeMenu,game.State().Player())==0,"Opening the menu while running must stop physical motion immediately");
  game.OpenPanel(ui::Panel::None); frame.focused=false; sampled.Update(frame); game.Update(1.0/60,sampled,0);
  Require(Distance(beforeMenu,game.State().Player())==0,"Focus loss must not retain running momentum");
  game.Initialize(); rpg::Command jump; jump.jump=true; jump.movement={0,1};
  game.ExecuteCommand(jump,.05f);
  Require(game.State().Height()>0 && game.PlayerPose().phase==0,"Airborne travel must not advance grounded footsteps");

  motion::Actor actor; actor.Reset(3.13f); actor.Update(-3.13f,0,true,true,0,.01f);
  Require(motion::AngleDelta(3.13f,actor.facing)>0 &&
    std::abs(motion::AngleDelta(actor.facing,-3.13f))<std::abs(motion::AngleDelta(3.13f,-3.13f)),
    "Turning across pi must ease forward along the short arc");
  actor.Update(0,0,true,true,1,1.f/60);
  Require(actor.attack>0 && actor.attack<1,"Attack pose must blend into the strike");
  actor.Update(0,0,true,false,1,1.f/60);
  Require(actor.attack==0 && actor.gait==0,"Defeat must clear residual animation immediately");

  Require(game.RestoreSnapshot(pursuit),"Pursuit fixture must load");
  bool sawWalk=false,sawWindup=false; float committedFacing=0;
  for (int i=0;i<300;++i) {
    const auto before=game.State().Enemies()[0];
    game.ExecuteCommand({},1.f/60);
    const auto& enemy=game.State().Enemies()[0];
    Require(game.State().CanStand(enemy.position,.45f),"Smoothed navigation must retain collision clearance");
    Require(Distance(before.position,enemy.position)<=2.2f/60+.0001f,"Pursuit must respect its speed limit");
    if (!sawWindup && enemy.movementDistance>.00001f) sawWalk=true;
    if (enemy.windup>0) {
      if (!sawWindup) committedFacing=enemy.facing;
      sawWindup=true;
      Require(enemy.facing==committedFacing && game.EnemyPose(0).facing==enemy.facing,
        "Visible facing and damage cone must stay committed throughout the windup");
    } else if (sawWindup) break;
  }
  Require(sawWalk && sawWindup,"Enemy must approach and perform an attack after smooth arrival");
  auto midSwing=std::string(pursuit);
  const auto actorLine=midSwing.find("100 -4 7 80 1 0 1 0 3.14159265");
  Require(actorLine!=std::string::npos,"Expected pursuit actor fields");
  midSwing.replace(actorLine,std::string("100 -4 7 80 1 0 1 0 3.14159265").size(),
    "100 -4 7 80 0 0 1 0.35 3.14159265");
  Require(game.RestoreSnapshot(midSwing) && std::abs(game.EnemyPose(0).attack-.5f)<.001f,
    "Loading a committed swing must immediately reconstruct its pose, without an idle flash");

  motion::CameraBoom boom; boom.Reset(10); boom.Update(10,2,1.f/60);
  Require(boom.distance==2,"Obstruction must retract the camera in the same frame");
  boom.Update(10,10,1.f/60);
  Require(boom.distance==2,"Brief clearance must hold the camera steady before recovery");
  for (int i=0;i<7;++i) boom.Update(10,10,1.f/60);
  Require(boom.distance>2 && boom.distance<5,"Sustained clearance must ease the camera out");
  for (int i=0;i<60;++i) boom.Update(10,10,1.f/60);
  Require(boom.distance>9.99f && boom.distance<=10,"Camera return must converge without overshooting");
  boom.Update(3,10,1.f/60);
  Require(boom.distance>3 && boom.distance<9.99f,"Wheel zoom inward must also ease in clear space");
  game.Initialize(); const float range=game.CameraRange(); const Input input;
  game.Update(1.0/60,input,2);
  Require(game.CameraRange()<range && game.CameraRange()>range-2,"Interactive wheel input must ease zoom");
  game.SetCameraOrbit(0,.45f,6);
  Require(game.CameraRange()==6,"Explicit camera placement must remain immediate for captures/tools");
  std::cout<<"Movement, animation, windup and camera smoothing checks passed.\n";
}
