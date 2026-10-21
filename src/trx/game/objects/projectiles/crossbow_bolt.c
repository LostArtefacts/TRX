#include <trx/game/objects/projectiles/crossbow_bolt.h>

#include <trx/core/json/util/read_io.h>
#include <trx/core/json/util/write_io.h>
#include <trx/core/math.h>
#include <trx/game/collision/common.h>
#include <trx/game/creature.h>
#include <trx/game/fx/debris.h>
#include <trx/game/fx/shockwave.h>
#include <trx/game/fx/water.h>
#include <trx/game/gun/common.h>
#include <trx/game/gun/misc.h>
#include <trx/game/gun/registry.h>
#include <trx/game/gun/smashing.h>
#include <trx/game/inventory.h>
#include <trx/game/items.h>
#include <trx/game/objects/common.h>
#include <trx/game/random.h>
#include <trx/game/rooms.h>
#include <trx/game/sound.h>
#include <trx/game/sparks.h>
#include <trx/game/spawn.h>

// clang-format off
#define M_SPEED                 512
#define M_MIN_SPEED_UW          64
#define M_HIT_RADIUS            128
#define M_BLAST_RADIUS          2048
// The shockwaves of a blast, as TR4 sets them off: a big one at the bolt and
// at each item it smashes, and a small one at each static it shatters.
#define M_SHOCKWAVE_RISE        128
#define M_BLAST_COLOR           ((RGB_888) { .r = 0x80, .g = 0x60, .b = 0x00 })
#define M_BIG_SHOCKWAVE_INNER   48
#define M_BIG_SHOCKWAVE_OUTER   304
#define M_BIG_SHOCKWAVE_SPEED   96
#define M_BIG_SHOCKWAVE_LIFE    24
#define M_SMALL_SHOCKWAVE_INNER 40
#define M_SMALL_SHOCKWAVE_OUTER 176
#define M_SMALL_SHOCKWAVE_SPEED 64
#define M_SMALL_SHOCKWAVE_LIFE  16
#define M_SPLASH_COUNT          8
// How many faces of a shattered static fly off as debris. A negative count
// picks the faces at random.
#define M_STATIC_DEBRIS_FACES   (-128)
#define M_BOLT_DEBRIS_FACES     (-64)
// Makes debris take the shade of the bolt rather than of its room.
#define M_DEBRIS_OWN_SHADE      0x400
// clang-format on

typedef enum {
    M_KIND_NORMAL,
    M_KIND_POISON,
    M_KIND_EXPLOSIVE,
} M_KIND;

typedef struct {
    M_KIND kind;
} M_PRIV;

static RESULT M_LoadPriv(ITEM *const item, JSON_READ_IO *const io)
{
    M_PRIV *const p = item->priv;
    int32_t kind = M_KIND_NORMAL;
    MUST(JSON_READ_OPT(io, "kind", &kind));
    p->kind = (M_KIND)kind;
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
    p->kind = M_KIND_NORMAL;
}

// Breaks the bolt into pieces where it stops, and removes it.
static void M_Break(const int16_t item_num)
{
    const ITEM *const item = Item_Get(item_num);
    SPHERE spheres[1];
    if (Collide_GetSpheres(item, spheres, true) > 0) {
        const OBJECT *const obj = Object_Get(item->object_id);
        const SHATTER_ITEM shatter_item = {
            .pos = spheres[0].pos,
            .mesh = Object_GetMesh(obj->mesh_idx),
            .bit = 1,
            .yaw = item->rot.y,
            .flags = M_DEBRIS_OWN_SHADE,
        };
        FX_Debris_ShatterItem(
            &shatter_item, M_BOLT_DEBRIS_FACES, item->room_num, 0);
    }
    Item_Destroy(item_num);
}

static void M_HitItems(
    const ITEM *const item, const GAME_VECTOR old_pos,
    const COLLIDED_OBJECTS *const collided)
{
    const M_PRIV *const p = item->priv;
    const GAME_VECTOR hit_pos = {
        .pos = item->pos,
        .room_num = item->room_num,
    };
    for (int32_t i = 0; i < collided->item_count; i++) {
        ITEM *const target = collided->items[i];
        // TODO: TR4 bolts also trip the shootable switches and break the
        // skeletons, which TRX lacks.
        if (!Object_Get(target->object_id)->intelligent
            || !Item_CanBeProjectileTarget(target)) {
            continue;
        }
        Gun_HitTarget(
            target, &old_pos, &hit_pos, Gun_Registry_Get(LGT_CROSSBOW)->damage);
        if (p->kind == M_KIND_POISON && Item_CanTakeDamage(target)) {
            target->is_poisoned = true;
        }
    }
}

