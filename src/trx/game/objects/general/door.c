#include <trx/game/objects/general/door.h>

#include <trx/config.h>
#include <trx/game/const.h>
#include <trx/game/game_flow/common.h>
#include <trx/game/game_flow/sequencer.h>
#include <trx/game/inventory.h>
#include <trx/game/inventory_ring/control.h>
#include <trx/game/lara.h>
#include <trx/game/objects/common.h>
#include <trx/game/pathing.h>
#include <trx/game/pathing/lot.h>
#include <trx/game/rooms.h>
#include <trx/version.h>

typedef struct {
    SECTOR *sector;
    SECTOR old_sector;
    int16_t box_num;
} M_DOOR_POS;

typedef struct {
    M_DOOR_POS d1;
    M_DOOR_POS d1flip;
    M_DOOR_POS d2;
    M_DOOR_POS d2flip;
    bool crowbar;
    bool lift;
    int32_t lift_count;
    int32_t lift_base_y;
} M_PRIV;

static const OBJECT_BOUNDS m_CrowbarDoorBounds = {
    .shift = {
        .min = { .x = -WALL_L / 2, .y = -WALL_L, .z = +0, },
        .max = { .x = +WALL_L / 2, .y = +0, .z = +WALL_L / 2, },
    },
    .rot = {
        .min = { .x = -80 * DEG_1, .y = -80 * DEG_1, .z = -80 * DEG_1, },
        .max = { .x = +80 * DEG_1, .y = +80 * DEG_1, .z = +80 * DEG_1, },
    },
};

static const SECTOR m_BlockedSector = {
    .idx = 0,
    .box = NO_BOX,
    .ceiling.height = NO_HEIGHT,
    .floor.height = NO_HEIGHT,
    .ceiling.tilt = {},
    .floor.tilt = {},
    .portal_room.sky = NO_ROOM,
    .portal_room.pit = NO_ROOM,
    .portal_room.wall = NO_ROOM,
};

static const XYZ_32 m_CrowbarDoorPosition = { .x = -412, .y = 0, .z = 140 };

static bool M_ShowCrowbarInventory(void)
{
    if (!Inv_HasItem(O_CROWBAR_ITEM)) {
        return false;
    }

    InvRing_SetRequestedObjectID(O_CROWBAR_OPTION);
    const GF_COMMAND gf_cmd = GF_ShowInventory(INV_KEYS_MODE);
    if (gf_cmd.action != GF_NOOP) {
        GF_OverrideCommand(gf_cmd);
    }
    return true;
}

static SECTOR *M_GetRoomRelSector(
    const ROOM *const room, const ITEM *item, const int32_t sector_dx,
    const int32_t sector_dz)
{
    const XZ_32 sector = {
        .x = ((item->pos.x - room->pos.x) >> WALL_SHIFT) + sector_dx,
        .z = ((item->pos.z - room->pos.z) >> WALL_SHIFT) + sector_dz,
    };
    return Room_GetUnitSector(room, sector.x, sector.z);
}

static bool M_TestBlockedSector(
    const M_PRIV *const p, const XYZ_32 pos, int16_t room_num)
{
    const SECTOR *const sector = Room_GetSector(pos, &room_num);
    return sector == p->d1.sector || sector == p->d2.sector
        || sector == p->d1flip.sector || sector == p->d2flip.sector;
}

static bool M_LaraDoorCollision(const M_PRIV *const p)
{
    // Check if Lara is on, or is climbing onto, the same tile as the invisible
    // block.
    const ITEM *const lara = Lara_GetItem();
    if (lara == nullptr) {
        return false;
    }

    if (M_TestBlockedSector(p, lara->pos, lara->room_num)) {
        return true;
    }

    if (g_Config.gameplay.wall_glitch_mode != WALL_GLITCH_FIXED) {
        return false;
    }

    XYZ_32 pending_pos;
    return Item_GetPendingOrigin(lara, &pending_pos)
        && M_TestBlockedSector(p, pending_pos, lara->room_num);
}

