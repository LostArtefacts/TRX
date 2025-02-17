#include "game/effects.h"

#include "game/output.h"
#include "game/room.h"
#include "global/const.h"
#include "global/types.h"
#include "global/vars.h"

#include <libtrx/game/game_buf.h>
#include <libtrx/game/matrix.h>

static EFFECT *m_Effects = nullptr;
static int16_t m_NextEffectActive = NO_EFFECT;
static int16_t m_NextEffectFree = NO_EFFECT;

void Effect_InitialiseArray(void)
{
    m_Effects = GameBuf_Alloc(NUM_EFFECTS * sizeof(EFFECT), GBUF_EFFECTS);
    m_NextEffectActive = NO_EFFECT;
    m_NextEffectFree = 0;
    for (int i = 0; i < NUM_EFFECTS - 1; i++) {
        m_Effects[i].next_draw = i + 1;
        m_Effects[i].next_free = i + 1;
    }
    m_Effects[NUM_EFFECTS - 1].next_draw = NO_EFFECT;
    m_Effects[NUM_EFFECTS - 1].next_free = NO_EFFECT;
}

void Effect_Control(void)
{
    int16_t effect_num = m_NextEffectActive;
    while (effect_num != NO_EFFECT) {
        EFFECT *effect = Effect_Get(effect_num);
        const OBJECT *const obj = Object_Get(effect->object_id);
        if (obj->control_func != nullptr) {
            obj->control_func(effect_num);
        }
        effect_num = effect->next_active;
    }
}

EFFECT *Effect_Get(const int16_t effect_num)
{
    return &m_Effects[effect_num];
}

int16_t Effect_GetNum(const EFFECT *effect)
{
    return effect - m_Effects;
}

int16_t Effect_GetActiveNum(void)
{
    return m_NextEffectActive;
}

int16_t Effect_Create(int16_t room_num)
{
    int16_t effect_num = m_NextEffectFree;
    if (effect_num == NO_EFFECT) {
        return effect_num;
    }

    EFFECT *effect = Effect_Get(effect_num);
    m_NextEffectFree = effect->next_free;

    ROOM *const room = Room_Get(room_num);
    effect->room_num = room_num;
    effect->next_draw = room->effect_num;
    room->effect_num = effect_num;

    effect->next_active = m_NextEffectActive;
    m_NextEffectActive = effect_num;

    return effect_num;
}

void Effect_Kill(int16_t effect_num)
{
    EFFECT *effect = Effect_Get(effect_num);

    if (m_NextEffectActive == effect_num) {
        m_NextEffectActive = effect->next_active;
    } else {
        int16_t link_num = m_NextEffectActive;
        while (link_num != NO_EFFECT) {
            EFFECT *fx_link = Effect_Get(link_num);
            if (fx_link->next_active == effect_num) {
                fx_link->next_active = effect->next_active;
            }
            link_num = fx_link->next_active;
        }
    }

    ROOM *const room = Room_Get(effect->room_num);
    if (room->effect_num == effect_num) {
        room->effect_num = effect->next_draw;
    } else {
        int16_t link_num = room->effect_num;
        while (link_num != NO_EFFECT) {
            EFFECT *fx_link = Effect_Get(link_num);
            if (fx_link->next_draw == effect_num) {
                fx_link->next_draw = effect->next_draw;
                break;
            }
            link_num = fx_link->next_draw;
        }
    }

    effect->next_free = m_NextEffectFree;
    m_NextEffectFree = effect_num;
}

void Effect_NewRoom(int16_t effect_num, int16_t room_num)
{
    EFFECT *effect = Effect_Get(effect_num);
    ROOM *room = Room_Get(effect->room_num);

    int16_t link_num = room->effect_num;
    if (link_num == effect_num) {
        room->effect_num = effect->next_draw;
    } else {
        for (; link_num != NO_EFFECT;
             link_num = Effect_Get(link_num)->next_draw) {
            if (Effect_Get(link_num)->next_draw == effect_num) {
                Effect_Get(link_num)->next_draw = effect->next_draw;
                break;
            }
        }
    }

    room = Room_Get(room_num);
    effect->room_num = room_num;
    effect->next_draw = room->effect_num;
    room->effect_num = effect_num;
}

void Effect_Draw(const int16_t effect_num)
{
    const EFFECT *const effect = Effect_Get(effect_num);
    const OBJECT *const obj = Object_Get(effect->object_id);
    if (!obj->loaded) {
        return;
    }

    if (obj->mesh_count < 0) {
        Output_DrawSprite(
            effect->interp.result.pos.x, effect->interp.result.pos.y,
            effect->interp.result.pos.z, obj->mesh_idx - effect->frame_num,
            4096);
    } else {
        Matrix_Push();
        Matrix_TranslateAbs32(effect->interp.result.pos);
        if (g_MatrixPtr->_23 > Output_GetNearZ()
            && g_MatrixPtr->_23 < Output_GetFarZ()) {
            Matrix_Rot16(effect->interp.result.rot);
            if (obj->mesh_count) {
                Output_CalculateStaticLight(effect->shade);
                Object_DrawMesh(obj->mesh_idx, -1, false);
            } else {
                Output_CalculateLight(
                    effect->interp.result.pos, effect->room_num);
                Object_DrawMesh(effect->frame_num, -1, false);
            }
        }
        Matrix_Pop();
    }
}
