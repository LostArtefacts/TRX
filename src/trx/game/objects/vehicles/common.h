#pragma once

#include <trx/game/items/types.h>
#include <trx/game/music.h>
#include <trx/game/objects/types.h>

#define VEHICLE_TRACK_POOL_SIZE 4

int32_t Vehicle_DoShift(ITEM *vehicle, const XYZ_32 *pos, const XYZ_32 *old);
int32_t Vehicle_GetCollisionAnim(const ITEM *vehicle, XYZ_32 *moved);
void Vehicle_PlayTrackPool(
    const ITEM *item, const char *key_prefix, MUSIC_PLAY_MODE mode);
void Vehicle_PlayOneShotTrackPool(const ITEM *item, const char *key_prefix);
void Vehicle_TestTriggers(const ITEM *lara_item, const ITEM *vehicle_item);
void Vehicle_HandleEvent(ITEM *item, OBJECT_EVENT event, const void *data);
