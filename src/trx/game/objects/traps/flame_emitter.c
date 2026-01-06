#include <trx/game/effects.h>
#include <trx/game/objects.h>
#include <trx/game/objects/effects/flame.h>
#include <trx/game/random.h>
#include <trx/game/sound.h>
#include <trx/version.h>

typedef void (*FLAME_INIT_FUNC)(EFFECT *const effect, const ITEM *const item);

static void M_KillIfAlive(ITEM *const item)
{
    if (item->data == nullptr) {
        return;
    }

    const int32_t flame_num = ((int32_t)(intptr_t)item->data) - 1;
    Effect_Kill(flame_num);
    item->data = nullptr;

    if (g_TRVersion == 1) {
        Sound_StopEffect(SFX_LOOP_FOR_SMALL_FIRES);
    }
}

static void M_SpawnIfNeeded(ITEM *const item, const FLAME_INIT_FUNC init_func)
{
    const int16_t effect_num = Effect_Create(item->room_num);
    if (effect_num != NO_EFFECT) {
        EFFECT *const effect = Effect_Get(effect_num);
        effect->pos = item->pos;
        effect->object_id = O_FLAME;
        effect->counter = 0;
        init_func(effect, item);
    }
    item->data = (void *)(intptr_t)(effect_num + 1);
}

static void M_ControlCommon(
    const int16_t item_num, const FLAME_INIT_FUNC init_func)
{
    ITEM *const item = Item_Get(item_num);

    if (!Item_IsTriggerActive(item)) {
        M_KillIfAlive(item);
    } else if (item->data == nullptr) {
        M_SpawnIfNeeded(item, init_func);
    }
}

static void M_InitDefault(EFFECT *const effect, const ITEM *const item)
{
    effect->frame_num = 0;
}

static void M_InitBig(EFFECT *const effect, const ITEM *const item)
{
    effect->rot.y = item->rot.y;
    effect->frame_num = FLAME_BIG;
}

static void M_InitSmall(EFFECT *const effect, const ITEM *const item)
{
    effect->rot.y = item->rot.y;
    effect->frame_num = FLAME_SMALL;
}

static void M_InitJet(EFFECT *const effect, const ITEM *const item)
{
    effect->rot.y = item->rot.y;
    effect->frame_num = FLAME_JET;
    effect->flag1 = 0;
    effect->flag2 = Random_GetControl() & 0x3F;
}

static void M_InitSide(EFFECT *const effect, const ITEM *const item)
{
    effect->rot.y = item->rot.y;
    effect->frame_num = FLAME_SIDE;
    effect->flag1 = 0;
    effect->flag2 = 0;
}

static void M_Control(const int16_t item_num)
{
    M_ControlCommon(item_num, M_InitDefault);
}
static void M_ControlBig(const int16_t item_num)
{
    M_ControlCommon(item_num, M_InitBig);
}
static void M_ControlSmall(const int16_t item_num)
{
    M_ControlCommon(item_num, M_InitSmall);
}
static void M_ControlJet(const int16_t item_num)
{
    M_ControlCommon(item_num, M_InitJet);
}
static void M_ControlSide(const int16_t item_num)
{
    M_ControlCommon(item_num, M_InitSide);
}

static void M_SetupCommon(OBJECT *const obj, void (*control_func)(int16_t))
{
    obj->control_func = control_func;
    obj->draw_func = nullptr;
    obj->save_flags = true;
}

static void M_Setup(OBJECT *const obj)
{
    M_SetupCommon(obj, M_Control);
}
static void M_SetupBig(OBJECT *const obj)
{
    M_SetupCommon(obj, M_ControlBig);
}
static void M_SetupSmall(OBJECT *const obj)
{
    M_SetupCommon(obj, M_ControlSmall);
}
static void M_SetupJet(OBJECT *const obj)
{
    M_SetupCommon(obj, M_ControlJet);
}
static void M_SetupSide(OBJECT *const obj)
{
    M_SetupCommon(obj, M_ControlSide);
}

REGISTER_OBJECT(O_FLAME_EMITTER, M_Setup)
REGISTER_OBJECT(O_FLAME_EMITTER_BIG, M_SetupBig)
REGISTER_OBJECT(O_FLAME_EMITTER_SMALL, M_SetupSmall)
REGISTER_OBJECT(O_FLAME_EMITTER_JET, M_SetupJet)
REGISTER_OBJECT(O_FLAME_EMITTER_SIDE, M_SetupSide)
