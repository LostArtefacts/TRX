#include "game/traps/movable_block.h"

#include "game/collide.h"
#include "game/control.h"
#include "game/draw.h"
#include "game/effects/dino_stomp.h"
#include "game/input.h"
#include "game/items.h"
#include "game/lara.h"
#include "game/sound.h"
#include "global/vars.h"
#include "src/game/collide.h"
#include "src/game/sphere.h"

static bool TestDoorCoveringBlock(ITEM_INFO *lara_item, COLL_INFO *coll);

static bool TestDoorCoveringBlock(ITEM_INFO *lara_item, COLL_INFO *coll)
{
    // OG fix: stop pushing blocks through doors
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
             || item->object_number > O_DOOR_TYPE8)) {
            continue;
        }

        if (TestBoundsCollide(item, lara_item, coll->radius)
            && TestCollision(item, lara_item)) {
            return true;
        }
    }
    return false;
}

void SetupMovableBlock(OBJECT_INFO *obj)
{
    obj->initialise = InitialiseMovableBlock;
    obj->control = MovableBlockControl;
    obj->draw_routine = DrawMovableBlock;
    obj->collision = MovableBlockCollision;
    obj->save_position = 1;
    obj->save_anim = 1;
    obj->save_flags = 1;
}

void InitialiseMovableBlock(int16_t item_num)
{
    ITEM_INFO *item = &g_Items[item_num];
    if (item->status != IS_INVISIBLE) {
        AlterFloorHeight(item, -WALL_L);
    }
}

void MovableBlockControl(int16_t item_num)
{
    ITEM_INFO *item = &g_Items[item_num];

    if (item->flags & IF_ONESHOT) {
        AlterFloorHeight(item, WALL_L);
        KillItem(item_num);
        return;
    }

    AnimateItem(item);

    int16_t room_num = item->room_number;
    FLOOR_INFO *floor =
        GetFloor(item->pos.x, item->pos.y, item->pos.z, &room_num);
    int32_t height = GetHeight(floor, item->pos.x, item->pos.y, item->pos.z);

    if (item->pos.y < height) {
        item->gravity_status = 1;
    } else if (item->gravity_status) {
        item->gravity_status = 0;
        item->pos.y = height;
        item->status = IS_DEACTIVATED;
        DinoStomp(item);
        Sound_Effect(SFX_T_REX_FOOTSTOMP, &item->pos, SPM_NORMAL);
    }

    if (item->room_number != room_num) {
        ItemNewRoom(item_num, room_num);
    }

    if (item->status == IS_DEACTIVATED) {
        item->status = IS_NOT_ACTIVE;
        RemoveActiveItem(item_num);
        AlterFloorHeight(item, -WALL_L);

        room_num = item->room_number;
        floor = GetFloor(item->pos.x, item->pos.y, item->pos.z, &room_num);
        GetHeight(floor, item->pos.x, item->pos.y, item->pos.z);
        TestTriggers(g_TriggerIndex, 1);
    }
}

void MovableBlockCollision(
    int16_t item_num, ITEM_INFO *lara_item, COLL_INFO *coll)
{
    ITEM_INFO *item = &g_Items[item_num];

    if (!g_Input.action || item->status == IS_ACTIVE
        || lara_item->gravity_status || lara_item->pos.y != item->pos.y) {
        return;
    }

    uint16_t quadrant = ((uint16_t)lara_item->pos.y_rot + PHD_45) / PHD_90;
    if (lara_item->current_anim_state == AS_STOP) {
        if (g_Input.forward || g_Input.back
            || g_Lara.gun_status != LGS_ARMLESS) {
            return;
        }

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

        if (!TestLaraPosition(g_MovingBlockBounds, item, lara_item)) {
            return;
        }

        // OG fix: stop pushing blocks through doors
        if (TestDoorCoveringBlock(lara_item, coll)) {
            return;
        }

        switch (quadrant) {
        case DIR_NORTH:
            lara_item->pos.z &= -WALL_L;
            lara_item->pos.z += WALL_L - LARA_RAD;
            break;
        case DIR_SOUTH:
            lara_item->pos.z &= -WALL_L;
            lara_item->pos.z += LARA_RAD;
            break;
        case DIR_EAST:
            lara_item->pos.x &= -WALL_L;
            lara_item->pos.x += WALL_L - LARA_RAD;
            break;
        case DIR_WEST:
            lara_item->pos.x &= -WALL_L;
            lara_item->pos.x += LARA_RAD;
            break;
        }

        lara_item->pos.y_rot = item->pos.y_rot;
        lara_item->goal_anim_state = AS_PPREADY;

        AnimateLara(lara_item);

        if (lara_item->current_anim_state == AS_PPREADY) {
            g_Lara.gun_status = LGS_HANDSBUSY;
        }
    } else if (lara_item->current_anim_state == AS_PPREADY) {
        if (lara_item->frame_number != AF_PPREADY) {
            return;
        }

        if (!TestLaraPosition(g_MovingBlockBounds, item, lara_item)) {
            return;
        }

        if (g_Input.forward) {
            if (!TestBlockPush(item, 1024, quadrant)) {
                return;
            }
            item->goal_anim_state = MBS_PUSH;
            lara_item->goal_anim_state = AS_PUSHBLOCK;
        } else if (g_Input.back) {
            if (!TestBlockPull(item, 1024, quadrant)) {
                return;
            }
            item->goal_anim_state = MBS_PULL;
            lara_item->goal_anim_state = AS_PULLBLOCK;
        } else {
            return;
        }

        AddActiveItem(item_num);
        AlterFloorHeight(item, WALL_L);
        item->status = IS_ACTIVE;
        AnimateItem(item);
        AnimateLara(lara_item);
    }
}

