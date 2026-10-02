#include <trx/config.h>
#include <trx/core/json/util/read_io.h>
#include <trx/core/json/util/write_io.h>
#include <trx/game/camera.h>
#include <trx/game/lara.h>
#include <trx/game/random.h>
#include <trx/game/rooms.h>
#include <trx/game/sound.h>

// clang-format off
#define M_SHAKE_RANGE (WALL_L * 16) // = 16384
#define M_HALF_BLOCK  (STEP_L * 2) // = 512
#define M_HALF_CLICK  (STEP_L / 2) // = 128
#define M_TURN_RATE   (DEG_45 / 16) // = 512
#define M_MAX_VEL     (WALL_L * 3) // = 3072
#define M_MIN_VEL     (-M_MAX_VEL) // = -3072
// clang-format on

typedef struct {
    struct {
        int32_t front;
        int32_t back;
        int32_t right;
        int32_t left;
    } near, far;
} M_TERRAIN;

typedef struct {
    XZ_16 velocity;
} M_PRIV;

static RESULT M_LoadPriv(ITEM *const item, JSON_READ_IO *const io)
{
    M_PRIV *const p = item->priv;
    MUST(JSON_READ_OPT(io, "velocity.x", &p->velocity.x));
    MUST(JSON_READ_OPT(io, "velocity.z", &p->velocity.z));
    return OK;
}

static void M_SavePriv(const ITEM *const item, JSON_WRITE_IO *const io)
{
    const M_PRIV *const p = item->priv;
    JSONW_WRITE(io, "velocity.x", p->velocity.x);
    JSONW_WRITE(io, "velocity.z", p->velocity.z);
}

static void M_Collision(
    const int16_t item_num, ITEM *const lara_item, COLL_INFO *const coll)
{
    ITEM *const item = Item_Get(item_num);
    if (!Lara_TestBoundsCollide(item, coll->radius)
        || !Collide_TestCollision(item, lara_item)) {
        return;
    }

    const M_PRIV *const p = item->priv;
    if (g_Config.debug.enable_invulnerability || !Item_IsTriggerActive(item)
        || (p->velocity.x == 0 && item->fall_speed == 0)) {
        Object_Collision(item_num, lara_item, coll);
        return;
    }

    Item_SwitchToAnim(lara_item, LA(LA_BOULDER_DEATH), 0);
    lara_item->current_anim_state = LS(LS_DEATH);
    lara_item->goal_anim_state = LS(LS_DEATH);
    lara_item->gravity = false;
}

static int32_t M_GetHeight(
    const int32_t x, const int32_t y, const int32_t z, int16_t *const room_num)
{
    const XYZ_32 pos = { .x = x, .y = y, .z = z };
    const SECTOR *const sector = Room_GetSector(pos, room_num);
    const int32_t height = Room_GetHeight(sector, pos);
    return height == NO_HEIGHT ? NO_HEIGHT : (height - M_HALF_BLOCK);
}

static M_TERRAIN M_GetTerrain(const ITEM *const item, int16_t *const room)
{
    const int32_t x = item->pos.x;
    const int32_t y = item->pos.y;
    const int32_t z = item->pos.z;
    return (M_TERRAIN) {
        // clang-format off
        .near.front = M_GetHeight(x,                y, z + M_HALF_CLICK, room),
        .near.back  = M_GetHeight(x,                y, z - M_HALF_CLICK, room),
        .near.right = M_GetHeight(x + M_HALF_CLICK, y, z,                room),
        .near.left  = M_GetHeight(x - M_HALF_CLICK, y, z,                room),
        .far.front  = M_GetHeight(x,                y, z + M_HALF_BLOCK, room),
        .far.back   = M_GetHeight(x,                y, z - M_HALF_BLOCK, room),
        .far.right  = M_GetHeight(x + M_HALF_BLOCK, y, z,                room),
        .far.left   = M_GetHeight(x - M_HALF_BLOCK, y, z,                room),
        // clang-format on
    };
}

static void M_DoEffects(const ITEM *const item)
{
    const int32_t fall_speed = ABS(item->fall_speed);
    if (fall_speed <= 16) {
        return;
    }

    Sound_Effect(SFX_ROLLING_BALL_FALL, &item->pos, SPM_NORMAL);

    if (!g_Config.gameplay.enable_boulder_shake) {
        return;
    }

    const int32_t dist = XYZ_32_GetDistance(g_Camera.pos.pos, item->pos);
    if (dist < M_SHAKE_RANGE) {
        g_Camera.bounce = -(((M_SHAKE_RANGE - dist) * fall_speed) >> W2V_SHIFT);
    }
}

static void M_Bounce(ITEM *const item, const int32_t height)
{
    M_DoEffects(item);

    if (item->pos.y - height < M_HALF_BLOCK) {
        item->pos.y = height;
    }

    if (item->fall_speed > 64) {
        item->fall_speed = -(item->fall_speed >> 2);
    } else if (
        ABS(item->speed) <= M_HALF_BLOCK || (Random_GetControl() & 0x1F) != 0) {
        item->fall_speed = 0;
    } else {
        item->fall_speed = -(Random_GetControl() % (item->speed >> 3));
    }
}

static void M_DampenVelocity(int16_t *const velocity)
{
    if (ABS(*velocity) <= 64) {
        *velocity = 0;
    } else {
        *velocity -= *velocity >> 6;
    }
}

