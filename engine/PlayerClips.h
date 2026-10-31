#pragma once
#include <DirectXMath.h>
#include <array>
#include <vector>
#include <string_view>
namespace playerclips {
enum class Clip:unsigned { Slash,Overhead,Jump,Land,WalkForward,WalkBackward,WalkLeft,WalkRight,
 RunForward,RunBackward,RunLeft,RunRight,Start,Stop,StaffSweep,StaffThrust,
 SwordWalk,SwordRun,StaffWalk,StaffRun,SwordGuard,StaffGuard,Idle,Fall,
 LegacySlash,LegacyOverhead,LegacyJump,LegacyLand,BowGuard,BowRelease,BowPowerRelease,BowWalk,BowRun,Count };
enum class Equipment { None,Sword,Staff,Bow };
struct Frame {
 std::array<DirectX::XMFLOAT3,2> hands{},feet{};
 float hip=0;
 DirectX::XMFLOAT3 weapon{},torso{};
};
struct Track {
 float duration=0,load=0,contact=0,takeoff=0,landing=0,stance=0,speed=0;
 bool loop=false;
 std::vector<Frame> frames;
  std::vector<float> arc;
};
using Library=std::array<Track,unsigned(Clip::Count)>;
Library Decode(std::string_view bytes);
std::string_view EmbeddedBytes();
const Library& Collection();
Frame Sample(Clip clip,float phase);
bool Looping(Clip clip);
// Direction weights are forward/backward/left/right; sum <= 1 retains idle.
Frame Blend(const Frame& a,const Frame& b,float weight);
Frame Locomotion(const std::array<float,4>& weights,float run,float phase,Equipment equipment=Equipment::None);
float SwingHeight(const std::array<float,4>& weights,float run,int side,float phase);
float AttackPhase(Clip clip,float seconds,float start,float contact,float end);
float FlightPhase(float progress);
}
