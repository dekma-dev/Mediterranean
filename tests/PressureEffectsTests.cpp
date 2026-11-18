#include "engine/Renderer.h"
#include "game/CombatEffects.h"
#include "game/ImpactPressure.h"
#include "platform/Win32Window.h"
#include <wincodec.h>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <limits>
using namespace DirectX;
using Pixels=std::vector<unsigned char>;
void Check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
Pixels Capture(Renderer& r,const std::filesystem::path& path){
 Check(r.Capture(path.wstring()) && r.EndFrame(false),"Native pressure capture failed");
 Microsoft::WRL::ComPtr<IWICImagingFactory> factory;Microsoft::WRL::ComPtr<IWICBitmapDecoder> decoder;
 Microsoft::WRL::ComPtr<IWICBitmapFrameDecode> frame;Microsoft::WRL::ComPtr<IWICFormatConverter> convert;
 Check(SUCCEEDED(CoCreateInstance(CLSID_WICImagingFactory,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&factory))),"WIC factory");
 Check(SUCCEEDED(factory->CreateDecoderFromFilename(path.c_str(),nullptr,GENERIC_READ,WICDecodeMetadataCacheOnLoad,&decoder)) &&
   SUCCEEDED(decoder->GetFrame(0,&frame)) && SUCCEEDED(factory->CreateFormatConverter(&convert)) &&
   SUCCEEDED(convert->Initialize(frame.Get(),GUID_WICPixelFormat32bppRGBA,WICBitmapDitherTypeNone,nullptr,0,WICBitmapPaletteTypeCustom)),"WIC decode");
 Pixels pixels(std::size_t(r.Width())*r.Height()*4);Check(SUCCEEDED(convert->CopyPixels(nullptr,r.Width()*4,UINT(pixels.size()),pixels.data())),"Read pixels");return pixels;
}
unsigned Changed(const Pixels& a,const Pixels& b){Check(a.size()==b.size(),"Image dimensions");unsigned count=0;
 for(std::size_t n=0;n<a.size();n+=4)if(std::max({abs(int(a[n])-b[n]),abs(int(a[n+1])-b[n+1]),abs(int(a[n+2])-b[n+2])})>2)++count;return count;}