static void M_UpdateVelocityZ(
    ITEM *const item, const int32_t height, const M_TERRAIN *const terrain)
{
    M_PRIV *const p = item->priv;
    int16_t flat_count = 0;

    if (terrain->near.front - height <= STEP_L) {
        if (terrain->far.front - height < -WALL_L
            || terrain->near.front - height < -STEP_L) {
            if (p->velocity.z <= 0) {
                if (p->velocity.z == 0 && p->velocity.x != 0) {
                    item->pos.z = ROUND_TO_SECTOR_MID(item->pos.z);
                }
            } else {
                p->velocity.z = -p->velocity.z >> 1;
                item->pos.z = ROUND_TO_SECTOR_MID(item->pos.z);
            }
        } else if (terrain->near.front == height) {
            flat_count++;
        } else {
            p->velocity.z += (terrain->near.front - height) >> 1;
        }
    }

    if (terrain->near.back - height > STEP_L) {
        flat_count++;
    } else if (
        terrain->far.back - height < -WALL_L
        || terrain->near.back - height < -STEP_L) {
        if (p->velocity.z >= 0) {
            if (p->velocity.z == 0 && p->velocity.x != 0) {
                item->pos.z = ROUND_TO_SECTOR_MID(item->pos.z);
            }
        } else {
            p->velocity.z = -p->velocity.z >> 1;
            item->pos.z = ROUND_TO_SECTOR_MID(item->pos.z);
        }
    } else if (terrain->near.back == height) {
        flat_count++;
    } else {
        p->velocity.z -= (terrain->near.back - height) >> 1;
    }

    if (flat_count == 2) {
        M_DampenVelocity(&p->velocity.z);
    }
}

static void M_UpdateVelocityX(
    ITEM *const item, const int32_t height, const M_TERRAIN *const terrain)
{
    M_PRIV *const p = item->priv;
    int16_t flat_count = 0;

    if (terrain->near.left - height <= STEP_L) {
        if (terrain->far.left - height < -WALL_L
            || terrain->near.left - height < -STEP_L) {
            if (p->velocity.x >= 0) {
                if (p->velocity.x == 0 && p->velocity.z != 0) {
                    item->pos.x = ROUND_TO_SECTOR_MID(item->pos.x);
                }
            } else {
                p->velocity.x = -p->velocity.x >> 1;
                item->pos.x = ROUND_TO_SECTOR_MID(item->pos.x);
            }
        } else if (terrain->near.left == height) {
            flat_count++;
        } else {
            p->velocity.x -= (terrain->near.left - height) >> 1;
        }
    }

    if (terrain->near.right - height <= STEP_L) {
        if (terrain->far.right - height < -WALL_L
            || terrain->near.right - height < -STEP_L) {
            if (p->velocity.x <= 0) {
                if (p->velocity.x == 0 && p->velocity.z != 0) {
                    item->pos.x = ROUND_TO_SECTOR_MID(item->pos.x);
                }
            } else {
                p->velocity.x = -p->velocity.x >> 1;
                item->pos.x = ROUND_TO_SECTOR_MID(item->pos.x);
            }
        } else if (terrain->near.right == height) {
            flat_count++;
        } else {
            p->velocity.x += (terrain->near.right - height) >> 1;
        }
    }

    if (flat_count == 2) {
        M_DampenVelocity(&p->velocity.x);
    }
}

static void M_UpdateRotation(ITEM *const item)
{
    const M_PRIV *const p = item->priv;
    if (p->velocity.x == 0 && p->velocity.z == 0) {
        return;
    }

    const uint16_t current = item->rot.y;
    const uint16_t target = Math_Atan(p->velocity.z, p->velocity.x);
    const uint16_t delta = target - current;

    if ((delta & INT16_MAX) < M_TURN_RATE) {
        item->rot.y = target;
    } else if (target <= current || delta >= DEG_180) {
        item->rot.y = current - M_TURN_RATE;
    } else {
        item->rot.y = current + M_TURN_RATE;
    }

    item->rot.x -= (ABS(p->velocity.x) + ABS(p->velocity.z)) >> 1;
}

static void M_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    if (!Item_IsTriggerActive(item)) {
        return;
    }

    M_PRIV *const p = item->priv;

    item->fall_speed += GRAVITY;
    item->pos.y += item->fall_speed;
    item->pos.x += p->velocity.x >> 5;
    item->pos.z += p->velocity.z >> 5;

    int16_t room_num = item->room_num;
    const int32_t height =
        M_GetHeight(item->pos.x, item->pos.y, item->pos.z, &room_num);
    if (item->pos.y > height) {
        M_Bounce(item, height);
    }

    const M_TERRAIN terrain = M_GetTerrain(item, &room_num);
    if (item->pos.y - height > -STEP_L
        || item->pos.y - terrain.far.front >= M_HALF_BLOCK
        || item->pos.y - terrain.far.right >= M_HALF_BLOCK
        || item->pos.y - terrain.far.back >= M_HALF_BLOCK
        || item->pos.y - terrain.far.left >= M_HALF_BLOCK) {
        M_UpdateVelocityZ(item, height, &terrain);
        M_UpdateVelocityX(item, height, &terrain);
    }

    Room_GetSector(item->pos, &room_num);
    Item_UpdateRoom(item_num, room_num);

    CLAMP(p->velocity.x, M_MIN_VEL, M_MAX_VEL);
    CLAMP(p->velocity.z, M_MIN_VEL, M_MAX_VEL);
    M_UpdateRotation(item);

    Room_TestTriggers(item);
}

static void M_Setup(OBJECT *const obj)
{
    if (!obj->loaded) {
        return;
    }

    obj->collision_func = M_Collision;
    obj->control_func = M_Control;

    obj->priv_size = sizeof(M_PRIV);
    obj->priv_load_func = M_LoadPriv;
    obj->priv_save_func = M_SavePriv;
    obj->save_position = true;
    obj->save_flags = true;
}

REGISTER_OBJECT(O_BOUNCING_BOULDER, M_Setup)
