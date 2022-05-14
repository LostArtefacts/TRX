#include "game/objects/general/keyhole.h"

#include "config.h"
#include "game/input.h"
#include "game/inv.h"
#include "game/lara/lara_main.h"
#include "game/sound.h"
#include "global/vars.h"

PHD_VECTOR g_KeyHolePosition = { 0, 0, WALL_L / 2 - LARA_RAD - 50 };
int16_t g_KeyHoleBounds[12] = {
    -200,
    +200,
    +0,
    +0,
    +WALL_L / 2 - 200,
    +WALL_L / 2,
    -10 * PHD_DEGREE,
    +10 * PHD_DEGREE,
    -30 * PHD_DEGREE,
    +30 * PHD_DEGREE,
    -10 * PHD_DEGREE,
    +10 * PHD_DEGREE,
};

int32_t g_PickUpX;
int32_t g_PickUpY;
int32_t g_PickUpZ;

void KeyHole_Setup(OBJECT_INFO *obj)
{
    obj->collision = KeyHole_Collision;
    obj->save_flags = 1;
}

void KeyHole_Collision(int16_t item_num, ITEM_INFO *lara_item, COLL_INFO *coll)
{
    ITEM_INFO *item = &g_Items[item_num];

    if (lara_item->current_anim_state != LS_STOP) {
        return;
    }

    if ((g_InvChosen == -1 && !g_Input.action)
        || g_Lara.gun_status != LGS_ARMLESS || lara_item->gravity_status) {
        return;
    }

    if (!Lara_TestPosition(item, g_KeyHoleBounds)) {
        return;
    }

    if (item->status != IS_NOT_ACTIVE) {
        if (lara_item->pos.x != g_PickUpX || lara_item->pos.y != g_PickUpY
            || lara_item->pos.z != g_PickUpZ) {
            g_PickUpX = lara_item->pos.x;
            g_PickUpY = lara_item->pos.y;
            g_PickUpZ = lara_item->pos.z;
            Sound_Effect(SFX_LARA_NO, &lara_item->pos, SPM_NORMAL);
        }
        return;
    }

    if (g_InvChosen == -1) {
        Inv_Display(INV_KEYS_MODE);
    } else {
        g_PickUpY = lara_item->pos.y - 1;
    }

    if (g_InvChosen == -1 && g_InvKeysObjects) {
        return;
    }

    if (g_InvChosen != -1) {
        g_PickUpY = lara_item->pos.y - 1;
    }

    int32_t correct = 0;
    switch (item->object_number) {
    case O_KEY_HOLE1:
        if (g_InvChosen == O_KEY_OPTION1) {
            Inv_RemoveItem(O_KEY_OPTION1);
            correct = 1;
        }
        break;

    case O_KEY_HOLE2:
        if (g_InvChosen == O_KEY_OPTION2) {
            Inv_RemoveItem(O_KEY_OPTION2);
            correct = 1;
        }
        break;

    case O_KEY_HOLE3:
        if (g_InvChosen == O_KEY_OPTION3) {
            Inv_RemoveItem(O_KEY_OPTION3);
            correct = 1;
        }
        break;

    case O_KEY_HOLE4:
        if (g_InvChosen == O_KEY_OPTION4) {
            Inv_RemoveItem(O_KEY_OPTION4);
            correct = 1;
        }
        break;

    default:
        break;
    }

    g_InvChosen = -1;
    if (correct) {
        Lara_AlignPosition(item, &g_KeyHolePosition);
        Lara_AnimateUntil(lara_item, LS_USE_KEY);
        lara_item->goal_anim_state = LS_STOP;
        g_Lara.gun_status = LGS_HANDS_BUSY;
        item->status = IS_ACTIVE;
        g_PickUpX = lara_item->pos.x;
        g_PickUpY = lara_item->pos.y;
        g_PickUpZ = lara_item->pos.z;
    } else if (
        lara_item->pos.x != g_PickUpX || lara_item->pos.y != g_PickUpY
        || lara_item->pos.z != g_PickUpZ) {
        Sound_Effect(SFX_LARA_NO, &lara_item->pos, SPM_NORMAL);
        g_PickUpX = lara_item->pos.x;
        g_PickUpY = lara_item->pos.y;
        g_PickUpZ = lara_item->pos.z;
    }
}

bool KeyHole_Trigger(int16_t item_num)
{
    ITEM_INFO *item = &g_Items[item_num];
    if (item->status == IS_ACTIVE && g_Lara.gun_status != LGS_HANDS_BUSY) {
        item->status = IS_DEACTIVATED;
        return true;
    }
    return false;
}
