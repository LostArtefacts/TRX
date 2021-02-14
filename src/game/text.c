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

void TR1MInjectText()
{
    INJECT(0x004399A0, T_CentreH);
}
