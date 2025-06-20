#pragma once

#include "../../rooms.h"

void MovableBlock_Initialise(int16_t item_num);
void MovableBlock_UpdateRotation(ITEM *item, int16_t rot_y);
void MovableBlock_UpdateBox(const ITEM *item, bool blocked);
void MovableBlock_SetupFloor(void);
void MovableBlock_HandleFlipMap(ROOM_FLIP_STATUS flip_status);
void MovableBlock_SetPushPull(ITEM *item, bool enable);
bool MovableBlock_IsPushPull(const ITEM *item);
void MovableBlock_SetGravityFrames(ITEM *item, uint8_t frames);
uint16_t MovableBlock_GetGravityFrames(const ITEM *item);
void MovableBlock_ActivateStack(const ITEM *base_item, XYZ_32 sector_pos);

typedef struct {
    int16_t counter_rot[3];
    int16_t original_rot;
    uint16_t gravity_frames;
    bool is_push_pull;
} MovableBlock_Info;
