#include "game/Game.h"
#include "tests/DashFixture.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace {
void Require(bool value,const char* message) {
  if (!value) { std::cerr<<message<<'\n'; std::exit(1); }
}
float Distance(rpg::Vec2 a,rpg::Vec2 b) { return std::hypot(a.x-b.x,a.z-b.z); }
void PivotTraction() {
  rpg::Vec2 reference{};
  for(int hz:{1000,144,60,30,22}) {
    rpg::Simulation s;rpg::Command run;run.movement={0,-1};
    const auto advance=[&](float duration) {
      for(float elapsed=0;elapsed<duration-1e-6f;) {
        const float dt=std::min(1.f/hz,duration-elapsed);s.Update(dt,run);elapsed+=dt;
        Require(s.CanStand(s.Player()),"Pivot movement escaped collision bounds");
      }
    };
    advance(.5f);const auto before=s.Player();run.movement={0,1};
    float overshoot=0;
    for(int tick=0;tick<100;++tick) {
      s.Update(.001f,run);overshoot=std::max(overshoot,before.z-s.Player().z);
    }
    Require(overshoot>.02f && overshoot<.072f,"Planted reversal must preserve momentum while shortening the skid");
    advance(.4f);
    if(hz==1000) reference=s.Player();
    else Require(Distance(reference,s.Player())<.003f,"Pivot traction changed with frame rate");
  }
}
const char* pursuit="MEDITERRANEAN 3\n-4 3 100 0 0 0\n2 0 0 0\n0\n0 0 0 0\n"
  "100 -4 7 80 1 0 1 0 3.14159265\n101 4 9 80 0 0 0 0 0\n102 0 12 120 0 0 0 0 0\n";
