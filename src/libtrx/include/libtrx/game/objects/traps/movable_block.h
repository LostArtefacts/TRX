#pragma once

#include "../../rooms.h"

void MovableBlock_Initialise(int16_t item_num);
void MovableBlock_UpdateRotation(ITEM *item, int16_t rot_y);
void MovableBlock_UpdateBox(const ITEM *item, bool blocked);
void MovableBlock_SetPushPull(ITEM *item, bool enable);
bool MovableBlock_IsPushPull(const ITEM *item);
void MovableBlock_SetGravityFrames(ITEM *item, uint8_t frames);
uint16_t MovableBlock_GetGravityFrames(const ITEM *item);
void MovableBlock_SetLinked(ITEM *item);
GAME_VECTOR MovableBlock_GetLinked(const ITEM *item);
void MovableBlock_DropStack(
    int32_t stack_height, XYZ_32 sector_pos, int16_t room_num);
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
