#include "game/effects.h"

#include "global/vars.h"

void Effect_InitialiseArray(void)
{
    g_NextFxActive = NO_ITEM;
    g_NextFxFree = 0;
    for (int i = 0; i < NUM_EFFECTS - 1; i++) {
        g_Effects[i].next_fx = i + 1;
    }
    g_Effects[NUM_EFFECTS - 1].next_fx = NO_ITEM;
}

int16_t Effect_Create(int16_t room_num)
{
    int16_t fx_num = g_NextFxFree;
    if (fx_num == NO_ITEM) {
        return fx_num;
    }

    FX_INFO *fx = &g_Effects[fx_num];
    g_NextFxFree = fx->next_fx;

    ROOM_INFO *r = &g_RoomInfo[room_num];
    fx->room_number = room_num;
    fx->next_fx = r->fx_number;
    r->fx_number = fx_num;

    fx->next_active = g_NextFxActive;
    g_NextFxActive = fx_num;

    return fx_num;
}

void Effect_Kill(int16_t fx_num)
{
    FX_INFO *fx = &g_Effects[fx_num];

    int16_t linknum = g_NextFxActive;
    if (linknum == fx_num) {
        g_NextFxActive = fx->next_active;
    } else {
        for (; linknum != NO_ITEM; linknum = g_Effects[linknum].next_active) {
            if (g_Effects[linknum].next_active == fx_num) {
                g_Effects[linknum].next_active = fx->next_active;
                break;
            }
        }
    }

    ROOM_INFO *r = &g_RoomInfo[fx->room_number];
    linknum = r->fx_number;
    if (linknum == fx_num) {
        r->fx_number = fx->next_fx;
    } else {
        for (; linknum != NO_ITEM; linknum = g_Effects[linknum].next_fx) {
            if (g_Effects[linknum].next_fx == fx_num) {
                g_Effects[linknum].next_fx = fx->next_fx;
                break;
            }
        }
    }

    fx->next_fx = g_NextFxFree;
    g_NextFxFree = fx_num;
}

void Effect_NewRoom(int16_t fx_num, int16_t room_num)
{
    FX_INFO *fx = &g_Effects[fx_num];
    ROOM_INFO *r = &g_RoomInfo[fx->room_number];

    int16_t linknum = r->fx_number;
    if (linknum == fx_num) {
        r->fx_number = fx->next_fx;
    } else {
        for (; linknum != NO_ITEM; linknum = g_Effects[linknum].next_fx) {
            if (g_Effects[linknum].next_fx == fx_num) {
                g_Effects[linknum].next_fx = fx->next_fx;
                break;
            }
        }
    }

    r = &g_RoomInfo[room_num];
    fx->room_number = room_num;
    fx->next_fx = r->fx_number;
    r->fx_number = fx_num;
}
