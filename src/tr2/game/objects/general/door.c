#include "game/objects/general/door.h"

#include "game/box.h"
#include "game/collide.h"
#include "game/items.h"
#include "game/lara/misc.h"
#include "game/objects/common.h"
#include "game/room.h"
#include "global/utils.h"
#include "global/vars.h"

#include <libtrx/game/game_buf.h>

typedef struct {
    DOORPOS_DATA d1;
    DOORPOS_DATA d1flip;
    DOORPOS_DATA d2;
    DOORPOS_DATA d2flip;
} DOOR_DATA;

static SECTOR *M_GetRoomRelSector(
    const ROOM *room, const ITEM *item, int32_t sector_dx, int32_t sector_dz);
static void M_InitialisePortal(
    const ROOM *room, const ITEM *item, int32_t sector_dx, int32_t sector_dz,
    DOORPOS_DATA *door_pos);
static void M_Shut(DOORPOS_DATA *d);
static void M_Open(DOORPOS_DATA *d);
static void M_Setup(OBJECT *obj);
static void M_Initialise(int16_t item_num);
static void M_Control(int16_t item_num);

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

static void Door_Shut(DOORPOS_DATA *const d)
{
    SECTOR *const sector = d->sector;
    if (d->sector == nullptr) {
        return;
    }

    sector->idx = 0;
    sector->box = NO_BOX;
    sector->ceiling.height = NO_HEIGHT;
    sector->floor.height = NO_HEIGHT;
    sector->floor.tilt = 0;
    sector->ceiling.tilt = 0;
    sector->portal_room.sky = NO_ROOM_NEG;
    sector->portal_room.pit = NO_ROOM_NEG;
    sector->portal_room.wall = NO_ROOM;

    const int16_t box_num = d->block;
    if (box_num != NO_BOX) {
        g_Boxes[box_num].overlap_index |= BOX_BLOCKED;
    }
}

static void Door_Open(DOORPOS_DATA *const d)
{
    if (d->sector == nullptr) {
        return;
    }

    *d->sector = d->old_sector;

    const int16_t box_num = d->block;
    if (box_num != NO_BOX) {
        g_Boxes[box_num].overlap_index &= ~BOX_BLOCKED;
    }
}

static void M_InitialisePortal(
    const ROOM *const room, const ITEM *const item, const int32_t sector_dx,
    const int32_t sector_dz, DOORPOS_DATA *const door_pos)
{
    door_pos->sector = M_GetRoomRelSector(room, item, sector_dx, sector_dz);

    const SECTOR *sector = door_pos->sector;

    const int16_t room_num = door_pos->sector->portal_room.wall;
    if (room_num != NO_ROOM) {
        sector =
            M_GetRoomRelSector(Room_Get(room_num), item, sector_dx, sector_dz);
    }

    int16_t box_num = sector->box;
    if (!(g_Boxes[box_num].overlap_index & BOX_BLOCKABLE)) {
        box_num = NO_BOX;
    }
    door_pos->block = box_num;
    door_pos->old_sector = *door_pos->sector;
}

static void M_Setup(OBJECT *const obj)
{
    obj->initialise_func = M_Initialise;
    obj->control_func = M_Control;
    obj->draw_func = Object_DrawUnclippedItem;
    obj->collision_func = Door_Collision;
    obj->save_flags = 1;
    obj->save_anim = 1;
}

static void M_Initialise(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    DOOR_DATA *door = GameBuf_Alloc(sizeof(DOOR_DATA), GBUF_ITEM_DATA);
    item->data = door;

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
    M_InitialisePortal(room, item, dx, dz, &door->d1);

    if (room->flipped_room == NO_ROOM_NEG) {
        door->d1flip.sector = nullptr;
    } else {
        room = Room_Get(room->flipped_room);
        M_InitialisePortal(room, item, dx, dz, &door->d1flip);
    }

    room_num = door->d1.sector->portal_room.wall;
    Door_Shut(&door->d1);
    Door_Shut(&door->d1flip);

    if (room_num == NO_ROOM) {
        door->d2.sector = nullptr;
        door->d2flip.sector = nullptr;
    } else {
        room = Room_Get(room_num);
        M_InitialisePortal(room, item, 0, 0, &door->d2);
        if (room->flipped_room == NO_ROOM_NEG) {
            door->d2flip.sector = nullptr;
        } else {
            room = Room_Get(room->flipped_room);
            M_InitialisePortal(room, item, 0, 0, &door->d2flip);
        }

        Door_Shut(&door->d2);
        Door_Shut(&door->d2flip);

        const int16_t prev_room = item->room_num;
        Item_NewRoom(item_num, room_num);
        item->room_num = prev_room;
    }
}

static void M_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    DOOR_DATA *const data = item->data;

    if (Item_IsTriggerActive(item)) {
        if (item->current_anim_state == DOOR_STATE_CLOSED) {
            item->goal_anim_state = DOOR_STATE_OPEN;
        } else {
            Door_Open(&data->d1);
            Door_Open(&data->d2);
            Door_Open(&data->d1flip);
            Door_Open(&data->d2flip);
        }
    } else {
        if (item->current_anim_state == DOOR_STATE_OPEN) {
            item->goal_anim_state = DOOR_STATE_CLOSED;
        } else {
            Door_Shut(&data->d1);
            Door_Shut(&data->d2);
            Door_Shut(&data->d1flip);
            Door_Shut(&data->d2flip);
        }
    }

    Item_Animate(item);
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
        Lara_Push(
            item, lara_item, coll,
            item->current_anim_state != item->goal_anim_state ? coll->enable_hit
                                                              : false,
            true);
    }
}

REGISTER_OBJECT(O_DOOR_TYPE_1, M_Setup)
REGISTER_OBJECT(O_DOOR_TYPE_2, M_Setup)
REGISTER_OBJECT(O_DOOR_TYPE_3, M_Setup)
REGISTER_OBJECT(O_DOOR_TYPE_4, M_Setup)
REGISTER_OBJECT(O_DOOR_TYPE_5, M_Setup)
REGISTER_OBJECT(O_DOOR_TYPE_6, M_Setup)
REGISTER_OBJECT(O_DOOR_TYPE_7, M_Setup)
REGISTER_OBJECT(O_DOOR_TYPE_8, M_Setup)
