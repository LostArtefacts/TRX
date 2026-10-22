#include <trx/game/objects/projectiles/grenade_tr4.h>

#include <trx/core/json/util/read_io.h>
#include <trx/core/json/util/write_io.h>
#include <trx/core/math.h>
#include <trx/game/collision/common.h>
#include <trx/game/fx/water.h>
#include <trx/game/gun/common.h>
#include <trx/game/inventory.h>
#include <trx/game/items.h>
#include <trx/game/objects/projectiles/blast.h>
#include <trx/game/rooms.h>
#include <trx/game/sparks.h>

// clang-format off
// How many ticks a grenade flies before it goes off. It touches nothing for
// the first two, so that it clears the gun.
#define M_FUSE             120
#define M_ARMED            (M_FUSE - 2)
// clang-format on

typedef struct {
    GRENADE_TR4_KIND kind;
} M_PRIV;

static RESULT M_LoadPriv(ITEM *const item, JSON_READ_IO *const io)
{
    M_PRIV *const p = item->priv;
    int32_t kind = GRENADE_TR4_KIND_NORMAL;
    MUST(JSON_READ_OPT(io, "kind", &kind));
    p->kind = (GRENADE_TR4_KIND)kind;
    return OK;
}

static void M_SavePriv(const ITEM *const item, JSON_WRITE_IO *const io)
{
    const M_PRIV *const p = item->priv;
    JSONW_WRITE(io, "kind", (int32_t)p->kind);
}

static void M_Initialise(const int16_t item_num)
{
    M_PRIV *const p = Item_Get(item_num)->priv;
    p->kind = GRENADE_TR4_KIND_NORMAL;
}

// Tumbles the grenade as it flies, faster in air than in water, and slows
// it in water.
static void M_Tumble(ITEM *const item, const bool underwater)
{
    if (underwater) {
        item->fall_speed += (5 - item->fall_speed) >> 1;
        item->speed -= item->speed >> 2;
    } else {
        item->fall_speed += 3;
    }
    if (item->speed == 0) {
        return;
    }

    const int32_t base = underwater ? 3 : 7;
    const int32_t roll_shift = underwater ? 4 : 2;
    const int32_t spin_shift = underwater ? 2 : 1;
    item->rot.z += DEG_1 * ((item->speed >> roll_shift) + base);
    if (item->required_anim_state != 0) {
        item->rot.y += DEG_1 * ((item->speed >> spin_shift) + base);
    } else {
        item->rot.x += DEG_1 * ((item->speed >> spin_shift) + base);
    }
}

// Moves the grenade along its heading, which it keeps in goal_anim_state as
// it tumbles, and bounces it off what it meets.
static void M_Move(ITEM *const item, const GAME_VECTOR old_pos)
{
    XYZ_32 vel =
        XYZ_32_RotateYaw((XYZ_32) { .z = item->speed }, item->goal_anim_state);
    vel.y = item->fall_speed;
    item->pos = XYZ_32_Add(item->pos, vel);

    const int16_t y_rot = item->rot.y;
    item->rot.y = item->goal_anim_state;
    Collide_DoProperDetection(item, old_pos.pos);
    item->goal_anim_state = item->rot.y;
    item->rot.y = y_rot;
}

static void M_SplashIntoWater(const ITEM *const item)
{
    const ROOM *const room = Room_Get(item->room_num);
    FX_Water_SetupSplash(&(FX_WATER_SPLASH_SETUP) {
        .pos = { .x = item->pos.x, .y = room->max_ceiling, .z = item->pos.z },
        .inner_xz_off = 16,
        .inner_xz_size = 12,
        .inner_y_size = -96,
        .inner_xz_vel = 160,
        .inner_gravity = 128,
        .inner_y_vel = -2048 - ((int32_t)item->fall_speed << 5),
        .inner_friction = 7,
        .middle_xz_off = 24,
        .middle_xz_size = 24,
        .middle_y_size = -64,
        .middle_xz_vel = 224,
        .middle_gravity = 72,
        .middle_y_vel = -1024 - ((int32_t)item->fall_speed << 4),
        .middle_friction = 8,
        .outer_xz_off = 32,
        .outer_xz_size = 32,
        .outer_xz_vel = 272,
        .outer_friction = 9,
    });
}

