#include "game/collide.h"
#include "game/control.h"
#include "game/effects.h"
#include "game/game.h"
#include "game/items.h"
#include "game/objects.h"
#include "game/vars.h"
#include "specific/init.h"
#include "util.h"

typedef enum {
    CABIN_START = 0,
    CABIN_DROP1 = 1,
    CABIN_DROP2 = 2,
    CABIN_DROP3 = 3,
    CABIN_FINISH = 4,
} CABIN_ANIM;

typedef enum {
    BOAT_EMPTY = 0,
    BOAT_SET = 1,
    BOAT_MOVE = 2,
    BOAT_STOP = 3,
} BOAT_ANIM;

void ShutThatDoor(DOORPOS_DATA *d)
{
    FLOOR_INFO *floor = d->floor;
    if (!floor) {
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
        Boxes[box_num].overlap_index |= BLOCKED;
    }
}

void OpenThatDoor(DOORPOS_DATA *d)
{
    FLOOR_INFO *floor = d->floor;
    if (!floor) {
        return;
    }

    *floor = d->data;

    int16_t box_num = d->block;
    if (box_num != NO_BOX) {
        Boxes[box_num].overlap_index &= ~BLOCKED;
    }
}

#ifdef T1M_FEAT_CHEATS
void OpenClosestDoors(ITEM_INFO *lara_item)
{
    int16_t best_door = NO_ITEM;
    int32_t best_dist = 0x7FFFFFFF;
    int32_t max_dist = WALL_L * 2;

    int16_t item_num = RoomInfo[lara_item->room_number].item_number;
    while (item_num != NO_ITEM) {
        ITEM_INFO *item = &Items[item_num];
        int32_t x = (item->pos.x - lara_item->pos.x) >> 8;
        int32_t y = (item->pos.y - lara_item->pos.y) >> 8;
        int32_t z = (item->pos.z - lara_item->pos.z) >> 8;
        int32_t dist = SQUARE(x) + SQUARE(y) + SQUARE(z);
        TRACE("found item %d: dist=%d", item_num, dist);
        if (item->object_number >= O_DOOR_TYPE1
            && item->object_number <= O_DOOR_TYPE8) {
            TRACE("found door %d: dist=%d", item_num, dist);
            if (dist < best_dist) {
                best_door = item_num;
                best_dist = dist;
            }
        }
        item_num = item->next_item;
    }

    if (best_door == NO_ITEM || best_dist >= SQUARE(max_dist >> 8)) {
        return;
    }

    ITEM_INFO *item = &Items[best_door];
    if (!item->active) {
        AddActiveItem(best_door);
        item->touch_bits = 0;
        item->flags |= IF_CODE_BITS;
        item->timer = 0;
    }
}
#endif

