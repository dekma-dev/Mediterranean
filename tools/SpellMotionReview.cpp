// Offline native game capture. Gameplay runs at 60 Hz; the file plays at 60 Hz.
// No player profile writes, audio playback, external encoder or downloads.
#include "game/Game.h"
#include "engine/Renderer.h"
#include "platform/Win32Window.h"
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <wincodec.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <cstring>
#include <memory>
using Microsoft::WRL::ComPtr;
namespace {
constexpr unsigned width=1280,height=720,fps=60;
void Check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
void Hr(HRESULT value){if(FAILED(value)){std::cerr<<"HRESULT 0x"<<std::hex<<value<<std::dec<<'\n';throw std::runtime_error("Video encoder/decoder operation failed");}}
class Movie {
 ComPtr<IMFSinkWriter> writer;ComPtr<IWICImagingFactory> images;DWORD stream=0;unsigned frame=0;
public:
 explicit Movie(const std::filesystem::path& path){
  ComPtr<IMFAttributes> options;Hr(MFCreateAttributes(&options,1));Hr(options->SetUINT32(MF_SINK_WRITER_DISABLE_THROTTLING,TRUE));
  Hr(MFCreateSinkWriterFromURL(path.c_str(),nullptr,options.Get(),&writer));
  ComPtr<IMFMediaType> output,input;Hr(MFCreateMediaType(&output));Hr(output->SetGUID(MF_MT_MAJOR_TYPE,MFMediaType_Video));
  Hr(output->SetGUID(MF_MT_SUBTYPE,MFVideoFormat_H264));Hr(output->SetUINT32(MF_MT_AVG_BITRATE,8000000));
  Hr(output->SetUINT32(MF_MT_INTERLACE_MODE,MFVideoInterlace_Progressive));
  Hr(MFSetAttributeSize(output.Get(),MF_MT_FRAME_SIZE,width,height));Hr(MFSetAttributeRatio(output.Get(),MF_MT_FRAME_RATE,fps,1));
  Hr(MFSetAttributeRatio(output.Get(),MF_MT_PIXEL_ASPECT_RATIO,1,1));Hr(writer->AddStream(output.Get(),&stream));
  Hr(MFCreateMediaType(&input));Hr(input->SetGUID(MF_MT_MAJOR_TYPE,MFMediaType_Video));Hr(input->SetGUID(MF_MT_SUBTYPE,MFVideoFormat_RGB32));
  Hr(input->SetUINT32(MF_MT_INTERLACE_MODE,MFVideoInterlace_Progressive));Hr(input->SetUINT32(MF_MT_DEFAULT_STRIDE,width*4));
  Hr(MFSetAttributeSize(input.Get(),MF_MT_FRAME_SIZE,width,height));Hr(MFSetAttributeRatio(input.Get(),MF_MT_FRAME_RATE,fps,1));
  Hr(MFSetAttributeRatio(input.Get(),MF_MT_PIXEL_ASPECT_RATIO,1,1));Hr(writer->SetInputMediaType(stream,input.Get(),nullptr));Hr(writer->BeginWriting());
  Hr(CoCreateInstance(CLSID_WICImagingFactory,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&images)));
 }
 void Write(const std::filesystem::path& path){
  ComPtr<IWICBitmapDecoder> decoder;ComPtr<IWICBitmapFrameDecode> image;ComPtr<IWICFormatConverter> converter;
  Hr(images->CreateDecoderFromFilename(path.c_str(),nullptr,GENERIC_READ,WICDecodeMetadataCacheOnLoad,&decoder));Hr(decoder->GetFrame(0,&image));
  Hr(images->CreateFormatConverter(&converter));Hr(converter->Initialize(image.Get(),GUID_WICPixelFormat32bppBGR,WICBitmapDitherTypeNone,nullptr,0,WICBitmapPaletteTypeCustom));
  ComPtr<IMFMediaBuffer> buffer;ComPtr<IMFSample> sample;Hr(MFCreateMemoryBuffer(width*height*4,&buffer));
  BYTE* bytes=nullptr;Hr(buffer->Lock(&bytes,nullptr,nullptr));
  const HRESULT copied=converter->CopyPixels(nullptr,width*4,width*height*4,bytes);Hr(buffer->Unlock());Hr(copied);
  Hr(buffer->SetCurrentLength(width*height*4));Hr(MFCreateSample(&sample));Hr(sample->AddBuffer(buffer.Get()));
  const LONGLONG time=LONGLONG(frame)*10000000/fps,next=LONGLONG(frame+1)*10000000/fps;
  Hr(sample->SetSampleTime(time));Hr(sample->SetSampleDuration(next-time));Hr(writer->WriteSample(stream,sample.Get()));++frame;
 }
 void Finish(){Hr(writer->Finalize());writer.Reset();}
};
rpg::Simulation Hero(rpg::ClassId id,bool major){
 rpg::Simulation s;Check(s.Load("MEDITERRANEAN 3\n-4 5.2 50 0 3 90\n5 1 1 0\n4\n0 0 0 0\n"
 "100 -4 7 0 0 1 3 0 0\n101 4 9 0 0 1 3 0 0\n102 0 12 0 0 1 3 0 0\n"),"Review hero");
 auto save=s.Save();const auto p=save.find("PROFILE ");Check(p!=std::string::npos,"Profile record");
 save.replace(p,save.find('\n',p)-p,"PROFILE "+std::to_string(int(id))+(major?" \"Review\" 8 1 3":" \"Review\" 4 1 3"));Check(s.Load(save),"Review class");return s;
}
}
int wmain(int argc,wchar_t** argv){try{
 if(argc<2 || argc>4 || (argc>=3 && std::wstring(argv[2])!=L"--major" && std::wstring(argv[2])!=L"--fields" && std::wstring(argv[2])!=L"--moving" && std::wstring(argv[2])!=L"--crowded" && std::wstring(argv[2])!=L"--signatures" && std::wstring(argv[2])!=L"--invocations" && std::wstring(argv[2])!=L"--opportunities") || (argc==4 && std::wstring(argv[3])!=L"--check-events")){std::cerr<<"Usage: SpellMotionReview <output-directory> [--major|--fields|--moving|--crowded|--signatures|--invocations|--opportunities] [--check-events]\n";return 2;}
 const bool fields=argc>=3 && std::wstring(argv[2])==L"--fields",moving=argc>=3 && std::wstring(argv[2])==L"--moving";
 const bool crowded=argc>=3 && std::wstring(argv[2])==L"--crowded";
 const bool signatures=argc>=3 && std::wstring(argv[2])==L"--signatures";
 const bool invocations=argc>=3 && std::wstring(argv[2])==L"--invocations";
 const bool opportunities=argc>=3 && std::wstring(argv[2])==L"--opportunities",checkOnly=argc==4;
 const bool major=argc>=3 && !moving;const int duration=opportunities?9:fields || crowded?7:moving || signatures || invocations?5:major?4:3;
 Hr(CoInitializeEx(nullptr,COINIT_MULTITHREADED));Hr(MFStartup(MF_VERSION));
 const auto out=std::filesystem::absolute(argv[1]);std::filesystem::create_directories(out);
 Win32Window window;Renderer r;if(!checkOnly){Check(window.Create(GetModuleHandleW(nullptr),width,height,L"Native spell motion review",false),"Window");
 Check(r.Initialize(window.GetHandle(),width,height),"Renderer");}
 struct Scene{rpg::ClassId school;int ability;const char* name;};
 const std::vector<Scene> scenes=opportunities?std::vector<Scene>{{rpg::ClassId::Warrior,0,"riposte"},{rpg::ClassId::Scout,0,"pursuit"},
  {rpg::ClassId::Mage,14,"confluence"},{rpg::ClassId::Priest,1,"benediction"}}:
  invocations?std::vector<Scene>{{rpg::ClassId::Warrior,9,"steel-load"},{rpg::ClassId::Scout,9,"bow-draw"},
  {rpg::ClassId::Mage,8,"frost-gather"},{rpg::ClassId::Mage,13,"arcane-weave"},{rpg::ClassId::Priest,6,"invocation"},{rpg::ClassId::Priest,3,"greater-heal"}}:
  signatures?std::vector<Scene>{{rpg::ClassId::Warrior,8,"steel-cleave"},{rpg::ClassId::Scout,9,"piercing-arrow"},
  {rpg::ClassId::Mage,9,"falling-star"},{rpg::ClassId::Priest,6,"sanctuary"}}:crowded?std::vector<Scene>{{rpg::ClassId::Warrior,8,"warrior-crowded"},{rpg::ClassId::Mage,8,"frost-crowded"},
  {rpg::ClassId::Scout,8,"arrows-crowded"},{rpg::ClassId::Priest,6,"sanctuary-crowded"}}:moving?std::vector<Scene>{{rpg::ClassId::Warrior,0,"warrior-moving"},{rpg::ClassId::Mage,0,"fire-moving"},
  {rpg::ClassId::Scout,1,"arrow-moving"},{rpg::ClassId::Priest,1,"holy-moving"}}:fields?std::vector<Scene>{{rpg::ClassId::Mage,8,"frost-field"},{rpg::ClassId::Mage,9,"ember-ground"},
  {rpg::ClassId::Priest,6,"holy-ground"},{rpg::ClassId::Mage,13,"arcane-ground"}}:major?std::vector<Scene>{{rpg::ClassId::Mage,0,"fire"},{rpg::ClassId::Mage,1,"heavy-fire"},
  {rpg::ClassId::Mage,8,"frost-bloom"},{rpg::ClassId::Mage,9,"meteor"},{rpg::ClassId::Mage,11,"sun-storm"},
  {rpg::ClassId::Mage,14,"thermal"},{rpg::ClassId::Priest,5,"absolution"},{rpg::ClassId::Priest,6,"sanctuary"}}:
  std::vector<Scene>{{rpg::ClassId::Mage,0,"fire"},{rpg::ClassId::Mage,1,"heavy-fire"},{rpg::ClassId::Mage,5,"frost"},
  {rpg::ClassId::Mage,13,"arcane"},{rpg::ClassId::Mage,14,"thermal"},{rpg::ClassId::Priest,1,"holy"}};
 std::ofstream manifest(out/"events.csv");manifest<<"scene,frame,seconds,event\n";
 std::ofstream budgets(out/"budgets.csv");budgets<<"scene,max_effect_vertices,max_lights,max_draw_calls,max_spell_volumes\n";
 std::unique_ptr<Movie> movie;if(!checkOnly)movie=std::make_unique<Movie>(out/"combat-motion.mp4");unsigned sceneIndex=0;
 for(const auto& scene:scenes){
  Game game;game.Initialize();Check(game.RestoreSnapshot(Hero(scene.school,major).Save()),"Review snapshot");
  auto& enemy=const_cast<rpg::Enemy&>(game.State().Enemies()[0]);enemy.position={-4,11.5f};enemy.home=enemy.position;
  enemy.kind=rpg::EnemyKind::Raider;enemy.health=enemy.MaxHealth();enemy.attackCooldown=100;enemy.rooted=100;enemy.state=rpg::EnemyState::Idle;
  if((signatures || invocations) && scene.school==rpg::ClassId::Warrior)enemy.position=enemy.home={-4,7.4f};
  if(moving){enemy.position=enemy.home={-4,scene.school==rpg::ClassId::Warrior?7.2f:11.5f};enemy.rooted=0;enemy.state=rpg::EnemyState::Pursuing;enemy.attackCooldown=.8f;}
  game.SetCameraOrbit(.35f,.55f,11);
  if(scene.ability==6 || scene.ability==11){
   // Earn the real resource using accepted opening attacks, before recording.
   // No fabricated reserve/contact events or private gameplay field writes.
   for(int tick=0;tick<1200 && !game.State().EmpoweredReady();++tick){rpg::Command warm;
    if(tick%120==0){enemy.health=enemy.MaxHealth();warm.selectActor=100;warm.ability=7;}game.ExecuteCommand(warm,1.f/fps);}
   Check(game.State().EmpoweredReady(),"Review could not earn signature reserve");
   for(int tick=0;tick<120;++tick)game.ExecuteCommand({},1.f/fps);
   enemy.health=enemy.MaxHealth();
  }
  if(crowded){
   for(unsigned i=0;i<3;++i){auto& foe=const_cast<rpg::Enemy&>(game.State().Enemies()[i]);
    foe.kind=i==0?rpg::EnemyKind::Raider:i==1?rpg::EnemyKind::Wraith:rpg::EnemyKind::Captain;
    foe.rank=1;foe.health=foe.MaxHealth();foe.rooted=foe.stunned=foe.snared=foe.windup=0;
    foe.position=foe.home={-4.f+(i==1?2.2f:i==2?-2.2f:0.f),(scene.school==rpg::ClassId::Warrior?7.2f:11.5f)+(i?.5f:0.f)};
    foe.state=rpg::EnemyState::Pursuing;foe.attackCooldown=.3f+i*.3f;foe.path.clear();
   }
  }
  if(opportunities){
   // Training props render at authored map positions. Use a real presented
   // enemy here, rather than moving only the simulation half of a dummy.
   enemy.position=enemy.home={-4,8.2f};enemy.rank=20;enemy.health=enemy.MaxHealth();
   if(scene.school==rpg::ClassId::Warrior){enemy.position=enemy.home={-4,6.5f};enemy.rank=9;enemy.health=enemy.MaxHealth();
    enemy.rooted=0;enemy.facing=3.14159265f;enemy.state=rpg::EnemyState::Pursuing;enemy.attackCooldown=0;}
   game.SetCameraOrbit(.35f,.55f,11);
  }
  bool released=false,contact=false,prepared=false;int releaseFrame=-1,hitFrame=-1;
  bool readyCaptured=false,opportunityReady=false,opportunitySpent=false;
  int rotationStep=0,nextCastFrame=20,readyFrame=-1;
  unsigned peakVertices=0,peakLights=0,peakDraws=0,peakVolumes=0;
  for(int frame=0;frame<duration*fps;++frame){
   rpg::Command command;if(frame==20 && !opportunities){command.selectActor=100;command.ability=scene.ability;}
   if(opportunities){
    command.selectActor=100;
    if(frame==20 && scene.school!=rpg::ClassId::Mage)command.ability=scene.school==rpg::ClassId::Warrior || scene.school==rpg::ClassId::Priest?3:
      scene.school==rpg::ClassId::Scout?2:0;
    if(scene.school==rpg::ClassId::Mage){
     static constexpr int rotation[]={0,12,13,14};
     if(rotationStep<4 && frame>=nextCastFrame && !game.State().Attacking() &&
       (rotationStep<3 || (readyFrame>=0 && frame>=readyFrame+36)))command.ability=rotation[rotationStep];
     if(rotationStep>=3 && readyFrame>=0 && frame>=readyFrame+24 && frame<readyFrame+105)command.movement={.15f,0};
    }else{
     if(frame==(scene.school==rpg::ClassId::Priest?180:130))command.ability=scene.ability;
     if(scene.school!=rpg::ClassId::Warrior && frame>=70 && frame<110 && !game.State().CastingRecovery())command.movement={.2f,0};
    }
   }
   if(moving && frame>100 && scene.school!=rpg::ClassId::Warrior)command.movement={.6f,-.6f};
   if(moving && frame==120){command.selectActor=100;command.ability=0;}
   if(crowded && frame>150 && frame<280)command.movement={.8f,-.4f};
   if(crowded && frame==150){command.selectActor=100;command.ability=0;}
   game.ExecuteCommand(command,1.f/fps);
   if(opportunities && scene.school==rpg::ClassId::Mage && command.ability>=0 && game.State().Attacking() && game.State().AttackAbility()==command.ability){++rotationStep;nextCastFrame=frame+12;}
   if(opportunities && readyFrame<0 && game.State().DisciplineSeconds()>0)readyFrame=frame;
   if(fields)game.SetCameraOrbit(.35f+.8f*combat::Smooth((frame/float(fps)-2.f)/3.f),.55f,11);
   for(std::size_t i=0;i<game.State().VisualEventCount();++i){const auto kind=game.State().VisualEvents()[i].kind;
    if(opportunities && (kind==rpg::CombatVisualKind::DisciplineReady || kind==rpg::CombatVisualKind::DisciplineSpent)){
     opportunityReady|=kind==rpg::CombatVisualKind::DisciplineReady;opportunitySpent|=kind==rpg::CombatVisualKind::DisciplineSpent;
     manifest<<scene.name<<','<<frame<<','<<(sceneIndex*duration+frame/60.f)<<','<<(kind==rpg::CombatVisualKind::DisciplineReady?"opportunity-ready":"opportunity-spent")<<'\n';
    }
    if(scene.ability==3 && kind==rpg::CombatVisualKind::Mend){released=contact=true;releaseFrame=hitFrame=frame;manifest<<scene.name<<','<<frame<<','<<(sceneIndex*duration+frame/60.f)<<",healed\n";}
    if(kind==rpg::CombatVisualKind::SpellCast || kind==rpg::CombatVisualKind::Strike || kind==rpg::CombatVisualKind::HeavyStrike){released=true;releaseFrame=frame;manifest<<scene.name<<','<<frame<<','<<(sceneIndex*duration+frame/60.f)<<",release\n";}
    if(kind==rpg::CombatVisualKind::Hit || kind==rpg::CombatVisualKind::HeavyHit){contact=true;hitFrame=frame;manifest<<scene.name<<','<<frame<<','<<(sceneIndex*duration+frame/60.f)<<",contact\n";}
   }
   if(checkOnly)continue;
   r.BeginFrame();game.Render(r);
   const auto image=out/L"frame.png";Check(r.Capture(image.wstring()) && r.EndFrame(false),"Motion capture");movie->Write(image);
   if(r.EffectVertices()>peakVertices){peakVertices=r.EffectVertices();std::filesystem::copy_file(image,out/(std::string(scene.name)+"-dense.png"),std::filesystem::copy_options::overwrite_existing);}
   peakLights=std::max(peakLights,r.CombatLightCount());peakDraws=std::max(peakDraws,r.LastDrawCalls());
   peakVolumes=std::max(peakVolumes,r.SpellVolumeCount());
   if(opportunities && !readyCaptured && game.State().DisciplineSeconds()>0 && readyFrame>=0 && frame>=readyFrame+15){readyCaptured=true;
    std::filesystem::copy_file(image,out/(std::string(scene.name)+"-ready.png"),std::filesystem::copy_options::overwrite_existing);}
   if(opportunities && scene.school==rpg::ClassId::Mage && frame==130)
    std::filesystem::copy_file(image,out/"confluence-elements.png",std::filesystem::copy_options::overwrite_existing);
   if(!prepared && ((game.State().Attacking() && game.State().AttackSeconds()/combat::StrokeStart(game.State().HeavyAttack())>.70f) ||
       (game.State().CastingRecovery() && game.State().RecoveryProgress()>.70f))){
    prepared=true;std::filesystem::copy_file(image,out/(std::string(scene.name)+"-prepare.png"),std::filesystem::copy_options::overwrite_existing);}
   if(frame==releaseFrame+3 && released)std::filesystem::copy_file(image,out/(std::string(scene.name)+"-flight.png"),std::filesystem::copy_options::overwrite_existing);
   if(frame==releaseFrame+8 && released)std::filesystem::copy_file(image,out/(std::string(scene.name)+"-travel.png"),std::filesystem::copy_options::overwrite_existing);
   if(frame==hitFrame+5 && contact)std::filesystem::copy_file(image,out/(std::string(scene.name)+"-contact.png"),std::filesystem::copy_options::overwrite_existing);
   if(fields && frame==hitFrame+75 && contact)std::filesystem::copy_file(image,out/(std::string(scene.name)+"-sustain.png"),std::filesystem::copy_options::overwrite_existing);
  }
  Check(released && contact,"Review spell did not produce real release/contact");
  if(opportunities)Check(opportunityReady && opportunitySpent,"Review did not earn and consume a real class opportunity");
  Check(peakVertices<=Renderer::maxEffectVertices && peakLights<=4 && peakVolumes<=24,"Review exceeded effects budgets");
  budgets<<scene.name<<','<<peakVertices<<','<<peakLights<<','<<peakDraws<<','<<peakVolumes<<'\n';
  std::cout<<scene.name<<": real release/contact recorded at 60 Hz\n"<<std::flush;++sceneIndex;
 }
 if(movie){movie->Finish();Check(r.ValidateDebugMessages(),"DirectX review warnings");}MFShutdown();
 std::cout<<"PASS: "<<scenes.size()*duration<<(checkOnly?"-second event preflight; no rendering.":"-second 720p60 native combat movie; no profile writes or synthetic contact events.")<<'\n';
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
