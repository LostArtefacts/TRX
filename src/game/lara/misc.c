#include "game/effects.h"

#include <libtrx/game/lara/misc.h>

void Lara_Extinguish(void)
{
    // put out flame objects
    int16_t fx_num = g_NextFxActive;
    while (fx_num != NO_ITEM) {
        FX_INFO *const fx = &g_Effects[fx_num];
        const int16_t next_fx_num = fx->next_active;
        if (fx->object_id == O_FLAME && fx->counter < 0) {
            fx->counter = 0;
            Effect_Kill(fx_num);
        }
        fx_num = next_fx_num;
    }
}
