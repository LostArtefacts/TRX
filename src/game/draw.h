#pragma once

#include "global/types.h"

#include <stdbool.h>
#include <stdint.h>

extern bool g_CameraUnderwater;

void DrawEffect(int16_t fxnum);
void DrawSpriteItem(ITEM_INFO *item);
void DrawDummyItem(ITEM_INFO *item);
void DrawPickupItem(ITEM_INFO *item);
void DrawAnimatingItem(ITEM_INFO *item);
void DrawUnclippedItem(ITEM_INFO *item);
void DrawGunFlash(int32_t weapon_type, int32_t clip);
void CalculateObjectLighting(ITEM_INFO *item, int16_t *frame);

int32_t GetFrames(ITEM_INFO *item, int16_t *frmptr[], int32_t *rate);
int16_t *GetBoundsAccurate(ITEM_INFO *item);
int16_t *GetBestFrame(ITEM_INFO *item);
