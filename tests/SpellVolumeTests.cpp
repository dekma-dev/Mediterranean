#include "engine/Renderer.h"
#include "game/ClassEffects.h"
#include "game/CombatEffects.h"
#include "platform/Win32Window.h"
#include <wincodec.h>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <algorithm>
#include <limits>
#include <fstream>

namespace {
using namespace DirectX;using Medium=Renderer::SpellMedium;using Pixels=std::vector<unsigned char>;
void Check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
Pixels Capture(Renderer& r,const std::filesystem::path& path){
 Check(r.Capture(path.wstring())&&r.EndFrame(false),"Capture volume frame");
 Microsoft::WRL::ComPtr<IWICImagingFactory> f;Microsoft::WRL::ComPtr<IWICBitmapDecoder> d;
 Microsoft::WRL::ComPtr<IWICBitmapFrameDecode> frame;Microsoft::WRL::ComPtr<IWICFormatConverter> c;
 Check(SUCCEEDED(CoCreateInstance(CLSID_WICImagingFactory,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&f))),"WIC");
 Check(SUCCEEDED(f->CreateDecoderFromFilename(path.c_str(),nullptr,GENERIC_READ,WICDecodeMetadataCacheOnLoad,&d))&&
  SUCCEEDED(d->GetFrame(0,&frame))&&SUCCEEDED(f->CreateFormatConverter(&c))&&SUCCEEDED(c->Initialize(frame.Get(),GUID_WICPixelFormat32bppRGBA,WICBitmapDitherTypeNone,nullptr,0,WICBitmapPaletteTypeCustom)),"Read volume image");
 Pixels pixels(std::size_t(r.Width())*r.Height()*4);Check(SUCCEEDED(c->CopyPixels(nullptr,r.Width()*4,UINT(pixels.size()),pixels.data())),"Volume pixels");return pixels;
}
double Difference(const Pixels& a,const Pixels& b){Check(a.size()==b.size(),"Pixel dimensions");double sum=0;
 for(std::size_t n=0;n<a.size();++n)sum+=std::abs(int(a[n])-int(b[n]));return sum/a.size();}
std::size_t ChangedPixels(const Pixels& a,const Pixels& b){std::size_t count=0;
 for(std::size_t n=0;n<a.size();n+=4)if(std::max({std::abs(int(a[n])-int(b[n])),std::abs(int(a[n+1])-int(b[n+1])),std::abs(int(a[n+2])-int(b[n+2]))})>=3)++count;
 return count;}