void InitialiseDoor(int16_t item_num)
{
    ITEM_INFO *item = &Items[item_num];
    DOOR_DATA *door = game_malloc(sizeof(DOOR_DATA), GBUF_EXTRA_DOOR_STUFF);
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

    r = &RoomInfo[item->room_number];
    x_floor = ((item->pos.z - r->z) >> WALL_SHIFT) + dx;
    y_floor = ((item->pos.x - r->x) >> WALL_SHIFT) + dy;
    door->d1.floor = &r->floor[x_floor + y_floor * r->x_size];
    room_num = GetDoor(door->d1.floor);
    if (room_num == NO_ROOM) {
        box_num = door->d1.floor->box;
    } else {
        b = &RoomInfo[room_num];
        x_floor = ((item->pos.z - b->z) >> WALL_SHIFT) + dx;
        y_floor = ((item->pos.x - b->x) >> WALL_SHIFT) + dy;
        box_num = b->floor[x_floor + y_floor * b->x_size].box;
    }
    if (!(Boxes[box_num].overlap_index & BLOCKABLE)) {
        box_num = NO_BOX;
    }
    door->d1.block = box_num;
    door->d1.data = *door->d1.floor;

    if (r->flipped_room != -1) {
        r = &RoomInfo[r->flipped_room];
        x_floor = ((item->pos.z - r->z) >> WALL_SHIFT) + dx;
        y_floor = ((item->pos.x - r->x) >> WALL_SHIFT) + dy;
        door->d1flip.floor = &r->floor[x_floor + y_floor * r->x_size];
        room_num = GetDoor(door->d1flip.floor);
        if (room_num == NO_ROOM) {
            box_num = door->d1flip.floor->box;
        } else {
            b = &RoomInfo[room_num];
            x_floor = ((item->pos.z - b->z) >> WALL_SHIFT) + dx;
            y_floor = ((item->pos.x - b->x) >> WALL_SHIFT) + dy;
            box_num = b->floor[x_floor + y_floor * b->x_size].box;
        }
        if (!(Boxes[box_num].overlap_index & BLOCKABLE)) {
            box_num = NO_BOX;
        }
        door->d1flip.block = box_num;
        door->d1flip.data = *door->d1flip.floor;
    } else {
        door->d1flip.floor = NULL;
    }

    room_num = GetDoor(door->d1.floor);
    ShutThatDoor(&door->d1);
    ShutThatDoor(&door->d1flip);

    if (room_num == NO_ROOM) {
        door->d2.floor = NULL;
        door->d2flip.floor = NULL;
        return;
    }

    r = &RoomInfo[room_num];
    x_floor = (item->pos.z - r->z) >> WALL_SHIFT;
    y_floor = (item->pos.x - r->x) >> WALL_SHIFT;
    door->d2.floor = &r->floor[x_floor + y_floor * r->x_size];
    room_num = GetDoor(door->d2.floor);
    if (room_num == NO_ROOM) {
        box_num = door->d2.floor->box;
    } else {
        b = &RoomInfo[room_num];
        x_floor = (item->pos.z - b->z) >> WALL_SHIFT;
        y_floor = (item->pos.x - b->x) >> WALL_SHIFT;
        box_num = b->floor[x_floor + y_floor * b->x_size].box;
    }
    if (!(Boxes[box_num].overlap_index & BLOCKABLE)) {
        box_num = NO_BOX;
    }
    door->d2.block = box_num;
    door->d2.data = *door->d2.floor;

    if (r->flipped_room != -1) {
        r = &RoomInfo[r->flipped_room];
        x_floor = (item->pos.z - r->z) >> WALL_SHIFT;
        y_floor = (item->pos.x - r->x) >> WALL_SHIFT;
        door->d2flip.floor = &r->floor[x_floor + y_floor * r->x_size];
        room_num = GetDoor(door->d2flip.floor);
        if (room_num == NO_ROOM) {
            box_num = door->d2flip.floor->box;
        } else {
            b = &RoomInfo[room_num];
            x_floor = (item->pos.z - b->z) >> WALL_SHIFT;
            y_floor = (item->pos.x - b->x) >> WALL_SHIFT;
            box_num = b->floor[x_floor + y_floor * b->x_size].box;
        }
        if (!(Boxes[box_num].overlap_index & BLOCKABLE)) {
            box_num = NO_BOX;
        }
        door->d2flip.block = box_num;
        door->d2flip.data = *door->d2flip.floor;
    } else {
        door->d2flip.floor = NULL;
    }

    ShutThatDoor(&door->d2);
    ShutThatDoor(&door->d2flip);
}

void DoorControl(int16_t item_num)
{
    ITEM_INFO *item = &Items[item_num];
    DOOR_DATA *door = item->data;

    if (TriggerActive(item)) {
        if (item->current_anim_state == DOOR_CLOSED) {
            item->goal_anim_state = DOOR_OPEN;
        } else {
            OpenThatDoor(&door->d1);
            OpenThatDoor(&door->d2);
            OpenThatDoor(&door->d1flip);
            OpenThatDoor(&door->d2flip);
        }
    } else {
        if (item->current_anim_state == DOOR_OPEN) {
            item->goal_anim_state = DOOR_CLOSED;
        } else {
            ShutThatDoor(&door->d1);
            ShutThatDoor(&door->d2);
            ShutThatDoor(&door->d1flip);
            ShutThatDoor(&door->d2flip);
        }
    }

    AnimateItem(item);
}

