#include "game/collide.h"
#include "game/health.h"
#include "game/inv.h"
#include "game/items.h"
#include "game/lara.h"
#include "game/pickup.h"
#include "game/vars.h"
#include "config.h"

static int16_t PickUpBounds[12] = {
    -256, +256, -100, +100, -256, +100, -10 * PHD_DEGREE, +10 * PHD_DEGREE,
    0,    0,    0,    0,
};

static int16_t PickUpBoundsUW[12] = {
    -512,
    +512,
    -512,
    +512,
    -512,
    +512,
    -45 * PHD_DEGREE,
    +45 * PHD_DEGREE,
    -45 * PHD_DEGREE,
    +45 * PHD_DEGREE,
    -45 * PHD_DEGREE,
    +45 * PHD_DEGREE,
};

static PHD_VECTOR PickUpPosition = { 0, 0, -100 };
static PHD_VECTOR PickUpPositionUW = { 0, -200, -350 };

static int16_t PickUpScionBounds[12] = {
    -256,
    +256,
    +640 - 100,
    +640 + 100,
    -350,
    -200,
    -10 * PHD_DEGREE,
    +10 * PHD_DEGREE,
    0,
    0,
    0,
    0,
};
static PHD_VECTOR PickUpScionPosition = { 0, 640, -310 };

static int16_t PickUpScion4Bounds[12] = {
    -256,
    +256,
    +256 - 50,
    +256 + 50,
    -512 - 350,
    -200,
    -10 * PHD_DEGREE,
    +10 * PHD_DEGREE,
    0,
    0,
    0,
    0,
};
static PHD_VECTOR PickUpScion4Position = { 0, 280, -512 + 105 };

void AnimateLaraUntil(ITEM_INFO* lara_item, int32_t goal)
{
    lara_item->goal_anim_state = goal;
    do {
        AnimateLara(lara_item);
    } while (lara_item->current_anim_state != goal);
}

void PickUpCollision(int16_t item_num, ITEM_INFO* lara_item, COLL_INFO* coll)
{
    ITEM_INFO* item = &Items[item_num];
    item->pos.y_rot = lara_item->pos.y_rot;
    item->pos.z_rot = 0;

    if (Lara.water_status == LWS_ABOVEWATER) {
        item->pos.x_rot = 0;
        if (!TestLaraPosition(PickUpBounds, item, lara_item)) {
            return;
        }

        if (lara_item->current_anim_state == AS_PICKUP) {
            if (lara_item->frame_number != AF_PICKUP) {
                return;
            }
            if (item->object_number == O_SHOTGUN_ITEM) {
                Lara.mesh_ptrs[LM_TORSO] =
                    Meshes[Objects[O_SHOTGUN].mesh_index + LM_TORSO];
            }
            AddDisplayPickup(item->object_number);
            Inv_AddItem(item->object_number);
            item->status = IS_INVISIBLE;
            RemoveDrawnItem(item_num);
            SaveGame[0].pickups++;
            return;
        }

        if (CHK_ANY(Input, IN_ACTION) && Lara.gun_status == LGS_ARMLESS
            && !lara_item->gravity_status
            && lara_item->current_anim_state == AS_STOP) {
            AlignLaraPosition(&PickUpPosition, item, lara_item);
            AnimateLaraUntil(lara_item, AS_PICKUP);
            lara_item->goal_anim_state = AS_STOP;
            Lara.gun_status = LGS_HANDSBUSY;
            return;
        }
    } else if (Lara.water_status == LWS_UNDERWATER) {
        item->pos.x_rot = -25 * PHD_DEGREE;
        if (!TestLaraPosition(PickUpBoundsUW, item, lara_item)) {
            return;
        }

        if (lara_item->current_anim_state == AS_PICKUP) {
            if (lara_item->frame_number != AF_PICKUP_UW) {
                return;
            }
            AddDisplayPickup(item->object_number);
            Inv_AddItem(item->object_number);
            item->status = IS_INVISIBLE;
            RemoveDrawnItem(item_num);
            SaveGame[0].pickups++;
            return;
        }

        if (CHK_ANY(Input, IN_ACTION)
            && lara_item->current_anim_state == AS_TREAD) {
            if (!MoveLaraPosition(&PickUpPositionUW, item, lara_item)) {
                return;
            }
            AnimateLaraUntil(lara_item, AS_PICKUP);
            lara_item->goal_anim_state = AS_TREAD;
        }
    }
}

