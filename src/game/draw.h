#ifndef TOMB1MAIN_GAME_DRAW_H
#define TOMB1MAIN_GAME_DRAW_H

#include "game/types.h"
#include <stdint.h>

void __cdecl DrawRooms(int16_t current_room);
void __cdecl GetRoomBounds(int16_t room_num);
int32_t __cdecl SetRoomBounds(
    int16_t* objptr, int16_t room_num, ROOM_INFO* parent);
void __cdecl PrintRooms(int16_t room_number);
void __cdecl DrawEffect(int16_t fxnum);
void __cdecl DrawSpriteItem(ITEM_INFO* item);
void __cdecl DrawDummyItem(ITEM_INFO* item);
void __cdecl DrawAnimatingItem(ITEM_INFO* item);
void __cdecl DrawLara(ITEM_INFO* item);
void __cdecl DrawGunFlash(int32_t weapon_type, int32_t clip);
void __cdecl CalculateObjectLighting(ITEM_INFO* item, int16_t* frame);
void __cdecl DrawLaraInt(
    ITEM_INFO* item, int16_t* frame1, int16_t* frame2, int frac, int rate);
void __cdecl InitInterpolate(int32_t frac, int32_t rate);

void __cdecl phd_PushMatrix_I();
void __cdecl phd_PopMatrix_I();
void __cdecl phd_TranslateRel_I(int32_t x, int32_t y, int32_t z);
void __cdecl phd_TranslateRel_ID(
    int32_t x, int32_t y, int32_t z, int32_t x2, int32_t y2, int32_t z2);
void __cdecl phd_RotY_I(int16_t ang);
void __cdecl phd_RotX_I(int16_t ang);
void __cdecl phd_RotZ_I(int16_t ang);
void __cdecl phd_RotYXZ_I(int16_t y, int16_t x, int16_t z);
void __cdecl phd_RotYXZpack_I(int32_t r1, int32_t r2);
void __cdecl phd_PutPolygons_I(int16_t* ptr, int clip);

void __cdecl InterpolateMatrix();
void __cdecl InterpolateArmMatrix();
int32_t __cdecl GetFrames(ITEM_INFO* item, int16_t* frmptr[], int32_t* rate);
int16_t* __cdecl GetBoundsAccurate(ITEM_INFO* item);
int16_t* __cdecl GetBestFrame(ITEM_INFO* item);

void Tomb1MInjectGameDraw();

#endif
