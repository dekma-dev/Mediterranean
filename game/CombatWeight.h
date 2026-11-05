#pragma once
#include <algorithm>
#include <cmath>

namespace animation {
// Reverse an imported stroke around the same contact key. The two Hermite
// segments share a negative harmonic tangent there, with zero endpoint velocity.
inline float ReturnCutPhase(float phase,float contact){
 if(!std::isfinite(phase) || !std::isfinite(contact) || contact<=0 || contact>=1)return 0;
 phase=std::clamp(phase,0.f,1.f);
 const float before=(1-contact)/contact,after=contact/(1-contact),speed=2*before*after/(before+after);
 const auto hermite=[](float a,float b,float ma,float mb,float t){const float u=t*t,v=u*t;return (2*v-3*u+1)*a+(v-2*u+t)*ma+(-2*v+3*u)*b+(v-u)*mb;};
 return phase<=contact?hermite(1,contact,0,-speed*contact,phase/contact):
   hermite(contact,0,-speed*(1-contact),0,(phase-contact)/(1-contact));
}
struct CombatWeight {
 float pitch=0,yaw=0,roll=0,lift=0,side=0,forward=0,pelvisPitch=0,pelvisYaw=0,pelvisRoll=0;
};
inline CombatWeight operator*(CombatWeight a,float w){return {a.pitch*w,a.yaw*w,a.roll*w,a.lift*w,a.side*w,a.forward*w,a.pelvisPitch*w,a.pelvisYaw*w,a.pelvisRoll*w};}
inline CombatWeight operator+(CombatWeight a,CombatWeight b){return {a.pitch+b.pitch,a.yaw+b.yaw,a.roll+b.roll,a.lift+b.lift,a.side+b.side,a.forward+b.forward,a.pelvisPitch+b.pelvisPitch,a.pelvisYaw+b.pelvisYaw,a.pelvisRoll+b.pelvisRoll};}
// 0: Warrior, 1: Scout, 2: Mage, 3: Priest. Stored in the controller so an
// interrupted or outgoing attack retains its own choreography through blending.
inline CombatWeight AbilityAccent(int style,int ability,float phase,float contact){
 if(style<0 || style>3 || ability<0 || !std::isfinite(phase) || !std::isfinite(contact) || phase<=0 || phase>=1 || contact<=0 || contact>=1)return {};
 const auto smooth=[](float x){x=std::clamp(x,0.f,1.f);return x*x*(3-2*x);};
 const auto pulse=[&](float t,float begin,float peak,float end){return t<peak?smooth((t-begin)/(peak-begin)):1-smooth((t-peak)/(end-peak));};
 const float load=pulse(phase,0,contact*.70f,contact),drive=pulse(phase,contact*.65f,contact+(1-contact)*.18f,1);
 CombatWeight r;
 if(style==0){
  const float turn=ability==12?-.11f:ability==8 || ability==11?.16f:ability==14?.12f:.035f;
  const bool crush=ability==6 || ability==9;
  r.yaw=turn*(drive-load*.65f);r.pelvisYaw=turn*(drive*.65f-load*.45f);
  r.pitch=(crush?.11f:.025f)*drive-(crush?.065f:.018f)*load;
  r.lift=-(crush?.035f:.012f)*load;r.forward=(crush?.042f:.012f)*drive;
  r.side=turn*.15f*(drive-load);
 }else if(style==1){
  const bool aim=ability==1 || ability==6 || ability==9 || ability==11;
  const bool spread=ability==8 || ability==14;
  r.yaw=(spread?.095f:.025f)*(load-drive);r.pelvisYaw=r.yaw*.45f;
  r.pitch=(aim?.035f:.012f)*load-(aim?.06f:.022f)*drive;r.forward=(aim?.018f:.006f)*(load-drive);
 }else if(style==2){
  const bool grand=ability==8 || ability==9 || ability==11 || ability==14;
  r.pitch=(grand?-.07f:-.025f)*load+(grand?.085f:.035f)*drive;
  r.yaw=(ability==10 || ability==13?.065f:.025f)*(drive-load);r.pelvisYaw=r.yaw*.4f;
  r.lift=-(grand?.025f:.008f)*load;r.forward=(grand?.035f:.012f)*drive;
 }else{
  const bool prayer=ability==5 || ability==6;
  r.pitch=-(prayer?.085f:.025f)*load+(prayer?.03f:.015f)*drive;
  r.lift=(prayer?.025f:.008f)*load;r.yaw=.02f*(drive-load);r.forward=.012f*drive;
 }
 return r;
}
// Contact-locked body mechanics layered over imported clips. C1 envelopes
// return to rest without moving gameplay position or any facial channel.
inline CombatWeight AttackWeight(bool bow,bool heavy,float phase,float contact) {
 if(!std::isfinite(phase) || !std::isfinite(contact) || phase<=0 || phase>=1 || contact<=0 || contact>=1)return {};
 const auto smooth=[](float x){x=std::clamp(x,0.f,1.f);return x*x*(3-2*x);};
 const auto pulse=[&](float t,float start,float peak,float end){return t<peak?smooth((t-start)/(peak-start)):1-smooth((t-peak)/(end-peak));};
 const float load=pulse(phase,0,contact*.63f,contact);
 const float drive=pulse(phase,contact*.55f,contact+(1-contact)*.14f,1);
 const float brace=pulse(phase,contact+(1-contact)*.20f,contact+(1-contact)*.51f,1);
 const float mass=heavy?1.f:.68f;
 if(bow)return {mass*(.035f*load-.065f*drive+.022f*brace),mass*(.06f*load-.075f*drive),-.015f*mass*drive,
   -.014f*load*mass,mass*(.012f*load-.009f*drive),mass*(.018f*load-.028f*drive+.008f*brace),
   mass*(.016f*load-.022f*drive),mass*(.026f*load-.032f*drive),0};
 return {mass*(-.055f*load+.11f*drive-.027f*brace),mass*(-.08f*load+.10f*drive),mass*.032f*drive,
   -mass*(.038f*load+.014f*brace),mass*(-.024f*load+.038f*drive),mass*(-.024f*load+.067f*drive-.016f*brace),
   mass*(-.028f*load+.035f*drive),mass*(-.095f*load+.12f*drive),mass*(-.012f*load+.024f*drive)};
}
}
