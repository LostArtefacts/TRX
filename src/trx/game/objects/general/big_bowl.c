#include <trx/game/effects.h>
#include <trx/game/objects/common.h>
#include <trx/game/random.h>
#include <trx/game/rooms.h>

// clang-format off
#define M_POUR_TIME         7
#define M_FLIP_TIME         (M_POUR_TIME - 2) // = 5
#define M_DEFAULT_FLIP_SLOT 4
// clang-format on

typedef enum {
    M_STATE_TIP,
    M_STATE_POUR,
} M_STATE;

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

static void M_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);

    if (item->current_anim_state == M_STATE_POUR) {
        M_CreateHotLiquid(item);
        item->timer++;
        if (item->timer == M_FLIP_TIME * LOGIC_FPS && !Room_GetFlipStatus()) {
            // TODO: poorly hardcoded flimap number
            Room_SetFlipSlotFlags(
                M_DEFAULT_FLIP_SLOT, IF_CODE_BITS | IF_ONE_SHOT);
            Room_FlipMap();
        }
    }

    Item_Animate(item);

    if (item->status == IS_DEACTIVATED
        && item->timer >= LOGIC_FPS * M_POUR_TIME) {
        Item_RemoveActive(item_num);
    }
}

static void M_Setup(OBJECT *const obj)
{
    obj->control_func = M_Control;
    obj->save_flags = true;
    obj->save_anim = true;
}

REGISTER_OBJECT(O_BIG_BOWL, M_Setup)