static void M_HitStatics(
    const ITEM *const item, const COLLIDED_OBJECTS *const collided)
{
    for (int32_t i = 0; i < collided->static_count; i++) {
        const STATIC_MESH *const mesh = collided->statics[i];
        if (Object_Get3DStatic(mesh->static_num)->shatter.by_shot) {
            FX_Debris_ShatterStatic(
                mesh, M_STATIC_DEBRIS_FACES, item->room_num, 0);
            Room_ShatterStatic(mesh);
        }
    }
}

// Sets off a shockwave at a place, a little above it, as TR4 explosions do.
static void M_TriggerShockwave(
    const XYZ_32 pos, const int16_t inner_radius, const int16_t outer_radius,
    const int16_t speed, const uint8_t life)
{
    FX_Shockwave_Create(&(FX_SHOCKWAVE_SETUP) {
        .pos = { .x = pos.x, .y = pos.y - M_SHOCKWAVE_RISE, .z = pos.z },
        .inner_radius = inner_radius,
        .outer_radius = outer_radius,
        .speed = speed,
        .color = M_BLAST_COLOR,
        .life = life,
    });
}

static void M_BlastItems(
    const ITEM *const item, const COLLIDED_OBJECTS *const collided)
{
    for (int32_t i = 0; i < collided->item_count; i++) {
        ITEM *const target = collided->items[i];
        // TODO: TR4 blasts also trip the shootable switches, which TRX lacks.
        if (Gun_GetSmashPolicy(target) != GUN_SMASH_POLICY_NONE) {
            Sparks_TriggerExplosionSparks(
                target->pos, 3, -2, 0, target->room_num);
            M_TriggerShockwave(
                target->pos, M_BIG_SHOCKWAVE_INNER, M_BIG_SHOCKWAVE_OUTER,
                M_BIG_SHOCKWAVE_SPEED, M_BIG_SHOCKWAVE_LIFE);
            Gun_SmashItem(Item_GetIndex(target));
        } else if (
            Object_Get(target->object_id)->intelligent
            && Item_CanBeProjectileTarget(target)) {
            Gun_HitTargetWithBlast(target, item);
        }
    }
}

static void M_BlastStatics(
    const ITEM *const item, const COLLIDED_OBJECTS *const collided)
{
    for (int32_t i = 0; i < collided->static_count; i++) {
        const STATIC_MESH *const mesh = collided->statics[i];
        if (!Object_Get3DStatic(mesh->static_num)->shatter.by_shot) {
            continue;
        }
        Sparks_TriggerExplosionSparks(mesh->pos, 3, -2, 0, item->room_num);
        M_TriggerShockwave(
            mesh->pos, M_SMALL_SHOCKWAVE_INNER, M_SMALL_SHOCKWAVE_OUTER,
            M_SMALL_SHOCKWAVE_SPEED, M_SMALL_SHOCKWAVE_LIFE);
        FX_Debris_ShatterStatic(mesh, M_STATIC_DEBRIS_FACES, item->room_num, 0);
        Room_ShatterStatic(mesh);
    }
}

// Sets off the bolt's own blast: a fireball and a shockwave out of water, or
// a burst of bubbles in it.
static void M_Explode(ITEM *const item, const XYZ_32 old_pos)
{
    if (Room_Get(item->room_num)->flags.underwater) {
        Sparks_TriggerUnderwaterExplosion(item);
    } else {
        M_TriggerShockwave(
            item->pos, M_BIG_SHOCKWAVE_INNER, M_BIG_SHOCKWAVE_OUTER,
            M_BIG_SHOCKWAVE_SPEED, M_BIG_SHOCKWAVE_LIFE);
        Sparks_TriggerExplosionSparks(old_pos, 3, -2, 0, item->room_num);
        for (int32_t i = 0; i < 2; i++) {
            Sparks_TriggerExplosionSparks(old_pos, 3, -1, 0, item->room_num);
        }
    }
    Creature_AlertNearbyGuards(item);
    Sound_Effect(SFX_EXPLOSION_1, &item->pos, 0x1800000 | SPM_PITCH);
    Sound_Effect(SFX_EXPLOSION_2, &item->pos, SPM_NORMAL);
}

