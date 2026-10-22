#include <trx/game/objects/projectiles/blast.h>

#include <trx/game/creature.h>
#include <trx/game/fx/debris.h>
#include <trx/game/fx/shockwave.h>
#include <trx/game/gun/misc.h>
#include <trx/game/gun/smashing.h>
#include <trx/game/items.h>
#include <trx/game/lara.h>
#include <trx/game/objects/common.h>
#include <trx/game/rooms.h>
#include <trx/game/sound.h>
#include <trx/game/sparks.h>

// clang-format off
// The shockwaves of a blast, as TR4 sets them off: a big one at the blast
// and at each item it smashes, and a small one at each static it shatters.
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
// How many faces of a shattered static fly off as debris. A negative count
// picks the faces at random.
#define M_STATIC_DEBRIS_FACES   (-128)
// clang-format on

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

static void M_HitItems(
    const ITEM *const source, const COLLIDED_OBJECTS *const found,
    const bool hurts_lara)
{
    for (int32_t i = 0; i < found->item_count; i++) {
        ITEM *const target = found->items[i];
        // TODO: TR4 blasts also trip the shootable switches, which TRX lacks.
        if (Gun_GetSmashPolicy(target) != GUN_SMASH_POLICY_NONE) {
            Sparks_TriggerExplosionSparks(
                target->pos, 3, -2, 0, target->room_num);
            M_TriggerShockwave(
                target->pos, M_BIG_SHOCKWAVE_INNER, M_BIG_SHOCKWAVE_OUTER,
                M_BIG_SHOCKWAVE_SPEED, M_BIG_SHOCKWAVE_LIFE);
            Gun_SmashItem(Item_GetIndex(target));
        } else if (target == Lara_GetItem()) {
            if (hurts_lara) {
                Gun_HitTargetWithBlast(target, source);
            }
        } else if (
            Object_Get(target->object_id)->intelligent
            && Item_CanBeProjectileTarget(target)) {
            Gun_HitTargetWithBlast(target, source);
        }
    }
}

static void M_HitStatics(
    const ITEM *const source, const COLLIDED_OBJECTS *const found)
{
    for (int32_t i = 0; i < found->static_count; i++) {
        const STATIC_MESH *const mesh = found->statics[i];
        if (!Object_Get3DStatic(mesh->static_num)->shatter.by_shot) {
            continue;
        }
        Sparks_TriggerExplosionSparks(mesh->pos, 3, -2, 0, source->room_num);
        M_TriggerShockwave(
            mesh->pos, M_SMALL_SHOCKWAVE_INNER, M_SMALL_SHOCKWAVE_OUTER,
            M_SMALL_SHOCKWAVE_SPEED, M_SMALL_SHOCKWAVE_LIFE);
        FX_Debris_ShatterStatic(
            mesh, M_STATIC_DEBRIS_FACES, source->room_num, 0);
        Room_ShatterStatic(mesh);
    }
}

void Blast_HitObjects(
    const ITEM *const source, const COLLIDED_OBJECTS *const found,
    const bool hurts_lara)
{
    M_HitItems(source, found, hurts_lara);
    M_HitStatics(source, found);
}

void Blast_Explode(ITEM *const source, const XYZ_32 old_pos)
{
    if (Room_Get(source->room_num)->flags.underwater) {
        Sparks_TriggerUnderwaterExplosion(source);
    } else {
        M_TriggerShockwave(
            source->pos, M_BIG_SHOCKWAVE_INNER, M_BIG_SHOCKWAVE_OUTER,
            M_BIG_SHOCKWAVE_SPEED, M_BIG_SHOCKWAVE_LIFE);
        Sparks_TriggerExplosionSparks(old_pos, 3, -2, 0, source->room_num);
        for (int32_t i = 0; i < 2; i++) {
            Sparks_TriggerExplosionSparks(old_pos, 3, -1, 0, source->room_num);
        }
    }
    Creature_AlertNearbyGuards(source);
    Sound_Effect(SFX_EXPLOSION_1, &source->pos, 0x1800000 | SPM_PITCH);
    Sound_Effect(SFX_EXPLOSION_2, &source->pos, SPM_NORMAL);
}