int32_t OnDrawBridge(ITEM_INFO *item, int32_t x, int32_t y)
{
    int32_t ix = item->pos.z >> WALL_SHIFT;
    int32_t iy = item->pos.x >> WALL_SHIFT;

    x >>= WALL_SHIFT;
    y >>= WALL_SHIFT;

    if (item->pos.y_rot == 0 && y == iy && (x == ix - 1 || x == ix - 2)) {
        return 1;
    }
    if (item->pos.y_rot == -PHD_180 && y == iy
        && (x == ix + 1 || x == ix + 2)) {
        return 1;
    }
    if (item->pos.y_rot == PHD_90 && x == ix && (y == iy - 1 || y == iy - 2)) {
        return 1;
    }
    if (item->pos.y_rot == -PHD_90 && x == ix && (y == iy + 1 || y == iy + 2)) {
        return 1;
    }

    return 0;
}

void DrawBridgeFloor(
    ITEM_INFO *item, int32_t x, int32_t y, int32_t z, int16_t *height)
{
    if (item->current_anim_state != DOOR_OPEN) {
        return;
    }
    if (!OnDrawBridge(item, z, x)) {
        return;
    }

    if (y <= item->pos.y) {
        *height = item->pos.y;
    }
}

void DrawBridgeCeiling(
    ITEM_INFO *item, int32_t x, int32_t y, int32_t z, int16_t *height)
{
    if (item->current_anim_state != DOOR_OPEN) {
        return;
    }
    if (!OnDrawBridge(item, z, x)) {
        return;
    }

    if (y > item->pos.y) {
        *height = item->pos.y + STEP_L;
    }
}

void DrawBridgeCollision(
    int16_t item_num, ITEM_INFO *lara_item, COLL_INFO *coll)
{
    ITEM_INFO *item = &Items[item_num];
    if (item->current_anim_state == DOOR_CLOSED) {
        DoorCollision(item_num, lara_item, coll);
    }
}

void BridgeFlatFloor(
    ITEM_INFO *item, int32_t x, int32_t y, int32_t z, int16_t *height)
{
    if (y <= item->pos.y) {
        *height = item->pos.y;
    }
}

void BridgeFlatCeiling(
    ITEM_INFO *item, int32_t x, int32_t y, int32_t z, int16_t *height)
{
    if (y > item->pos.y) {
        *height = item->pos.y + STEP_L;
    }
}

int32_t GetOffset(ITEM_INFO *item, int32_t x, int32_t z)
{
    if (item->pos.y_rot == 0) {
        return (WALL_L - x) & (WALL_L - 1);
    } else if (item->pos.y_rot == -PHD_180) {
        return x & (WALL_L - 1);
    } else if (item->pos.y_rot == PHD_90) {
        return z & (WALL_L - 1);
    } else {
        return (WALL_L - z) & (WALL_L - 1);
    }
}

void BridgeTilt1Floor(
    ITEM_INFO *item, int32_t x, int32_t y, int32_t z, int16_t *height)
{
    int32_t level = item->pos.y + (GetOffset(item, x, z) >> 2);
    if (y <= level) {
        *height = level;
    }
}

void BridgeTilt1Ceiling(
    ITEM_INFO *item, int32_t x, int32_t y, int32_t z, int16_t *height)
{
    int32_t level = item->pos.y + (GetOffset(item, x, z) >> 2);
    if (y > level) {
        *height = level + STEP_L;
    }
}

void BridgeTilt2Floor(
    ITEM_INFO *item, int32_t x, int32_t y, int32_t z, int16_t *height)
{
    int32_t level = item->pos.y + (GetOffset(item, x, z) >> 1);
    if (y <= level) {
        *height = level;
    }
}

void BridgeTilt2Ceiling(
    ITEM_INFO *item, int32_t x, int32_t y, int32_t z, int16_t *height)
{
    int32_t level = item->pos.y + (GetOffset(item, x, z) >> 1);
    if (y > level) {
        *height = level + STEP_L;
    }
}

void CogControl(int16_t item_num)
{
    ITEM_INFO *item = &Items[item_num];
    if (TriggerActive(item)) {
        item->goal_anim_state = DOOR_OPEN;
    } else {
        item->goal_anim_state = DOOR_CLOSED;
    }

    AnimateItem(item);

    int16_t room_num = item->room_number;
    GetFloor(item->pos.x, item->pos.y, item->pos.z, &room_num);
    if (room_num != item->room_number) {
        ItemNewRoom(item_num, room_num);
    }
}

