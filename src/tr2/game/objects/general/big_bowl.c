#include "game/objects/general/big_bowl.h"

#include "game/effects.h"
#include "game/items.h"
#include "game/objects/common.h"
#include "game/random.h"
#include "game/room.h"
#include "global/vars.h"

typedef enum {
    // clang-format off
    BIG_BOWL_STATE_TIP  = 0,
    BIG_BOWL_STATE_POUR = 1,
    // clang-format on
} BIG_BOWL_STATE;

static void M_CreateHotLiquid(const ITEM *bowl_item);

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

void BigBowl_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);

    if (item->current_anim_state == BIG_BOWL_STATE_POUR) {
        M_CreateHotLiquid(item);
        item->timer++;
        if (item->timer == 5 * FRAMES_PER_SECOND && !Room_GetFlipStatus()) {
            // TODO: poorly hardcoded flimap number
            g_FlipMaps[4] = IF_CODE_BITS | IF_ONE_SHOT;
            Room_FlipMap();
        }
    }

    Item_Animate(item);

    if (item->status == IS_DEACTIVATED
        && item->timer >= FRAMES_PER_SECOND * 7) {
        Item_RemoveActive(item_num);
    }
}

void BigBowl_Setup(void)
{
    OBJECT *const obj = Object_Get(O_BIG_BOWL);
    obj->control = BigBowl_Control;
    obj->save_flags = 1;
    obj->save_anim = 1;
}
