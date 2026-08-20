#pragma once

#include <trx/game/objects/types.h>
#include <trx/game/rooms.h>

// Reports whether a movable block stands on the square at pos, or has begun
// to move onto it, with the top of the block at or above the height pos
// gives. A block claims the square it enters from the moment its animation
// shifts it, which the floor data reports only once the move ends.
bool MovableBlock_TestSquareClaimed(XYZ_32 pos);

// Block or unblock a block's box overlap index.
void MovableBlock_UpdateBox(const ITEM *item, bool blocked);

// Lock a stack that is about to drop.
void MovableBlock_LockStack(XYZ_32 drop_pos, int16_t room_num);
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
