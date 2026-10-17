#include "game/Game.h"

#include "engine/Renderer.h"
#include <algorithm>
#include <cmath>
#include <sstream>
#include <filesystem>
#include <fstream>

void Game::Initialize() { simulation_ = rpg::Simulation{}; }

void Game::Update(double seconds,const Input& input,float wheel) {
  const float dt=static_cast<float>(seconds);
  if (input.Pressed(VK_F5)) SaveProgress();
  if (input.Pressed(VK_F9)) { LoadProgress(); return; }
  if (input.Pressed(VK_ESCAPE)) {
    if (panel_!=Panel::None) panel_=Panel::None;
    else paused_=!paused_;
  }
  const auto togglePanel=[&](int key,Panel panel) {
    if (input.Pressed(key)) panel_=panel_==panel?Panel::None:panel;
  };
  togglePanel('C',Panel::Character); togglePanel('I',Panel::Inventory);
  togglePanel('J',Panel::Quest); togglePanel('M',Panel::Map);
  if (paused_) return;
  time_+=dt;
  yaw_=std::remainder(yaw_+input.MouseX()*.006f,6.2831853f);
  pitch_=std::clamp(pitch_+input.MouseY()*.004f,.15f,1.1f);
  distance_=std::clamp(distance_-wheel,3.f,18.f);
  const float forward=float(input.Held('W'))-float(input.Held('S'));
  const float right=float(input.Held('D'))-float(input.Held('A'));
  rpg::Command command;
  command.movement={std::sin(yaw_)*forward+std::cos(yaw_)*right,
    std::cos(yaw_)*forward-std::sin(yaw_)*right};
  command.sprint=input.Held(VK_SHIFT); command.jump=input.Pressed(VK_SPACE);
  command.nextTarget=input.Pressed(VK_TAB); command.interact=input.Pressed('E');
  command.restart=input.Pressed('R');
  if (command.restart) panel_=Panel::None;
  for (int i=0;i<4;++i) if (input.Pressed('1'+i)) { command.ability=i; break; }
  simulation_.Update(dt,command);
}

