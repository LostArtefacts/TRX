#ifndef T1M_GAME_DRAW_H
#define T1M_GAME_DRAW_H

#include "global/types.h"

#include <stdint.h>

int32_t DrawPhaseCinematic();
int32_t DrawPhaseGame();
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
void DrawLara(ITEM_INFO *item);
void DrawGunFlash(int32_t weapon_type, int32_t clip);
void CalculateObjectLighting(ITEM_INFO *item, int16_t *frame);
void DrawLaraInt(
    ITEM_INFO *item, int16_t *frame1, int16_t *frame2, int32_t frac,
    int32_t rate);
void InitInterpolate(int32_t frac, int32_t rate);

int32_t GetFrames(ITEM_INFO *item, int16_t *frmptr[], int32_t *rate);
int16_t *GetBoundsAccurate(ITEM_INFO *item);
int16_t *GetBestFrame(ITEM_INFO *item);

#endif
