#include "game/control.h"
#include "game/effects.h"
#include "game/items.h"
#include "game/moveblock.h"
#include "game/vars.h"
#include "game/collide.h"
#include "game/lara.h"
#include "util.h"

#define MB_MAXOFF 300
#define MB_MAXOFF_Z (LARA_RAD + 80) // = 180

static int16_t MovingBlockBounds[12] = {
    -MB_MAXOFF,
    +MB_MAXOFF,
    0,
    0,
    -WALL_L / 2 - MB_MAXOFF_Z,
    -WALL_L / 2,
    -10 * PHD_DEGREE,
    +10 * PHD_DEGREE,
    -30 * PHD_DEGREE,
    +30 * PHD_DEGREE,
    -10 * PHD_DEGREE,
    +10 * PHD_DEGREE,
};

typedef enum {
    MBS_STILL = 1,
    MBS_PUSH = 2,
    MBS_PULL = 3,
} MOVABLE_BLOCK_STATES;

typedef enum {
    RBS_START = 0,
    RBS_END = 1,
    RBS_MOVING = 2,
} ROLLING_BLOCK_STATES;

// original name: InitialiseMovingBlock
void InitialiseMovableBlock(int16_t item_num)
{
    ITEM_INFO* item = &Items[item_num];
    if (item->status != IS_INVISIBLE) {
        AlterFloorHeight(item, -1024);
    }
}

// original name: MovableBlock
void MovableBlockControl(int16_t item_num)
{
    ITEM_INFO* item = &Items[item_num];

    if (item->flags & IF_ONESHOT) {
        AlterFloorHeight(item, 1024);
        KillItem(item_num);
        return;
    }

    AnimateItem(item);

    int16_t room_num = item->room_number;
    FLOOR_INFO* floor =
        GetFloor(item->pos.x, item->pos.y, item->pos.z, &room_num);
    int32_t height = GetHeight(floor, item->pos.x, item->pos.y, item->pos.z);

    if (item->pos.y < height) {
        item->gravity_status = 1;
    } else if (item->gravity_status) {
        item->gravity_status = 0;
        item->pos.y = height;
        item->status = IS_DEACTIVATED;
        FxDinoStomp(item);
        SoundEffect(70, &item->pos, 0);
    }

    if (item->room_number != room_num) {
        ItemNewRoom(item_num, room_num);
    }

    if (item->status == IS_DEACTIVATED) {
        item->status = IS_NOT_ACTIVE;
        RemoveActiveItem(item_num);
        AlterFloorHeight(item, -1024);

        room_num = item->room_number;
        floor = GetFloor(item->pos.x, item->pos.y, item->pos.z, &room_num);
        GetHeight(floor, item->pos.x, item->pos.y, item->pos.z);
        TestTriggers(TriggerIndex, 1);
    }
}

void MovableBlockCollision(
    int16_t item_num, ITEM_INFO* lara_item, COLL_INFO* coll)
{
    ITEM_INFO* item = &Items[item_num];

    if (!CHK_ANY(Input, IN_ACTION) || item->status == IS_ACTIVE
        || lara_item->gravity_status || lara_item->pos.y != item->pos.y) {
        return;
    }

    uint16_t quadrant = ((uint16_t)lara_item->pos.y_rot + PHD_45) / PHD_90;
    if (lara_item->current_anim_state == AS_STOP) {
        if (CHK_ANY(Input, IN_FORWARD | IN_BACK)
            || Lara.gun_status != LGS_ARMLESS) {
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

        if (!TestLaraPosition(MovingBlockBounds, item, lara_item)) {
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
            Lara.gun_status = LGS_HANDSBUSY;
        }
    } else if (lara_item->current_anim_state == AS_PPREADY) {
        if (lara_item->frame_number != AF_PPREADY) {
            return;
        }

        if (!TestLaraPosition(MovingBlockBounds, item, lara_item)) {
            return;
        }

        if (CHK_ANY(Input, IN_FORWARD)) {
            if (!TestBlockPush(item, 1024, quadrant)) {
                return;
            }
            item->goal_anim_state = MBS_PUSH;
            lara_item->goal_anim_state = AS_PUSHBLOCK;
        } else if (CHK_ANY(Input, IN_BACK)) {
            if (!TestBlockPull(item, 1024, quadrant)) {
                return;
            }
            item->goal_anim_state = MBS_PULL;
            lara_item->goal_anim_state = AS_PULLBLOCK;
        } else {
            return;
        }

        AddActiveItem(item_num);
        AlterFloorHeight(item, 1024);
        item->status = IS_ACTIVE;
        AnimateItem(item);
        AnimateLara(lara_item);
    }
}

int32_t TestBlockMovable(ITEM_INFO* item, int32_t blockhite)
{
    int16_t room_num = item->room_number;
    FLOOR_INFO* floor =
        GetFloor(item->pos.x, item->pos.y, item->pos.z, &room_num);
    if (floor->floor == NO_HEIGHT / 256) {
        return 1;
    }

    if ((floor->floor << 8) != item->pos.y - blockhite) {
        return 0;
    }

    return 1;
}

int32_t TestBlockPush(ITEM_INFO* item, int32_t blockhite, uint16_t quadrant)
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

    FLOOR_INFO* floor = GetFloor(x, y, z, &room_num);
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

int32_t TestBlockPull(ITEM_INFO* item, int32_t blockhite, uint16_t quadrant)
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
    FLOOR_INFO* floor = GetFloor(x, y, z, &room_num);
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

    x = LaraItem->pos.x + x_add;
    y = LaraItem->pos.y;
    z = LaraItem->pos.z + z_add;
    room_num = LaraItem->room_number;
    floor = GetFloor(x, y, z, &room_num);
    coll.radius = LARA_RAD;
    coll.quadrant = (quadrant + 2) & 3;
    if (CollideStaticObjects(&coll, x, y, z, room_num, LARA_HITE)) {
        return 0;
    }

    return 1;
}

void InitialiseRollingBlock(int16_t item_num)
{
    ITEM_INFO* item = &Items[item_num];
    AlterFloorHeight(item, -2048);
}

// original name: RollingBlock
void RollingBlockControl(int16_t item_num)
{
    ITEM_INFO* item = &Items[item_num];
    if (TriggerActive(item)) {
        if (item->current_anim_state == RBS_START) {
            item->goal_anim_state = RBS_END;
            AlterFloorHeight(item, 2048);
        }
    } else if (item->current_anim_state == RBS_END) {
        item->goal_anim_state = RBS_START;
        AlterFloorHeight(item, 2048);
    }

    AnimateItem(item);

    int16_t room_num = item->room_number;
    GetFloor(item->pos.x, item->pos.y, item->pos.z, &room_num);
    if (item->room_number != room_num) {
        ItemNewRoom(item_num, room_num);
    }

    if (item->status == IS_DEACTIVATED) {
        item->status = IS_ACTIVE;
        AlterFloorHeight(item, -2048);
        item->pos.x &= -WALL_L;
        item->pos.x += WALL_L / 2;
        item->pos.z &= -WALL_L;
        item->pos.z += WALL_L / 2;
    }
}

void T1MInjectGameMoveBlock()
{
    INJECT(0x0042B430, InitialiseMovableBlock);
    INJECT(0x0042B460, MovableBlockControl);
    INJECT(0x0042B5B0, MovableBlockCollision);
    INJECT(0x0042B7E0, TestBlockPush);
    INJECT(0x0042B940, TestBlockPull);
    INJECT(0x0042BB90, InitialiseRollingBlock);
    INJECT(0x0042BBC0, RollingBlockControl);
}
