#pragma once
#include <DirectXMath.h>
#include <array>
#include <vector>
#include <string_view>
namespace playerclips {
enum class Clip:unsigned { Slash,Overhead,Jump,Land,WalkForward,WalkBackward,WalkLeft,WalkRight,
 RunForward,RunBackward,RunLeft,RunRight,Start,Stop,Count };
struct Frame {
 std::array<DirectX::XMFLOAT3,2> hands{},feet{};
 float hip=0;
 DirectX::XMFLOAT3 weapon{},torso{};
};
struct Track { float duration=0;std::vector<Frame> frames; };
using Library=std::array<Track,unsigned(Clip::Count)>;
Library Decode(std::string_view bytes);
std::string_view EmbeddedBytes();
const Library& Collection();
Frame Sample(Clip clip,float phase);
bool Looping(Clip clip);
// Direction weights are forward/backward/left/right; sum <= 1 retains idle.
Frame Locomotion(const std::array<float,4>& weights,float run,float phase);
float SwingHeight(const std::array<float,4>& weights,float run,int side,float phase);
float AttackPhase(float seconds,float start,float contact,float end);
}
