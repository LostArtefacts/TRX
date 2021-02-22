#ifndef T1M_GAME_TEXT_H
#define T1M_GAME_TEXT_H

#include "game/types.h"
#include <stdint.h>

void __cdecl T_InitPrint();
TEXTSTRING* __cdecl T_Print(
    int16_t xpos, int16_t ypos, int16_t zpos, const char* string);
void __cdecl T_ChangeText(TEXTSTRING* textstring, const char* string);
void __cdecl T_SetScale(
    TEXTSTRING* textstring, int32_t scale_h, int32_t scale_v);
void __cdecl T_FlashText(TEXTSTRING* textstring, int16_t b, int16_t rate);
void __cdecl T_AddBackground(
    TEXTSTRING* textstring, int16_t xsize, int16_t ysize, int16_t xoff,
    int16_t yoff, int16_t zoff, int16_t colour, SG_COL* gourptr, int16_t flags);
void __cdecl T_RemoveBackground(TEXTSTRING* textstring);
void __cdecl T_AddOutline(
    TEXTSTRING* textstring, int b, int16_t colour, SG_COL* gourptr,
    int16_t flags);
void __cdecl T_RemoveOutline(TEXTSTRING* textstring);
void __cdecl T_CentreH(TEXTSTRING* textstring, int16_t b);
void __cdecl T_CentreV(TEXTSTRING* textstring, int16_t b);
void __cdecl T_RightAlign(TEXTSTRING* textstring, int16_t b);
void __cdecl T_BottomAlign(TEXTSTRING* textstring, int16_t b);
int32_t __cdecl T_GetTextWidth(TEXTSTRING* textstring);
void __cdecl T_RemovePrint(TEXTSTRING* textstring);
void __cdecl T_RemoveAllPrints();
void __cdecl T_DrawText();
void __cdecl T_DrawThisText(TEXTSTRING* textstring);

void T1MInjectGameText();

#endif
