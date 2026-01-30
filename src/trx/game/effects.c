#include <trx/game/effects.h>

#include <trx/game/game_buf.h>
#include <trx/game/matrix.h>
#include <trx/game/objects.h>
#include <trx/game/output.h>
#include <trx/game/sparks.h>
#include <trx/version.h>

static EFFECT *m_Effects = nullptr;
static int16_t m_NextEffectFree = NO_EFFECT;
static int16_t m_NextEffectActive = NO_EFFECT;

static void M_RemoveActive(const int16_t effect_num)
{
    EFFECT *const effect = Effect_Get(effect_num);
    int16_t link_num = m_NextEffectActive;
    if (link_num == effect_num) {
        m_NextEffectActive = effect->next_active;
        return;
    }
    while (link_num != NO_EFFECT) {
        EFFECT *const fx_link = Effect_Get(link_num);
        if (fx_link->next_active == effect_num) {
            fx_link->next_active = effect->next_active;
            return;
        }
        link_num = fx_link->next_active;
    }
}

static void M_RemoveDrawn(const int16_t effect_num)
{
    EFFECT *const effect = Effect_Get(effect_num);
    ROOM *const room = Room_Get(effect->room_num);
    int16_t link_num = room->effect_num;
    if (link_num == effect_num) {
        room->effect_num = effect->next_free;
        return;
    }
    while (link_num != NO_EFFECT) {
        EFFECT *const fx_link = Effect_Get(link_num);
        if (fx_link->next_free == effect_num) {
            fx_link->next_free = effect->next_free;
            return;
        }
        link_num = fx_link->next_free;
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
    int16_t effect_num = m_NextEffectActive;
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

int16_t Effect_GetInOrderNum(const int16_t effect_num)
{
    int16_t order_num = 0;
    for (int16_t link_num = Effect_GetActiveNum(); link_num != NO_EFFECT;
         link_num = Effect_Get(link_num)->next_active) {
        if (link_num == effect_num) {
            return order_num;
        }
        order_num++;
    }
    return NO_EFFECT;
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
    effect->shade = SHADE_NEUTRAL;

    return effect_num;
}

void Effect_Kill(const int16_t effect_num)
{
    EFFECT *const effect = Effect_Get(effect_num);
    Sparks_DetachEffect(effect_num);
    M_RemoveActive(effect_num);
    M_RemoveDrawn(effect_num);

    effect->next_free = m_NextEffectFree;
    m_NextEffectFree = effect_num;
}

void Effect_KillAllActive(void)
{
    int16_t effect_num = Effect_GetActiveNum();
    while (effect_num != NO_EFFECT) {
        EFFECT *const effect = Effect_Get(effect_num);
        int16_t next_effect_num = effect->next_active;
        const OBJECT *const obj = Object_Get(effect->object_id);

        if (obj->control_func != nullptr
            && (effect->object_id != O_FLAME || effect->counter >= 0)) {
            Effect_Kill(effect_num);
        }
        effect_num = next_effect_num;
    }
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

    // TR3 uses BUBBLES1 as a dummy effect carrier and renders the bubble via
    // an attached spark; keep TR1/TR2 legacy bubble rendering intact.
    if (g_TRVersion == 3 && effect->object_id == O_BUBBLE_1) {
        return;
    }

    if (effect->object_id == O_GLOW) {
        Output_DrawSprite(
            effect->interp.result.pos.x, effect->interp.result.pos.y,
            effect->interp.result.pos.z, Object_Get(O_GLOW)->mesh_idx,
            effect->shade, (RGB_F) { 1.0f, 1.0f, 1.0f }, DRAW_BLEND);
        return;
    }

    if (obj->mesh_count < 0) {
        const RGB_F tint =
            Object_IsType(effect->object_id, g_WaterSpriteObjects)
            ? (RGB_F) { 1.0f, 1.0f, 1.0f }
            : Output_GetTint();
        int16_t shade = effect->shade;
        if (shade == -1) {
            Output_CalculateLight(effect->pos, effect->room_num);
            shade = Output_GetLightAdder();
        }
        Output_DrawSprite(
            effect->interp.result.pos.x, effect->interp.result.pos.y,
            effect->interp.result.pos.z, obj->mesh_idx - effect->frame_num,
            shade, tint, DRAW_BLEND);
    } else {
        Matrix_Push();
        Matrix_TranslateAbs32(effect->interp.result.pos);
        Matrix_Rot16(effect->interp.result.rot);
        if (obj->mesh_count != 0) {
            Output_CalculateStaticLight(effect->shade);
            Object_DrawMesh(obj->mesh_idx, -1, false);
        } else {
            Output_CalculateLight(effect->interp.result.pos, effect->room_num);
            Object_DrawMesh(effect->frame_num, -1, false);
        }
        Matrix_Pop();
    }
}
