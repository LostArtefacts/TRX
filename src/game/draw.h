#ifndef T1M_GAME_DRAW_H
#define T1M_GAME_DRAW_H

#include "game/types.h"
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
void DrawAnimatingItem(ITEM_INFO *item);
void DrawUnclippedItem(ITEM_INFO *item);
void DrawLara(ITEM_INFO *item);
void DrawGunFlash(int32_t weapon_type, int32_t clip);
void CalculateObjectLighting(ITEM_INFO *item, int16_t *frame);
void DrawLaraInt(
    ITEM_INFO *item, int16_t *frame1, int16_t *frame2, int32_t frac,
    int32_t rate);
void InitInterpolate(int32_t frac, int32_t rate);

void phd_PushMatrix_I();
void phd_PopMatrix_I();
void phd_TranslateRel_I(int32_t x, int32_t y, int32_t z);
void phd_TranslateRel_ID(
    int32_t x, int32_t y, int32_t z, int32_t x2, int32_t y2, int32_t z2);
void phd_RotY_I(int16_t ang);
void phd_RotX_I(int16_t ang);
void phd_RotZ_I(int16_t ang);
void phd_RotYXZ_I(int16_t y, int16_t x, int16_t z);
void phd_RotYXZpack_I(int32_t r1, int32_t r2);
void phd_PutPolygons_I(int16_t *ptr, int32_t clip);

void InterpolateMatrix();
void InterpolateArmMatrix();
int32_t GetFrames(ITEM_INFO *item, int16_t *frmptr[], int32_t *rate);
int16_t *GetBoundsAccurate(ITEM_INFO *item);
int16_t *GetBestFrame(ITEM_INFO *item);

void T1MInjectGameDraw();

#endif
