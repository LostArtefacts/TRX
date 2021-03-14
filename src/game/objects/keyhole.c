#include "game/collide.h"
#include "game/effects.h"
#include "game/inv.h"
#include "game/lara.h"
#include "game/objects/keyhole.h"
#include "game/vars.h"
#include "config.h"

PHD_VECTOR KeyHolePosition = { 0, 0, WALL_L / 2 - LARA_RAD - 50 };
int16_t KeyHoleBounds[12] = {
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

int32_t PickUpX;
int32_t PickUpY;
int32_t PickUpZ;

void SetupKeyHole(OBJECT_INFO *obj)
{
    obj->collision = KeyHoleCollision;
    obj->save_flags = 1;
}

void KeyHoleCollision(int16_t item_num, ITEM_INFO *lara_item, COLL_INFO *coll)
{
    ITEM_INFO *item = &Items[item_num];

    if (lara_item->current_anim_state != AS_STOP) {
        return;
    }

    if ((InventoryChosen == -1 && !CHK_ANY(Input, IN_ACTION))
        || Lara.gun_status != LGS_ARMLESS || lara_item->gravity_status) {
        return;
    }

    if (!TestLaraPosition(KeyHoleBounds, item, lara_item)) {
        return;
    }

    if (item->status != IS_NOT_ACTIVE) {
        if (lara_item->pos.x != PickUpX || lara_item->pos.y != PickUpY
            || lara_item->pos.z != PickUpZ) {
            PickUpX = lara_item->pos.x;
            PickUpY = lara_item->pos.y;
            PickUpZ = lara_item->pos.z;
            SoundEffect(SFX_LARA_NO, &lara_item->pos, SPM_NORMAL);
        }
        return;
    }

    if (InventoryChosen == -1) {
        Display_Inventory(INV_KEYS_MODE);
    } else {
        PickUpY = lara_item->pos.y - 1;
    }

    if (InventoryChosen == -1 && InvKeysObjects) {
        return;
    }

    if (InventoryChosen != -1) {
        PickUpY = lara_item->pos.y - 1;
    }

    int32_t correct = 0;
    switch (item->object_number) {
    case O_KEY_HOLE1:
        if (InventoryChosen == O_KEY_OPTION1) {
            Inv_RemoveItem(O_KEY_OPTION1);
            correct = 1;
        }
        break;

    case O_KEY_HOLE2:
        if (InventoryChosen == O_KEY_OPTION2) {
            Inv_RemoveItem(O_KEY_OPTION2);
            correct = 1;
        }
        break;

    case O_KEY_HOLE3:
        if (InventoryChosen == O_KEY_OPTION3) {
            Inv_RemoveItem(O_KEY_OPTION3);
            correct = 1;
        }
        break;

    case O_KEY_HOLE4:
        if (InventoryChosen == O_KEY_OPTION4) {
            Inv_RemoveItem(O_KEY_OPTION4);
            correct = 1;
        }
        break;
    }

    InventoryChosen = -1;
    if (correct) {
        AlignLaraPosition(&KeyHolePosition, item, lara_item);
        AnimateLaraUntil(lara_item, AS_USEKEY);
        lara_item->goal_anim_state = AS_STOP;
        Lara.gun_status = LGS_HANDSBUSY;
        item->status = IS_ACTIVE;
        PickUpX = lara_item->pos.x;
        PickUpY = lara_item->pos.y;
        PickUpZ = lara_item->pos.z;
    } else if (
        lara_item->pos.x != PickUpX || lara_item->pos.y != PickUpY
        || lara_item->pos.z != PickUpZ) {
        SoundEffect(SFX_LARA_NO, &lara_item->pos, SPM_NORMAL);
        PickUpX = lara_item->pos.x;
        PickUpY = lara_item->pos.y;
        PickUpZ = lara_item->pos.z;
    }
}

int32_t KeyTrigger(int16_t item_num)
{
    ITEM_INFO *item = &Items[item_num];
    if (item->status == IS_ACTIVE
        && (T1MConfig.fix_key_triggers ? Lara.gun_status != LGS_HANDSBUSY
                                       : Lara.gun_status == LGS_ARMLESS)) {
        item->status = IS_DEACTIVATED;
        return 1;
    }
    return 0;
}