static void M_CopySectorProperties(
    const SECTOR *const source_sector, SECTOR *const target_sector)
{
    target_sector->idx = source_sector->idx;
    target_sector->box = source_sector->box;
    target_sector->ceiling.height = source_sector->ceiling.height;
    target_sector->floor.height = source_sector->floor.height;
    target_sector->floor.tilt = source_sector->floor.tilt;
    target_sector->ceiling.tilt = source_sector->ceiling.tilt;
    target_sector->portal_room.sky = source_sector->portal_room.sky;
    target_sector->portal_room.pit = source_sector->portal_room.pit;
    target_sector->portal_room.wall = source_sector->portal_room.wall;
}

static void M_SetBoxBlocked(const int16_t box_num, const bool blocked)
{
    if (box_num == NO_BOX) {
        return;
    }

    BOX_INFO *const box = Box_GetBox(box_num);
    const bool was_blocked = (box->overlap_index & BOX_BLOCKED) != 0;
    TOGGLE_BIT(box->overlap_index, BOX_BLOCKED, blocked);

    if (g_TRVersion >= 4 && blocked != was_blocked) {
        LOT_ClearRoutes();
    }
}

static void M_Open(M_DOOR_POS *const d)
{
    if (d->sector == nullptr) {
        return;
    }

    M_CopySectorProperties(&d->old_sector, d->sector);

    M_SetBoxBlocked(d->box_num, false);
}

static void M_Check(ITEM *const item)
{
    // Forcefully remove the invisible block if Lara happens to occupy the same
    // tile. This ensures that Lara doesn't void if a timed door happens to
    // close right on her, or the player loads the game while standing on a
    // closed door's block tile. Both sides of the doorway are opened so that
    // she can cross it in one piece.
    if (M_LaraDoorCollision(item->priv)) {
        Door_OpenPortals(item);
    }
}

static void M_Shut(M_DOOR_POS *const d)
{
    if (d->sector == nullptr) {
        return;
    }

    M_CopySectorProperties(&m_BlockedSector, d->sector);

    M_SetBoxBlocked(d->box_num, true);
}

static void M_InitialisePortal(
    const ROOM *const room, const ITEM *const item, const int32_t sector_dx,
    const int32_t sector_dz, M_DOOR_POS *const door_pos)
{
    door_pos->sector = M_GetRoomRelSector(room, item, sector_dx, sector_dz);

    const SECTOR *sector = door_pos->sector;

    const int16_t room_num = door_pos->sector->portal_room.wall;
    if (room_num != NO_ROOM) {
        sector =
            M_GetRoomRelSector(Room_Get(room_num), item, sector_dx, sector_dz);
    }

    int16_t box_num = sector->box;
    const BOX_INFO *const box = Box_GetBox(box_num);
    if (box != nullptr && (box->overlap_index & BOX_BLOCKABLE) == 0) {
        box_num = NO_BOX;
    }
    door_pos->box_num = box_num;
    door_pos->old_sector = *door_pos->sector;
}

static void M_ControlLift(ITEM *const item)
{
    M_PRIV *const p = item->priv;

    if (p->lift_count > 0) {
        p->lift_count--;
        item->pos.y -= 12;

        const int32_t min_y =
            Item_GetBoundsAccurate(item)->min.y + p->lift_base_y - STEP_L;
        if (item->pos.y < min_y) {
            item->pos.y = min_y;
            p->lift_count = 0;
        }

        Door_OpenPortals(item);
    } else if (item->pos.y < p->lift_base_y) {
        item->pos.y += 4;
        if (item->pos.y >= p->lift_base_y) {
            item->pos.y = p->lift_base_y;
            Door_ShutPortals(item);
        }
    }

    M_Check(item);
}

