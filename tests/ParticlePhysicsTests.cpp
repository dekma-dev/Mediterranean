#include "engine/ParticlePhysics.h"
#include <cstdlib>
#include <iostream>
#include <limits>
using namespace particles;
void Require(bool value,const char* message){if(!value){std::cerr<<message<<'\n';std::exit(1);}}
struct TestWorld final:CollisionWorld {
 float slope=0,hardness=.7f;bool wall=false;
 bool Trace(Vec3 from,Vec3 to,float radius,Contact& hit)const override{
  bool found=false;const float a=from.y-slope*from.x-radius,b=to.y-slope*to.x-radius;
  if(b<=0 || a<0){const float t=a<=0?0:std::clamp(a/(a-b),0.f,1.f);Vec3 p=from+(to-from)*t;p.y=slope*p.x+radius;
   hit={t,p,Normal({-slope,1,0}),hardness,.35f,true};found=true;}
  if(wall && SweepBox(from,to,radius,{{.4f,-1,-2},{.42f,4,2}},hit))found=true;
  return found;
 }
};
Particle Seed(){Particle p;p.position={0,1.1f,0};p.velocity={2.6f,1.7f,.6f};p.lifetime=2.5f;return p;}
void Advance(Pool& pool,const CollisionWorld& world,float duration,float rate){
 double time=0;while(time<duration-1e-9){const float dt=float(std::min(1.0/rate,double(duration)-time));pool.Advance(dt,world);time+=dt;}
}
int main(){
 TestWorld world;
 // Free fall agrees with the analytic trajectory, before the first contact.
 Pool free;auto p=Seed();p.drag=0;p.velocity={0,0,0};free.Spawn(p,0,world);Advance(free,world,.1f,60);
 Require(std::abs(free.Items()[0].position.y-(1.1f-9.81f*.1f*.1f*.5f))<.0001f,"Fixed integration must preserve ballistic displacement");
 // Real collision outcomes and interpolated positions are independent of frame rate.
 Pool reference;reference.Spawn(Seed(),.013f,world);Advance(reference,world,1.35f,120);
 for(float rate:{22.f,30.f,60.f,144.f,1000.f}){
  Pool test;test.Spawn(Seed(),.013f,world);Advance(test,world,1.35f,rate);
  const auto& a=reference.Items()[0];const auto& b=test.Items()[0];
  Require(a.ticks==b.ticks && a.contacts==b.contacts,"Refresh rate changed collision count");
  Require(Dot(a.Position()-b.Position(),a.Position()-b.Position())<1e-8f,"Refresh rate changed interpolated particle trajectory");
 }
 Require(reference.CollisionCount()>0,"A falling fragment must hit the floor");
 TestWorld hard,earth;hard.hardness=1;earth.hardness=.1f;Pool stone,soil;
 p=Seed();p.position={0,.12f,0};p.velocity={0,-3,0};p.drag=0;p.bounce=1;
 stone.Spawn(p,0,hard);soil.Spawn(p,0,earth);Advance(stone,hard,.12f,60);Advance(soil,earth,.12f,60);
 Require(stone.Items()[0].position.y>soil.Items()[0].position.y+.1f,"Hard paving and soil must dissipate different amounts of impact energy");
 Pool slope;world.slope=.3f;slope.Spawn(Seed(),0,world);
 for(int n=0;n<240;++n){slope.Advance(1.f/120,world);const auto& a=slope.Items()[0];
  Require(a.position.y>=world.slope*a.position.x+a.radius-.0001f,"A fragment penetrated sloped ground");}
 Require(slope.Items()[0].resting,"Dissipated fragments must settle");
 world.slope=0;world.wall=true;Pool wall;p=Seed();p.position={0,1,0};p.velocity={20,0,0};wall.Spawn(p,0,world);
 Advance(wall,world,.05f,22);Require(wall.CollisionCount()>0 && wall.Items()[0].position.x<.4f,"Fast debris tunneled through a thin wall");
 Contact embedded;Require(SweepBox({.41f,1,0},{.6f,1,0},.02f,{{.4f,0,-1},{.42f,2,1}},embedded),"Embedded fragments need depenetration");
 Require(embedded.point.x<.38f || embedded.point.x>.44f,"Embedded fragment was not ejected to a safe face");
 Contact miss;Require(!SweepBox({0,5,0},{1,5,0},.02f,{{.4f,0,-1},{.42f,2,1}},miss),"Wall bounds must not block particles above the prop");
 // A birth late in a frame matches an earlier birth advanced by the same tail.
 Pool late,early;late.Spawn(Seed(),.025f,world);early.Spawn(Seed(),0,world);early.Advance(.025f,world);
 Require(late.Items()[0].position.x==early.Items()[0].position.x && late.Items()[0].ticks==early.Items()[0].ticks,"Birth catch-up must use the accepted event's frame tail");
 Pool bounds;for(unsigned i=0;i<Pool::capacity;++i)Require(bounds.Spawn(Seed(),0,world),"Pool rejected valid capacity");
 Require(!bounds.Spawn(Seed(),0,world) && bounds.Count()==Pool::capacity,"Overflow must drop cosmetics without allocation or replacement");
 const auto before=bounds.Items()[0].position;
 for(float dt:{0.f,-1.f,std::numeric_limits<float>::quiet_NaN(),std::numeric_limits<float>::infinity()})bounds.Advance(dt,world);
 Require(bounds.Items()[0].ticks==0 && before.y==bounds.Items()[0].position.y,"Invalid clocks advanced particles");
 bounds.Advance(10,world);Require(bounds.Items()[0].ticks==6,"A stalled frame must not run unbounded physics");
 bounds.Reset();Require(bounds.Count()==0 && bounds.CollisionCount()==0,"Reset leaked old particles");
 p=Seed();p.velocity.x=std::numeric_limits<float>::quiet_NaN();Require(!bounds.Spawn(p,0,world),"NaN velocity accepted");
 world.wall=false;world.hardness=0;Pool soft;soft.Spawn(Seed(),0,world);Advance(soft,world,2.5f,60);
 Require(soft.Count()==0,"Expired particles must release their slots");
 std::cout<<"PASS: ballistic accuracy, 22-1000 Hz collision parity, slope settling, swept walls, event tails, saturation, invalid clocks and expiry. Pool bytes="<<sizeof(Pool)<<'\n';
}