void Game::Render(Renderer& renderer) {
  using DirectX::XMFLOAT3;
  using DirectX::XMFLOAT4;
  const auto player=simulation_.Player();
  const float y=simulation_.Height();
  const rpg::Vec2 direction{-std::sin(yaw_)*std::cos(pitch_),-std::cos(yaw_)*std::cos(pitch_)};
  const float cameraDistance=simulation_.CameraDistance(player,y+1.4f,direction,std::sin(pitch_),distance_);
  renderer.SetCamera({player.x+direction.x*cameraDistance,y+1.4f+std::sin(pitch_)*cameraDistance,
    player.z+direction.z*cameraDistance},{player.x,y+1.4f,player.z});
  const XMFLOAT4 grass{.31f,.49f,.32f,1}, stone{.36f,.33f,.43f,1}, timber{.43f,.24f,.13f,1};
  renderer.DrawBox({0,-1,0},{34,2,38},grass);
  renderer.DrawBox({0,-3,0},{29,3,33},stone,.04f);
  renderer.DrawBox({1,-5,1},{21,2,25},{.23f,.25f,.36f,1},-.08f);
  renderer.DrawBox({0,.025f,0},{3,.05f,30},{.64f,.52f,.35f,1});
  for (const auto& o : simulation_.Obstacles()) {
    renderer.DrawBox({o.center.x,o.height*.5f,o.center.z},{o.halfX*2,o.height,o.halfZ*2},timber);
    renderer.DrawBox({o.center.x,o.height+.25f,o.center.z},{o.halfX*2+.5f,.5f,o.halfZ*2+.5f},{.56f,.18f,.19f,1});
    for (int tier=1;tier<=3;++tier)
      renderer.DrawBox({o.center.x,o.height+.25f+tier*.22f,o.center.z},
        {(o.halfX*2+.5f)*(1-tier*.2f),.25f,o.halfZ*2+.5f},{.56f,.18f,.19f,1});
    renderer.DrawBox({o.center.x,.8f,o.center.z-o.halfZ-.03f},{.8f,1.6f,.1f},{.16f,.12f,.16f,1});
    for (float side : {-1.f,1.f})
      renderer.DrawBox({o.center.x+side*o.halfX*.65f,2.f,o.center.z-o.halfZ-.08f},{.6f,.7f,.12f},{.96f,.65f,.25f,1});
  }
  // Distant fragments establish the floating-island silhouette without imported assets.
  for (int i=0;i<7;++i) {
    const float angle=i*.9f;
    renderer.DrawBox({std::cos(angle)*48,-4+float(i%3)*3,std::sin(angle)*48},{8,4,10},stone,angle);
    renderer.DrawBox({std::cos(angle)*48,-1.9f+float(i%3)*3,std::sin(angle)*48},{8,.3f,10},grass,angle);
  }
  for (float x : {-14.f,14.f}) for (float z : {-14.f,-2.f,10.f}) {
    renderer.DrawBox({x,1,z},{.5f,2,.5f},timber);
    renderer.DrawBox({x,2.7f,z},{2.4f,2.5f,2.4f},{.18f,.36f,.25f,1},.3f);
  }
  const auto giver=rpg::Simulation::questGiver;
  renderer.DrawBox({giver.x,1,giver.z},{.8f,2,.8f},{.75f,.58f,.2f,1});
  renderer.DrawBox({giver.x,2.8f+std::sin(time_*2)*.15f,giver.z},{.4f,.4f,.4f},{1,.8f,.25f,1},time_);
  auto character=[&](rpg::Vec2 p,float base,float yaw,XMFLOAT4 color,float swing) {
    renderer.DrawBox({p.x,base+1,p.z},{.7f,1,.45f},color,yaw);
    renderer.DrawBox({p.x,base+1.75f,p.z},{.48f,.5f,.48f},{.84f,.65f,.47f,1},yaw);
    for (float side : {-1.f,1.f}) {
      const float dx=std::cos(yaw)*side*.22f, dz=-std::sin(yaw)*side*.22f;
      renderer.DrawBox({p.x+dx,base+.3f,p.z+dz},{.25f,.6f,.3f},{.18f,.18f,.25f,1},yaw);
    }
    renderer.DrawBox({p.x+std::cos(yaw)*.55f,base+1+swing,p.z-std::sin(yaw)*.55f},
      {.12f,1.1f,.15f},{.76f,.8f,.86f,1},yaw);
  };
  character(player,y,simulation_.Facing(),{.22f,.43f,.76f,1},simulation_.AttackFlash()*2);
  const auto& enemies=simulation_.Enemies();
  for (int i=0;i<static_cast<int>(enemies.size());++i) {
    const auto& enemy=enemies[i];
    if (enemy.health<=0) continue;
    character(enemy.position,0,std::atan2(player.x-enemy.position.x,player.z-enemy.position.z),
      enemy.hitFlash>0?XMFLOAT4{1,.7f,.55f,1}:XMFLOAT4{.65f,.2f,.25f,1},0);
    if (i==simulation_.Target()) {
      renderer.DrawBox({enemy.position.x,.04f,enemy.position.z},{1.3f,.06f,1.3f},{1,.72f,.2f,1});
      renderer.DrawBox({enemy.position.x,2.25f,enemy.position.z},{enemy.health/80.f, .1f,.1f},{.8f,.15f,.2f,1});
    }
  }
  const XMFLOAT4 panel{.065f,.075f,.12f,1}, ink{.94f,.87f,.71f,1};
  renderer.DrawHudRect(.015f,.02f,.37f,.14f,panel);
  renderer.DrawText(.025f,.035f,"ASTRAL FRONTIER  -  OUTPOST",ink);
  renderer.DrawHudRect(.025f,.08f,.24f,.025f,{.22f,.16f,.2f,1});
  renderer.DrawHudRect(.025f,.08f,.24f*simulation_.Health()/100,.025f,{.3f,.72f,.46f,1});
  renderer.DrawText(.025f,.12f,"HP "+std::to_string(int(simulation_.Health()))+"   GOLD "+std::to_string(simulation_.Gold()),ink);
  renderer.DrawText(.025f,.175f,"C CHARACTER  I INVENTORY  J QUEST  M MAP",ink,1.5f);
  if (simulation_.Target()>=0) {
    renderer.DrawHudRect(.65f,.02f,.33f,.1f,panel);
    renderer.DrawText(.67f,.035f,"RAIDER  HP "+std::to_string(int(enemies[simulation_.Target()].health)),ink);
    renderer.DrawHudRect(.67f,.08f,.28f*enemies[simulation_.Target()].health/80,.018f,{.8f,.22f,.27f,1});
  }
  renderer.DrawHudRect(.015f,.77f,.97f,.215f,panel);
  renderer.DrawText(.025f,.785f,simulation_.Message(),ink,1.5f);
  const char* names[]={"1 STRIKE","2 HEAVY","3 DASH","4 MEND"};
  for (int i=0;i<4;++i) {
    const float x=.025f+i*.19f;
    renderer.DrawText(x,.83f,names[i],ink);
    renderer.DrawHudRect(x,.866f,.16f,.012f,{.2f,.22f,.3f,1});
    renderer.DrawHudRect(x,.866f,.16f*(1-simulation_.Cooldown(i)/rpg::Simulation::abilityCooldowns[i]),.012f,{.75f,.58f,.27f,1});
  }
  renderer.DrawText(.025f,.91f,"WASD MOVE  SHIFT RUN  SPACE JUMP  TAB TARGET  E TALK  R RESET",ink,1.5f);
  renderer.DrawText(.025f,.95f,"RIGHT MOUSE ORBIT  WHEEL ZOOM  ESC PAUSE  F5 SAVE  F9 LOAD",ink,1.5f);
  if (panel_!=Panel::None) {
    renderer.DrawHudRect(.245f,.215f,.51f,.51f,{.42f,.32f,.19f,1});
    renderer.DrawHudRect(.25f,.22f,.5f,.5f,panel);
    if (panel_==Panel::Map) {
      renderer.DrawText(.28f,.24f,"OUTPOST MAP",ink);
      const float mx=.3f,my=.29f,mw=.4f,mh=.33f;
      renderer.DrawHudRect(mx,my,mw,mh,{.21f,.32f,.24f,1});
      for (const auto& o : simulation_.Obstacles())
        renderer.DrawHudRect(mx+(o.center.x-o.halfX+17)/34*mw,
          my+(19-o.center.z-o.halfZ)/38*mh,o.halfX*2/34*mw,o.halfZ*2/38*mh,{.52f,.36f,.25f,1});
      const auto marker=[&](rpg::Vec2 p,XMFLOAT4 color) {
        renderer.DrawHudRect(mx+(p.x+17)/34*mw-.003f,my+(19-p.z)/38*mh-.004f,.006f,.008f,color);
      };
      marker(player,{.45f,.7f,1,1}); marker(giver,{1,.8f,.2f,1});
      for (const auto& enemy : enemies) if (enemy.health>0) marker(enemy.position,{1,.3f,.3f,1});
      renderer.DrawText(.28f,.65f,"BLUE YOU  GOLD BEACON  RED RAIDERS",ink,1.5f);
    } else if (panel_==Panel::Quest) {
      renderer.DrawText(.28f,.25f,"PATROL THE FRONTIER",ink);
      const char* states[]={"AVAILABLE","DEFEAT ONE RAIDER","RETURN TO THE BEACON","COMPLETED"};
      renderer.DrawText(.28f,.32f,states[static_cast<int>(simulation_.QuestState())],ink);
      renderer.DrawText(.28f,.4f,"REWARD 25 GOLD\n\nPRESS E NEAR THE GOLD BEACON\n\nRAIDERS ARE NORTH OF THE OUTPOST",ink,1.5f);
    } else if (panel_==Panel::Character) {
      renderer.DrawText(.28f,.25f,"FRONTIER GUARD",ink);
      renderer.DrawText(.28f,.33f,"MELEE PROTOTYPE\n\nHEALTH "+std::to_string(int(simulation_.Health()))+
        " OF 100\n\nSTRIKE 20 DAMAGE\n\nHEAVY STRIKE 35 DAMAGE\n\nMEND RESTORES 35 HEALTH",ink,1.5f);
    } else {
      renderer.DrawText(.28f,.25f,"INVENTORY SUMMARY",ink);
      renderer.DrawText(.28f,.33f,"STARTER SWORD EQUIPPED\n\nGOLD "+std::to_string(simulation_.Gold())+
        "\n\nEQUIPMENT MANAGEMENT NOT YET AVAILABLE",ink,1.5f);
    }
    renderer.DrawText(.28f,.69f,paused_?"ESC CLOSE  -  WORLD PAUSED":"ESC CLOSE  -  WORLD REMAINS ACTIVE",ink,1.5f);
  }
  if (paused_ || simulation_.Health()<=0) {
    renderer.DrawHudRect(.3f,.4f,.4f,.1f,panel);
    renderer.DrawText(.32f,.435f,paused_?"PAUSED - ESC TO RESUME":"DEFEATED - R TO RESTART",ink);
  }
}

