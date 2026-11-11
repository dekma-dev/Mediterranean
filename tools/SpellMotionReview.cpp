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
 if(argc<2 || argc>3 || (argc==3 && std::wstring(argv[2])!=L"--major")){std::cerr<<"Usage: SpellMotionReview <output-directory> [--major]\n";return 2;}
 const bool major=argc==3;const int duration=major?4:3;
 Hr(CoInitializeEx(nullptr,COINIT_MULTITHREADED));Hr(MFStartup(MF_VERSION));
 const auto out=std::filesystem::absolute(argv[1]);std::filesystem::create_directories(out);
 Win32Window window;Check(window.Create(GetModuleHandleW(nullptr),width,height,L"Native spell motion review",false),"Window");
 Renderer r;Check(r.Initialize(window.GetHandle(),width,height),"Renderer");
 struct Scene{rpg::ClassId school;int ability;const char* name;};
 const std::vector<Scene> scenes=major?std::vector<Scene>{{rpg::ClassId::Mage,0,"fire"},{rpg::ClassId::Mage,1,"heavy-fire"},
  {rpg::ClassId::Mage,8,"frost-bloom"},{rpg::ClassId::Mage,9,"meteor"},{rpg::ClassId::Mage,11,"sun-storm"},
  {rpg::ClassId::Mage,14,"thermal"},{rpg::ClassId::Priest,5,"absolution"},{rpg::ClassId::Priest,6,"sanctuary"}}:
  std::vector<Scene>{{rpg::ClassId::Mage,0,"fire"},{rpg::ClassId::Mage,1,"heavy-fire"},{rpg::ClassId::Mage,5,"frost"},
  {rpg::ClassId::Mage,13,"arcane"},{rpg::ClassId::Mage,14,"thermal"},{rpg::ClassId::Priest,1,"holy"}};
 std::ofstream manifest(out/"events.csv");manifest<<"scene,frame,seconds,event\n";
 Movie movie(out/"combat-motion.mp4");unsigned sceneIndex=0;
 for(const auto& scene:scenes){
  Game game;game.Initialize();Check(game.RestoreSnapshot(Hero(scene.school,major).Save()),"Review snapshot");
  auto& enemy=const_cast<rpg::Enemy&>(game.State().Enemies()[0]);enemy.position={-4,11.5f};enemy.home=enemy.position;
  enemy.kind=rpg::EnemyKind::Raider;enemy.health=enemy.MaxHealth();enemy.attackCooldown=100;enemy.rooted=100;enemy.state=rpg::EnemyState::Idle;
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
  bool released=false,contact=false;int releaseFrame=-1,hitFrame=-1;
  for(int frame=0;frame<duration*fps;++frame){
   rpg::Command command;if(frame==20){command.selectActor=100;command.ability=scene.ability;}
   game.ExecuteCommand(command,1.f/fps);
   for(std::size_t i=0;i<game.State().VisualEventCount();++i){const auto kind=game.State().VisualEvents()[i].kind;
    if(kind==rpg::CombatVisualKind::SpellCast){released=true;releaseFrame=frame;manifest<<scene.name<<','<<frame<<','<<(sceneIndex*duration+frame/60.f)<<",release\n";}
    if(kind==rpg::CombatVisualKind::Hit || kind==rpg::CombatVisualKind::HeavyHit){contact=true;hitFrame=frame;manifest<<scene.name<<','<<frame<<','<<(sceneIndex*duration+frame/60.f)<<",contact\n";}
   }
   r.BeginFrame();game.Render(r);
   const auto image=out/L"frame.png";Check(r.Capture(image.wstring()) && r.EndFrame(false),"Motion capture");movie.Write(image);
   if(frame==releaseFrame+3 && released)std::filesystem::copy_file(image,out/(std::string(scene.name)+"-flight.png"),std::filesystem::copy_options::overwrite_existing);
   if(frame==hitFrame+5 && contact)std::filesystem::copy_file(image,out/(std::string(scene.name)+"-contact.png"),std::filesystem::copy_options::overwrite_existing);
  }
  Check(released && contact,"Review spell did not produce real release/contact");
  std::cout<<scene.name<<": real release/contact recorded at 60 Hz\n"<<std::flush;++sceneIndex;
 }
 movie.Finish();Check(r.ValidateDebugMessages(),"DirectX review warnings");MFShutdown();
 std::cout<<"PASS: "<<scenes.size()*duration<<"-second 720p60 native combat movie; no profile writes or synthetic contact events.\n";
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