static void M_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    M_PRIV *const p = item->priv;

    if (p->lift) {
        M_ControlLift(item);
        return;
    }

    if (Item_IsTriggerActive(item)) {
        if (item->current_anim_state == DOOR_STATE_CLOSED) {
            item->goal_anim_state = DOOR_STATE_OPEN;
        } else {
            M_Open(&p->d1);
            M_Open(&p->d2);
            M_Open(&p->d1flip);
            M_Open(&p->d2flip);
        }
    } else {
        if (item->current_anim_state == DOOR_STATE_OPEN) {
            item->goal_anim_state = DOOR_STATE_CLOSED;
        } else {
            M_Shut(&p->d1);
            M_Shut(&p->d2);
            M_Shut(&p->d1flip);
            M_Shut(&p->d2flip);
        }
    }

    M_Check(item);
    Item_Animate(item);
}

static void M_CrowbarCollision(
    const int16_t item_num, ITEM *const lara_item, COLL_INFO *const coll)
{
    ITEM *const item = Item_Get(item_num);
    LARA_INFO *const lara = Lara_GetLaraInfo();

    if (Lara_Interact_CanControl(LARA_INTERACT_DOOR, item_num)) {
        item->rot.y += DEG_180;

        if (Lara_TestPosition(item, &m_CrowbarDoorBounds)) {
            if (!lara->interact_target.is_moving) {
                // Undo the temporary flip before the inventory phase runs.
                // The inventory is a blocking nested loop that renders the
                // paused scene, so a flipped door would appear on the wrong
                // side of the wall while the player selects the crowbar.
                item->rot.y += DEG_180;
                if (!M_ShowCrowbarInventory()) {
                    Lara_RefuseInteraction();
                }
                return;
            }

            if (lara_item->current_anim_state == LS(LS_STOP)) {
                lara->interact_target.is_moving = false;
            }

            if (Lara_MovePosition(item, &m_CrowbarDoorPosition)) {
                Item_SwitchToAnim(lara_item, LA(LA_PRY_DOOR), 0);
                lara_item->current_anim_state = LS(LS_CONTROLLED);
                Item_AddSimulated(item_num);
                item->trigger.mask = TRIGGER_MASK_ALL;
                item->goal_anim_state = DOOR_STATE_OPEN;
                lara->interact_target.is_moving = false;
                lara->gun_status = LGS_HANDS_BUSY;
            } else {
                lara->interact_target.item_num = item_num;
                lara->interact_target.is_moving = true;
            }
        } else if (Lara_Interact_HasActiveTarget(item_num)) {
            lara->interact_target.is_moving = false;
            lara->gun_status = LGS_ARMLESS;
        }

        item->rot.y += DEG_180;
    }
}

static void M_Collision(
    const int16_t item_num, ITEM *const lara_item, COLL_INFO *const coll)
{
    const ITEM *const item = Item_Get(item_num);
    const M_PRIV *const p = item->priv;
    if (p->crowbar && !Item_IsInPlay(item)) {
        M_CrowbarCollision(item_num, lara_item, coll);
    }
    Door_Collision(item_num, lara_item, coll);
}

static void M_Setup(OBJECT *const obj)
{
    obj->initialise_func = Door_Initialise;
    obj->control_func = M_Control;
    obj->draw_func = Object_DrawUnclippedItem;
    obj->collision_func = M_Collision;
    obj->priv_size = sizeof(M_PRIV);
    obj->save_flags = true;
    obj->save_anim = true;
    obj->save_position = true;
    OBJECT_PROPERTIES(
        obj,
        OBJECT_PROPERTY(
            M_PRIV, crowbar, false,
            "Whether the door must be pried open with the crowbar."),
        OBJECT_PROPERTY(
            M_PRIV, lift, false,
            "Whether the door rises vertically while activated instead of "
            "playing its open animation."));
}