void SkyCameraClearance() {
  Game game; game.Initialize(); Input input; Input::Frame frame;
  frame.focused=true; frame.client={600,300}; frame.screen=frame.client; frame.relativeMouse=true;
  const auto step=[&]() { input.Update(frame); game.Update(1.0/60,input,0); };
  step(); frame.down[VK_RBUTTON]=true; step(); frame.mouseY=-300; step(); frame.mouseY=0;
  for (int i=0;i<120;++i) {
    step();
    Require(game.CameraHeight()+std::sin(game.CameraPitch())*game.CameraRange()>=.2999f,
      "Looking up with RMB must keep the camera above the ground throughout the transition.");
  }
  Require(game.CameraPitch()<-.70f && game.CameraRange()<3,"RMB must let the player inspect the sun and overhead sky.");
  game.SetCameraOrbit(0,.45f,10);
  Require(game.CameraRange()>3,"Returning to the normal view must restore camera distance.");
}
void RmbBodySteering() {
  for (int hz : {30,60,144}) {
    Game game; game.Initialize(); Input input; Input::Frame frame;
    frame.focused=true; frame.client={600,300}; frame.screen=frame.client; frame.relativeMouse=true;
    const auto step=[&]() { input.Update(frame); game.Update(1.0/hz,input,0); };
    step(); frame.down[VK_RBUTTON]=true; step(); const auto standing=game.State().Player();
    frame.mouseX=70; step(); frame.mouseX=0;
    for (int i=0;i<hz;++i) step();
    Require(Distance(standing,game.State().Player())==0 && game.State().Facing()>.4f &&
      std::abs(motion::AngleDelta(game.PlayerPose().facing,game.CameraYaw()))<.001f,
      "RMB must turn the visible body right while planted, at every update rate.");
    frame.mouseX=-140; step(); frame.mouseX=0;
    for (int i=0;i<hz;++i) step();
    Require(Distance(standing,game.State().Player())==0 && game.State().Facing()<-.4f &&
      std::abs(motion::AngleDelta(game.PlayerPose().facing,game.CameraYaw()))<.001f,
      "RMB must reverse the planted body smoothly to the left.");
    frame.down['D']=true;
    for (int i=0;i<hz/5;++i) {
      step();
      Require(std::abs(motion::AngleDelta(game.State().Facing(),game.CameraYaw()))<.0001f,
        "RMB strafing must keep body heading with the camera, not lateral travel.");
    }
    Require(Distance(standing,game.State().Player())>.1f,"RMB strafing must still move the player.");
    frame.down['D']=false; frame.down['S']=true;
    for (int i=0;i<hz/5;++i) step();
    Require(std::abs(motion::AngleDelta(game.State().Facing(),game.CameraYaw()))<.0001f,
      "RMB backward movement must not flip body heading.");
    frame.down['S']=false; frame.focused=false; frame.mouseX=100;
    const float facing=game.State().Facing(); step();
    Require(game.State().Facing()==facing && !game.WantsMouseLook(),"Focus loss cancels body steering and capture together.");
  }
  rpg::Simulation simulation; rpg::Command command; command.steerFacing=true; command.facingYaw=.9f;
  simulation.Update(.01f,command); const float facing=simulation.Facing();
  command.facingYaw=std::numeric_limits<float>::quiet_NaN(); simulation.Update(.01f,command);
  Require(simulation.Facing()==facing,"Nonfinite steering must not corrupt facing or saves.");
  command.facingYaw=-1; command.haltMovement=true; simulation.Update(.01f,command);
  Require(simulation.Facing()==facing,"Modal capture must suppress steering.");
  Require(simulation.Load(test::dashFixture),"Target dash steering fixture must load.");
  command.haltMovement=false; command.facingYaw=.9f; command.ability=2; command.nextTarget=true; simulation.Update(.01f,command);
  const float dashFacing=simulation.Facing(); command.nextTarget=false;
  Require(simulation.Dashing(),"Prepare a committed dash.");
  command.ability=-1; command.facingYaw=-1; simulation.Update(.01f,command);
  Require(simulation.Facing()==dashFacing && dashFacing==0,"Active dashes face their target despite RMB turns.");
  rpg::Simulation combat;
  std::string closeTarget=pursuit; const auto at=closeTarget.find("-4 3 100"); closeTarget.replace(at,8,"-4 5 100");
  Require(combat.Load(closeTarget),"Close melee steering fixture must load.");
  command={}; command.nextTarget=true; command.ability=0; command.steerFacing=true; command.facingYaw=1;
  combat.Update(.01f,command);
  Require(combat.AttackFlash()>0,"Prepare an accepted strike.");
  const float strikeFacing=combat.Facing(); command.nextTarget=false; command.ability=-1; command.facingYaw=-1;
  combat.Update(.01f,command);
  Require(combat.Facing()==strikeFacing,"RMB cannot turn an accepted strike during its committed presentation.");
  for (int i=0;i<60;++i) combat.Update(.01f,command);
  Require(combat.Facing()==-1,"Camera steering resumes when the committed strike finishes.");
}

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
void SmoothCameraTransitions() {
  motion::CameraBoom boom; boom.Reset(10);
  boom.Update(6,18,.001f);
  Require(boom.distance<10 && boom.distance>9.999f,"Wheel zoom must accelerate smoothly from rest");
  for (int i=0;i<12;++i) boom.Update(6,18,1.f/120);
  constexpr float dt=.0001f;
  const float a=boom.distance; boom.Update(6,18,dt);
  const float b=boom.distance; boom.Update(14,18,dt);
  Require(std::abs((b-a)/dt-(boom.distance-b)/dt)<.5f,"Wheel reversal must preserve camera velocity");
  float reference=0;
  for (int hz : {30,60,144}) {
    boom.Reset(10);
    for (int i=0;i<hz/2;++i) boom.Update(6,18,1.f/hz);
    Require(boom.distance>6 && boom.distance<6.04f,"Smooth zoom must settle promptly without overshooting a fixed target");
    if (reference) Require(std::abs(boom.distance-reference)<.0001f,"Zoom must agree across update rates");
    reference=boom.distance;
  }
  boom.Reset(3); boom.Update(18,18,.05f); boom.Update(18,2,.01f);
  Require(boom.distance==2,"Collision must override outward zoom momentum immediately");
  for (int i=0;i<5;++i) boom.Update(18,18,.01f);
  Require(boom.distance==2,"Collision must discard momentum during the recovery delay");
  boom.Reset(10);
  for (float dtBad : {0.f,-1.f,std::numeric_limits<float>::quiet_NaN()}) boom.Update(3,2,dtBad);
  Require(boom.distance==10,"Invalid elapsed time must not alter camera distance");
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
    float apexTime=1.f/hz;
    while(apexTime<.3f-1e-6f) {
      const float step=std::min(1.f/hz,.3f-apexTime);
      simulation.Update(step,{}); apexTime+=step;
    }
    check(std::abs(simulation.Height()-1.08f)<.0001f,"Jump apex must be 1.08 m at 0.30 seconds at every update rate");
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
  PivotTraction();
  RmbBodySteering();
  SkyCameraClearance();
  {
    Game focusedGame; focusedGame.Initialize(); Input input; Input::Frame frame;
    frame.focused=true; input.Update(frame); frame.down['W']=true;
    for(int i=0;i<15;++i) { input.Update(frame); focusedGame.Update(1.0/60,input,0); }
    const auto beforeFocusLoss=focusedGame.State().Player();
    frame.focused=false; frame.down['W']=false; input.Update(frame); focusedGame.Update(1.0/60,input,0);
    Require(Distance(beforeFocusLoss,focusedGame.State().Player())==0,
      "Focus loss while running must clear momentum without a braking step");
    frame.focused=true; input.Update(frame); focusedGame.Update(1.0/60,input,0);
    Require(Distance(beforeFocusLoss,focusedGame.State().Player())==0,
      "Returning focus without movement input must not resume stored momentum");
    rpg::Command move; move.movement={0,1}; focusedGame.ExecuteCommand(move,.05f);
    const auto suspended=focusedGame.State().Save(); const auto position=focusedGame.State().Player();
    focusedGame.SuspendInput(); focusedGame.SuspendInput();
    Require(focusedGame.State().Save()==suspended && focusedGame.State().MovementDistance()==0 &&
      focusedGame.State().GroundMovementDistance()==0,"Suspending input must clear motion without advancing or resetting gameplay");
    focusedGame.ExecuteCommand({},.05f);
    Require(Distance(position,focusedGame.State().Player())==0,
      "Engine suspension without a Game update must still discard stored velocity");
  }
  for(int hz : {22,30,60,144}) {
    rpg::Simulation flight; rpg::Command command; command.movement={0,1};
    for(int i=0;i<hz/2;++i) flight.Update(1.f/hz,command);
    const float launchSpeed=4.f*(1-std::exp(-8.f));
    const auto launch=flight.Player();
    float elapsed=0,groundTravel=0; int landings=0;
    while(elapsed<.7f) {
      const float dt=std::min(1.f/hz,.7f-elapsed);
      command={}; command.jump=elapsed==0;
      flight.Update(dt,command);
      groundTravel+=flight.GroundMovementDistance();
      landings+=flight.LandingSpeed()>0;
      if(flight.Height()>0) Require(flight.GroundMovementDistance()==0,
        "Airborne travel must never count as grounded footsteps");
      elapsed+=dt;
    }
    const float expectedGround=launchSpeed/24*(1-std::exp(-24*(.7f-.6f)));
    Require(std::abs(groundTravel-expectedGround)<.0001f,
      "Landing must apply braking only for the grounded fraction of the step");
    Require(std::abs(flight.Player().z-launch.z-(launchSpeed*.6f+expectedGround))<.0001f,
      "Jump distance must agree with ballistic flight followed by ground braking");
    Require(landings==1 && flight.Height()==0,"Contact must generate exactly one landing event");
    flight=rpg::Simulation{}; command={}; command.jump=true; command.movement={1,0};
    flight.Update(1.f/hz,command);
    const float expectedTakeoff=4.f/hz-1.6f/3*(1-std::exp(-3.f/hz));
    Require(std::abs(flight.Player().x-expectedTakeoff)<.00001f,
      "Standing directional jump must apply its launch push before air steering");
    auto noSprint=flight;
    command.jump=false; command.sprint=true;
    flight.Update(1.f/hz,command); command.sprint=false; noSprint.Update(1.f/hz,command);
    Require(Distance(flight.Player(),noSprint.Player())<.00001f,
      "Pressing sprint in flight must not grant a ground-running acceleration boost");
    flight=rpg::Simulation{}; command={}; command.jump=true; command.haltMovement=true;
    flight.Update(1.f/hz,command);
    Require(flight.Height()==0 && !flight.JumpedThisUpdate(),"Movement capture must suppress takeoff and its sound event");
  }
  rpg::Vec2 momentumReference{};
  for (int hz : {22,30,60,144}) {
    for (int mode=0;mode<5;++mode) {
      rpg::Simulation buffered; float elapsed=0; int jumps=0,contacts=0;
      const auto advance=[&](float until,bool press) {
        bool first=true;
        while(elapsed<until-1e-6f) {
          const float dt=std::min(1.f/hz,until-elapsed);
          rpg::Command command; command.jump=first && press;
          buffered.Update(dt,command); elapsed+=dt; first=false;
          jumps+=buffered.JumpedThisUpdate(); contacts+=buffered.LandingSpeed()>0;
          Require(buffered.GroundMovementDistance()==0,"Stationary buffered jump must not invent footsteps");
        }
      };
      advance(mode==1?.4f:.52f,true);
      advance(mode==1?.42f:.54f,true);
      if(mode==2) buffered.StopMovement();
      if(mode==4) Require(buffered.Load(buffered.Save()),"Queued jump load fixture must restore");
      if(mode==3) {
        rpg::Command capture; capture.haltMovement=true;
        buffered.Update(.01f,capture); elapsed+=.01f;
      }
      advance(.65f,false);
      if(mode==0) {
        Require(jumps==2 && contacts==1 && std::abs(buffered.Height()-.33f)<.0001f,
          "Buffered jump must launch at exact contact without a frame of ground delay");
        Require(std::abs(buffered.VerticalSpeed()-6.f)<.0001f,"Buffered flight must integrate the remaining contact-frame time");
      } else Require(jumps==1 && buffered.Height()==0,
        "Expired, suspended, loaded or menu-cancelled jump must not replay on landing");
    }
  }
  {
    rpg::Simulation chain; rpg::Command run; run.movement={0,1};
    for(int frame=0;frame<10;++frame) chain.Update(.05f,run);
    const auto launch=chain.Player();
    for(int frame=0;frame<13;++frame) {
      rpg::Command input; input.jump=frame==0 || frame==10;
      chain.Update(.05f,input);
      Require(chain.GroundMovementDistance()==0,"Buffered rebound must never apply ground traction or gait travel");
    }
    Require(std::abs(chain.Player().z-launch.z-4.f*(1-std::exp(-8.f))*.65f)<.0001f,
      "Running rebound must preserve horizontal momentum through floor contact");
  }
  {
    rpg::Simulation standing,starting; rpg::Command command; command.movement={1,0};
    starting.Update(.01f,command); const auto before=starting.Player();
    command.jump=true; standing.Update(.01f,command); starting.Update(.01f,command);
    Require(std::abs((starting.Player().x-before.x)-standing.Player().x)<.00001f,
      "A jump shortly after starting movement must receive the same minimum launch push as a standing jump");
  }
  for(int hz : {30,60,144}) {
    rpg::Simulation simulation; rpg::Command command;
    for(auto direction : {rpg::Vec2{1,0},rpg::Vec2{0,1},rpg::Vec2{-1,0},rpg::Vec2{0,0}}) {
      command.movement=direction;
      for(int i=0;i<hz/2;++i) simulation.Update(1.f/hz,command);
    }
    if(hz!=30) Require(Distance(momentumReference,simulation.Player())<.001f,
      "Turning and braking trajectory must agree at 30/60/144 Hz");
    momentumReference=simulation.Player();
  }
  for(int hz : {30,60,144}) {
    rpg::Simulation momentum; rpg::Command command; command.movement={0,1};
    for(int i=0;i<hz/2;++i) momentum.Update(1.f/hz,command);
    const auto beforeReverse=momentum.Player(); command.movement={0,-1};
    momentum.Update(1.f/hz,command);
    Require(momentum.Player().z>beforeReverse.z,"Reversal must brake existing momentum before changing direction");
    Require(std::abs(momentum.Facing())<.001f,
      "A reversal must face the remaining forward travel until momentum actually changes direction");
    momentum=rpg::Simulation{}; command.movement={0,1};
    for(int i=0;i<hz/2;++i) momentum.Update(1.f/hz,command);
    command.jump=true; momentum.Update(1.f/hz,command);
    const auto takeoff=momentum.Player();
    momentum.Update(1.f/hz,{});
    Require(momentum.Height()>0 && momentum.Player().z-takeoff.z>3.9f/hz,
      "Releasing movement in flight must retain takeoff momentum");
    command={}; command.movement={0,-1}; const auto airborne=momentum.Player();
    momentum.Update(1.f/hz,command);
    Require(momentum.Player().z>airborne.z,"Air steering must not instantly reverse the trajectory");
    const auto saved=momentum.Save(); Require(momentum.Load(saved),"Momentum save must load");
    const auto loaded=momentum.Player(); momentum.Update(1.f/hz,{});
    Require(Distance(loaded,momentum.Player())==0,"Load must clear transient velocity");
    momentum=rpg::Simulation{}; command={}; command.movement={-1,0}; command.sprint=true;
    for(int i=0;i<hz*2;++i) momentum.Update(1.f/hz,command);
    Require(momentum.CanStand(momentum.Player()),"Momentum must not penetrate a wall");
    const auto wall=momentum.Player(); momentum.Update(1.f/hz,{});
    Require(Distance(wall,momentum.Player())<.001f,"Wall contact must discard inward momentum without rebound");
    command.movement={0,1}; momentum.Update(1.f/hz,command);
    Require(momentum.Player().z>wall.z && momentum.CanStand(momentum.Player()),
      "Wall contact must permit tangential movement without sticking");
  }
  SmoothPoseTransitions();
  SmoothCameraTransitions();
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
    Require(travel>3.74f && travel<3.76f,"The longer acceleration ramp must cover 3.75 m in the first second");
    if (reference) Require(std::abs(reference-travel)<.001f,"Acceleration must agree at 30/60/144 Hz");
    reference=travel;
    const auto stopped=simulation.Player(); simulation.Update(1.f/hz,{});
    Require(Distance(stopped,simulation.Player())>0 && simulation.MovementDistance()<4.f/hz,
      "Releasing input must brake smoothly below running speed");
    for(int i=0;i<hz/2;++i) simulation.Update(1.f/hz,{});
    Require(Distance(stopped,simulation.Player())>.16f && Distance(stopped,simulation.Player())<.168f && simulation.MovementDistance()==0,
      "Ground braking must settle promptly within a short stopping distance");
    move.sprint=true; simulation.Update(1.f/hz,move);
    Require(simulation.MovementDistance()<7.f/hz,"Sprint must accelerate after stopping");
  }
  rpg::Simulation straight,diagonal;
  for(int hz : {30,60,144}) {
    float distance[3]{};
    for(int direction=0;direction<3;++direction) {
      rpg::Simulation s; const auto start=s.Player();
      rpg::Command input; input.steerFacing=true; input.facingYaw=0;
      input.movement=direction==0?rpg::Vec2{0,1}:direction==1?rpg::Vec2{1,0}:rpg::Vec2{0,-1};
      for(int i=0;i<hz/2;++i) s.Update(1.f/hz,input);
      distance[direction]=Distance(start,s.Player());
      Require(s.Facing()==0,"Strafing and backing must preserve the camera-facing heading");
    }
    Require(std::abs(distance[1]/distance[0]-.85f)<.001f && std::abs(distance[2]/distance[0]-.65f)<.001f,
      "Side and backward strides must have deliberate directional speeds across frame rates");
  }
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
  Require(Distance(standing,game.State().Player())>0 && Distance(standing,game.State().Player())<.067f &&
    std::abs(game.PlayerPose().gait)>0,"Braking must retain a short physical step and a moving pose");
  for (int i=0;i<42;++i) game.ExecuteCommand({},1.f/60);
  Require(std::abs(game.PlayerPose().gait)<.0001f,"Idle pose must settle without walking in place");
  Require(game.RestoreSnapshot(test::dashFixture),"Target dash fixture must load.");
  const float phase=game.PlayerPose().phase;
  rpg::Command dash; dash.ability=2; dash.nextTarget=true; game.ExecuteCommand(dash);
  Require(game.State().MovementDistance()==0 && game.PlayerPose().phase==phase,
    "Dash must not count burst distance as several walking steps");
  for(int hz : {22,30,60,144}) {
    Game burst; burst.Initialize(); Require(burst.RestoreSnapshot(test::dashFixture),"Target dash fixture"); const auto start=burst.State().Player();
    float elapsed=0; bool animated=false;
    while(elapsed<.35f-1e-6f) {
      const float dt=std::min(1.f/hz,.35f-elapsed); rpg::Command input;
      if(elapsed==0) input=dash;
      const auto before=burst.State().Player(); burst.ExecuteCommand(input,dt);
      Require(Distance(before,burst.State().Player())<1.f,"Dash must move continuously rather than teleport");
      Require(burst.PlayerPose().facing==burst.State().Facing(),"The body must face the target during the burst.");
      animated=animated || burst.PlayerAnimation().dash>.5f;
      Require(burst.State().GroundMovementDistance()==0,"Dash must not generate walking steps");
      elapsed+=dt;
    }
    Require(std::abs(Distance(start,burst.State().Player())-3.5f)<.0001f && !burst.State().Dashing(),
      "Timed dash must cover 3.5 m and finish independently of update rate");
    Require(animated && burst.PlayerAnimation().dash==0,"Dash must animate through its lifetime then settle");
    burst.Initialize(); Require(burst.RestoreSnapshot(test::dashFixture),"Target dash reset fixture"); burst.ExecuteCommand(dash,.05f); const auto stopped=burst.State().Player();
    burst.SuspendInput(); burst.ExecuteCommand({},.05f);
    Require(Distance(stopped,burst.State().Player())==0 && !burst.State().Dashing(),"Suspension must cancel a timed dash");
    burst.Initialize(); Require(burst.RestoreSnapshot(test::dashFixture),"Target dash reset fixture"); burst.ExecuteCommand(dash,.05f); const auto dashSave=burst.State().Save();
    Require(burst.RestoreSnapshot(dashSave) && !burst.State().Dashing(),"Loading must discard unfinished dash movement");
  }
  // Check equal-time displacement and phase at several real update rates.
  std::array<float,4> referenceDash{};
  float referenceSettle=0;
  for (int hz : {22,30,60,144,1000}) {
    Game burst; burst.Initialize(); Require(burst.RestoreSnapshot(test::dashFixture),"Equal-time target fixture"); const auto origin=burst.State().Player();
    const float checkpoints[]={.075f,.17f,.30f,.35f};
    float elapsed=0; int starts=0,stops=0;
    for (int point=0;point<4;++point) {
      while (elapsed<checkpoints[point]-1e-7f) {
        const float dt=std::min(1.f/hz,checkpoints[point]-elapsed);
        rpg::Command input; if (elapsed==0) input=dash;
        burst.ExecuteCommand(input,dt); elapsed+=dt;
        for (std::size_t i=0;i<burst.State().VisualEventCount();++i) {
          const auto kind=burst.State().VisualEvents()[i].kind;
          starts+=kind==rpg::CombatVisualKind::DashStart;
          stops+=kind==rpg::CombatVisualKind::DashStop;
          if (kind==rpg::CombatVisualKind::DashStop) Require(
            std::abs(elapsed-burst.State().VisualEvents()[i].elapsed-motion::dashDuration)<.0001f,
            "Normal dash finish must retain its time inside the final update");
          Require(kind!=rpg::CombatVisualKind::DashImpact,"Open ground must not invent an impact");
        }
      }
      const float travel=Distance(origin,burst.State().Player());
      if (hz==22) referenceDash[point]=travel;
      else Require(std::abs(travel-referenceDash[point])<.0001f,"Dash distance at equal times must not depend on update rate");
      if (point==0) Require(travel>.7f && travel<.9f && burst.PlayerAnimation().dash>.99f,
        "The push-off must reach peak speed early without covering the whole dash");
      if (point==1) Require(travel>2.4f && travel<2.7f && burst.PlayerAnimation().dashBrake>.7f,
        "The second half must visibly brake after the initial burst");
      if (point==2) Require(burst.PlayerAnimation().dash<.1f,"Final travel must slow into the stop");
    }
    Require(starts==1 && stops==1,"A completed dash must emit exactly one launch and one stop");
    Require(burst.PlayerAnimation().dashBrake>.3f,"Normal finish must retain a short bracing recovery");
    while (elapsed<.42f-1e-7f) {
      const float dt=std::min(1.f/hz,.42f-elapsed); burst.ExecuteCommand({},dt); elapsed+=dt;
    }
    if (hz==22) referenceSettle=burst.PlayerAnimation().dashBrake;
    else Require(std::abs(burst.PlayerAnimation().dashBrake-referenceSettle)<.0001f,
      "Equal-time settling must agree across update rates");
    while (elapsed<.52f-1e-7f) {
      const float dt=std::min(1.f/hz,.52f-elapsed); burst.ExecuteCommand({},dt); elapsed+=dt;
    }
    Require(burst.PlayerAnimation().dashBrake==0 && burst.PlayerAnimation().dashLoad==0,
      "Dash recovery must finish completely without drift");
  }
  const auto save=game.State().Save();
  {
    rpg::Simulation blocked;
    Require(blocked.Load(test::dashWallFixture),"Dash wall fixture must load");
    for(int frame=0;frame<20;++frame) blocked.Update(.05f,frame==0?dash:rpg::Command{});
    Require(blocked.CanStand(blocked.Player()) && blocked.Player().z<3.5f && !blocked.Dashing(),
      "Timed dash must stop at a wall without tunnelling or persistent force");
  }
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