void CabinControl(int16_t item_num)
{
    ITEM_INFO *item = &Items[item_num];

    if ((item->flags & IF_CODE_BITS) == IF_CODE_BITS) {
        switch (item->current_anim_state) {
        case CABIN_START:
            item->goal_anim_state = CABIN_DROP1;
            break;
        case CABIN_DROP1:
            item->goal_anim_state = CABIN_DROP2;
            break;
        case CABIN_DROP2:
            item->goal_anim_state = CABIN_DROP3;
            break;
        }
        item->flags = 0;
    }

    if (item->current_anim_state == CABIN_FINISH) {
        FlipMapTable[3] = IF_CODE_BITS;
        FlipMap();
        KillItem(item_num);
    }

    AnimateItem(item);
}

void BoatControl(int16_t item_num)
{
    ITEM_INFO *item = &Items[item_num];

    switch (item->current_anim_state) {
    case BOAT_SET:
        item->goal_anim_state = BOAT_MOVE;
        break;
    case BOAT_MOVE:
        item->goal_anim_state = BOAT_STOP;
        break;
    case BOAT_STOP:
        KillItem(item_num);
        break;
    }

    AnimateItem(item);
}

void ScionControl(int16_t item_num)
{
    AnimateItem(&Items[item_num]);
}

void Scion3Control(int16_t item_num)
{
    static int32_t counter = 0;
    ITEM_INFO *item = &Items[item_num];

    if (item->hit_points > 0) {
        counter = 0;
        AnimateItem(item);
        return;
    }

    if (counter == 0) {
        item->status = IS_INVISIBLE;
        item->hit_points = DONT_TARGET;
        int16_t room_num = item->room_number;
        FLOOR_INFO *floor =
            GetFloor(item->pos.x, item->pos.y, item->pos.z, &room_num);
        GetHeight(floor, item->pos.x, item->pos.y, item->pos.z);
        TestTriggers(TriggerIndex, 1);
        RemoveDrawnItem(item_num);
    }

    if (counter % 10 == 0) {
        int16_t fx_num = CreateEffect(item->room_number);
        if (fx_num != NO_ITEM) {
            FX_INFO *fx = &Effects[fx_num];
            fx->pos.x = item->pos.x + (GetRandomControl() - 0x4000) / 32;
            fx->pos.y = item->pos.y + (GetRandomControl() - 0x4000) / 256 - 500;
            fx->pos.z = item->pos.z + (GetRandomControl() - 0x4000) / 32;
            fx->speed = 0;
            fx->frame_number = 0;
            fx->object_number = O_EXPLOSION1;
            fx->counter = 0;
            SoundEffect(104, &fx->pos, 0);
            Camera.bounce = -200;
        }
    }

    counter++;
    if (counter == 30 * 3) {
        RemoveActiveItem(item_num);
    }
}

// original name: EarthQuake
void EarthQuakeControl(int16_t item_num)
{
    ITEM_INFO *item = &Items[item_num];
    if (TriggerActive(item)) {
        if (GetRandomDraw() < 0x100) {
            Camera.bounce = -150;
            SoundEffect(147, NULL, 0);
        } else if (GetRandomControl() < 0x400) {
            Camera.bounce = 50;
            SoundEffect(70, NULL, 0);
        }
    }
}

void T1MInjectGameObjects()
{
    INJECT(0x0042CA40, InitialiseDoor);
    INJECT(0x0042CEF0, DoorControl);
    INJECT(0x0042D130, OnDrawBridge);
    INJECT(0x0042D1F0, DrawBridgeFloor);
    INJECT(0x0042D230, DrawBridgeCeiling);
    INJECT(0x0042D270, DrawBridgeCollision);
    INJECT(0x0042D2A0, BridgeFlatFloor);
    INJECT(0x0042D2C0, BridgeFlatCeiling);
    INJECT(0x0042D2E0, BridgeTilt1Floor);
    INJECT(0x0042D330, BridgeTilt1Ceiling);
    INJECT(0x0042D380, BridgeTilt2Floor);
    INJECT(0x0042D3D0, BridgeTilt2Ceiling);
    INJECT(0x0042D420, CogControl);
    INJECT(0x0042D4A0, CabinControl);
    INJECT(0x0042D520, BoatControl);
    INJECT(0x0042D580, Scion3Control);
    INJECT(0x0042D700, EarthQuakeControl);
}
