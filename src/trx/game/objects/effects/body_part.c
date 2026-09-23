#include <trx/game/objects/effects/body_part.h>

#include <trx/core/json/util/read_io.h>
#include <trx/core/json/util/write_io.h>
#include <trx/game/effects.h>
#include <trx/game/fx/debris.h>
#include <trx/game/lara.h>
#include <trx/game/output.h>
#include <trx/game/random.h>
#include <trx/game/rooms.h>
#include <trx/game/sound.h>
#include <trx/game/sparks/spawners.h>
#include <trx/game/spawn.h>
#include <trx/version.h>

#define M_SPEED_NORMAL 128

typedef struct {
    GIB_FLAGS gib_flags;
    int16_t flame_variant;
    int16_t damage;
    int16_t settle;
} M_PRIV;

static int16_t M_Throw(const int16_t max_speed)
{
    const int32_t max = max_speed != 0 ? max_speed : M_SPEED_NORMAL;
    return (int16_t)((Random_GetControl() * max) >> 15);
}

static void M_TrailBlood(const EFFECT *const effect)
{
    if (g_TRVersion >= 3) {
        Sparks_TriggerBloodTR3(effect->pos, effect->rot.y >> 4, 1);
    } else {
        Spawn_Blood(
            effect->pos.x, effect->pos.y, effect->pos.z, effect->speed,
            effect->rot.y, effect->room_num);
    }
}

static void M_SpawnSplash(const GAME_VECTOR pos)
{
    const int16_t effect_num = Effect_Create(O_SPLASH_1, pos.room_num);
    if (effect_num != NO_EFFECT) {
        EFFECT *const effect = Effect_Get(effect_num);
        effect->pos = pos.pos;
        effect->rot.y = 0;
        effect->speed = 0;
        effect->frame_num = 0;
    }
}

// Replaces a body part with an explosion. Destroys the part first to free its
// effect slot.
static EFFECT *M_ExplodePart(const int16_t effect_num, const int16_t room_num)
{
    const EFFECT *const part = Effect_Get(effect_num);
    const XYZ_32 pos = part->pos;
    const XYZ_16 rot = part->rot;
    Effect_Destroy(effect_num);

    const int16_t explosion_num = Effect_Create(O_EXPLOSION_1, room_num);
    if (explosion_num == NO_EFFECT) {
        return nullptr;
    }

    EFFECT *const explosion = Effect_Get(explosion_num);
    explosion->pos = pos;
    explosion->rot = rot;
    explosion->speed = 0;
    explosion->fall_speed = 0;
    explosion->frame_num = 0;
    explosion->counter = 0;
    explosion->shade = SHADE_NEUTRAL;
    Sound_Effect(SFX_EXPLOSION_1, &explosion->pos, SPM_NORMAL);
    return explosion;
}

static void M_Control_TR12(const int16_t effect_num)
{
    EFFECT *const effect = Effect_Get(effect_num);
    const M_PRIV *const p = effect->priv;
    if ((p->gib_flags & GIB_BLOOD) != 0) {
        M_TrailBlood(effect);
    }
    effect->rot.x += 5 * DEG_1;
    effect->rot.z += 10 * DEG_1;
    effect->pos = XYZ_32_OffsetYaw(effect->pos, effect->rot.y, effect->speed);
    effect->pos.y += effect->fall_speed;
    effect->fall_speed += GRAVITY;

    int16_t room_num = effect->room_num;
    const SECTOR *const sector = Room_GetSector(effect->pos, &room_num);

    const ROOM *const current_room = Room_Get(effect->room_num);
    const ROOM *const next_room = Room_Get(room_num);
    if (!current_room->flags.underwater && next_room->flags.underwater) {
        M_SpawnSplash(
            (GAME_VECTOR) { .pos = effect->pos, .room_num = effect->room_num });
    }

    const int32_t ceiling = Room_GetCeiling(sector, effect->pos);
    if (effect->pos.y < ceiling) {
        effect->pos.y = ceiling;
        effect->fall_speed = MAX(1, -effect->fall_speed);
    }

    const int32_t height = Room_GetHeight(sector, effect->pos);
    if (effect->pos.y >= height) {
        if ((p->gib_flags & GIB_BLAST) != 0) {
            M_ExplodePart(effect_num, effect->room_num);
        } else {
            Effect_Destroy(effect_num);
        }
        return;
    }

    const bool blast_on_contact =
        (p->gib_flags & GIB_BLAST) != 0 && g_TRVersion == 1;

    if (Lara_IsNearItem(&effect->pos, p->damage * 2)) {
        Lara_TakeDamage(p->damage, true);

        if (blast_on_contact) {
            EFFECT *const explosion = M_ExplodePart(effect_num, room_num);
            if (explosion != nullptr) {
                LARA_INFO *const lara = Lara_GetLaraInfo();
                lara->hit_effect_count = 5;
                lara->hit_effect = explosion;
            }
        } else {
            Effect_Destroy(effect_num);
        }
        return;
    }

    if (room_num != effect->room_num) {
        Effect_UpdateRoom(effect_num, room_num);
    }
}

