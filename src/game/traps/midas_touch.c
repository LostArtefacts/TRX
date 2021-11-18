#include "game/traps/midas_touch.h"

#include "game/collide.h"
#include "game/draw.h"
#include "game/inv.h"
#include "global/vars.h"

int16_t MidasBounds[12] = {
    -700,
    +700,
    +384 - 100,
    +384 + 100 + 512,
    -700,
    +700,
    -10 * PHD_DEGREE,
    +10 * PHD_DEGREE,
    -30 * PHD_DEGREE,
    +30 * PHD_DEGREE,
    -10 * PHD_DEGREE,
    +10 * PHD_DEGREE,
};

void SetupMidasTouch(OBJECT_INFO *obj)
{
    obj->collision = MidasCollision;
    obj->draw_routine = DrawDummyItem;
}

void MidasCollision(int16_t item_num, ITEM_INFO *lara_item, COLL_INFO *coll)
{
    ITEM_INFO *item = &Items[item_num];

    if (!lara_item->gravity_status && lara_item->current_anim_state == AS_STOP
        && lara_item->pos.x > item->pos.x - 512
        && lara_item->pos.x < item->pos.x + 512
        && lara_item->pos.z > item->pos.z - 512
        && lara_item->pos.z < item->pos.z + 512) {
        lara_item->current_anim_state = AS_DIEMIDAS;
        lara_item->goal_anim_state = AS_DIEMIDAS;
        lara_item->anim_number = Objects[O_LARA_EXTRA].anim_index + 1;
        lara_item->frame_number = Anims[lara_item->anim_number].frame_base;
        lara_item->hit_points = -1;
        lara_item->gravity_status = 0;
        Lara.air = -1;
        Lara.gun_status = LGS_HANDSBUSY;
        Lara.gun_type = LGT_UNARMED;
        Camera.type = CAM_CINEMATIC;
        CineFrame = 0;
        CinePosition = lara_item->pos;
        return;
    }

    if ((InvChosen == -1 && !g_Input.action) || Lara.gun_status != LGS_ARMLESS
        || lara_item->gravity_status
        || lara_item->current_anim_state != AS_STOP) {
        return;
    }

    uint16_t quadrant = (uint16_t)(lara_item->pos.y_rot + PHD_45) / PHD_90;
    switch (quadrant) {
    case DIR_NORTH:
        item->pos.y_rot = 0;
        break;
    case DIR_EAST:
        item->pos.y_rot = PHD_90;
        break;
    case DIR_SOUTH:
        item->pos.y_rot = -PHD_180;
        break;
    case DIR_WEST:
        item->pos.y_rot = -PHD_90;
        break;
    }

    if (!TestLaraPosition(MidasBounds, item, lara_item)) {
        return;
    }

    if (InvChosen == -1) {
        Display_Inventory(INV_KEYS_MODE);
    }

    if (InvChosen == O_LEADBAR_OPTION) {
        Inv_RemoveItem(O_LEADBAR_OPTION);
        Inv_AddItem(O_PUZZLE_ITEM1);
        lara_item->current_anim_state = AS_USEMIDAS;
        lara_item->goal_anim_state = AS_USEMIDAS;
        lara_item->anim_number = Objects[O_LARA_EXTRA].anim_index;
        lara_item->frame_number = Anims[item->anim_number].frame_base;
        Lara.gun_status = LGS_HANDSBUSY;
    }
}