void Stage(Renderer& r,bool wall=false,bool rod=false){
 r.BeginFrame();r.SetCamera({0,1,-6},{0,1,0});r.SetSpellGlowEnabled(false);
 for(int x=-8;x<=8;++x)for(int y=-5;y<=5;++y)r.DrawMarkerBox({x*.40f,1+y*.40f,2.4f},{.39f,.39f,.08f},
   (x+y)%2?XMFLOAT4{.25f,.27f,.29f,1}:XMFLOAT4{.72f,.68f,.59f,1});
 if(wall)r.DrawMarkerBox({0,1,-1.2f},{10,10,.10f},{.28f,.30f,.32f,1});
 if(rod)r.DrawMarkerBox({0,1,-2},{.12f,3,.10f},{.8f,.06f,.04f,1});
}
void Wave(Renderer& r,float phase=.15f){r.DrawPressureWave({0,1,0},{1.25f,.95f,.9f},.35f,phase,1,1);}
int main(int argc,char** argv){try{
 const auto out=std::filesystem::absolute(argc>1?argv[1]:"pressure-review");std::filesystem::create_directories(out);
 Check(SUCCEEDED(CoInitializeEx(nullptr,COINIT_MULTITHREADED)),"COM");Win32Window window;Renderer r;
 Check(window.Create(GetModuleHandleW(nullptr),640,480,L"Pressure rendering checks",false) && r.Initialize(window.GetHandle(),640,480),"Renderer startup");
 Stage(r);const auto base=Capture(r,out/"base.png");Check(r.PressureBytes()==0,"Idle rendering allocated a pressure texture");
 Stage(r);Wave(r);const auto front=Capture(r,out/"refraction.png");const auto changed=Changed(base,front);
 std::cout<<"Refracted checker pixels="<<changed<<'\n';
 Check(changed>150 && changed<640*480/2,"Refraction must move a local background without a full-screen filter");
 Check(r.PressurePasses()==1 && r.PressureBytes()==640u*480*4,"One copy/pass and one RGBA8 texture required");
 Stage(r);Wave(r);Check(front==Capture(r,out/"repeat.png"),"Equal-age refraction depends on render history");
 Stage(r);Wave(r,.28f);Check(Changed(front,Capture(r,out/"flow.png"))>50,"Accepted effect age must animate the air front");
 Stage(r,true);const auto wall=Capture(r,out/"wall.png");Stage(r,true);Wave(r);
 Check(wall==Capture(r,out/"occluded.png"),"A fully occluded air volume distorted the foreground wall");
 Stage(r,false,true);const auto rod=Capture(r,out/"rod.png");Stage(r,false,true);Wave(r);const auto behind=Capture(r,out/"rod-pressure.png");
 unsigned red=0,disturbed=0;for(std::size_t n=0;n<rod.size();n+=4)if(rod[n]>rod[n+1]*2 && rod[n]>rod[n+2]*2){++red;
  disturbed+=std::max({abs(int(rod[n])-behind[n]),abs(int(rod[n+1])-behind[n+1]),abs(int(rod[n+2])-behind[n+2])})>2;}
 Check(red>100 && disturbed==0,"Foreground silhouettes must not be dragged by a rear shockwave");
 const auto hud=[&]{r.DrawHudRect(.35f,.30f,.30f,.40f,{.1f,.75f,.2f,1});};
 Stage(r);hud();const auto uiBase=Capture(r,out/"hud-base.png");Stage(r);Wave(r);hud();const auto ui=Capture(r,out/"hud-pressure.png");
 for(int y=145;y<335;++y)for(int x=225;x<415;++x)for(int c=0;c<3;++c)
  Check(ui[(y*640+x)*4+c]==uiBase[(y*640+x)*4+c],"World refraction changed HUD pixels");
 Stage(r);r.SetPressureEnabled(false);Wave(r);Check(base==Capture(r,out/"disabled.png") && r.PressurePasses()==0,"Disabled pressure must be a no-op");r.SetPressureEnabled(true);
 Stage(r);for(int i=0;i<100;++i)Wave(r);Check(r.PressureWaveCount()==8,"Pressure submission exceeded its eight-volume bound");
 Capture(r,out/"saturated.png");Check(r.PressurePasses()==1,"Crowd must share one pressure pass");
 Stage(r);r.DrawPressureWave({0,1,0},{0,1,1},0,.1f,1);r.DrawPressureWave({0,1,0},{1,1,1},0,std::numeric_limits<float>::quiet_NaN(),1);
 r.DrawPressureWave({100,1,0},{1,1,1},0,.1f,1);r.DrawPressureWave({0,1,-20},{1,1,1},0,.1f,1);
 r.DrawPressureWave({0,1,0},{1,1,1},0,2,1);Check(r.PressureWaveCount()==0,"Invalid or expired pressure was accepted");Capture(r,out/"invalid.png");
 Stage(r);Check(r.BeginPortrait(.2f,.2f,.4f,.5f,0,true),"Portrait begin");Wave(r);Check(r.PressureWaveCount()==0,"Pressure leaked into a portrait");Check(r.EndPortrait(),"Portrait end");
 Wave(r);Check(r.PressureWaveCount()==0,"Late refraction could warp an already-composed interface");Capture(r,out/"portrait.png");
 using C=rpg::ClassId;using K=rpg::CombatVisualKind;
 for(const auto id:{C::Warrior,C::Mage,C::Scout,C::Priest}){
  rpg::CombatVisualEvent event{K::HeavyHit,{0,0},1.2f,0,100};event.school=id;event.ability=1;event.heavy=true;
  const auto pressure=impactfx::PressureAt(event,.15f);Check(pressure.strength>0 && pressure.groundRadius>0,"Heavy confirmed hit lost its shared pressure front");
  auto rejected=event;rejected.primary=false;Check(impactfx::PressureAt(rejected,.15f).strength==0,"Secondary hit duplicated the main pressure spectacle");
  rejected=event;rejected.kind=K::SpellCast;Check(impactfx::PressureAt(rejected,.15f).strength==0,"Release fabricated an impact");
  Check(impactfx::PressureAt(event,1).strength==0,"Expired pressure profile survived");
  Pixels reference;
  for(float rate:{22.f,60.f,144.f,1000.f}){CombatEffects effects;effects.Emit(event);float elapsed=0;
   while(elapsed<.15f-1e-7f){const float dt=std::min(1/rate,.15f-elapsed);effects.Advance(dt);elapsed+=dt;}
   Stage(r);effects.Render(r);const auto pixels=Capture(r,out/(std::to_string(int(id))+"-"+std::to_string(int(rate))+".png"));
   if(reference.empty())reference=pixels;else Check(Changed(reference,pixels)<40,"Combat pressure changed with frame rate");
   Check(r.PressurePasses()==1 && r.EffectVertices()<=Renderer::maxEffectVertices,"Integrated pressure missed its pass or exceeded geometry limits");
  }
 }
 Check(r.Resize(800,600) && r.PressureBytes()==0,"Resize retained an incompatible scene copy");Stage(r);Wave(r);Capture(r,out/"resized.png");
 Check(r.PressureBytes()==800u*600*4 && r.PressurePasses()==1,"Resized pressure allocation/pass incorrect");
 Stage(r);Capture(r,out/"idle-after.png");Check(r.PressurePasses()==0,"An idle frame replayed a previous wave");
 Check(r.ValidateDebugMessages(),"DirectX pressure resource/binding hazard");
 std::cout<<"PASS: visible depth-clipped refraction, opaque silhouettes, HUD/portrait isolation, deterministic clocks, four-class ground fronts, resize and fixed limits\n";
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
