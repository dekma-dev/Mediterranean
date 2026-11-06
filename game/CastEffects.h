#pragma once
#include "game/Simulation.h"
#include "game/CharacterPose.h"
#include <DirectXMath.h>
class Renderer;
namespace castfx {
// World-space sockets from the same solved pose used to draw the character.
// Borrowed for a draw only: no animation, gameplay or emitter state is owned.
struct Attachments {
 DirectX::XMFLOAT3 root{},focus{},left{},right{};
 float facing=0;
 bool alive=true;
 bool terrain=true; // Character-stage previews use their own flat floor.
};
Attachments FromPose(const CharacterPose&,DirectX::XMFLOAT3 root,float facing,rpg::WeaponForm,bool shield,bool weapon=true,bool alive=true);
Attachments Preview(DirectX::XMFLOAT3 root,DirectX::XMFLOAT3 focus,float facing);
void Prepare(Renderer&,const rpg::Simulation&,const Attachments&);
// Shared preparation artwork for gameplay and the isolated character audition.
void PrepareAction(Renderer&,rpg::ClassId,int ability,bool heavy,float seconds,float rate,
                   bool recovery,float recoveryProgress,const Attachments&);
void Complete(Renderer&,const rpg::CombatVisualEvent&,float age,const Attachments&);
void Discipline(Renderer&,const rpg::Simulation&,const Attachments&,float clock);
void DisciplinePulse(Renderer&,const rpg::CombatVisualEvent&,float age,const Attachments*);
}
