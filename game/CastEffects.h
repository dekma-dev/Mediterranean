#pragma once
#include "game/Simulation.h"
#include "game/CharacterPose.h"
#include <DirectXMath.h>
class Renderer;
namespace presentation {struct Actor;}
namespace castfx {
// World-space sockets from the same solved pose used to draw the character.
// Borrowed for a draw only: no animation, gameplay or emitter state is owned.
struct Attachments {
 DirectX::XMFLOAT3 root{},focus{},left{},right{};
 float facing=0;
 bool alive=true;
 bool terrain=true; // Character-stage previews use their own flat floor.
 DirectX::XMFLOAT3 edgeBase{},edgeTip{}; // Exact equipped blade sockets.
 bool blade=false;
};
Attachments FromPose(const CharacterPose&,DirectX::XMFLOAT3 root,float facing,rpg::WeaponForm,bool shield,bool weapon=true,bool alive=true);
Attachments Preview(DirectX::XMFLOAT3 root,DirectX::XMFLOAT3 focus,float facing);
Attachments FromEnemy(const rpg::Enemy&,const presentation::Actor&);
void PrepareEnemy(Renderer&,const rpg::Enemy&,const Attachments&);
void Prepare(Renderer&,const rpg::Simulation&,const Attachments&);
// Shared preparation artwork for gameplay and the isolated character audition.
void PrepareAction(Renderer&,rpg::ClassId,int ability,bool heavy,float seconds,float rate,
                   bool recovery,float recoveryProgress,const Attachments&);
void Complete(Renderer&,const rpg::CombatVisualEvent&,float age,const Attachments&);
void Discipline(Renderer&,const rpg::Simulation&,const Attachments&,float clock);
void DisciplinePulse(Renderer&,const rpg::CombatVisualEvent&,float age,const Attachments*);
// Class-specific preparation/recovery shapes, sharing the accepted cast clock.
bool ClassCharge(Renderer&,rpg::ClassId,int ability,float progress,float age,const Attachments&);
void PrayerCompletion(Renderer&,float age,const Attachments&);
bool Discharge(Renderer&,const rpg::CombatVisualEvent&,DirectX::XMFLOAT3 origin,float age);
}
