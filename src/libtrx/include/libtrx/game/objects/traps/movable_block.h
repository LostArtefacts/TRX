#pragma once

#include "../../rooms.h"
#include "../types.h"

// Block or unblock a block's box overlap index.
void MovableBlock_UpdateBox(const ITEM *item, bool blocked);

// Drop a stack of blocks.
void MovableBlock_DropStack(XYZ_32 drop_pos, int16_t room_num);

// Shift a stack of blocks up or down in the y direction.
void MovableBlock_ShiftStackY(
    int32_t stack_height, XYZ_32 old_pos, int32_t new_y, int16_t room_num,
    bool reposition);

// Shift a stack of blocks in the x or z direction..
void MovableBlock_SlideStack(
    int32_t stack_height, XYZ_32 old_sector, const ITEM *dest_item,
    bool reposition);

typedef struct {
    int16_t counter_rot[3];
    int16_t original_rot;
    uint16_t gravity_frames;
    bool is_push_pull;
    bool is_forced_moving;
    GAME_VECTOR initial;
    GAME_VECTOR linked;
} MOVABLE_BLOCK_INFO;
