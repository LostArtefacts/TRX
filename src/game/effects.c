#include "game/effects.h"

#include "game/output.h"
#include "game/room.h"
#include "global/const.h"
#include "global/types.h"
#include "global/vars.h"
#include "math/matrix.h"

#include <stddef.h>

FX_INFO *g_Effects = NULL;
int16_t g_NextFxActive = NO_ITEM;

static int16_t m_NextFxFree = NO_ITEM;

void Effect_InitialiseArray(void)
{
    g_NextFxActive = NO_ITEM;
    m_NextFxFree = 0;
    for (int i = 0; i < NUM_EFFECTS - 1; i++) {
        g_Effects[i].next_fx = i + 1;
    }
    g_Effects[NUM_EFFECTS - 1].next_fx = NO_ITEM;
}

void Effect_Control(void)
{
    int16_t fx_num = g_NextFxActive;
    while (fx_num != NO_ITEM) {
        FX_INFO *fx = &g_Effects[fx_num];
        OBJECT_INFO *obj = &g_Objects[fx->object_number];
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

    FX_INFO *fx = &g_Effects[fx_num];
    m_NextFxFree = fx->next_fx;

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

    fx->next_fx = m_NextFxFree;
    m_NextFxFree = fx_num;
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

void Effect_Draw(int16_t fxnum)
{
    FX_INFO *fx = &g_Effects[fxnum];
    OBJECT_INFO *object = &g_Objects[fx->object_number];
    if (!object->loaded) {
        return;
    }

    if (object->nmeshes < 0) {
        Output_DrawSprite(
            fx->pos.x, fx->pos.y, fx->pos.z,
            object->mesh_index - fx->frame_number, 4096);
    } else {
        Matrix_Push();
        Matrix_TranslateAbs(fx->pos.x, fx->pos.y, fx->pos.z);
        if (g_MatrixPtr->_23 > Output_GetNearZ()
            && g_MatrixPtr->_23 < Output_GetFarZ()) {
            Matrix_RotYXZ(fx->pos.y_rot, fx->pos.x_rot, fx->pos.z_rot);
            if (object->nmeshes) {
                Output_CalculateStaticLight(fx->shade);
                Output_DrawPolygons(g_Meshes[object->mesh_index], -1);
            } else {
                Output_CalculateLight(
                    fx->pos.x, fx->pos.y, fx->pos.z, fx->room_number);
                Output_DrawPolygons(g_Meshes[fx->frame_number], -1);
            }
        }
        Matrix_Pop();
    }
}

void Effect_DoRoutine()
{
    // XXX: Some of the FX routines rely on the item to be not null!
    if (g_FlipEffect != -1) {
        g_EffectRoutines[g_FlipEffect](NULL);
    }
}
