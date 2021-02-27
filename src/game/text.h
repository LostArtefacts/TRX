#ifndef T1M_GAME_TEXT_H
#define T1M_GAME_TEXT_H

#include "game/types.h"
#include <stdint.h>

void T_InitPrint();
TEXTSTRING*
T_Print(int16_t xpos, int16_t ypos, int16_t zpos, const char* string);
void T_ChangeText(TEXTSTRING* textstring, const char* string);
void T_SetScale(TEXTSTRING* textstring, int32_t scale_h, int32_t scale_v);
void T_FlashText(TEXTSTRING* textstring, int16_t b, int16_t rate);
void T_AddBackground(
    TEXTSTRING* textstring, int16_t xsize, int16_t ysize, int16_t xoff,
    int16_t yoff, int16_t zoff, int16_t colour, SG_COL* gourptr, int16_t flags);
void T_RemoveBackground(TEXTSTRING* textstring);
void T_AddOutline(
    TEXTSTRING* textstring, int16_t b, int16_t colour, SG_COL* gourptr,
    int16_t flags);
void T_RemoveOutline(TEXTSTRING* textstring);
void T_CentreH(TEXTSTRING* textstring, int16_t b);
void T_CentreV(TEXTSTRING* textstring, int16_t b);
void T_RightAlign(TEXTSTRING* textstring, int16_t b);
void T_BottomAlign(TEXTSTRING* textstring, int16_t b);
int32_t T_GetTextWidth(TEXTSTRING* textstring);
void T_RemovePrint(TEXTSTRING* textstring);
void T_RemoveAllPrints();
void T_DrawText();
void T_DrawThisText(TEXTSTRING* textstring);

void T1MInjectGameText();

#endif