static void M_DrawTR3Fire(
    const M_PRIV *const p, const XYZ_32 pos, const int16_t effect_num)
{
    if ((p->gib_flags & GIB_FLAME) != 0) {
        Sparks_TriggerFireFlame(pos, effect_num, p->flame_variant);
    }
    if ((p->gib_flags & GIB_SMOKE) != 0) {
        Sparks_TriggerFireSmoke(pos, -1, 0);
    }
}

static void M_BurstTR3(
    const M_PRIV *const p, const EFFECT *const effect, const int32_t height)
{
    if ((p->gib_flags & (GIB_FLAME | GIB_SMOKE)) == 0) {
        return;
    }

    for (int32_t i = 0; i < 3; i++) {
        M_DrawTR3Fire(p, (XYZ_32) { effect->pos.x, height, effect->pos.z }, -1);
    }
    Sound_Effect(SFX_EXPLOSION_1, &effect->pos, SPM_NORMAL);
}

static void M_Control_TR3(const int16_t effect_num)
{
    EFFECT *const effect = Effect_Get(effect_num);
    const M_PRIV *const p = effect->priv;
    if ((p->gib_flags & GIB_BLOOD) != 0) {
        M_TrailBlood(effect);
    }
    effect->rot.x += 5 * DEG_1;
    effect->rot.z += 10 * DEG_1;
    effect->fall_speed += 3;
    const XYZ_32 step =
        XYZ_32_RotateYaw((XYZ_32) { .z = effect->speed }, effect->rot.y);
    effect->pos.x += step.x >> 2;
    effect->pos.y += effect->fall_speed;
    effect->pos.z += step.z >> 2;

    const int32_t time4 = (int32_t)Output_GetTimeInGame() * 4;
    if (!(time4 & 0xC)) {
        M_DrawTR3Fire(p, effect->pos, effect_num);
    }

    int16_t room_num = effect->room_num;
    SECTOR *const sector = Room_GetSector(effect->pos, &room_num);
    int32_t c = Room_GetCeiling(sector, effect->pos);

    if (effect->pos.y < c) {
        effect->pos.y = c;
        effect->fall_speed = -effect->fall_speed;
    }

    int32_t h = Room_GetHeight(sector, effect->pos);

    if (effect->pos.y >= h) {
        M_BurstTR3(p, effect, h);
        Effect_Destroy(effect_num);
        return;
    }

    if (Lara_IsNearItem(&effect->pos, p->damage * 4)) {
        Lara_TakeDamage(p->damage, true);
        M_BurstTR3(p, effect, h);
        Effect_Destroy(effect_num);
        return;
    }

    if (effect->room_num != room_num) {
        Effect_UpdateRoom(effect_num, room_num);
    }
}

// Creates the TR4 burst with sparks because TR4 has no explosion sprite.
static void M_SpawnTR4Explosion(const XYZ_32 pos, const int16_t room_num)
{
    Sparks_TriggerExplosionSparks(pos, 3, -2, 0, room_num);
    for (int32_t i = 0; i < 2; i++) {
        Sparks_TriggerExplosionSparks(pos, 3, -1, 0, room_num);
    }
    Sound_Effect(SFX_EXPLOSION_1, &pos, SPM_NORMAL);
}

static void M_SpawnTR4Shatter(const EFFECT *const effect)
{
    const SHATTER_ITEM shatter_item = {
        .mesh = Object_GetMesh(effect->frame_num),
        .pos = effect->pos,
        .yaw = effect->rot.y,
    };
    FX_Debris_ShatterItem(&shatter_item, 32, effect->room_num, -1);
}

