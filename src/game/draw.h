#pragma once

#include "global/types.h"

void Draw_PrintRoomNumStack(void);
void DrawRooms(int16_t current_room);
void GetRoomBounds(int16_t room_num);
int32_t SetRoomBounds(int16_t *objptr, int16_t room_num, ROOM_INFO *parent);
void PrintRooms(int16_t room_number);
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

void Draw_DrawScene(bool draw_overlay);
int32_t Draw_ProcessFrame(void);