void PickUpScionCollision(
    int16_t item_num, ITEM_INFO* lara_item, COLL_INFO* coll)
{
    ITEM_INFO* item = &Items[item_num];
    item->pos.y_rot = lara_item->pos.y_rot;
    item->pos.x_rot = 0;
    item->pos.z_rot = 0;

    if (!TestLaraPosition(PickUpScionBounds, item, lara_item)) {
        return;
    }

    if (lara_item->current_anim_state == AS_PICKUP) {
        if (lara_item->frame_number
            == Anims[lara_item->anim_number].frame_base + AF_PICKUPSCION) {
            AddDisplayPickup(item->object_number);
            Inv_AddItem(item->object_number);
            item->status = IS_INVISIBLE;
            RemoveDrawnItem(item_num);
            SaveGame[0].pickups++;
        }
    } else if (
        CHK_ANY(Input, IN_ACTION) && Lara.gun_status == LGS_ARMLESS
        && !lara_item->gravity_status
        && lara_item->current_anim_state == AS_STOP) {
        AlignLaraPosition(&PickUpScionPosition, item, lara_item);
        lara_item->current_anim_state = AS_PICKUP;
        lara_item->goal_anim_state = AS_PICKUP;
        lara_item->anim_number = Objects[O_LARA_EXTRA].anim_index;
        lara_item->frame_number = Anims[lara_item->anim_number].frame_base;
        Lara.gun_status = LGS_HANDSBUSY;
        Camera.type = CAM_CINEMATIC;
        CineFrame = 0;
        CinematicPosition = lara_item->pos;
    }
}

void PickUpScion4Collision(
    int16_t item_num, ITEM_INFO* lara_item, COLL_INFO* coll)
{
    ITEM_INFO* item = &Items[item_num];
    item->pos.y_rot = lara_item->pos.y_rot;
    item->pos.x_rot = 0;
    item->pos.z_rot = 0;

    if (!TestLaraPosition(PickUpScion4Bounds, item, lara_item)) {
        return;
    }

    if (CHK_ANY(Input, IN_ACTION) && Lara.gun_status == LGS_ARMLESS
        && !lara_item->gravity_status
        && lara_item->current_anim_state == AS_STOP) {
        AlignLaraPosition(&PickUpScion4Position, item, lara_item);
        lara_item->current_anim_state = AS_PICKUP;
        lara_item->goal_anim_state = AS_PICKUP;
        lara_item->anim_number = Objects[O_LARA_EXTRA].anim_index;
        lara_item->frame_number = Anims[lara_item->anim_number].frame_base;
        Lara.gun_status = LGS_HANDSBUSY;
        Camera.type = CAM_CINEMATIC;
        CineFrame = 0;
        CinematicPosition = lara_item->pos;
        CinematicPosition.y_rot -= PHD_90;
    }
}

int32_t KeyTrigger(int16_t item_num)
{
    ITEM_INFO* item = &Items[item_num];
#ifdef T1M_FEAT_OG_FIXES
    if (item->status == IS_ACTIVE
        && (T1MConfig.fix_key_triggers ? Lara.gun_status != LGS_HANDSBUSY
                                       : Lara.gun_status == LGS_ARMLESS)) {
        item->status = IS_DEACTIVATED;
        return 1;
    }
#else
    if (item->status == IS_ACTIVE && Lara.gun_status == LGS_ARMLESS) {
        item->status = IS_DEACTIVATED;
        return 1;
    }
#endif
    return 0;
}

void T1MInjectGamePickup()
{
    INJECT(0x00433080, PickUpCollision);
    INJECT(0x00433240, PickUpScionCollision);
    INJECT(0x004333B0, PickUpScion4Collision);
    INJECT(0x00433EA0, KeyTrigger);
}
