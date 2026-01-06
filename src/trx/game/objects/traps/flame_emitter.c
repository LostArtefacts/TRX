#include <trx/game/effects.h>
#include <trx/game/objects.h>
#include <trx/game/objects/effects/flame.h>
#include <trx/game/random.h>
#include <trx/game/sound.h>
#include <trx/version.h>

static void M_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    if (!Item_IsTriggerActive(item)) {
        if (item->data != nullptr) {
            const int32_t flame_num = ((int32_t)(intptr_t)item->data) - 1;
            Effect_Kill(flame_num);
            item->data = nullptr;
            if (g_TRVersion == 1) {
                Sound_StopEffect(SFX_LOOP_FOR_SMALL_FIRES);
            }
        }
    } else if (item->data == nullptr) {
        const int16_t effect_num = Effect_Create(item->room_num);
        if (effect_num != NO_EFFECT) {
            EFFECT *const effect = Effect_Get(effect_num);
            effect->pos = item->pos;
            effect->frame_num = 0;
            effect->object_id = O_FLAME;
            effect->counter = 0;
        }
        item->data = (void *)(intptr_t)(effect_num + 1);
    }
}

static void M_ControlBig(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    if (!Item_IsTriggerActive(item)) {
        if (item->data != nullptr) {
            const int32_t flame_num = ((int32_t)(intptr_t)item->data) - 1;
            Effect_Kill(flame_num);
            item->data = nullptr;
            if (g_TRVersion == 1) {
                Sound_StopEffect(SFX_LOOP_FOR_SMALL_FIRES);
            }
        }
    } else if (item->data == nullptr) {
        const int16_t effect_num = Effect_Create(item->room_num);
        if (effect_num != NO_EFFECT) {
            EFFECT *const effect = Effect_Get(effect_num);
            effect->pos = item->pos;
            effect->rot.y = item->rot.y;
            effect->frame_num = FLAME_BIG;
            effect->object_id = O_FLAME;
            effect->counter = 0;
        }
        item->data = (void *)(intptr_t)(effect_num + 1);
    }
}

static void M_ControlSmall(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    if (!Item_IsTriggerActive(item)) {
        if (item->data != nullptr) {
            const int32_t flame_num = ((int32_t)(intptr_t)item->data) - 1;
            Effect_Kill(flame_num);
            item->data = nullptr;
            if (g_TRVersion == 1) {
                Sound_StopEffect(SFX_LOOP_FOR_SMALL_FIRES);
            }
        }
    } else if (item->data == nullptr) {
        const int16_t effect_num = Effect_Create(item->room_num);
        if (effect_num != NO_EFFECT) {
            EFFECT *const effect = Effect_Get(effect_num);
            effect->pos = item->pos;
            effect->rot.y = item->rot.y;
            effect->frame_num = FLAME_SMALL;
            effect->object_id = O_FLAME;
            effect->counter = 0;
        }
        item->data = (void *)(intptr_t)(effect_num + 1);
    }
}

static void M_ControlJet(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    if (!Item_IsTriggerActive(item)) {
        if (item->data != nullptr) {
            const int32_t flame_num = ((int32_t)(intptr_t)item->data) - 1;
            Effect_Kill(flame_num);
            item->data = nullptr;
            if (g_TRVersion == 1) {
                Sound_StopEffect(SFX_LOOP_FOR_SMALL_FIRES);
            }
        }
    } else if (item->data == nullptr) {
        const int16_t effect_num = Effect_Create(item->room_num);
        if (effect_num != NO_EFFECT) {
            EFFECT *const effect = Effect_Get(effect_num);
            effect->pos = item->pos;
            effect->rot.y = item->rot.y;
            effect->frame_num = FLAME_JET;
            effect->object_id = O_FLAME;
            effect->flag1 = 0;
            effect->flag2 = Random_GetControl() & 0x3F;
            effect->counter = 0;
        }
        item->data = (void *)(intptr_t)(effect_num + 1);
    }
}

static void M_ControlSide(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    if (!Item_IsTriggerActive(item)) {
        if (item->data != nullptr) {
            const int32_t flame_num = ((int32_t)(intptr_t)item->data) - 1;
            Effect_Kill(flame_num);
            item->data = nullptr;
            if (g_TRVersion == 1) {
                Sound_StopEffect(SFX_LOOP_FOR_SMALL_FIRES);
            }
        }
    } else if (item->data == nullptr) {
        const int16_t effect_num = Effect_Create(item->room_num);
        if (effect_num != NO_EFFECT) {
            EFFECT *const effect = Effect_Get(effect_num);
            effect->pos = item->pos;
            effect->rot.y = item->rot.y;
            effect->frame_num = FLAME_SIDE;
            effect->object_id = O_FLAME;
            effect->flag1 = 0;
            effect->flag2 = 0;
            effect->counter = 0;
        }
        item->data = (void *)(intptr_t)(effect_num + 1);
    }
}

static void M_Setup(OBJECT *const obj)
{
    obj->control_func = M_Control;
    obj->draw_func = nullptr;
    obj->save_flags = true;
}

static void M_SetupBig(OBJECT *const obj)
{
    obj->control_func = M_ControlBig;
    obj->draw_func = nullptr;
    obj->save_flags = true;
}

static void M_SetupSmall(OBJECT *const obj)
{
    obj->control_func = M_ControlSmall;
    obj->draw_func = nullptr;
    obj->save_flags = true;
}

static void M_SetupJet(OBJECT *const obj)
{
    obj->control_func = M_ControlJet;
    obj->draw_func = nullptr;
    obj->save_flags = true;
}

static void M_SetupSide(OBJECT *const obj)
{
    obj->control_func = M_ControlSide;
    obj->draw_func = nullptr;
    obj->save_flags = true;
}

REGISTER_OBJECT(O_FLAME_EMITTER, M_Setup)
REGISTER_OBJECT(O_FLAME_EMITTER_BIG, M_SetupBig)
REGISTER_OBJECT(O_FLAME_EMITTER_SMALL, M_SetupSmall)
REGISTER_OBJECT(O_FLAME_EMITTER_JET, M_SetupJet)
REGISTER_OBJECT(O_FLAME_EMITTER_SIDE, M_SetupSide)
