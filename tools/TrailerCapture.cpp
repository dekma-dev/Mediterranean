// Offline 720p30 native trailer with a stereo AAC soundtrack and game cues.
// Staged in-memory encounters; no profile writes, playback or downloads.
#include "game/Game.h"
#include "engine/Audio.h"
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
constexpr unsigned width=1280,height=720,fps=30;
void Check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
void Hr(HRESULT value){if(FAILED(value)){std::cerr<<"HRESULT 0x"<<std::hex<<value<<std::dec<<'\n';throw std::runtime_error("Video encoder/decoder operation failed");}}
class Movie {
 ComPtr<IMFSinkWriter> writer;ComPtr<IWICImagingFactory> images;DWORD stream=0,audioStream=0;unsigned frame=0;std::vector<short> audio;std::array<std::vector<short>,size_t(sound::Cue::Count)> effects;
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
  Hr(MFSetAttributeRatio(input.Get(),MF_MT_PIXEL_ASPECT_RATIO,1,1));Hr(writer->SetInputMediaType(stream,input.Get(),nullptr));
  std::ifstream music(path.parent_path()/"music.pcm",std::ios::binary|std::ios::ate);Check(bool(music),"Missing soundtrack");
  audio.resize(size_t(music.tellg())/2);music.seekg(0);music.read(reinterpret_cast<char*>(audio.data()),audio.size()*2);
  ComPtr<IMFMediaType> ao,ai;Hr(MFCreateMediaType(&ao));Hr(ao->SetGUID(MF_MT_MAJOR_TYPE,MFMediaType_Audio));Hr(ao->SetGUID(MF_MT_SUBTYPE,MFAudioFormat_AAC));
  Hr(ao->SetUINT32(MF_MT_AUDIO_NUM_CHANNELS,2));Hr(ao->SetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND,48000));Hr(ao->SetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE,16));Hr(ao->SetUINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND,24000));Hr(writer->AddStream(ao.Get(),&audioStream));
  Hr(MFCreateMediaType(&ai));Hr(ai->SetGUID(MF_MT_MAJOR_TYPE,MFMediaType_Audio));Hr(ai->SetGUID(MF_MT_SUBTYPE,MFAudioFormat_PCM));
  Hr(ai->SetUINT32(MF_MT_AUDIO_NUM_CHANNELS,2));Hr(ai->SetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND,48000));Hr(ai->SetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE,16));Hr(ai->SetUINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT,4));Hr(ai->SetUINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND,192000));Hr(writer->SetInputMediaType(audioStream,ai.Get(),nullptr));Hr(writer->BeginWriting());
  Hr(CoCreateInstance(CLSID_WICImagingFactory,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&images)));
 }
 void Effect(sound::Cue cue){
  auto& pcm=effects[size_t(cue)];if(pcm.empty())pcm=sound::Generate(cue);
  const size_t offset=size_t(frame)*1600*2;
  for(size_t i=0;i<pcm.size()*48000/22050 && offset+i*2+1<audio.size();++i){
   const float p=float(i)*22050/48000;const size_t k=size_t(p);const float f=p-k;
   const float v=(pcm[k]*(1-f)+pcm[std::min(k+1,pcm.size()-1)]*f)*.40f;
   for(int c=0;c<2;++c)audio[offset+i*2+c]=short(std::clamp(float(audio[offset+i*2+c])+v,-31000.f,31000.f));
  }
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
  Hr(sample->SetSampleTime(time));Hr(sample->SetSampleDuration(next-time));Hr(writer->WriteSample(stream,sample.Get()));
  Check((size_t(frame)+1)*3200<=audio.size(),"Soundtrack shorter than movie");
  ComPtr<IMFMediaBuffer> ab;ComPtr<IMFSample> as;Hr(MFCreateMemoryBuffer(6400,&ab));BYTE* dest=nullptr;Hr(ab->Lock(&dest,nullptr,nullptr));std::memcpy(dest,audio.data()+size_t(frame)*3200,6400);Hr(ab->Unlock());Hr(ab->SetCurrentLength(6400));Hr(MFCreateSample(&as));Hr(as->AddBuffer(ab.Get()));Hr(as->SetSampleTime(time));Hr(as->SetSampleDuration(next-time));Hr(writer->WriteSample(audioStream,as.Get()));++frame;
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
 Check(argc==2,"TrailerCapture <output-directory containing music.pcm>");
 Hr(CoInitializeEx(nullptr,COINIT_MULTITHREADED));Hr(MFStartup(MF_VERSION));
 const auto out=std::filesystem::absolute(argv[1]);std::filesystem::create_directories(out);
 Win32Window window;Renderer r;Check(window.Create(GetModuleHandleW(nullptr),width,height,L"Mediterranean trailer capture",false),"Window");
 Check(r.Initialize(window.GetHandle(),width,height),"Renderer");Movie movie(out/"Mediterranean-preview.mp4");
 struct Shot{rpg::ClassId school;int ability,seconds,kind;const char* title;};
 const Shot shots[]={
 {rpg::ClassId::Mage,-1,6,1,"MEDITERRANEAN"},{rpg::ClassId::Mage,-1,6,2,"ДОРОГИ ПОГРАНИЧЬЯ"},
 {rpg::ClassId::Priest,-1,4,3,"ЖРЕЦ"},{rpg::ClassId::Priest,1,7,0,"СВЕТ ПРОТИВ ТЬМЫ"},{rpg::ClassId::Priest,6,7,0,"СВЯТИЛИЩЕ"},
 {rpg::ClassId::Psionic,-1,4,3,"МИСТИК"},{rpg::ClassId::Psionic,1,8,0,"СИЛА РАЗУМА"},
 {rpg::ClassId::Mage,-1,4,3,"МАГ"},{rpg::ClassId::Mage,1,6,0,"ПЛАМЯ"},{rpg::ClassId::Mage,9,7,0,"ПАДАЮЩАЯ ЗВЕЗДА"},
 {rpg::ClassId::Mage,-1,5,1,"MEDITERRANEAN"}};
 std::ofstream log(out/"shots.csv");log<<"shot,frame,event\n";unsigned shotIndex=0;
 for(const auto& shot:shots){
  Game game;game.Initialize();Check(game.RestoreSnapshot(Hero(shot.school,true).Save()),"Hero fixture");
  game.OpenPanel(ui::Panel::None);auto& enemy=const_cast<rpg::Enemy&>(game.State().Enemies()[0]);
  if(shot.kind==0){enemy.kind=rpg::EnemyKind::Raider;enemy.rank=8;enemy.position=enemy.home={-4,11.5f};enemy.health=enemy.MaxHealth();enemy.rooted=100;enemy.attackCooldown=100;enemy.state=rpg::EnemyState::Idle;}
  if(shot.ability==6){
   for(int tick=0;tick<1200&&!game.State().EmpoweredReady();++tick){rpg::Command warm;if(tick%120==0){enemy.health=enemy.MaxHealth();warm.selectActor=100;warm.ability=7;}game.ExecuteCommand(warm,1.f/60);}
   Check(game.State().EmpoweredReady(),"Priest reserve");for(int n=0;n<120;++n)game.ExecuteCommand({},1.f/60);
  }
  if(shot.kind==0){enemy.health=enemy.MaxHealth();enemy.rooted=0;enemy.attackCooldown=3.5f;enemy.state=rpg::EnemyState::Pursuing;}
  sound::Events soundEvents;bool contact=false;
  for(int frame=0;frame<shot.seconds*fps;++frame){
   const float time=float(frame)/fps,u=time/shot.seconds; rpg::Command cmd;
   if(shot.kind==0){cmd.selectActor=100;if(frame==24)cmd.ability=shot.ability;
    if(frame==120 && shot.ability!=6)cmd.ability=0;
    if(time>4.8f && !game.State().CastingRecovery())cmd.movement={.35f,-.25f};}
   game.ExecuteCommand(cmd,1.f/fps);soundEvents.Update(game.State());
   if(shot.kind==0)for(unsigned i=0;i<soundEvents.count;++i)movie.Effect(soundEvents.cues[i]);
   for(size_t i=0;i<game.State().VisualEventCount();++i){const auto& e=game.State().VisualEvents()[i];
    if(e.kind==rpg::CombatVisualKind::Hit||e.kind==rpg::CombatVisualKind::HeavyHit){contact=true;log<<shotIndex<<','<<frame<<",contact\n";}}
   game.SetCameraOrbit(shot.kind==3?3.0f+u*.65f:.30f+u*.48f,shot.kind==3?.12f:.48f,shot.kind==3?3.4f:10.5f);
   r.BeginFrame();game.Render(r,false);
   if(shot.kind==1)r.SetCamera({-20+u*12,17-u*3,-27+u*4},{0,2,-5});
   if(shot.kind==2){const float y=rpg::WorldMap::Height({-55,-90});r.SetCamera({-70+u*12,y+18,-76-u*5},{-51,y+2,-95});}
   r.DrawHudRect(0,0,1,.065f,{0,0,0,1});r.DrawHudRect(0,.90f,1,.10f,{0,0,0,1});
   const float caption=std::clamp((time-.3f)*2,0.f,1.f)*std::clamp((shot.seconds-time-.25f)*2,0.f,1.f);
   r.DrawText(.055f,.923f,shot.title,{.86f,.78f,.56f,caption},1.8f,true);
   r.DrawText(.73f,.938f,"ТЕКУЩАЯ ВЕРСИЯ • GAMEPLAY",{.60f,.62f,.65f,caption},.85f,true);
   const float fade=1-std::min(std::clamp(time/.35f,0.f,1.f),std::clamp((shot.seconds-time)/.35f,0.f,1.f));
   r.DrawHudRect(0,0,1,1,{0,0,0,fade});
   const auto file=out/"frame.png";Check(r.Capture(file.wstring())&&r.EndFrame(false),"Trailer frame");movie.Write(file);
   if(frame==shot.seconds*fps/2)std::filesystem::copy_file(file,out/("shot-"+std::to_string(shotIndex)+".png"),std::filesystem::copy_options::overwrite_existing);
  }
  if(shot.kind==0)Check(contact,"A combat shot had no actual player hit");
  std::cout<<"Shot "<<shotIndex<<" complete\n"<<std::flush;++shotIndex;
 }
 movie.Finish();Check(r.ValidateDebugMessages(),"DirectX validation");MFShutdown();std::cout<<"PASS: 64-second native preview with game music and effects\n";
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
