#include "game/effects.h"

#include "game/output.h"
#include "game/room.h"
#include "global/const.h"
#include "global/types.h"
#include "global/vars.h"
#include "math/matrix.h"

#include <stddef.h>

FX *g_Effects = NULL;
int16_t g_NextFxActive = NO_ITEM;

static int16_t m_NextFxFree = NO_ITEM;

void Effect_InitialiseArray(void)
{
    g_NextFxActive = NO_ITEM;
    m_NextFxFree = 0;
    for (int i = 0; i < NUM_EFFECTS - 1; i++) {
        g_Effects[i].next_draw = i + 1;
        g_Effects[i].next_free = i + 1;
    }
    g_Effects[NUM_EFFECTS - 1].next_draw = NO_ITEM;
    g_Effects[NUM_EFFECTS - 1].next_free = NO_ITEM;
}

void Effect_Control(void)
{
    int16_t fx_num = g_NextFxActive;
    while (fx_num != NO_ITEM) {
        FX *fx = &g_Effects[fx_num];
        OBJECT *obj = &g_Objects[fx->object_id];
        if (obj->control) {
            obj->control(fx_num);
        }
        fx_num = fx->next_active;
    }
}

int16_t Effect_Create(int16_t room_num)
{
    int16_t fx_num = m_NextFxFree;
    if (fx_num == NO_ITEM) {
        return fx_num;
    }

    FX *fx = &g_Effects[fx_num];
    m_NextFxFree = fx->next_free;

    ROOM *r = &g_RoomInfo[room_num];
    fx->room_num = room_num;
    fx->next_draw = r->fx_num;
    r->fx_num = fx_num;

    fx->next_active = g_NextFxActive;
    g_NextFxActive = fx_num;

    return fx_num;
}

void Effect_Kill(int16_t fx_num)
{
    FX *fx = &g_Effects[fx_num];

    if (g_NextFxActive == fx_num) {
        g_NextFxActive = fx->next_active;
    } else {
        int16_t linknum = g_NextFxActive;
        while (linknum != NO_ITEM) {
            FX *fx_link = &g_Effects[linknum];
            if (fx_link->next_active == fx_num) {
                fx_link->next_active = fx->next_active;
            }
            linknum = fx_link->next_active;
        }
    }

    ROOM *r = &g_RoomInfo[fx->room_num];
    if (r->fx_num == fx_num) {
        r->fx_num = fx->next_draw;
    } else {
        int16_t linknum = r->fx_num;
        while (linknum != NO_ITEM) {
            FX *fx_link = &g_Effects[linknum];
            if (fx_link->next_draw == fx_num) {
                fx_link->next_draw = fx->next_draw;
                break;
            }
            linknum = fx_link->next_draw;
        }
    }

    fx->next_free = m_NextFxFree;
    m_NextFxFree = fx_num;
}

void Effect_NewRoom(int16_t fx_num, int16_t room_num)
{
    FX *fx = &g_Effects[fx_num];
    ROOM *r = &g_RoomInfo[fx->room_num];

    int16_t linknum = r->fx_num;
    if (linknum == fx_num) {
        r->fx_num = fx->next_draw;
    } else {
        for (; linknum != NO_ITEM; linknum = g_Effects[linknum].next_draw) {
            if (g_Effects[linknum].next_draw == fx_num) {
                g_Effects[linknum].next_draw = fx->next_draw;
                break;
            }
        }
    }

    r = &g_RoomInfo[room_num];
    fx->room_num = room_num;
    fx->next_draw = r->fx_num;
    r->fx_num = fx_num;
}

void Effect_Draw(const int16_t fxnum)
{
    const FX *const fx = &g_Effects[fxnum];
    const OBJECT *const object = &g_Objects[fx->object_id];
    if (!object->loaded) {
        return;
    }

    if (object->nmeshes < 0) {
        Output_DrawSprite(
            fx->interp.result.pos.x, fx->interp.result.pos.y,
            fx->interp.result.pos.z, object->mesh_idx - fx->frame_num, 4096);
    } else {
        Matrix_Push();
        Matrix_TranslateAbs(
            fx->interp.result.pos.x, fx->interp.result.pos.y,
            fx->interp.result.pos.z);
        if (g_MatrixPtr->_23 > Output_GetNearZ()
            && g_MatrixPtr->_23 < Output_GetFarZ()) {
            Matrix_RotYXZ(
                fx->interp.result.rot.y, fx->interp.result.rot.x,
                fx->interp.result.rot.z);
            if (object->nmeshes) {
                Output_CalculateStaticLight(fx->shade);
                Output_DrawPolygons(g_Meshes[object->mesh_idx], -1);
            } else {
                Output_CalculateLight(
                    fx->interp.result.pos.x, fx->interp.result.pos.y,
                    fx->interp.result.pos.z, fx->room_num);
                Output_DrawPolygons(g_Meshes[fx->frame_num], -1);
            }
        }
        Matrix_Pop();
    }
}

void Effect_RunActiveFlipEffect(void)
{
    // XXX: Some of the FX routines rely on the item to be not null!
    if (g_FlipEffect != -1) {
        g_EffectRoutines[g_FlipEffect](NULL);
    }
}
