#ifndef TOMB1MAIN_GAME_DRAW_H
#define TOMB1MAIN_GAME_DRAW_H

#include "game/types.h"
#include <stdint.h>

// clang-format off
#define DrawEffect              ((void          __cdecl(*)(int16_t fx_num))0x00417400)
#define DrawAnimatingItem       ((void          __cdecl(*)(ITEM_INFO *item))0x00417550)
#define GetBoundsAccurate       ((int16_t*      __cdecl(*)(ITEM_INFO* item))0x00419DD0)
#define GetFrames               ((int32_t       __cdecl(*)(ITEM_INFO* item, int16_t* frmptr[], int32_t* rate))0x00419D30)
#define DrawLaraInt             ((void          __cdecl(*)(ITEM_INFO* item, int16_t* frame1, int16_t* frame2, int32_t frac, int32_t rate))0x00418680)
#define CalculateObjectLighting ((void          __cdecl(*)(ITEM_INFO* item, int16_t* frame))0x004185B0)
// clang-format on

void __cdecl PrintRooms(int16_t room_number);
void __cdecl DrawLara(ITEM_INFO* item);
void __cdecl DrawGunFlash(int32_t weapon_type, int32_t clip);

void Tomb1MInjectGameDraw();

#endif