void Door_Initialise(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    M_PRIV *const p = item->priv;

    p->lift_count = 0;
    p->lift_base_y = item->pos.y;

    int32_t dx = 0;
    int32_t dz = 0;
    if (item->rot.y == 0) {
        dz = -1;
    } else if (item->rot.y == -DEG_180) {
        dz = 1;
    } else if (item->rot.y == DEG_90) {
        dx = -1;
    } else {
        dx = 1;
    }

    int16_t room_num = item->room_num;
    const ROOM *room = Room_Get(room_num);
    M_InitialisePortal(room, item, dx, dz, &p->d1);

    if (room->flipped_room == NO_ROOM) {
        p->d1flip.sector = nullptr;
    } else {
        room = Room_Get(room->flipped_room);
        M_InitialisePortal(room, item, dx, dz, &p->d1flip);
    }

    room_num = p->d1.sector->portal_room.wall;
    M_Shut(&p->d1);
    M_Shut(&p->d1flip);

    if (room_num == NO_ROOM) {
        p->d2.sector = nullptr;
        p->d2flip.sector = nullptr;
    } else {
        room = Room_Get(room_num);
        M_InitialisePortal(room, item, 0, 0, &p->d2);
        if (room->flipped_room == NO_ROOM) {
            p->d2flip.sector = nullptr;
        } else {
            room = Room_Get(room->flipped_room);
            M_InitialisePortal(room, item, 0, 0, &p->d2flip);
        }

        M_Shut(&p->d2);
        M_Shut(&p->d2flip);
    }
}

void Door_OpenPortals(ITEM *const item)
{
    M_PRIV *const p = item->priv;
    M_Open(&p->d1);
    M_Open(&p->d2);
    M_Open(&p->d1flip);
    M_Open(&p->d2flip);
}

void Door_ShutPortals(ITEM *const item)
{
    M_PRIV *const p = item->priv;
    M_Shut(&p->d1);
    M_Shut(&p->d2);
    M_Shut(&p->d1flip);
    M_Shut(&p->d2flip);
}

size_t Door_GetPrivSize(void)
{
    return sizeof(M_PRIV);
}

void Door_LiftActivate(ITEM *const item, const int32_t count)
{
    M_PRIV *const p = item->priv;
    p->lift_count = count;
}

int16_t Door_FindNearbyCrowbarDoor(void)
{
    for (int16_t item_num = 0; item_num < Item_GetLevelCount(); item_num++) {
        const ITEM *const item = Item_Get(item_num);
        if (item->object_id < O_DOOR_1 || item->object_id > O_DOOR_8
            || Item_IsInPlay(item)) {
            continue;
        }

        const M_PRIV *const p = item->priv;
        if (p->crowbar && Lara_IsNearItem(&item->pos, WALL_L)) {
            return item_num;
        }
    }
    return NO_ITEM;
}

void Door_Collision(
    const int16_t item_num, ITEM *const lara_item, COLL_INFO *const coll)
{
    ITEM *const item = Item_Get(item_num);

    if (!Item_TestBoundsCollide(item, lara_item, coll->radius)) {
        return;
    }

    if (!Collide_TestCollision(item, lara_item)) {
        return;
    }

    if (coll->enable_baddie_push) {
        Lara_Col_ItemPush(
            item, coll,
            coll->enable_hit
                && item->current_anim_state != item->goal_anim_state,
            true);
    }
}

REGISTER_OBJECT(O_DOOR_1, M_Setup)
REGISTER_OBJECT(O_DOOR_2, M_Setup)
REGISTER_OBJECT(O_DOOR_3, M_Setup)
REGISTER_OBJECT(O_DOOR_4, M_Setup)
REGISTER_OBJECT(O_DOOR_5, M_Setup)
REGISTER_OBJECT(O_DOOR_6, M_Setup)
REGISTER_OBJECT(O_DOOR_7, M_Setup)
REGISTER_OBJECT(O_DOOR_8, M_Setup)