// Controls a TR4 body part. Creates debris on landing and draws fire for an
// exploding death.
static void M_Control_TR4(const int16_t effect_num)
{
    EFFECT *const effect = Effect_Get(effect_num);
    M_PRIV *const p = effect->priv;
    const XYZ_32 old_pos = effect->pos;

    if ((p->gib_flags & GIB_BLOOD) != 0) {
        M_TrailBlood(effect);
    }

    if (effect->speed != 0) {
        effect->rot.x += effect->fall_speed * 4;
    }
    effect->fall_speed += GRAVITY;
    effect->pos = XYZ_32_OffsetYaw(effect->pos, effect->rot.y, effect->speed);
    effect->pos.y += effect->fall_speed;

    const int32_t time4 = (int32_t)Output_GetTimeInGame() * 4;
    if ((time4 & 0xC) == 0 && (p->gib_flags & GIB_FLAME) != 0) {
        Sparks_TriggerFireFlame(effect->pos, effect_num, p->flame_variant);
    }

    int16_t room_num = effect->room_num;
    const SECTOR *const sector = Room_GetSector(effect->pos, &room_num);

    const int32_t ceiling = Room_GetCeiling(sector, effect->pos);
    if (effect->pos.y < ceiling) {
        effect->pos.y = ceiling;
        effect->fall_speed = -effect->fall_speed;
        effect->speed -= effect->speed >> 3;
    }

    const int32_t height = Room_GetHeight(sector, effect->pos);
    if (effect->pos.y >= height) {
        if ((p->gib_flags & GIB_BLAST) != 0) {
            M_SpawnTR4Explosion(
                (XYZ_32) { effect->pos.x, height, effect->pos.z }, room_num);
            Effect_Destroy(effect_num);
            return;
        }

        if ((p->gib_flags & GIB_DEBRIS) != 0) {
            M_SpawnTR4Shatter(effect);
            Sound_Effect(SFX_ROCK_FALL_LAND, &effect->pos, SPM_NORMAL);
            Effect_Destroy(effect_num);
            return;
        }

        if (old_pos.y <= height) {
            if (effect->fall_speed <= 32) {
                effect->fall_speed = 0;
            } else {
                effect->fall_speed = -effect->fall_speed >> 2;
            }
        } else {
            effect->rot.y += DEG_180;
            effect->pos.x = old_pos.x;
            effect->pos.z = old_pos.z;
        }

        effect->speed -= effect->speed >> 2;
        if (ABS(effect->speed) < 4) {
            effect->speed = 0;
        }
        effect->pos.y = old_pos.y;
    }

    if (effect->speed == 0) {
        p->settle++;
        if (p->settle > 32) {
            Effect_Destroy(effect_num);
            return;
        }
    }

    if (room_num != effect->room_num) {
        Effect_UpdateRoom(effect_num, room_num);
    }
}

static void M_Initialise(const int16_t effect_num)
{
    Effect_AllocPriv(effect_num, sizeof(M_PRIV));
}

static void M_SavePriv(const EFFECT *const effect, JSON_WRITE_IO *const io)
{
    const M_PRIV *const p = effect->priv;
    JSONW_WRITE(io, "gib_flags", (int32_t)p->gib_flags);
    JSONW_WRITE(io, "flame_variant", p->flame_variant);
    JSONW_WRITE(io, "damage", p->damage);
    JSONW_WRITE(io, "settle", p->settle);
}

static RESULT M_LoadPriv(EFFECT *const effect, JSON_READ_IO *const io)
{
    M_PRIV *const p = effect->priv;
    int32_t gib_flags = 0;
    SHOULD(JSON_READ_OPT(io, "gib_flags", &gib_flags));
    p->gib_flags = (GIB_FLAGS)gib_flags;
    SHOULD(JSON_READ_OPT(io, "flame_variant", &p->flame_variant));
    SHOULD(JSON_READ_OPT(io, "damage", &p->damage));
    SHOULD(JSON_READ_OPT(io, "settle", &p->settle));
    return OK;
}

static void M_Setup(OBJECT *const obj)
{
    obj->effect_initialise_func = M_Initialise;
    obj->effect_priv_save_func = M_SavePriv;
    obj->effect_priv_load_func = M_LoadPriv;
    switch (g_TRVersion) {
    case 4:
        obj->effect_control_func = M_Control_TR4;
        break;
    case 3:
        obj->effect_control_func = M_Control_TR3;
        break;
    default:
        obj->effect_control_func = M_Control_TR12;
        break;
    }
    obj->loaded = true;
    obj->mesh_count = 0;
}

REGISTER_OBJECT(O_BODY_PART, M_Setup)

EFFECT *BodyPart_Create(const BODY_PART_ARGS *const args)
{
    const int16_t effect_num = Effect_Create(O_BODY_PART, args->room_num);
    if (effect_num == NO_EFFECT) {
        return nullptr;
    }

    EFFECT *const effect = Effect_Get(effect_num);
    effect->pos = args->pos;
    effect->frame_num = args->mesh_idx;
    effect->shade = args->shade;
    effect->rot.y = g_TRVersion < 4 ? (Random_GetControl() - 0x4000) * 2
                                    : Random_GetControl() * 2;
    effect->speed = M_Throw(args->speed);
    effect->fall_speed = -M_Throw(args->fall_speed);

    M_PRIV *const p = effect->priv;
    p->gib_flags = args->gib_flags;
    p->flame_variant = args->flame_variant;
    p->damage = args->damage;

    if (g_TRVersion == 3 && (p->gib_flags & (GIB_FLAME | GIB_SMOKE)) != 0) {
        p->gib_flags &= ~(GIB_FLAME | GIB_SMOKE);
        p->gib_flags |= Random_GetControl() & (GIB_FLAME | GIB_SMOKE);
    }

    return effect;
}