static void M_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    const GAME_VECTOR old_pos = {
        .pos = item->pos,
        .room_num = item->room_num,
    };

    item->timer++;
    const bool was_underwater = Room_Get(item->room_num)->flags.underwater;
    if (was_underwater) {
        if (item->speed > M_MIN_SPEED_UW) {
            item->speed -= item->speed >> 4;
        }
        if ((item->timer & 1) != 0) {
            Spawn_BubbleEx(&item->pos, item->room_num, 4, 8);
        }
    }

    const int32_t speed_xz = (item->speed * Math_Cos(item->rot.x)) >> W2V_SHIFT;
    item->pos.x += (speed_xz * Math_Sin(item->rot.y)) >> W2V_SHIFT;
    item->pos.y += (item->speed * Math_Sin(-item->rot.x)) >> W2V_SHIFT;
    item->pos.z += (speed_xz * Math_Cos(item->rot.y)) >> W2V_SHIFT;

    const M_PRIV *const p = item->priv;
    bool exploded = false;
    int16_t room_num = item->room_num;
    const SECTOR *const sector = Room_GetSector(item->pos, &room_num);
    if (Room_GetHeight(sector, item->pos) < item->pos.y
        || Room_GetCeiling(sector, item->pos) > item->pos.y) {
        item->pos = old_pos.pos;
        if (p->kind != M_KIND_EXPLOSIVE) {
            M_Break(item_num);
            return;
        }
        exploded = true;
    }

    Item_UpdateRoom(item_num, room_num);
    const ROOM *const room = Room_Get(room_num);
    if (room->flags.underwater && !was_underwater) {
        const XYZ_32 surface = {
            .x = item->pos.x,
            .y = room->max_ceiling,
            .z = item->pos.z,
        };
        Sparks_TriggerSmallSplash(surface, M_SPLASH_COUNT);
        FX_Water_SetupRipple(surface, (Random_GetControl() & 7) + 8, 0);
    }

    // An explosive bolt that touches anything explodes, and looks again
    // with the reach of its blast.
    bool collided = false;
    int32_t radius = exploded ? M_BLAST_RADIUS : M_HIT_RADIUS;
    for (int32_t i = 0; i < 2; i++) {
        COLLIDED_OBJECTS found;
        Collide_GetCollidedObjects(item, radius, true, true, &found);
        if (found.item_count == 0 && found.static_count == 0) {
            break;
        }
        collided = true;
        if (p->kind != M_KIND_EXPLOSIVE || exploded) {
            if (exploded) {
                M_BlastItems(item, &found);
                M_BlastStatics(item, &found);
            } else {
                M_HitItems(item, old_pos, &found);
                M_HitStatics(item, &found);
            }
            break;
        }
        exploded = true;
        radius = M_BLAST_RADIUS;
    }

    if (exploded) {
        M_Explode(item, old_pos.pos);
    }
    if (collided || exploded) {
        M_Break(item_num);
    }
}

// Returns the kind of bolt the crossbow is loaded with: the first, second or
// third kind of ammunition it takes.
static M_KIND M_GetLoadedKind(void)
{
    const OBJECT_ID ammo = Inv_GetAmmoKind(LGT_CROSSBOW);
    if (Gun_GetAmmoKindCount(LGT_CROSSBOW) > 1
        && ammo == Gun_GetAmmoKind(LGT_CROSSBOW, 1)) {
        return M_KIND_POISON;
    }
    if (Gun_GetAmmoKindCount(LGT_CROSSBOW) > 2
        && ammo == Gun_GetAmmoKind(LGT_CROSSBOW, 2)) {
        return M_KIND_EXPLOSIVE;
    }
    return M_KIND_NORMAL;
}

static void M_Setup(OBJECT *const obj)
{
    obj->initialise_func = M_Initialise;
    obj->control_func = M_Control;
    obj->priv_size = sizeof(M_PRIV);
    obj->priv_load_func = M_LoadPriv;
    obj->priv_save_func = M_SavePriv;
    obj->save_position = true;
}

int16_t CrossbowBolt_Fire(
    const XYZ_32 pos, const int16_t room_num, const XYZ_16 rot)
{
    const int16_t item_num = Item_Create();
    if (item_num == NO_ITEM) {
        return NO_ITEM;
    }

    ITEM *const item = Item_Get(item_num);
    item->object_id = O_CROSSBOW_BOLT;
    item->room_num = room_num;
    item->pos = pos;
    item->interp.prev.pos = pos;
    Item_Initialise(item_num);

    item->rot = (XYZ_16) { .x = rot.x, .y = rot.y };
    item->speed = M_SPEED;
    // A negative shade lights the bolt by the room around it as it flies, as
    // the original lights its projectiles.
    item->shade.value_1 = -1;
    item->shade.value_2 = -1;
    M_PRIV *const p = item->priv;
    p->kind = M_GetLoadedKind();
    Item_AddSimulated(item_num);
    return item_num;
}

REGISTER_OBJECT(O_CROSSBOW_BOLT, M_Setup)