std::wstring Game::WindowTitle() const {
  return L"Mediterranean - Astral Frontier | Native prototype";
}

void Game::SaveProgress() {
  try {
    const std::filesystem::path folder=RPG_SAVE_DIRECTORY;
    std::filesystem::create_directories(folder);
    const auto temporary=folder / "patrol.tmp";
    const auto destination=folder / "patrol.save";
    std::ofstream output(temporary,std::ios::binary|std::ios::trunc);
    output << simulation_.Save();
    output.close();
    if (!output || !MoveFileExW(temporary.c_str(),destination.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)) {
      simulation_.Notify("Save failed. Previous save retained."); return;
    }
    simulation_.Notify("Progress saved.");
  } catch (const std::exception&) { simulation_.Notify("Save failed. Check project folder access."); }
}

void Game::LoadProgress() {
  try {
    const auto file=std::filesystem::path(RPG_SAVE_DIRECTORY) / "patrol.save";
    std::error_code error;
    const auto length=std::filesystem::file_size(file,error);
    if (error || length>4096) { simulation_.Notify("No valid save found."); return; }
    std::ifstream input(file,std::ios::binary);
    std::ostringstream content; content << input.rdbuf();
    if (!input || !simulation_.Load(content.str())) { simulation_.Notify("Save rejected. Current progress retained."); return; }
    paused_=false;
    panel_=Panel::None;
    yaw_=0; pitch_=.45f; distance_=10;
  } catch (const std::exception&) { simulation_.Notify("Load failed. Current progress retained."); }
}