void DrawMovableBlock(ITEM_INFO *item)
{
    if (item->status == IS_ACTIVE) {
        DrawUnclippedItem(item);
    } else {
        DrawAnimatingItem(item);
    }
}

int32_t TestBlockMovable(ITEM_INFO *item, int32_t blockhite)
{
    int16_t room_num = item->room_number;
    FLOOR_INFO *floor =
        GetFloor(item->pos.x, item->pos.y, item->pos.z, &room_num);
    if (floor->floor == NO_HEIGHT / 256) {
        return 1;
    }

    if ((floor->floor << 8) != item->pos.y - blockhite) {
        return 0;
    }

    return 1;
}

int32_t TestBlockPush(ITEM_INFO *item, int32_t blockhite, uint16_t quadrant)
{
    if (!TestBlockMovable(item, blockhite)) {
        return 0;
    }

    int32_t x = item->pos.x;
    int32_t y = item->pos.y;
    int32_t z = item->pos.z;
    int16_t room_num = item->room_number;

    switch (quadrant) {
    case DIR_NORTH:
        z += WALL_L;
        break;
    case DIR_EAST:
        x += WALL_L;
        break;
    case DIR_SOUTH:
        z -= WALL_L;
        break;
    case DIR_WEST:
        x -= WALL_L;
        break;
    }

    FLOOR_INFO *floor = GetFloor(x, y, z, &room_num);
    COLL_INFO coll;
    coll.quadrant = quadrant;
    coll.radius = 500;
    if (CollideStaticObjects(&coll, x, y, z, room_num, 1000)) {
        return 0;
    }

    if (((int32_t)floor->floor << 8) != y) {
        return 0;
    }

    floor = GetFloor(x, y - blockhite, z, &room_num);
    if (((int32_t)floor->ceiling << 8) > y - blockhite) {
        return 0;
    }

    return 1;
}

int32_t TestBlockPull(ITEM_INFO *item, int32_t blockhite, uint16_t quadrant)
{
    if (!TestBlockMovable(item, blockhite)) {
        return 0;
    }

    int32_t x_add = 0;
    int32_t z_add = 0;
    switch (quadrant) {
    case DIR_NORTH:
        z_add = -WALL_L;
        break;
    case DIR_EAST:
        x_add = -WALL_L;
        break;
    case DIR_SOUTH:
        z_add = WALL_L;
        break;
    case DIR_WEST:
        x_add = WALL_L;
        break;
    }

    int32_t x = item->pos.x + x_add;
    int32_t y = item->pos.y;
    int32_t z = item->pos.z + z_add;

    int16_t room_num = item->room_number;
    FLOOR_INFO *floor = GetFloor(x, y, z, &room_num);
    COLL_INFO coll;
    coll.quadrant = quadrant;
    coll.radius = 500;
    if (CollideStaticObjects(&coll, x, y, z, room_num, 1000)) {
        return 0;
    }

    if (((int32_t)floor->floor << 8) != y) {
        return 0;
    }

    floor = GetFloor(x, y - blockhite, z, &room_num);
    if (((int32_t)floor->ceiling << 8) > y - blockhite) {
        return 0;
    }

    x += x_add;
    z += z_add;
    room_num = item->room_number;
    floor = GetFloor(x, y, z, &room_num);

    if (((int32_t)floor->floor << 8) != y) {
        return 0;
    }

    floor = GetFloor(x, y - LARA_HITE, z, &room_num);
    if (((int32_t)floor->ceiling << 8) > y - LARA_HITE) {
        return 0;
    }

    x = g_LaraItem->pos.x + x_add;
    y = g_LaraItem->pos.y;
    z = g_LaraItem->pos.z + z_add;
    room_num = g_LaraItem->room_number;
    floor = GetFloor(x, y, z, &room_num);
    coll.radius = LARA_RAD;
    coll.quadrant = (quadrant + 2) & 3;
    if (CollideStaticObjects(&coll, x, y, z, room_num, LARA_HITE)) {
        return 0;
    }

    return 1;
}

void AlterFloorHeight(ITEM_INFO *item, int32_t height)
{
    int16_t room_num = item->room_number;
    FLOOR_INFO *floor =
        GetFloor(item->pos.x, item->pos.y, item->pos.z, &room_num);
    FLOOR_INFO *ceiling = GetFloor(
        item->pos.x, item->pos.y + height - WALL_L, item->pos.z, &room_num);

    if (floor->floor == NO_HEIGHT / 256) {
        floor->floor = ceiling->ceiling + height / 256;
    } else {
        floor->floor += height / 256;
        if (floor->floor == ceiling->ceiling) {
            floor->floor = NO_HEIGHT / 256;
        }
    }

    if (g_Boxes[floor->box].overlap_index & BLOCKABLE) {
        if (height < 0) {
            g_Boxes[floor->box].overlap_index |= BLOCKED;
        } else {
            g_Boxes[floor->box].overlap_index &= ~BLOCKED;
        }
    }
}
