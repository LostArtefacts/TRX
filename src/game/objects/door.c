#include "game/objects/door.h"

#include "game/control.h"
#include "game/draw.h"
#include "game/gamebuf.h"
#include "game/items.h"
#include "game/lara/lara.h"
#include "game/room.h"
#include "game/sphere.h"
#include "global/const.h"
#include "global/vars.h"
#include "util.h"

#include <stdbool.h>
#include <stddef.h>

static void Door_Open(DOORPOS_DATA *d);
static void Door_Shut(DOORPOS_DATA *d, ITEM_INFO *item);
static bool Door_LaraDoorCollision(ITEM_INFO *item);

static bool Door_LaraDoorCollision(ITEM_INFO *item)
{
    if (!g_LaraItem) {
        return false;
    }
    int32_t max_dist = SQUARE((WALL_L * 2) >> 8);
    int32_t dx = ABS(item->pos.x - g_LaraItem->pos.x) >> 8;
    int32_t dy = ABS(item->pos.y - g_LaraItem->pos.y) >> 8;
    int32_t dz = ABS(item->pos.z - g_LaraItem->pos.z) >> 8;
    int32_t dist = SQUARE(dx) + SQUARE(dy) + SQUARE(dz);
    return dist < max_dist;
}

static void Door_Shut(DOORPOS_DATA *d, ITEM_INFO *item)
{
    FLOOR_INFO *floor = d->floor;
    if (!floor) {
        return;
    }

    if (item && Door_LaraDoorCollision(item)) {
        return;
    }

    floor->index = 0;
    floor->box = NO_BOX;
    floor->floor = NO_HEIGHT / 256;
    floor->ceiling = NO_HEIGHT / 256;
    floor->sky_room = NO_ROOM;
    floor->pit_room = NO_ROOM;

    int16_t box_num = d->block;
    if (box_num != NO_BOX) {
        g_Boxes[box_num].overlap_index |= BLOCKED;
    }
}

static void Door_Open(DOORPOS_DATA *d)
{
    FLOOR_INFO *floor = d->floor;
    if (!floor) {
        return;
    }

    *floor = d->old_floor;

    int16_t box_num = d->block;
    if (box_num != NO_BOX) {
        g_Boxes[box_num].overlap_index &= ~BLOCKED;
    }
}

void Door_Setup(OBJECT_INFO *obj)
{
    obj->initialise = Door_Initialise;
    obj->control = Door_Control;
    obj->draw_routine = DrawUnclippedItem;
    obj->collision = Door_Collision;
    obj->save_anim = 1;
    obj->save_flags = 1;
}

void Door_Initialise(int16_t item_num)
{
    ITEM_INFO *item = &g_Items[item_num];
    DOOR_DATA *door = GameBuf_Alloc(sizeof(DOOR_DATA), GBUF_EXTRA_DOOR_STUFF);
    item->data = door;

    int32_t dx = 0;
    int32_t dy = 0;
    if (item->pos.y_rot == 0) {
        dx--;
    } else if (item->pos.y_rot == -PHD_180) {
        dx++;
    } else if (item->pos.y_rot == PHD_90) {
        dy--;
    } else {
        dy++;
    }

    ROOM_INFO *r;
    ROOM_INFO *b;
    int32_t x_floor;
    int32_t y_floor;
    int16_t room_num;
    int16_t box_num;

    r = &g_RoomInfo[item->room_number];
    x_floor = ((item->pos.z - r->z) >> WALL_SHIFT) + dx;
    y_floor = ((item->pos.x - r->x) >> WALL_SHIFT) + dy;
    door->d1.floor = &r->floor[x_floor + y_floor * r->x_size];
    room_num = Room_GetDoor(door->d1.floor);
    if (room_num == NO_ROOM) {
        box_num = door->d1.floor->box;
    } else {
        b = &g_RoomInfo[room_num];
        x_floor = ((item->pos.z - b->z) >> WALL_SHIFT) + dx;
        y_floor = ((item->pos.x - b->x) >> WALL_SHIFT) + dy;
        box_num = b->floor[x_floor + y_floor * b->x_size].box;
    }
    if (!(g_Boxes[box_num].overlap_index & BLOCKABLE)) {
        box_num = NO_BOX;
    }
    door->d1.block = box_num;
    door->d1.old_floor = *door->d1.floor;

    if (r->flipped_room != -1) {
        r = &g_RoomInfo[r->flipped_room];
        x_floor = ((item->pos.z - r->z) >> WALL_SHIFT) + dx;
        y_floor = ((item->pos.x - r->x) >> WALL_SHIFT) + dy;
        door->d1flip.floor = &r->floor[x_floor + y_floor * r->x_size];
        room_num = Room_GetDoor(door->d1flip.floor);
        if (room_num == NO_ROOM) {
            box_num = door->d1flip.floor->box;
        } else {
            b = &g_RoomInfo[room_num];
            x_floor = ((item->pos.z - b->z) >> WALL_SHIFT) + dx;
            y_floor = ((item->pos.x - b->x) >> WALL_SHIFT) + dy;
            box_num = b->floor[x_floor + y_floor * b->x_size].box;
        }
        if (!(g_Boxes[box_num].overlap_index & BLOCKABLE)) {
            box_num = NO_BOX;
        }
        door->d1flip.block = box_num;
        door->d1flip.old_floor = *door->d1flip.floor;
    } else {
        door->d1flip.floor = NULL;
    }

    room_num = Room_GetDoor(door->d1.floor);
    Door_Shut(&door->d1, NULL);
    Door_Shut(&door->d1flip, NULL);

    if (room_num == NO_ROOM) {
        door->d2.floor = NULL;
        door->d2flip.floor = NULL;
        return;
    }

    r = &g_RoomInfo[room_num];
    x_floor = (item->pos.z - r->z) >> WALL_SHIFT;
    y_floor = (item->pos.x - r->x) >> WALL_SHIFT;
    door->d2.floor = &r->floor[x_floor + y_floor * r->x_size];
    room_num = Room_GetDoor(door->d2.floor);
    if (room_num == NO_ROOM) {
        box_num = door->d2.floor->box;
    } else {
        b = &g_RoomInfo[room_num];
        x_floor = (item->pos.z - b->z) >> WALL_SHIFT;
        y_floor = (item->pos.x - b->x) >> WALL_SHIFT;
        box_num = b->floor[x_floor + y_floor * b->x_size].box;
    }
    if (!(g_Boxes[box_num].overlap_index & BLOCKABLE)) {
        box_num = NO_BOX;
    }
    door->d2.block = box_num;
    door->d2.old_floor = *door->d2.floor;

    if (r->flipped_room != -1) {
        r = &g_RoomInfo[r->flipped_room];
        x_floor = (item->pos.z - r->z) >> WALL_SHIFT;
        y_floor = (item->pos.x - r->x) >> WALL_SHIFT;
        door->d2flip.floor = &r->floor[x_floor + y_floor * r->x_size];
        room_num = Room_GetDoor(door->d2flip.floor);
        if (room_num == NO_ROOM) {
            box_num = door->d2flip.floor->box;
        } else {
            b = &g_RoomInfo[room_num];
            x_floor = (item->pos.z - b->z) >> WALL_SHIFT;
            y_floor = (item->pos.x - b->x) >> WALL_SHIFT;
            box_num = b->floor[x_floor + y_floor * b->x_size].box;
        }
        if (!(g_Boxes[box_num].overlap_index & BLOCKABLE)) {
            box_num = NO_BOX;
        }
        door->d2flip.block = box_num;
        door->d2flip.old_floor = *door->d2flip.floor;
    } else {
        door->d2flip.floor = NULL;
    }

    Door_Shut(&door->d2, NULL);
    Door_Shut(&door->d2flip, NULL);
}

