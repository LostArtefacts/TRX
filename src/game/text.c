#include "game/text.h"
#include "types.h"
#include "util.h"

void __cdecl T_CentreH(TEXTSTRING* textstring, int16_t b)
{
    if (b) {
        textstring->flags |= TF_CENTRE_H;
    } else {
        textstring->flags &= ~TF_CENTRE_H;
    }
}

void __cdecl T_CentreV(TEXTSTRING* textstring, int16_t b)
{
    if (b) {
        textstring->flags |= TF_CENTRE_V;
    } else {
        textstring->flags &= ~TF_CENTRE_V;
    }
}

void __cdecl T_RightAlign(TEXTSTRING* textstring, int16_t b)
{
    if (b) {
        textstring->flags |= TF_RIGHT;
    } else {
        textstring->flags &= ~TF_RIGHT;
    }
}

void __cdecl T_BottomAlign(TEXTSTRING* textstring, int16_t b)
{
    if (b) {
        textstring->flags |= TF_BOTTOM;
    } else {
        textstring->flags &= ~TF_BOTTOM;
    }
}

void TR1MInjectText()
{
    INJECT(0x004399A0, T_CentreH);
    INJECT(0x004399C0, T_CentreV);
    INJECT(0x004399E0, T_RightAlign);
    INJECT(0x00439A00, T_BottomAlign);
}
