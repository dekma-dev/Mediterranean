// Native in-memory review of group recovery and committed attack warnings.
#include "game/Game.h"
#include "engine/Renderer.h"
#include "platform/Win32Window.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
namespace {void Check(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}}
int wmain(int argc,wchar_t** argv){try {
  Check(argc==2,"EnemyTacticsReview <output-directory>");
  const auto out=std::filesystem::absolute(argv[1]);std::filesystem::create_directories(out);
  Win32Window window;Renderer renderer;
  Check(window.Create(GetModuleHandleW(nullptr),1280,720,L"Group combat review",false),"Window");
  Check(renderer.Initialize(window.GetHandle(),1280,720),"Renderer");
  Game game;game.Initialize();
  Check(game.RestoreSnapshot("MEDITERRANEAN 1\n-4 5.2 100 3.14159 0 0\n0 0 0 0\n-4 7 80 0\n4 9 80 0\n0 12 80 0\n"),"Fixture");
  game.OpenPanel(ui::Panel::None);
  auto& actors=const_cast<std::vector<rpg::Enemy>&>(game.State().Enemies());
  for(int i=0;i<3;++i){auto& e=actors[i];e.kind=rpg::EnemyKind::Raider;
    e.position=e.home={-4.f+(i-1),6.8f};e.state=rpg::EnemyState::Pursuing;
    e.attackCooldown=2;e.windup=0;e.facing=3.14159265f;}
  game.SetCameraOrbit(0,.65f,8.f);
  std::ofstream trace(out/"formation.csv");trace<<"frame,id,x,z,windup,health\n";
  for(int frame=0;frame<=180;++frame){
    if(frame>0)game.ExecuteCommand({},1.f/60);
    for(const auto& e:game.State().Enemies())trace<<frame<<','<<e.id<<','<<e.position.x<<','<<e.position.z<<','<<e.windup<<','<<game.State().Health()<<'\n';
    renderer.BeginFrame();game.Render(renderer,false);
    if(frame%15==0)Check(renderer.Capture((out/("frame-"+std::to_string(frame)+".png")).wstring()),"Capture");
    Check(renderer.EndFrame(false),"Present");
  }
  Check(renderer.ValidateDebugMessages(),"D3D validation");
  std::cout<<"PASS: native formation sequence and actual attack trace\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
