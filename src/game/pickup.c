#include "game/collide.h"
#include "game/control.h"
#include "game/effects.h"
#include "game/game.h"
#include "game/health.h"
#include "game/inv.h"
#include "game/items.h"
#include "game/lara.h"
#include "game/pickup.h"
#include "game/savegame.h"
#include "game/vars.h"
#include "config.h"

typedef enum {
    SS_OFF = 0,
    SS_ON = 1,
    SS_LINK = 2,
} SWITCH_STATE;

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

static int16_t MidasBounds[12] = {
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

static int16_t Switch1Bounds[12] = {
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

static int16_t Switch2Bounds[12] = {
    -WALL_L,          +WALL_L,          -WALL_L,          +WALL_L,
    -WALL_L,          +WALL_L / 2,      -80 * PHD_DEGREE, +80 * PHD_DEGREE,
    -80 * PHD_DEGREE, +80 * PHD_DEGREE, -80 * PHD_DEGREE, +80 * PHD_DEGREE,
};

static int16_t KeyHoleBounds[12] = {
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

static int16_t PuzzleHoleBounds[12] = {
    -200,
    +200,
    0,
    0,
    WALL_L / 2 - 200,
    WALL_L / 2,
    -10 * PHD_DEGREE,
    +10 * PHD_DEGREE,
    -30 * PHD_DEGREE,
    +30 * PHD_DEGREE,
    -10 * PHD_DEGREE,
    +10 * PHD_DEGREE,
};

static PHD_VECTOR PickUpPosition = { 0, 0, -100 };
static PHD_VECTOR PickUpPositionUW = { 0, -200, -350 };
static PHD_VECTOR PickUpScionPosition = { 0, 640, -310 };
static PHD_VECTOR PickUpScion4Position = { 0, 280, -512 + 105 };
static PHD_VECTOR Switch2Position = { 0, 0, 108 };
static PHD_VECTOR KeyHolePosition = { 0, 0, WALL_L / 2 - LARA_RAD - 50 };
static PHD_VECTOR PuzzleHolePosition = { 0, 0, WALL_L / 2 - LARA_RAD - 85 };

static int32_t PickUpX;
static int32_t PickUpY;
static int32_t PickUpZ;

void AnimateLaraUntil(ITEM_INFO *lara_item, int32_t goal)
{
    lara_item->goal_anim_state = goal;
    do {
        AnimateLara(lara_item);
    } while (lara_item->current_anim_state != goal);
}

void PickUpCollision(int16_t item_num, ITEM_INFO *lara_item, COLL_INFO *coll)
{
    ITEM_INFO *item = &Items[item_num];
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
            SaveGame.pickups++;
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
            SaveGame.pickups++;
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
    int16_t item_num, ITEM_INFO *lara_item, COLL_INFO *coll)
{
    ITEM_INFO *item = &Items[item_num];
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
            SaveGame.pickups++;
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
    int16_t item_num, ITEM_INFO *lara_item, COLL_INFO *coll)
{
    ITEM_INFO *item = &Items[item_num];
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
        CinematicPosition = lara_item->pos;
        return;
    }

    if ((InventoryChosen == -1 && !CHK_ANY(Input, IN_ACTION))
        || Lara.gun_status != LGS_ARMLESS || lara_item->gravity_status
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

    if (InventoryChosen == -1) {
        Display_Inventory(INV_KEYS_MODE);
    }

    if (InventoryChosen == O_LEADBAR_OPTION) {
        Inv_RemoveItem(O_LEADBAR_OPTION);
        Inv_AddItem(O_PUZZLE_ITEM1);
        lara_item->current_anim_state = AS_USEMIDAS;
        lara_item->goal_anim_state = AS_USEMIDAS;
        lara_item->anim_number = Objects[O_LARA_EXTRA].anim_index;
        lara_item->frame_number = Anims[item->anim_number].frame_base;
        Lara.gun_status = LGS_HANDSBUSY;
    }
}

void SwitchCollision(int16_t item_num, ITEM_INFO *lara_item, COLL_INFO *coll)
{
    ITEM_INFO *item = &Items[item_num];

    if (!CHK_ANY(Input, IN_ACTION) || item->status != IS_NOT_ACTIVE
        || Lara.gun_status != LGS_ARMLESS || lara_item->gravity_status) {
        return;
    }

    if (lara_item->current_anim_state != AS_STOP) {
        return;
    }

    if (!TestLaraPosition(Switch1Bounds, item, lara_item)) {
        return;
    }

    lara_item->pos.y_rot = item->pos.y_rot;
    if (item->current_anim_state == SS_ON) {
        AnimateLaraUntil(lara_item, AS_SWITCHON);
        lara_item->goal_anim_state = AS_STOP;
        Lara.gun_status = LGS_HANDSBUSY;
        item->status = IS_ACTIVE;
        item->goal_anim_state = SS_OFF;
        AddActiveItem(item_num);
        AnimateItem(item);
    } else if (item->current_anim_state == SS_OFF) {
        AnimateLaraUntil(lara_item, AS_SWITCHOFF);
        lara_item->goal_anim_state = AS_STOP;
        Lara.gun_status = LGS_HANDSBUSY;
        item->status = IS_ACTIVE;
        item->goal_anim_state = SS_ON;
        AddActiveItem(item_num);
        AnimateItem(item);
    }
}

void SwitchCollision2(int16_t item_num, ITEM_INFO *lara_item, COLL_INFO *coll)
{
    ITEM_INFO *item = &Items[item_num];

    if (!CHK_ANY(Input, IN_ACTION) || item->status != IS_NOT_ACTIVE
        || Lara.water_status != LWS_UNDERWATER) {
        return;
    }

    if (lara_item->current_anim_state != AS_TREAD) {
        return;
    }

    if (!TestLaraPosition(Switch2Bounds, item, lara_item)) {
        return;
    }

    if (item->current_anim_state == SS_ON
        || item->current_anim_state == SS_OFF) {
        if (!MoveLaraPosition(&Switch2Position, item, lara_item)) {
            return;
        }
        lara_item->fall_speed = 0;
        AnimateLaraUntil(lara_item, AS_SWITCHON);
        lara_item->goal_anim_state = AS_TREAD;
        Lara.gun_status = LGS_HANDSBUSY;
        item->status = IS_ACTIVE;
        if (item->current_anim_state == SS_ON) {
            item->goal_anim_state = SS_OFF;
        } else {
            item->goal_anim_state = SS_ON;
        }
        AddActiveItem(item_num);
        AnimateItem(item);
    }
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

void PuzzleHoleCollision(
    int16_t item_num, ITEM_INFO *lara_item, COLL_INFO *coll)
{
    ITEM_INFO *item = &Items[item_num];

    if (lara_item->current_anim_state == AS_USEPUZZLE) {
        if (!TestLaraPosition(PuzzleHoleBounds, item, lara_item)) {
            return;
        }

        if (lara_item->frame_number == AF_USEPUZZLE) {
            switch (item->object_number) {
            case O_PUZZLE_HOLE1:
                item->object_number = O_PUZZLE_DONE1;
                break;

            case O_PUZZLE_HOLE2:
                item->object_number = O_PUZZLE_DONE2;
                break;

            case O_PUZZLE_HOLE3:
                item->object_number = O_PUZZLE_DONE3;
                break;

            case O_PUZZLE_HOLE4:
                item->object_number = O_PUZZLE_DONE4;
                break;
            }
        }

        return;
    } else if (lara_item->current_anim_state != AS_STOP) {
        return;
    }

    if ((InventoryChosen == -1 && !CHK_ANY(Input, IN_ACTION))
        || Lara.gun_status != LGS_ARMLESS || lara_item->gravity_status) {
        return;
    }

    if (!TestLaraPosition(PuzzleHoleBounds, item, lara_item)) {
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
    case O_PUZZLE_HOLE1:
        if (InventoryChosen == O_PUZZLE_OPTION1) {
            Inv_RemoveItem(O_PUZZLE_OPTION1);
            correct = 1;
        }
        break;

    case O_PUZZLE_HOLE2:
        if (InventoryChosen == O_PUZZLE_OPTION2) {
            Inv_RemoveItem(O_PUZZLE_OPTION2);
            correct = 1;
        }
        break;

    case O_PUZZLE_HOLE3:
        if (InventoryChosen == O_PUZZLE_OPTION3) {
            Inv_RemoveItem(O_PUZZLE_OPTION3);
            correct = 1;
        }
        break;

    case O_PUZZLE_HOLE4:
        if (InventoryChosen == O_PUZZLE_OPTION4) {
            Inv_RemoveItem(O_PUZZLE_OPTION4);
            correct = 1;
        }
        break;
    }

    InventoryChosen = -1;
    if (correct) {
        AlignLaraPosition(&PuzzleHolePosition, item, lara_item);
        AnimateLaraUntil(lara_item, AS_USEPUZZLE);
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

void SwitchControl(int16_t item_num)
{
    ITEM_INFO *item = &Items[item_num];
    item->flags |= IF_CODE_BITS;
    if (!TriggerActive(item)) {
        item->goal_anim_state = SS_ON;
        item->timer = 0;
    }
    AnimateItem(item);
}

int32_t SwitchTrigger(int16_t item_num, int16_t timer)
{
    ITEM_INFO *item = &Items[item_num];
    if (item->status != IS_DEACTIVATED) {
        return 0;
    }
    if (item->current_anim_state == SS_OFF && timer > 0) {
        item->timer = timer;
        if (timer != 1) {
            item->timer *= 30;
        }
        item->status = IS_ACTIVE;
    } else {
        RemoveActiveItem(item_num);
        item->status = IS_NOT_ACTIVE;
    }
    return 1;
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

int32_t PickupTrigger(int16_t item_num)
{
    ITEM_INFO *item = &Items[item_num];
    if (item->status != IS_INVISIBLE) {
        return 0;
    }
    item->status = IS_DEACTIVATED;
    return 1;
}

void PickUpSaveGameCollision(
    int16_t item_num, ITEM_INFO *lara_item, COLL_INFO *coll)
{
#ifdef T1M_FEAT_SAVE_CRYSTALS
    ITEM_INFO *item = &Items[item_num];
    ObjectCollision(item_num, lara_item, coll);

    if (!CHK_ANY(Input, IN_ACTION) || Lara.gun_status != LGS_ARMLESS
        || lara_item->gravity_status) {
        return;
    }

    if (lara_item->current_anim_state != AS_STOP) {
        return;
    }

    item->pos.y_rot = lara_item->pos.y_rot;
    item->pos.z_rot = 0;
    item->pos.x_rot = 0;
    if (!TestLaraPosition(PickUpBounds, item, lara_item)) {
        return;
    }

    item->status = IS_INVISIBLE;
    CreateSaveGameInfo();
    if (S_SaveGame(&SaveGame, -1)) {
        item->status = IS_INVISIBLE;
        RemoveDrawnItem(item_num);
    } else {
        item->status = IS_ACTIVE;
    }
#endif
}

void InitialiseSaveGameItem(int16_t item_num)
{
#ifdef T1M_FEAT_SAVE_CRYSTALS
    AddActiveItem(item_num);
#else
    Items[item_num].status = IS_INVISIBLE;
#endif
}

void ControlSaveGameItem(int16_t item_num)
{
#ifdef T1M_FEAT_SAVE_CRYSTALS
    AnimateItem(&Items[item_num]);
#endif
}

void T1MInjectGamePickup()
{
    INJECT(0x00433080, PickUpCollision);
    INJECT(0x00433240, PickUpScionCollision);
    INJECT(0x004333B0, PickUpScion4Collision);
    INJECT(0x004334C0, MidasCollision);
    INJECT(0x004336F0, SwitchCollision);
    INJECT(0x00433810, SwitchCollision2);
    INJECT(0x00433900, KeyHoleCollision);
    INJECT(0x00433B40, PuzzleHoleCollision);
    INJECT(0x00433DE0, SwitchControl);
    INJECT(0x00433E20, SwitchTrigger);
    INJECT(0x00433EA0, KeyTrigger);
    INJECT(0x00433EF0, PickupTrigger);
}
