#pragma once
// Float4-row addresses used by the shared GPU palette. Skinning.h verifies
// each value against the actual CPU type; SceneShader.h embeds the same values.
#define RPG_SKIN_ROWS 294
#define RPG_SKIN_JOINT_ROWS 4
#define RPG_SKIN_NORMAL_ROW 128
#define RPG_SKIN_COLOR_ROW 256
#define RPG_SKIN_VOLUME_ROW 263
#define RPG_SKIN_VOLUME_ROWS 2
#define RPG_SKIN_DEFORMATION_ROW 293
