#include <trx/game/objects/projectiles/grenade_tr4.h>

#include <trx/core/json/util/read_io.h>
#include <trx/core/json/util/write_io.h>
#include <trx/core/math.h>
#include <trx/game/collision/common.h>
#include <trx/game/fx/water.h>
#include <trx/game/gun/common.h>
#include <trx/game/inventory.h>
#include <trx/game/items.h>
#include <trx/game/lara.h>
#include <trx/game/objects/projectiles/blast.h>
#include <trx/game/output/flash.h>
#include <trx/game/random.h>
#include <trx/game/rooms.h>
#include <trx/game/sparks.h>

// clang-format off
// How many ticks a grenade flies before it goes off. It touches nothing for
// the first two, so that it clears the gun.
#define M_FUSE             120
#define M_ARMED            (M_FUSE - 2)
// A fragment flies until it meets the floor, the ceiling or water.
#define M_FRAGMENT_FUSE    3000
#define M_FRAGMENT_SPEED   64
// How many ticks a super grenade keeps throwing fragments after its blast,
// and a flash grenade keeps flashing.
#define M_SUPER_AFTERMATH  4
#define M_FLASH_AFTERMATH  16
// How long the last flash of a flash grenade blinds the enemies that react
// to it, in ticks.
#define M_FLASH_BLIND_TIME 120
// clang-format on

typedef struct {
    GRENADE_TR4_KIND kind;
    // The ticks left after the blast, while the grenade stays behind unseen
    // to throw its fragments, or zero while it flies.
    int16_t aftermath;
} M_PRIV;

static RESULT M_LoadPriv(ITEM *const item, JSON_READ_IO *const io)
{
    M_PRIV *const p = item->priv;
    int32_t kind = GRENADE_TR4_KIND_NORMAL;
    MUST(JSON_READ_OPT(io, "kind", &kind));
    p->kind = (GRENADE_TR4_KIND)kind;
    MUST(JSON_READ_OPT(io, "aftermath", &p->aftermath));
    return OK;
}

static void M_SavePriv(const ITEM *const item, JSON_WRITE_IO *const io)
{
    const M_PRIV *const p = item->priv;
    JSONW_WRITE(io, "kind", (int32_t)p->kind);
    JSONW_WRITE_NZ(io, "aftermath", p->aftermath);
}

static void M_Initialise(const int16_t item_num)
{
    M_PRIV *const p = Item_Get(item_num)->priv;
    p->kind = GRENADE_TR4_KIND_NORMAL;
    p->aftermath = 0;
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
// it tumbles, and bounces it off what it meets. A fragment flies through and
// is set to go off on the next tick where it meets the floor or ceiling.
static void M_Move(ITEM *const item, const GAME_VECTOR old_pos)
{
    XYZ_32 vel =
        XYZ_32_RotateYaw((XYZ_32) { .z = item->speed }, item->goal_anim_state);
    vel.y = item->fall_speed;
    item->pos = XYZ_32_Add(item->pos, vel);

    const M_PRIV *const p = item->priv;
    if (p->kind == GRENADE_TR4_KIND_FRAGMENT) {
        int16_t room_num = item->room_num;
        const SECTOR *const sector = Room_GetSector(item->pos, &room_num);
        if (Room_GetHeight(sector, item->pos) < item->pos.y
            || Room_GetCeiling(sector, item->pos) > item->pos.y) {
            item->hit_points = 1;
        }
        Item_UpdateRoom(Item_GetIndex(item), room_num);
        return;
    }

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

// Throws one of a super grenade's fragments from where it went off.
static void M_ThrowFragment(const ITEM *const item)
{
    const XYZ_32 pos = {
        .x = item->pos.x + (Random_GetControl() & 0x1FF) - 256,
        .y = item->pos.y - 256,
        .z = item->pos.z + (Random_GetControl() & 0x1FF) - 256,
    };
    const XYZ_16 rot = {
        .x = (Random_GetControl() & 0x3FFF) + 0x2000,
        .y = (int16_t)(Random_GetControl() * 2),
    };
    const int16_t fragment_num = Grenade_TR4_Fire(
        pos, item->room_num, rot, M_FRAGMENT_SPEED, GRENADE_TR4_KIND_FRAGMENT);
    if (fragment_num == NO_ITEM) {
        return;
    }
    ITEM *const fragment = Item_Get(fragment_num);
    fragment->is_visible = false;
    fragment->hit_points =
        Room_Get(fragment->room_num)->flags.underwater ? 1 : M_FRAGMENT_FUSE;
}

// Flashes the screen and gives off smoke for a tick after a flash grenade
// goes off: a flicker of white at first, then a full white flash that
// blinds the enemies that react to it.
static void M_Flash(const ITEM *const item, const bool last)
{
    if (last) {
        Output_Flash_Trigger(COLOR_RGB_888_WHITE);
        Lara_GetLaraInfo()->blind_timer = M_FLASH_BLIND_TIME;
    } else {
        const uint8_t r = (Random_GetControl() & 0x1F) + 224;
        const uint8_t g = r - (Random_GetControl() & 0x1F);
        Output_Flash_Trigger((RGB_888) { .r = r, .g = g, .b = g });
    }
    for (int32_t i = 0; i < 2; i++) {
        Sparks_TriggerFlashSmoke(item->pos, item->room_num);
    }
}

// Runs a tick of what follows the blast, while the grenade stays behind
// unseen.
static void M_ControlAftermath(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    M_PRIV *const p = item->priv;
    p->aftermath--;
    if (p->aftermath == 0) {
        Item_Destroy(item_num);
        return;
    }
    if (p->kind == GRENADE_TR4_KIND_FLASH) {
        M_Flash(item, p->aftermath == 1);
    } else {
        M_ThrowFragment(item);
    }
}

static void M_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    M_PRIV *const p = item->priv;
    if (p->aftermath > 0) {
        M_ControlAftermath(item_num);
        return;
    }
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
        if (p->kind == GRENADE_TR4_KIND_FRAGMENT) {
            item->hit_points = 1;
        }
    }
    if (p->kind == GRENADE_TR4_KIND_FRAGMENT) {
        Sparks_TriggerFireFlame(item->pos, -1, 1);
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
    // deals its blast on both looks. A flash grenade deals no blast.
    const bool is_flash = p->kind == GRENADE_TR4_KIND_FLASH;
    for (int32_t i = 0; i < 2 && (!is_flash || !exploded); i++) {
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

    if (!exploded) {
        return;
    }
    if (is_flash) {
        Output_Flash_Trigger(COLOR_RGB_888_WHITE);
        for (int32_t i = 0; i < 2; i++) {
            Sparks_TriggerFlashSmoke(item->pos, item->room_num);
        }
        Blast_SoundOff(item);
    } else {
        Blast_Explode(item, old_pos.pos);
    }
    if (p->kind == GRENADE_TR4_KIND_SUPER || is_flash) {
        item->mesh_bits = 0;
        p->aftermath = is_flash ? M_FLASH_AFTERMATH : M_SUPER_AFTERMATH;
    } else {
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
    switch (Inv_GetAmmoKind(LGT_GRENADE)) {
    case O_GRENADE_AMMO_2_ITEM:
        return GRENADE_TR4_KIND_SUPER;
    case O_GRENADE_AMMO_3_ITEM:
        return GRENADE_TR4_KIND_FLASH;
    default:
        return GRENADE_TR4_KIND_NORMAL;
    }
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