void Fields(Renderer& r,const std::filesystem::path& out){
 using Field=Renderer::SpellField;std::array<float,25> ground{};
 const auto begin=[&]{r.BeginFrame();r.SetCamera({0,4,-5},{0,0,0});r.DrawMarkerBox({0,-.16f,0},{9,.3f,9},{.20f,.18f,.15f,1});};
 const auto field=[&](Field kind,float age){r.DrawSpellField(kind,{0,0},2,age,{1,1,1,1},ground,17);};
 Pixels frost;
 for(int kind=0;kind<4;++kind){begin();field(Field(kind),.6f);const auto first=Capture(r,out/("field-"+std::to_string(kind)+".png"));
  Check(r.SpellFieldCount()==1,"Field record missing");
  begin();field(Field(kind),.6f);Check(first==Capture(r,out/"field-repeat.png"),"Drawing a field advances its time");
  begin();field(Field(kind),1.2f);Check(ChangedPixels(first,Capture(r,out/"field-motion.png"))>100,"Field has no internal motion");
  if(kind==0)frost=first;else Check(ChangedPixels(frost,first)>1000,"Field schools lost distinct patterns");
 }
 begin();const auto blank=Capture(r,out/"field-empty.png");begin();field(Field::Frost,0);Check(blank==Capture(r,out/"field-birth.png"),"Field pops in before its birth");
 begin();Check(r.SpellFieldCount()==0,"Stale field records");
 field(Field::Holy,1);Check(r.EffectVertices()==96,"Field patch escaped geometry allowance");Check(r.EndFrame(false),"Field frame");
 begin();for(int n=0;n<30;++n)field(Field::Holy,.5f);
 Check(r.SpellFieldCount()==12 && r.EffectVertices()==1152,"Field pool escaped fixed allocation");Check(r.EndFrame(false),"Field limit frame");
 begin();ground[4]=NAN;field(Field::Frost,.5f);ground[4]=0;
 field(Field::Frost,NAN);field(Field::Frost,-1);field(Field(99),.5f);
 Check(r.SpellFieldCount()==0 && r.EffectVertices()==0,"Invalid field was submitted");Check(r.EndFrame(false),"Rejected field frame");
 // Cover the complete patch with opaque scenery, including its glare source.
 const auto wall=[&]{begin();r.DrawMarkerBox({0,.6f,0},{7,1.2f,7},{.25f,.25f,.25f,1});};
 wall();const auto blocked=Capture(r,out/"field-wall.png");wall();field(Field::Holy,.8f);
 Check(blocked==Capture(r,out/"field-occluded.png"),"Field/glow shines through an opaque floor");
 begin();Check(r.BeginPortrait(.1f,.1f,.6f,.8f),"Field isolated preview");field(Field::Holy,.5f);
 Check(r.SpellFieldCount()==0 && r.EndPortrait() && r.EndFrame(false),"Field entered an isolated portrait");
}
void Native(Renderer& r,const std::filesystem::path& out){
 const auto begin=[&]{r.BeginFrame();r.SetCamera({0,1,-5},{0,1,0});};
 for(int kind=0;kind<4;++kind){Pixels previous;
  for(int view=0;view<3;++view)for(int sample=0;sample<3;++sample){
   begin();if(view==1)r.SetCamera({4,2,-1},{0,1,0});if(view==2)r.SetCamera({0,5,-1},{0,1,0});
   r.DrawSpellVolume(Medium(kind),{0,1,0},{0,0,1},{.45f,.45f,.8f},.1f+sample*.08f,{1,1,1,1},37);
   Check(r.SpellVolumeCount()==1,"Visible medium not submitted");
   const auto image=Capture(r,out/("medium-"+std::to_string(kind)+"-"+std::to_string(view)+"-"+std::to_string(sample)+".png"));
   if(sample){const double difference=Difference(previous,image);
    std::cout<<"Medium "<<kind<<" view "<<view<<" phase "<<sample<<" motion difference "<<difference<<'\n';
    // Measure visible changes inside the effect, not a mean dominated by the
    // untouched sky. The narrow holy shaft intentionally moves less than fire.
    Check(ChangedPixels(previous,image)>64,"Fixed-position medium has no internal motion");}previous=image;
  }
 }
 const auto wall=[&]{begin();r.DrawMarkerBox({0,1,0},{4,4,.12f},{.2f,.2f,.2f,1});};
 wall();const auto base=Capture(r,out/"wall-base.png");
 const auto volume=[&](float depth){r.DrawSpellVolume(Medium::Fire,{0,1,depth},{0,0,1},{.45f,.45f,.65f},.2f,{1,1,1,1},5);};
 wall();volume(-.95f);const auto front=Capture(r,out/"wall-front.png");
 wall();volume(.10f);const auto intersect=Capture(r,out/"wall-intersect.png");
 wall();volume(.95f);const auto behind=Capture(r,out/"wall-behind.png");
 Check(Difference(base,front)>.1,"Volume has no radiance");
 Check(Difference(base,intersect)>.01 && Difference(base,intersect)<Difference(base,front)*.95,"Volume not clipped within its interior");
 Check(Difference(base,behind)<.001,"Volume leaked through opaque wall");
 begin();for(int n=0;n<100;++n)r.DrawSpellVolume(Medium::Fire,{float(n%6)*.12f,1,0},{0,0,1},{.12f,.12f,.2f},.2f,{1,1,1,1},n);
 Check(r.SpellVolumeCount()==24 && r.EffectVertices()==24*6,"Volume count/geometry escaped fixed budget");Check(r.EndFrame(false),"Bounded volumes");
 begin();Check(r.SpellVolumeCount()==0,"Stale volumes after frame reset");
 const float nan=std::numeric_limits<float>::quiet_NaN();
 r.DrawSpellVolume(Medium::Fire,{nan,1,0},{0,0,1},{1,1,1},0,{1,1,1,1});
 r.DrawSpellVolume(Medium::Fire,{0,1,0},{0,0,0},{1,1,1},0,{1,1,1,1});
 r.DrawSpellVolume(Medium::Fire,{0,1,0},{0,0,1},{1,-1,1},0,{1,1,1,1});
 r.DrawSpellVolume(Medium::Fire,{0,1,0},{0,0,1},{1,1,1},nan,{1,1,1,1});
 r.DrawSpellVolume(Medium::Fire,{0,1,0},{1e30f,0,1},{1,1,1},0,{1,1,1,1});
 r.DrawSpellVolume(Medium::Fire,{0,1,0},{0,0,1},{1e-30f,1,1},0,{1,1,1,1});
 r.DrawSpellVolume(Medium::Fire,{0,1,-5},{0,0,1},{1,1,1},0,{1,1,1,1});
 Check(r.EffectVertices()==0,"Invalid/lens-covering medium submitted");
 Check(r.BeginPortrait(0,0,180,200),"Portrait start");volume(0);
 Check(r.SpellVolumeCount()==0 && r.EndPortrait() && r.EndFrame(false),"Volume entered isolated portrait");
 for(auto size:{std::pair{1280,720},std::pair{960,600}}){Check(r.Resize(size.first,size.second),"Resize volume resources");begin();volume(0);Check(r.EndFrame(false),"Volume after resize");}
}
void Flights(Renderer& r,const std::filesystem::path& out){
 using namespace rpg;
 const auto begin=[&]{r.BeginFrame();r.SetCamera({5,6,-9},{0,2,1});};
 for(auto school:{ClassId::Mage,ClassId::Priest})for(int ability:{0,1,5,6,9,13,14}){
  if(school==ClassId::Priest && ability>=8)continue;
  CombatVisualEvent e;e.kind=CombatVisualKind::SpellCast;e.school=school;e.ability=ability;e.heavy=ability==1 || ability>=5;
  e.destination={2,3};e.destinationHeight=1.2f;e.flight=.40f;e.facing=.7f;const XMFLOAT3 origin{-2,1.2f,-1};
  Pixels first;
  for(int hz:{22,30,60,144,1000}){CombatEffects effects;effects.Emit(e,&origin);float age=0;
   while(age<.173f-1e-6f){const float dt=std::min(1.f/hz,.173f-age);effects.Advance(dt);age+=dt;}
   begin();effects.Render(r);Check(r.SpellVolumeCount()>0 && r.EffectVertices()<1200,"Spell lost volume or exceeded geometry budget");
   const auto image=Capture(r,out/("equal-"+std::to_string(int(school))+"-"+std::to_string(ability)+"-"+std::to_string(hz)+".png"));
   if(hz==22)first=image;else Check(Difference(first,image)<.03,"Medium/wake changed with refresh rate");
   for(int step=0;step<120;++step)effects.Advance(1.f/60);
   begin();effects.Render(r);Check(r.EffectVertices()==0 && r.SpellVolumeCount()==0 && r.EndFrame(false),"Expired volume remains");
  }
  begin();e.destination.x=std::numeric_limits<float>::quiet_NaN();classfx::Projectile(r,e,origin,.1f);
  Check(r.EffectVertices()==0 && r.EndFrame(false),"Malformed flight entered renderer");
 }
}
void Glow(Renderer& r,const std::filesystem::path& out){
 const auto begin=[&]{r.BeginFrame();r.SetCamera({0,1,-5},{0,1,0});
  r.DrawMesh(visual::Mesh::Box,{0,1,2},{15,15,.1f},{.025f,.025f,.025f,1},visual::Material::Glow);};
 const auto source=[&]{r.DrawSpellVolume(Medium::Fire,{0,1,0},{0,0,1},{.35f,.35f,.5f},.22f,{1,1,1,1},7);};
 r.SetSpellGlowEnabled(false);begin();const auto empty=Capture(r,out/"glow-empty.png");
 begin();source();const auto sharp=Capture(r,out/"glow-off.png");Check(r.SpellGlowPasses()==0,"Disabled glare still submits passes");
 r.SetSpellGlowEnabled(true);begin();source();const auto lit=Capture(r,out/"glow-on.png");
 Check(r.SpellGlowPasses()==4 && r.SpellGlowBytes()<=960u*540*24,"Glare allocation/draw ceiling");
 std::size_t halo=0;
 for(std::size_t n=0;n<lit.size();n+=4){int original=0,added=0;for(int c=0;c<3;++c){
  original=std::max(original,std::abs(int(sharp[n+c])-int(empty[n+c])));
  added=std::max(added,int(lit[n+c])-int(sharp[n+c]));}
  if(original<2 && added>2)++halo;
 }
 Check(halo>30,"Emission has no glow outside the sharp core");
 begin();const auto cleared=Capture(r,out/"glow-next-empty.png");
 Check(r.SpellGlowPasses()==0 && Difference(empty,cleared)==0,"Glare ghosts survive into an empty frame");
 Pixels smoke,occluded,hud,stage;
 for(bool enabled:{false,true}){
  r.SetSpellGlowEnabled(enabled);begin();r.DrawEffectSprite(Renderer::EffectArt::Smoke,{0,1,0},.6f,.6f,0,{1,1,1,.8f},true);
  const auto image=Capture(r,out/(enabled?"smoke-glow-on.png":"smoke-glow-off.png"));
  if(!enabled)smoke=image;else Check(Difference(smoke,image)==0,"Smoke incorrectly emits light");
  begin();r.DrawMarkerBox({0,1,-1},{3,3,.2f},{.1f,.1f,.1f,1});source();
  const auto behind=Capture(r,out/(enabled?"hidden-glow-on.png":"hidden-glow-off.png"));
  if(!enabled)occluded=behind;else Check(Difference(occluded,behind)==0,"Hidden source leaks glare through opaque geometry");
  begin();source();r.DrawHudRect(.4f,.4f,.2f,.2f,{.13f,.24f,.36f,1});
  const auto ui=Capture(r,out/(enabled?"hud-glow-on.png":"hud-glow-off.png"));
  if(!enabled)hud=ui;else for(int y=r.Height()*4/10+1;y<r.Height()*6/10-1;++y)for(int x=r.Width()*4/10+1;x<r.Width()*6/10-1;++x)
   for(int c=0;c<3;++c)Check(ui[(std::size_t(y)*r.Width()+x)*4+c]==hud[(std::size_t(y)*r.Width()+x)*4+c],"Spell glare blurred/tinted an opaque HUD element");
  begin();Check(r.BeginPortrait(.25f,.2f,.5f,.6f,0,true),"Glare stage");
  r.DrawSpellVolume(Medium::Holy,{.9f,1,0},{0,1,0},{.25f,.25f,.8f},.22f,{1,1,1,1},7);Check(r.EndPortrait(),"Glare stage end");
  const auto portrait=Capture(r,out/(enabled?"stage-glow-on.png":"stage-glow-off.png"));
  if(!enabled)stage=portrait;else {
   Check(ChangedPixels(stage,portrait)>30,"Creation stage has no spell glare");
   for(int y=0;y<r.Height();++y)for(int x=0;x<r.Width();++x)if(x<r.Width()/4 || x>=r.Width()*3/4 || y<r.Height()/5 || y>=r.Height()*4/5)
    for(int c=0;c<3;++c)Check(stage[(std::size_t(y)*r.Width()+x)*4+c]==portrait[(std::size_t(y)*r.Width()+x)*4+c],"Glare escaped the creation viewport");
  }
 }
 for(auto size:{std::pair{1279,719},std::pair{2560,1080},std::pair{960,600}}){
  Check(r.Resize(size.first,size.second),"Glare resize");begin();source();Check(r.EndFrame(false),"Glare resized frame");
  Check(r.SpellGlowBytes()<=960u*540*24,"Glare allocation grew beyond the fixed cap");
 }
}
void Cost(Renderer& r,const std::filesystem::path& out){
 std::ofstream report(out/"performance.txt");report<<r.AdapterDiagnostics()<<'\n';
 for(bool enabled:{false,true}){
  const auto first=r.GpuFrame()+1;std::vector<double> timings;unsigned vertices=0,volumes=0;
  for(int frame=0;frame<80;++frame){
   r.BeginFrame();r.SetCamera({5,4,-8},{0,1,1});r.SetAtmosphereTime(120);
   r.DrawMesh(visual::Mesh::Box,{0,-.2f,1},{8,.2f,8},{.4f,.36f,.31f,1},visual::Material::Stone);
   if(enabled)for(int spell=0;spell<6;++spell){
    rpg::CombatVisualEvent e;e.kind=rpg::CombatVisualKind::SpellCast;e.school=rpg::ClassId::Mage;e.ability=0;
    e.flight=.4f;e.destination={-2.f+spell*.8f,3};e.destinationHeight=1.2f;
    classfx::Projectile(r,e,{-2.f+spell*.8f,1.2f,-3},.19f);
   }
   vertices=r.EffectVertices();volumes=r.SpellVolumeCount();
   Check(vertices<Renderer::maxEffectVertices-1000,"Six flights exhaust warning reserve");
   if(enabled)Check(volumes==24,"Six-flight profile lost expected volumes to a budget");
   if(frame%8==7)Check(r.Capture((out/(frame==79?(enabled?"six-flights.png":"six-flights-base.png"):"gpu-sync.png")).wstring()),"GPU sync");
   Check(r.EndFrame(false),"GPU workload");
   for(unsigned n=0;n<r.GpuReadyCount();++n){const auto sample=r.GpuReady()[n];
    if(sample.frame>=first+8 && (sample.frame-first)%8!=7)timings.push_back(sample.milliseconds);}
  }
  Check(!timings.empty(),"No native GPU samples");std::sort(timings.begin(),timings.end());
  report<<(enabled?"Six simultaneous fire flights":"Empty scene")<<": median GPU "<<timings[timings.size()/2]<<" ms; "<<vertices<<" effect vertices; "<<volumes<<" volumes\n";
 }
}
}
int main(int argc,char** argv){try{
 Check(SUCCEEDED(CoInitializeEx(nullptr,COINIT_MULTITHREADED)),"COM");
 const auto out=argc>1?std::filesystem::path(argv[1]):std::filesystem::path("spell-volume-check");std::filesystem::create_directories(out);
 Win32Window window;Check(window.Create(GetModuleHandleW(nullptr),960,600,L"Spell volume checks",false),"Window");
 Renderer r;Check(r.Initialize(window.GetHandle(),960,600),"Renderer");Fields(r,out);Glow(r,out);Native(r,out);Flights(r,out);Cost(r,out);
 Check(r.ValidateDebugMessages(),"DirectX volume binding/depth hazard");
 std::cout<<"PASS: animated media from three views, in-volume opaque clipping, bounded submission, portrait/resize, equal-age flight and expiry.\n";
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
