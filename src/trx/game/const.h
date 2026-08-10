#pragma once

#include <trx/core/math/const.h>

#define LOGIC_FPS 30

#define STEP_L 256
#define WALL_L 1024 // = 1 << WALL_SHIFT
#define WALL_SHIFT 10

#define GRAVITY 6
#define FAST_FALL_SPEED 128

// The floordata trigger code bits occupy bits 9..13 of the encoded word. The
// 0..31 mask shifts here to pack into the IF_/FSF_/MTF_CODE_BITS save fields.
#define TRIGGER_MASK_SHIFT 9

// All five code bits set: a fully triggered item, flip slot or music track.
#define TRIGGER_MASK_ALL 0x1F

#define FOV_VALUE_PASSPORT 80
#define FOV_MODE_GAME FOV_MODE_PC
#define FOV_MODE_PASSPORT FOV_MODE_PS1_FIT
#define FOV_MODE_CUTSCENE (g_TRVersion == 1 ? FOV_MODE_VERTICAL : FOV_MODE_PC)
