#include "game/objects/general/switch.h"

#include "game/input.h"
#include "game/items.h"
#include "global/vars.h"

typedef enum {
    SWITCH_STATE_OFF = 0,
    SWITCH_STATE_ON = 1,
    SWITCH_STATE_LINK = 2,
} SWITCH_STATE;

static void M_AlignLara(ITEM *lara_item, ITEM *switch_item);
static void M_SwitchOn(ITEM *switch_item, ITEM *lara_item);
static void M_SwitchOff(ITEM *switch_item, ITEM *lara_item);

static void M_AlignLara(ITEM *const lara_item, ITEM *const switch_item)
{
    switch (switch_item->object_id) {
    case O_SWITCH_TYPE_1:
        if (switch_item->current_anim_state == SWITCH_STATE_ON) {
            return;
        }
        Item_AlignPosition(&g_AirlockPosition, switch_item, lara_item);
        break;

    case O_SWITCH_TYPE_2:
        Item_AlignPosition(&g_SmallSwitchPosition, switch_item, lara_item);
        break;

    case O_SWITCH_TYPE_3:
        Item_AlignPosition(&g_PushSwitchPosition, switch_item, lara_item);
        break;
    }
}

static void M_SwitchOn(ITEM *const switch_item, ITEM *const lara_item)
{
    switch (switch_item->object_id) {
    default:
        lara_item->anim_num = g_Objects[O_LARA].anim_idx + LA_WALL_SWITCH_DOWN;
        break;

    case O_SWITCH_TYPE_2:
        lara_item->anim_num = g_Objects[O_LARA].anim_idx + LA_SWITCH_SMALL_DOWN;
        break;

    case O_SWITCH_TYPE_3:
        lara_item->anim_num = g_Objects[O_LARA].anim_idx + LA_BUTTON_PUSH;
        break;
    }

    lara_item->current_anim_state = LS_SWITCH_ON;
    switch_item->goal_anim_state = SWITCH_STATE_OFF;
}

static void M_SwitchOff(ITEM *const switch_item, ITEM *const lara_item)
{
    lara_item->current_anim_state = LS_SWITCH_OFF;

    switch (switch_item->object_id) {
    case O_SWITCH_TYPE_1:
        lara_item->anim_num = g_Objects[O_LARA_EXTRA].anim_idx;
        lara_item->frame_num = g_Anims[lara_item->anim_num].frame_base;
        lara_item->current_anim_state = LA_EXTRA_BREATH;
        lara_item->goal_anim_state = LA_EXTRA_AIRLOCK;
        Item_Animate(lara_item);
        g_Lara.extra_anim = 1;
        break;

    case O_SWITCH_TYPE_2:
        lara_item->anim_num = g_Objects[O_LARA].anim_idx + LA_SWITCH_SMALL_UP;
        break;

    case O_SWITCH_TYPE_3:
        lara_item->anim_num = g_Objects[O_LARA].anim_idx + LA_BUTTON_PUSH;
        break;

    default:
        lara_item->anim_num = g_Objects[O_LARA].anim_idx + LA_WALL_SWITCH_UP;
        break;
    }

    switch_item->goal_anim_state = SWITCH_STATE_ON;
}

void __cdecl Switch_Collision(
    const int16_t item_num, ITEM *const lara_item, COLL_INFO *const coll)
{
    ITEM *const item = &g_Items[item_num];
    if ((g_Input & IN_ACTION) == 0 || item->status != IS_INACTIVE
        || g_Lara.gun_status != LGS_ARMLESS || lara_item->status
        || lara_item->current_anim_state != LS_STOP
        || !Item_TestPosition(g_SwitchBounds, &g_Items[item_num], lara_item)) {
        return;
    }

    lara_item->rot.y = item->rot.y;

    M_AlignLara(lara_item, item);

    if (item->current_anim_state == SWITCH_STATE_ON) {
        M_SwitchOn(item, lara_item);
    } else {
        M_SwitchOff(item, lara_item);
    }

    if (!g_Lara.extra_anim) {
        lara_item->goal_anim_state = LS_STOP;
        lara_item->frame_num = g_Anims[lara_item->anim_num].frame_base;
    }
    g_Lara.gun_status = LGS_HANDS_BUSY;

    item->status = IS_ACTIVE;
    Item_AddActive(item_num);
    Item_Animate(item);
}
