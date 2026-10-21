#pragma once
#include "engine/Skinning.h"
#include "game/CharacterPose.h"
#include "game/CharacterView.h"

visual::BodyPalette BuildCharacterRig(const CharacterPose& pose,const CharacterOutfit& outfit,DirectX::XMFLOAT4 cloth);
