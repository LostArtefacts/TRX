#include <trx/game/const.h>
#include <trx/game/effects.h>
#include <trx/game/objects/common.h>
#include <trx/game/random.h>
#include <trx/game/rooms.h>

// clang-format off
#define M_DEFAULT_POUR_TIME 7
#define M_FLIP_TIME_OFFSET  2
#define M_MIN_POUR_TIME     (M_FLIP_TIME_OFFSET + 1) // = 3
#define M_DEFAULT_FLIP_SLOT 4
// clang-format on

typedef enum {
    M_STATE_TIP,
    M_STATE_POUR,
} M_STATE;

typedef struct {
    int32_t pour_time;
    int32_t flip_slot;
} M_PRIV;

// The pour is stated in seconds and counted in frames, and the bowl tips over
// a little before it finishes.
static int32_t M_FlipTime(const M_PRIV *const p)
{
    return (p->pour_time - M_FLIP_TIME_OFFSET) * LOGIC_FPS;
}

static const char *M_CheckPourTime(const TRX_VALUE *const in)
{
    return in->as_int < M_MIN_POUR_TIME
        ? "pour time is below what the bowl can animate"
        : nullptr;
}

// -1 stands for no flip map at all; anything below it names nothing.
static const char *M_CheckFlipSlot(const TRX_VALUE *const in)
{
    return in->as_int < -1 || in->as_int >= MAX_FLIP_MAPS ? "no such flip map"
                                                          : nullptr;
}

static void M_CreateHotLiquid(const ITEM *const bowl_item)
{
    const int16_t effect_num = Effect_Create(bowl_item->room_num);
    const OBJECT *const obj = Object_Get(O_HOT_LIQUID);
    if (effect_num != NO_EFFECT) {
        EFFECT *const effect = Effect_Get(effect_num);
        effect->object_id = O_HOT_LIQUID;
        effect->pos.x = bowl_item->pos.x + STEP_L * 2;
        effect->pos.z = bowl_item->pos.z + STEP_L * 2;
        effect->pos.y = bowl_item->pos.y + STEP_L * 2 + 100;
        effect->room_num = bowl_item->room_num;
        effect->frame_num = (obj->mesh_count * Random_GetDraw()) >> 15;
        effect->fall_speed = 0;
        effect->shade = 2048;
    }
}

static bool M_ShouldFlipMap(const ITEM *const item)
{
    const M_PRIV *const p = item->priv;
    return p->flip_slot >= 0 && item->timer == M_FlipTime(p)
        && !Room_GetFlipStatus();
}

static void M_FlipMap(const M_PRIV *const p)
{
    FLIP_SLOT *const slot = Room_GetFlipSlot(p->flip_slot);
    slot->mask = TRIGGER_MASK_ALL;
    slot->is_one_shot = true;
    Room_FlipMap();
}

static void M_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    const M_PRIV *const p = item->priv;

    if (item->current_anim_state == M_STATE_POUR) {
        M_CreateHotLiquid(item);
        item->timer++;
        if (M_ShouldFlipMap(item)) {
            M_FlipMap(p);
        }
    }

    Item_Animate(item);

    if (item->is_finished && item->timer >= p->pour_time * LOGIC_FPS) {
        Item_RemoveSimulated(item_num);
    }
}

static void M_Setup(OBJECT *const obj)
{
    obj->control_func = M_Control;
    obj->save_flags = true;
    obj->save_anim = true;
    obj->priv_size = sizeof(M_PRIV);
    OBJECT_PROPERTIES(
        obj,
        OBJECT_PROPERTY_CHECKED(
            M_PRIV, pour_time, M_DEFAULT_POUR_TIME, M_CheckPourTime,
            "The amount of time hot liquid is poured from the bowl, in "
            "seconds. Value range: minimum 3."),
        OBJECT_PROPERTY_CHECKED(
            M_PRIV, flip_slot, M_DEFAULT_FLIP_SLOT, M_CheckFlipSlot,
            "The flip map slot to alter once liquid has finished pouring. -1 = "
            "no flipmap is performed. Value range: minimum -1; maximum 9."));
}

REGISTER_OBJECT(O_BIG_BOWL, M_Setup)
