#include "game/effects.h"

#include "game/objects/common.h"
#include "game/output.h"
#include "game/room.h"
#include "global/const.h"
#include "global/vars.h"

#include <libtrx/game/game_buf.h>
#include <libtrx/game/matrix.h>

static EFFECT *m_Effects = nullptr;
static int16_t m_NextEffectFree = NO_EFFECT;
static int16_t m_NextEffectActive = NO_EFFECT;

static void M_RemoveActive(const int16_t effect_num);
static void M_RemoveDrawn(const int16_t effect_num);

static void M_RemoveActive(const int16_t effect_num)
{
    EFFECT *const effect = Effect_Get(effect_num);
    int16_t link_num = m_NextEffectActive;
    if (link_num == effect_num) {
        m_NextEffectActive = effect->next_active;
        return;
    }

    while (link_num != NO_EFFECT) {
        if (m_Effects[link_num].next_active == effect_num) {
            m_Effects[link_num].next_active = effect->next_active;
            return;
        }
        link_num = m_Effects[link_num].next_active;
    }
}

static void M_RemoveDrawn(const int16_t effect_num)
{
    EFFECT *const effect = Effect_Get(effect_num);
    int16_t link_num = Room_Get(effect->room_num)->effect_num;
    if (link_num == effect_num) {
        Room_Get(effect->room_num)->effect_num = effect->next_free;
        return;
    }

    while (link_num != NO_EFFECT) {
        if (m_Effects[link_num].next_free == effect_num) {
            m_Effects[link_num].next_free = effect->next_free;
            return;
        }
        link_num = m_Effects[link_num].next_free;
    }
}

void Effect_InitialiseArray(void)
{
    m_Effects = GameBuf_Alloc(MAX_EFFECTS * sizeof(EFFECT), GBUF_EFFECTS);
    m_NextEffectFree = 0;
    m_NextEffectActive = NO_EFFECT;

    for (int32_t i = 0; i < MAX_EFFECTS - 1; i++) {
        EFFECT *const effect = Effect_Get(i);
        effect->next_free = i + 1;
    }
    m_Effects[MAX_EFFECTS - 1].next_free = NO_EFFECT;
}

void Effect_Control(void)
{
    int16_t effect_num = Effect_GetActiveNum();
    while (effect_num != NO_EFFECT) {
        const EFFECT *const effect = Effect_Get(effect_num);
        const OBJECT *const obj = Object_Get(effect->object_id);
        const int16_t next = effect->next_active;
        if (obj->control_func != nullptr) {
            obj->control_func(effect_num);
        }
        effect_num = next;
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

int16_t Effect_Create(const int16_t room_num)
{
    int16_t effect_num = m_NextEffectFree;
    if (effect_num == NO_EFFECT) {
        return NO_EFFECT;
    }

    EFFECT *const effect = Effect_Get(effect_num);
    m_NextEffectFree = effect->next_free;

    ROOM *const room = Room_Get(room_num);
    effect->room_num = room_num;
    effect->next_free = room->effect_num;
    room->effect_num = effect_num;

    effect->next_active = m_NextEffectActive;
    m_NextEffectActive = effect_num;

    effect->shade = 0x1000;

    return effect_num;
}

void Effect_Kill(const int16_t effect_num)
{
    EFFECT *const effect = Effect_Get(effect_num);
    M_RemoveActive(effect_num);
    M_RemoveDrawn(effect_num);

    effect->next_free = m_NextEffectFree;
    m_NextEffectFree = effect_num;
}

void Effect_NewRoom(const int16_t effect_num, const int16_t room_num)
{
    EFFECT *const effect = Effect_Get(effect_num);
    ROOM *room = Room_Get(effect->room_num);

    int16_t link_num = room->effect_num;
    if (link_num == effect_num) {
        room->effect_num = effect->next_free;
    } else {
        while (link_num != NO_EFFECT) {
            if (m_Effects[link_num].next_free == effect_num) {
                m_Effects[link_num].next_free = effect->next_free;
                break;
            }
            link_num = m_Effects[link_num].next_free;
        }
    }

    room = Room_Get(room_num);
    effect->room_num = room_num;
    effect->next_free = room->effect_num;
    room->effect_num = effect_num;
}

void Effect_Draw(const int16_t effect_num)
{
    const EFFECT *const effect = Effect_Get(effect_num);
    const OBJECT *const obj = Object_Get(effect->object_id);
    if (!obj->loaded) {
        return;
    }

    if (effect->object_id == O_GLOW) {
        Output_DrawSprite(
            (effect->rot.y << 16) | (uint16_t)effect->rot.x, effect->pos.x,
            effect->pos.y, effect->pos.z, Object_Get(O_GLOW)->mesh_idx,
            effect->shade, effect->frame_num);
        return;
    }

    if (obj->mesh_count < 0) {
        Output_DrawSprite(
            SPRITE_ABS | (obj->semi_transparent ? SPRITE_SEMI_TRANS : 0)
                | SPRITE_SHADE,
            effect->pos.x, effect->pos.y, effect->pos.z,
            obj->mesh_idx - effect->frame_num, effect->shade, 0);
        return;
    }

    Matrix_Push();
    Matrix_TranslateAbs32(effect->pos);
    if (g_MatrixPtr->_23 > g_PhdNearZ && g_MatrixPtr->_23 < g_PhdFarZ) {
        Matrix_Rot16(effect->rot);
        if (obj->mesh_count) {
            Output_CalculateStaticLight(effect->shade);
            Object_DrawMesh(obj->mesh_idx, -1, false);
        } else {
            Output_CalculateStaticLight(effect->shade);
            Object_DrawMesh(effect->frame_num, -1, false);
        }
    }
    Matrix_Pop();
}