void Door_Control(int16_t item_num)
{
    ITEM_INFO *item = &g_Items[item_num];
    DOOR_DATA *door = item->data;

    if (TriggerActive(item)) {
        if (item->current_anim_state == DOOR_CLOSED) {
            item->goal_anim_state = DOOR_OPEN;
        } else {
            Door_Open(&door->d1);
            Door_Open(&door->d2);
            Door_Open(&door->d1flip);
            Door_Open(&door->d2flip);
        }
    } else {
        if (item->current_anim_state == DOOR_OPEN) {
            item->goal_anim_state = DOOR_CLOSED;
        } else {
            Door_Shut(&door->d1, item);
            Door_Shut(&door->d2, item);
            Door_Shut(&door->d1flip, item);
            Door_Shut(&door->d2flip, item);
        }
    }

    AnimateItem(item);
}

void Door_Collision(int16_t item_num, ITEM_INFO *lara_item, COLL_INFO *coll)
{
    ITEM_INFO *item = &g_Items[item_num];

    if (!Lara_TestBoundsCollide(item, coll->radius)) {
        return;
    }
    if (!TestCollision(item, lara_item)) {
        return;
    }

    if (coll->enable_baddie_push) {
        if (item->current_anim_state != item->goal_anim_state) {
            Lara_Push(item, coll, coll->enable_spaz, true);
        } else {
            Lara_Push(item, coll, false, true);
        }
    }
}

void Door_OpenNearest(ITEM_INFO *lara_item)
{
    int32_t max_dist = SQUARE((WALL_L * 2) >> 8);

    for (int item_num = 0; item_num < g_LevelItemCount; item_num++) {
        ITEM_INFO *item = &g_Items[item_num];
        int32_t dx = (item->pos.x - lara_item->pos.x) >> 8;
        int32_t dy = (item->pos.y - lara_item->pos.y) >> 8;
        int32_t dz = (item->pos.z - lara_item->pos.z) >> 8;
        int32_t dist = SQUARE(dx) + SQUARE(dy) + SQUARE(dz);

        if (dist > max_dist) {
            continue;
        }

        if ((item->object_number < O_DOOR_TYPE1
             || item->object_number > O_DOOR_TYPE8)
            && item->object_number != O_TRAPDOOR
            && item->object_number != O_BIGTRAPDOOR
            && item->object_number != O_TRAPDOOR2) {
            continue;
        }

        if (!item->active) {
            Item_AddActive(item_num);
            item->flags |= IF_CODE_BITS;
        } else if (item->flags & IF_CODE_BITS) {
            item->flags &= ~IF_CODE_BITS;
        } else {
            item->flags |= IF_CODE_BITS;
        }
        item->timer = 0;
        item->touch_bits = 0;
    }
}
