#include "engine/Skeleton.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <functional>
using namespace DirectX;
using namespace animation;
void Require(bool ok,const char* message) { if(!ok) {std::cerr<<message<<'\n';std::exit(1);} }
void Reject(const std::function<void()>& action) {
  try {action();} catch(const std::runtime_error&) {return;}
  Require(false,"Invalid asset was accepted");
}
bool Near(float a,float b) {return std::abs(a-b)<.0001f;}
void Identity(const XMFLOAT4X4& m) {
  for(int row=0;row<4;++row) for(int col=0;col<4;++col)
    Require(Near(m.m[row][col],row==col?1.f:0.f),"Bind pose must preserve mesh-space vertices");
}
int main() {
  // Original two-joint hinge fixture, deliberately child-first in storage.
  std::vector<Joint> joints={{"hinge",1,{{0,1,0}}},{"root",-1,{}}};
  Skeleton skeleton(joints);for(const auto& m:skeleton.RestPose().skin) Identity(m);
  Clip clip;clip.duration=1;clip.tracks.resize(2);
  auto start=joints[0].local,end=start;
  XMStoreFloat4(&end.rotation,XMQuaternionRotationAxis(XMVectorSet(0,1,0,0),XM_PIDIV2));
  clip.tracks[0]={{0,start},{1,end}};
  const auto check=[&](double t,bool loop,float x,float z) {
    const auto pose=skeleton.Sample(clip,t,loop);XMFLOAT3 p;
    XMStoreFloat3(&p,XMVector3TransformCoord(XMVectorSet(1,1,0,1),XMLoadFloat4x4(&pose.skin[0])));
    Require(Near(p.x,x) && Near(p.y,1) && Near(p.z,z),"Hinge must rotate around its bind-space pivot");
  };
  check(0,false,1,0);check(1,false,0,-1);check(2,false,0,-1);check(-1,false,1,0);
  check(.5,false,std::sqrt(.5f),-std::sqrt(.5f));
  // Export rounding within the accepted normalization tolerance must not bias
  // interpolation toward whichever endpoint has a slightly longer quaternion.
  auto rounded=clip;
  auto& qa=rounded.tracks[0][0].value.rotation;qa.w*=1.0004f;
  auto& qb=rounded.tracks[0][1].value.rotation;qb.y*=.9996f;qb.w*=.9996f;
  const auto cleanPose=skeleton.Sample(clip,.25,false);
  const auto roundedPose=skeleton.Sample(rounded,.25,false);
  for(int r=0;r<4;++r) for(int c=0;c<4;++c)
    Require(std::abs(cleanPose.skin[0].m[r][c]-roundedPose.skin[0].m[r][c])<1e-5f,"Accepted quaternion rounding must not distort clip interpolation");
  check(1,true,1,0);check(-.5,true,std::sqrt(.5f),-std::sqrt(.5f));
  // Different update schedules sample the identical authored time.
  for(int hz:{30,60,144,1000}) check(double(hz)/hz,false,0,-1);
  auto moved=std::vector<Transform>{start,{}};moved[1].translation={3,2,1};
  auto pose=skeleton.Evaluate(moved);
  Require(Near(pose.global[0]._41,3) && Near(pose.global[0]._42,3),"Parent motion must reach children independent of storage order");
  moved[1].scale={2,3,4};pose=skeleton.Evaluate(moved);
  Require(Near(pose.normal[1]._11,.5f) && Near(pose.normal[1]._22,1.f/3) && Near(pose.normal[1]._33,.25f),"Normals need inverse transpose for nonuniform scale");
  auto antipodal=clip;antipodal.tracks[0][1].value.rotation={0,0,0,-1};
  Identity(skeleton.Sample(antipodal,.5,false).skin[0]);
  auto single=clip;single.tracks[0]={{.5f,end}};
  Require(Near(skeleton.Sample(single,0,false).skin[0]._11,0),"Single-key channels must hold their value");
  Reject([] {Skeleton empty({});});
  auto bad=joints;bad[0].parent=4;Reject([&]{Skeleton x(bad);});
  bad=joints;bad[0].name="root";Reject([&]{Skeleton x(bad);});
  bad=joints;bad[0].local.rotation={0,0,0,0};Reject([&]{Skeleton x(bad);});
  bad=joints;bad[0].local.scale.x=0;Reject([&]{Skeleton x(bad);});
  bad=joints;bad.push_back({"loop",0,{}});bad[0].parent=2;Reject([&]{Skeleton x(bad);});
  bad=joints;bad[0].local.translation.x=std::numeric_limits<float>::infinity();Reject([&]{Skeleton x(bad);});
  Reject([&]{skeleton.Evaluate({});});
  std::vector<XMFLOAT4X4> supplied(2);
  for(auto& m:supplied) XMStoreFloat4x4(&m,XMMatrixIdentity());
  Require(Near(Skeleton(joints,supplied).RestPose().skin[0]._42,1),"Explicit identity binds must not be silently recomputed");
  supplied[0]._44=0;Reject([&]{Skeleton x(joints,supplied);});
  supplied[0]._44=1;supplied[0]._11=0;Reject([&]{Skeleton x(joints,supplied);});
  supplied.resize(1);Reject([&]{Skeleton x(joints,supplied);});
  auto invalid=clip;invalid.tracks[0][1].time=0;Reject([&]{skeleton.ValidateClip(invalid);});
  invalid=clip;invalid.tracks[0][1].time=2;Reject([&]{skeleton.ValidateClip(invalid);});
  invalid=clip;invalid.duration=0;Reject([&]{skeleton.ValidateClip(invalid);});
  invalid=clip;invalid.tracks.clear();Reject([&]{skeleton.ValidateClip(invalid);});
  invalid=clip;invalid.tracks[0].resize(65537);Reject([&]{skeleton.ValidateClip(invalid);});
  Reject([&]{skeleton.Sample(clip,std::numeric_limits<double>::quiet_NaN(),true);});
  std::cout<<"Skeleton hierarchy, bind-space deformation and clip validation passed\n";
}
