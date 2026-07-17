#pragma once

#include <trx/game/items/actions/ids.h>
#include <trx/game/items/types.h>

#include <stdint.h>

void ItemAction_Register(
    ITEM_TRX_ACTION action, void (*action_func)(ITEM *item));
void ItemAction_Run(ITEM_TRX_ACTION action_id, ITEM *item);
void ItemAction_RunDirect(ITEM_ACTION action_id, ITEM *item);
void ItemAction_RunDirectWithFX(
    ITEM_ACTION action_id, ITEM *item, int16_t fx_type);
void ItemAction_RunActive(void);
int16_t ItemAction_GetFXType(void);

// An interceptor takes over an effect before it runs, whether a level trigger
// or an animation command reached it: when it returns true, the stock routine
// does not run. At most one is installed.
typedef bool (*ITEM_ACTION_INTERCEPTOR)(
    int32_t effect_num, int32_t timer, int16_t item_num);
void ItemAction_SetInterceptor(ITEM_ACTION_INTERCEPTOR interceptor);
bool ItemAction_Intercept(int32_t effect_num, int32_t timer, int16_t item_num);

#define REGISTER_ITEM_ACTION(action, action_func)                              \
    __attribute__((constructor)) static void M_RegisterActionHandler##action(  \
        void)                                                                  \
    {                                                                          \
        ItemAction_Register(action, action_func);                              \
    }