static void M_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    const GAME_VECTOR old_pos = {
        .pos = item->pos,
        .room_num = item->room_num,
    };

    // A negative shade lights the grenade by the room around it as it
    // flies, as the original lights its projectiles.
    item->shade.value_1 = -1;
    item->shade.value_2 = -1;

    const bool was_underwater = Room_Get(item->room_num)->flags.underwater;
    M_Tumble(item, was_underwater);
    if (item->speed != 0 && !was_underwater) {
        const XYZ_32 back = XYZ_32_FromYawPitch(item->rot.y, item->rot.x, -64);
        Sparks_TriggerRocketSmoke(
            XYZ_32_Add(item->pos, back), -1, item->room_num);
    }

    M_Move(item, old_pos);
    if (Room_Get(item->room_num)->flags.underwater && !was_underwater) {
        M_SplashIntoWater(item);
    }

    bool exploded = false;
    int32_t radius = 0;
    if (item->hit_points != 0) {
        item->hit_points--;
        if (item->hit_points == 0) {
            exploded = true;
            radius = BLAST_RADIUS;
        } else if (item->hit_points > M_ARMED) {
            return;
        }
    }

    // A grenade that touches anything goes off, and looks again with the
    // reach of its blast. As in the original, a grenade whose fuse runs out
    // deals its blast on both looks.
    for (int32_t i = 0; i < 2; i++) {
        COLLIDED_OBJECTS found;
        Collide_GetCollidedObjects(item, radius, true, true, &found);
        if (exploded) {
            Blast_HitObjects(item, &found, true);
            continue;
        }
        if (found.item_count == 0 && found.static_count == 0) {
            break;
        }
        exploded = true;
        radius = BLAST_RADIUS;
    }

    if (exploded) {
        Blast_Explode(item, old_pos.pos);
        Item_Destroy(item_num);
    }
}

void Grenade_TR4_Setup(OBJECT *const obj)
{
    obj->initialise_func = M_Initialise;
    obj->control_func = M_Control;
    obj->priv_size = sizeof(M_PRIV);
    obj->priv_load_func = M_LoadPriv;
    obj->priv_save_func = M_SavePriv;
    obj->save_position = true;
}

GRENADE_TR4_KIND Grenade_TR4_GetLoadedKind(void)
{
    const OBJECT_ID ammo = Inv_GetAmmoKind(LGT_GRENADE);
    if (Gun_GetAmmoKindCount(LGT_GRENADE) > 1
        && ammo == Gun_GetAmmoKind(LGT_GRENADE, 1)) {
        return GRENADE_TR4_KIND_SUPER;
    }
    if (Gun_GetAmmoKindCount(LGT_GRENADE) > 2
        && ammo == Gun_GetAmmoKind(LGT_GRENADE, 2)) {
        return GRENADE_TR4_KIND_FLASH;
    }
    return GRENADE_TR4_KIND_NORMAL;
}

int16_t Grenade_TR4_Fire(
    const XYZ_32 pos, const int16_t room_num, const XYZ_16 rot,
    const int16_t speed, const GRENADE_TR4_KIND kind)
{
    const int16_t item_num = Item_Create();
    if (item_num == NO_ITEM) {
        return NO_ITEM;
    }

    ITEM *const item = Item_Get(item_num);
    item->object_id = O_GRENADE;
    item->room_num = room_num;
    item->pos = pos;
    item->interp.prev.pos = pos;
    Item_Initialise(item_num);

    item->rot = (XYZ_16) { .x = rot.x, .y = rot.y };
    item->speed = speed;
    item->fall_speed = (-speed * Math_Sin(rot.x)) >> W2V_SHIFT;
    item->current_anim_state = rot.x;
    item->goal_anim_state = rot.y;
    item->required_anim_state = 0;
    item->hit_points = M_FUSE;
    M_PRIV *const p = item->priv;
    p->kind = kind;
    Item_AddSimulated(item_num);
    return item_num;
}
