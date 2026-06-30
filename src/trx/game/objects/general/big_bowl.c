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
    int32_t flip_time;
    int32_t flip_slot;
} M_PRIV;

static void M_Initialise(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    M_PRIV *const p = item->priv;
    p->pour_time = M_DEFAULT_POUR_TIME;
    p->flip_slot = M_DEFAULT_FLIP_SLOT;

    OBJECT_PROPERTY_VALUE value = {};
    if (ObjectProperty_GetItemValue(item, "pour_time", &value)
        && value.as_int > 0) {
        p->pour_time = value.as_int;
    }
    if (ObjectProperty_GetItemValue(item, "flip_slot", &value)
        && value.as_int < MAX_FLIP_MAPS) {
        p->flip_slot = value.as_int;
    }

    CLAMPL(p->pour_time, M_MIN_POUR_TIME);
    CLAMPL(p->flip_slot, -1);
    p->flip_time = p->pour_time - M_FLIP_TIME_OFFSET;
    p->pour_time *= LOGIC_FPS;
    p->flip_time *= LOGIC_FPS;
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
    return p->flip_slot >= 0 && item->timer == p->flip_time
        && !Room_GetFlipStatus();
}

static void M_FlipMap(const M_PRIV *const p)
{
    Room_SetFlipSlotFlags(p->flip_slot, IF_CODE_BITS | IF_ONE_SHOT);
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

    if (item->status == IS_DEACTIVATED && item->timer >= p->pour_time) {
        Item_RemoveActive(item_num);
    }
}

static void M_Setup(OBJECT *const obj)
{
    obj->initialise_func = M_Initialise;
    obj->control_func = M_Control;
    obj->save_flags = true;
    obj->save_anim = true;
    obj->priv_size = sizeof(M_PRIV);
    OBJECT_PROPERTIES(
        obj,
        OBJECT_PROPERTY_INT(
            "pour_time", M_DEFAULT_POUR_TIME,
            "The amount of time hot liquid is poured from the bowl, in "
            "seconds."),
        OBJECT_PROPERTY_INT(
            "flip_slot", M_DEFAULT_FLIP_SLOT,
            "The flip map slot to alter once liquid has finished pouring. -1 = "
            "no flipmap is performed."));
}

REGISTER_OBJECT(O_BIG_BOWL, M_Setup)
