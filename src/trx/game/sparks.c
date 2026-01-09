#include <trx/game/sparks.h>

#include <trx/config.h>
#include <trx/debug.h>
#include <trx/game/collision.h>
#include <trx/game/effects.h>
#include <trx/game/items.h>
#include <trx/game/lara.h>
#include <trx/game/math.h>
#include <trx/game/objects.h>
#include <trx/game/output/lights.h>
#include <trx/game/output/sources/poly_fx.h>
#include <trx/game/random.h>
#include <trx/game/rooms.h>
#include <trx/game/spawn.h>
#include <trx/game/water_fx.h>
#include <trx/utils.h>
#include <trx/version.h>

#define M_MAX_SPARKS 400
#define M_MAX_SPARK_DYNAMICS 32

typedef struct {
    bool on;
    uint8_t falloff;
    RGB_888 color;
    uint8_t flags;
} M_SPARK_DYNAMIC;

static SPARK m_Sparks[M_MAX_SPARKS];
static M_SPARK_DYNAMIC m_Dynamics[M_MAX_SPARK_DYNAMICS];
static int32_t m_NextSpark = 0;
static XZ_32 m_SmokeWind = {};
static int32_t m_HairWindZ = 0;
static int32_t m_TR3Wind = 0;
static int32_t m_TR3WindAngle = DEG_180;
static int32_t m_TR3DWindAngle = DEG_180;

static const BITE m_NodeOffsets[16] = {
    { .pos = { 0, 340, 64 }, .mesh_num = 7 },
    { .pos = { 0, 0, -96 }, .mesh_num = 10 },
    { .pos = { 16, 48, 320 }, .mesh_num = 13 },
    { .pos = { 0, -256, 0 }, .mesh_num = 5 },
    { .pos = { 0, 64, 0 }, .mesh_num = 10 },
    { .pos = { 0, 64, 0 }, .mesh_num = 13 },
    { .pos = { -32, -16, -192 }, .mesh_num = 13 },
    { .pos = { -64, 410, 0 }, .mesh_num = 20 },
    { .pos = { 64, 410, 0 }, .mesh_num = 23 },
    { .pos = { -160, -8, 16 }, .mesh_num = 5 },
    { .pos = { -160, -8, 16 }, .mesh_num = 9 },
    { .pos = { -160, -8, 16 }, .mesh_num = 13 },
    { .pos = { 0, 0, 0 }, .mesh_num = 0 },
    { .pos = { 0, 0, 0 }, .mesh_num = 0 },
    { .pos = { 0, 0, 0 }, .mesh_num = 0 },
    { .pos = { 0, 0, 0 }, .mesh_num = 0 },
};

XYZ_32 Sparks_GetWorldPos(const SPARK *const spark)
{
    if (spark == nullptr) {
        return (XYZ_32) { 0, 0, 0 };
    }

    if ((spark->flags & SPARK_F_FX) != 0U) {
        const EFFECT *const effect = Effect_Get(spark->effect_num);
        if ((spark->flags & SPARK_F_ATTACHED_POS) != 0U) {
            return effect->pos;
        }
        return (XYZ_32) {
            .x = effect->pos.x + spark->pos.x,
            .y = effect->pos.y + spark->pos.y,
            .z = effect->pos.z + spark->pos.z,
        };
    }

    if ((spark->flags & SPARK_F_ITEM) != 0U) {
        const ITEM *const item = Item_Get(spark->item_num);
        if ((spark->flags & SPARK_F_ATTACHED_POS) != 0U) {
            return item->pos;
        }

        if ((spark->flags & SPARK_F_ATTACHED_NODE) != 0U) {
            XYZ_32 joint_pos = m_NodeOffsets[spark->node_num & 0xF].pos;
            Collide_GetJointAbsPosition(
                item, &joint_pos,
                m_NodeOffsets[spark->node_num & 0xF].mesh_num);
            return (XYZ_32) {
                .x = joint_pos.x + spark->pos.x,
                .y = joint_pos.y + spark->pos.y,
                .z = joint_pos.z + spark->pos.z,
            };
        }

        return (XYZ_32) {
            .x = item->pos.x + spark->pos.x,
            .y = item->pos.y + spark->pos.y,
            .z = item->pos.z + spark->pos.z,
        };
    }

    return spark->pos;
}

static int32_t M_GetFreeSpark(void)
{
    int32_t idx = m_NextSpark;
    for (int32_t i = 0; i < M_MAX_SPARKS; i++) {
        if (!m_Sparks[idx].on) {
            m_NextSpark = (idx + 1) & 0xBF;
            return idx;
        }
        idx = idx == (M_MAX_SPARKS - 1) ? 0 : idx + 1;
    }

    int32_t free = 0;
    int32_t min_life = INT32_MAX;
    for (int32_t i = 0; i < M_MAX_SPARKS; i++) {
        const SPARK *const spark = &m_Sparks[i];
        if ((int32_t)spark->life < min_life && spark->dynamic == -1
            && ((spark->flags & SPARK_F_BLOOD) == 0U || (i & 1) != 0)) {
            free = i;
            min_life = (int32_t)spark->life;
        }
    }

    m_NextSpark = (free + 1) & 0xBF;
    return free;
}

SPARK *Sparks_GetFreeSpark(void)
{
    const int32_t idx = M_GetFreeSpark();
    return &m_Sparks[idx];
}

SPARK *Sparks_GetSpark(const int32_t idx)
{
    ASSERT(idx >= 0 && idx < M_MAX_SPARKS);
    return &m_Sparks[idx];
}

int8_t Sparks_AllocDynamic(const uint8_t flags)
{
    for (int32_t i = 0; i < M_MAX_SPARK_DYNAMICS; i++) {
        if (!m_Dynamics[i].on) {
            m_Dynamics[i].on = true;
            m_Dynamics[i].falloff = 4;
            m_Dynamics[i].flags = flags;
            m_Dynamics[i].color = (RGB_888) { 0, 0, 0 };
            return (int8_t)i;
        }
    }
    return -1;
}

void Sparks_FreeDynamic(const int8_t idx)
{
    if (idx < 0 || idx >= M_MAX_SPARK_DYNAMICS) {
        return;
    }
    m_Dynamics[idx].on = false;
}

void Sparks_Init(void)
{
    for (int32_t i = 0; i < M_MAX_SPARKS; i++) {
        m_Sparks[i].on = false;
        m_Sparks[i].dynamic = -1;
    }
    for (int32_t i = 0; i < M_MAX_SPARK_DYNAMICS; i++) {
        m_Dynamics[i].on = false;
    }
    m_NextSpark = 0;
    m_SmokeWind = (XZ_32) {};
    m_HairWindZ = 0;
    m_TR3Wind = 0;
    m_TR3WindAngle = DEG_180;
    m_TR3DWindAngle = DEG_180;
}

XZ_32 Sparks_GetSmokeWind(void)
{
    return m_SmokeWind;
}

void Sparks_SetSmokeWind(const XZ_32 wind)
{
    m_SmokeWind = wind;
}

int32_t Sparks_GetHairWindZ(void)
{
    return m_HairWindZ;
}

static void M_UpdateWind(void)
{
    if (!g_Config.visuals.enable_breeze) {
        m_SmokeWind = (XZ_32) {};
        m_HairWindZ = 0;
        return;
    }

    if (g_TRVersion != 3) {
        const ITEM *const lara_item = Lara_GetItem();
        if (lara_item == nullptr) {
            m_HairWindZ = 0;
            return;
        }

        const ROOM *const room = Room_Get(lara_item->room_num);
        if (room == nullptr || !room->flags.wind) {
            m_HairWindZ = 0;
            return;
        }

        const int32_t random = Random_GetDraw() & 7;
        if (random != 0) {
            m_HairWindZ += random - 4;
            if (m_HairWindZ < 0) {
                m_HairWindZ = 0;
            } else if (m_HairWindZ >= 8) {
                m_HairWindZ--;
            }
        }
        m_SmokeWind = (XZ_32) {};
        return;
    }

    // TR3 wind logic: a small random wind magnitude with a slowly-changing
    // direction, biased to the [90°, 270°] range.
    m_TR3Wind += (Random_GetControl() & 7) - 3;
    if (m_TR3Wind <= -2) {
        m_TR3Wind++;
    } else if (m_TR3Wind >= 9) {
        m_TR3Wind--;
    }

    // Original TR3 uses a 0..4095 angle space; keep the calculations faithful.
    m_TR3DWindAngle =
        (m_TR3DWindAngle + (((Random_GetControl() & 0x3F) - 32) << 1)) & 0x1FFE;

    if (m_TR3DWindAngle < 1024) { // DEG_90
        m_TR3DWindAngle += (1024 - m_TR3DWindAngle) << 1;
    } else if (m_TR3DWindAngle > 3072) { // DEG_270
        m_TR3DWindAngle -= (m_TR3DWindAngle - 3072) << 1;
    }
    m_TR3DWindAngle &= 0x1FFE;
    m_TR3WindAngle =
        (m_TR3WindAngle + ((m_TR3DWindAngle - m_TR3WindAngle) >> 3)) & 0x1FFE;

    // Promote to DEG_360 for Math_Sin/Cos just at the end.
    m_SmokeWind = (XZ_32) {
        .x = (m_TR3Wind * Math_Sin(m_TR3WindAngle << 3)) >> W2V_SHIFT,
        .z = (m_TR3Wind * Math_Cos(m_TR3WindAngle << 3)) >> W2V_SHIFT,
    };

    m_HairWindZ = 0;
}

void Sparks_Control(void)
{
    M_UpdateWind();

    for (int32_t i = 0; i < M_MAX_SPARKS; i++) {
        SPARK *const spark = &m_Sparks[i];
        if (!spark->on) {
            continue;
        }

        if ((spark->flags & SPARK_F_ATTACHED_POS) == 0U || spark->life > 16) {
            if (spark->life > 0) {
                spark->life--;
            }
        }

        if (spark->life == 0) {
            if (spark->dynamic != -1) {
                m_Dynamics[(uint8_t)spark->dynamic].on = false;
                spark->dynamic = -1;
            }

            spark->on = false;
            continue;
        }

        const int32_t lived = (int32_t)spark->s_life - (int32_t)spark->life;

        // Color fade: src -> dst, then fade-to-black.
        if (lived < (int32_t)spark->col_fade_speed
            && spark->col_fade_speed != 0U) {
            const float fade = lived / (float)spark->col_fade_speed;
            spark->color.r = LERP(
                (int32_t)spark->src_color.r, (int32_t)spark->dst_color.r, fade);
            spark->color.g = LERP(
                (int32_t)spark->src_color.g, (int32_t)spark->dst_color.g, fade);
            spark->color.b = LERP(
                (int32_t)spark->src_color.b, (int32_t)spark->dst_color.b, fade);
        } else if (
            spark->life < spark->fade_to_black && spark->fade_to_black != 0U) {
            const float fade = spark->life / (float)spark->fade_to_black;
            spark->color.r = spark->dst_color.r * fade;
            spark->color.g = spark->dst_color.g * fade;
            spark->color.b = spark->dst_color.b * fade;
        } else {
            spark->color = spark->dst_color;
        }

        if (spark->life == spark->fade_to_black
            && (spark->flags & SPARK_F_UNDERWATER) != 0U) {
            spark->dst_size.width >>= 2;
            spark->dst_size.height >>= 2;
        }

        if ((spark->flags & SPARK_F_ROTATE) != 0U) {
            spark->rot_angle = (spark->rot_angle + spark->rot_add) & 0xFFF;
        }

        if ((spark->flags & SPARK_F_ALT_SPRITE) != 0U) {
            const OBJECT *const explosion = Object_Get(O_EXPLOSION_1);
            if (explosion->loaded) {
                const int32_t base = explosion->mesh_idx;
                if (spark->color.r < 16 && spark->color.g < 16
                    && spark->color.b < 16) {
                    spark->sprite_idx = base + 3;
                } else if (
                    spark->color.r < 64 && spark->color.g < 64
                    && spark->color.b < 64) {
                    spark->sprite_idx = base + 2;
                } else if (
                    spark->color.r < 96 && spark->color.g < 96
                    && spark->color.b < 96) {
                    spark->sprite_idx = base + 1;
                } else {
                    spark->sprite_idx = base;
                }
            }
        }

        if (lived == (int32_t)(spark->extras >> 3)
            && (spark->extras & 7U) != 0U) {
            int32_t uw = 0;
            if ((spark->flags & SPARK_F_UNDERWATER) != 0U) {
                uw = 1;
            } else if ((spark->flags & SPARK_F_GREEN) != 0U) {
                uw = 2;
            }

            const XYZ_32 spark_pos = Sparks_GetWorldPos(spark);

            for (int32_t j = 0; j < (int32_t)(spark->extras & 7U); j++) {
                Sparks_TriggerExplosionSparks(
                    spark_pos, (int32_t)(spark->extras & 7U) - 1,
                    spark->dynamic, uw, spark->room_num);
                spark->dynamic = -1;
            }

            if ((spark->flags & SPARK_F_UNDERWATER) != 0U) {
                Sparks_TriggerExplosionBubble(spark_pos, spark->room_num);
            }

            spark->extras = 0;
        }

        // Physics
        spark->vel.y += spark->gravity;
        if (spark->max_y_vel != 0) {
            const int32_t limit = (int32_t)spark->max_y_vel << 5;
            if ((spark->vel.y < 0 && spark->vel.y < limit)
                || (spark->vel.y > 0 && spark->vel.y > limit)) {
                spark->vel.y = limit;
            }
        }

        if ((spark->friction & 0x0FU) != 0U) {
            spark->vel.x -= spark->vel.x >> (spark->friction & 0x0FU);
            spark->vel.z -= spark->vel.z >> (spark->friction & 0x0FU);
        }

        if ((spark->friction & 0xF0U) != 0U) {
            spark->vel.y -= spark->vel.y >> (spark->friction >> 4);
        }

        spark->pos.x += spark->vel.x >> 5;
        spark->pos.y += spark->vel.y >> 5;
        spark->pos.z += spark->vel.z >> 5;

        if ((spark->flags & SPARK_F_OUTSIDE) != 0U) {
            spark->pos.x += m_SmokeWind.x >> 1;
            spark->pos.z += m_SmokeWind.z >> 1;
        }

        // Size lerp across lifetime.
        if (spark->s_life != 0U) {
            const float fade = lived / (float)spark->s_life;
            spark->size.width = LERP(
                (int32_t)spark->src_size.width, (int32_t)spark->dst_size.width,
                fade);
            spark->size.height = LERP(
                (int32_t)spark->src_size.height,
                (int32_t)spark->dst_size.height, fade);
        } else {
            spark->size = spark->src_size;
        }

        // If attached to a node, detach after a short random delay for some
        // node types.
        if ((spark->flags & (SPARK_F_ITEM | SPARK_F_ATTACHED_NODE))
                == (SPARK_F_ITEM | SPARK_F_ATTACHED_NODE)
            && (spark->node_num == 2 || spark->node_num == 3)) {
            const int32_t b = spark->node_num == 3 ? (Random_GetDraw() & 3) + 12
                                                   : (Random_GetDraw() & 3) + 8;
            if (lived > b) {
                spark->pos = Sparks_GetWorldPos(spark);
                spark->flags &= ~(SPARK_F_ATTACHED_NODE | SPARK_F_ITEM);
            }
        }
    }

    // Dynamic light pass.
    for (int32_t i = 0; i < M_MAX_SPARKS; i++) {
        const SPARK *const spark = &m_Sparks[i];
        if (!spark->on || spark->dynamic == -1) {
            continue;
        }

        M_SPARK_DYNAMIC *const dl = &m_Dynamics[(uint8_t)spark->dynamic];
        if (!dl->on) {
            continue;
        }

        const XYZ_32 world_pos = Sparks_GetWorldPos(spark);
        const int32_t rnd = Random_GetControl();
        XYZ_32 pos = {
            .x = world_pos.x + ((rnd & 0xF) << 4),
            .y = world_pos.y + (rnd & 0xF0),
            .z = world_pos.z + ((rnd >> 4) & 0xF0),
        };

        int32_t falloff = (int32_t)spark->s_life - (int32_t)spark->life - 1;
        int32_t r = 0;
        int32_t g = 0;
        int32_t b = 0;

        if (falloff < 2) {
            if (dl->falloff < 28) {
                dl->falloff = (uint8_t)MIN((int32_t)dl->falloff + 6, 255);
            }
            r = 255 - (rnd & 0x1F) - (falloff << 3);
            g = 255 - (rnd & 0x1F) - (falloff << 4);
            b = 255 - (rnd & 0x1F) - (falloff << 6);
        } else if (falloff < 4) {
            if (dl->falloff < 28) {
                dl->falloff = (uint8_t)MIN((int32_t)dl->falloff + 6, 255);
            }
            r = 255 - (rnd & 0x1F) - (falloff << 3);
            g = 128 - (falloff << 3);
            b = (4 - falloff) << 2;
            if (b < 0) {
                b = 0;
            } else {
                b <<= 3;
            }
        } else {
            if (dl->falloff != 0U) {
                dl->falloff--;
            }
            r = (rnd & 0x1F) + 224;
            g = ((rnd >> 4) & 0x1F) + 128;
            b = (rnd >> 8) & 0x3F;
        }

        falloff = (int32_t)dl->falloff;
        if (falloff > 31) {
            falloff = 31;
        }

        Output_AddDynamicLightRGB(pos, falloff, (RGB_888) { r, g, b });
    }
}

void Sparks_Draw(void)
{
    for (int32_t i = 0; i < M_MAX_SPARKS; i++) {
        SPARK *const spark = &m_Sparks[i];
        if (!spark->on) {
            continue;
        }

        OutputSource_PolyFX_StageSpark(spark);
    }
}

void Sparks_DetachEffect(const int16_t effect_num)
{
    for (int32_t i = 0; i < M_MAX_SPARKS; i++) {
        SPARK *const spark = &m_Sparks[i];
        if (!spark->on) {
            continue;
        }

        if ((spark->flags & SPARK_F_FX) != 0U
            && spark->effect_num == effect_num) {
            if ((spark->flags & SPARK_F_ATTACHED_POS) != 0U) {
                spark->on = false;
                continue;
            }

            const EFFECT *const effect = Effect_Get(effect_num);
            spark->pos.x += effect->pos.x;
            spark->pos.y += effect->pos.y;
            spark->pos.z += effect->pos.z;
            spark->flags &= ~SPARK_F_FX;
        }
    }
}

void Sparks_DetachItem(const int16_t item_num)
{
    for (int32_t i = 0; i < M_MAX_SPARKS; i++) {
        SPARK *const spark = &m_Sparks[i];
        if (!spark->on) {
            continue;
        }

        if ((spark->flags & SPARK_F_ITEM) != 0U
            && spark->item_num == item_num) {
            if ((spark->flags & SPARK_F_ATTACHED_POS) != 0U) {
                spark->on = false;
                continue;
            }

            const ITEM *const item = Item_Get(item_num);
            spark->pos.x += item->pos.x;
            spark->pos.y += item->pos.y;
            spark->pos.z += item->pos.z;
            spark->flags &= ~SPARK_F_ITEM;
            spark->flags &= ~SPARK_F_ATTACHED_NODE;
        }
    }
}

void Sparks_TriggerBubble(
    const int32_t x, const int32_t y, const int32_t z, const int32_t size,
    const int32_t size_range, const int16_t effect_num)
{
    const ITEM *const lara_item = Lara_GetItem();
    const int32_t dx = lara_item->pos.x - x;
    const int32_t dz = lara_item->pos.z - z;
    const int32_t max_dist = 16 * WALL_L;
    if (dx < -max_dist || dx > max_dist || dz < -max_dist || dz > max_dist) {
        return;
    }

    const OBJECT *const bubble_obj = Object_Get(O_BUBBLE_1);
    if (!bubble_obj->loaded) {
        return;
    }

    const int32_t idx = M_GetFreeSpark();
    SPARK *const spark = &m_Sparks[idx];
    *spark = (SPARK) {
        .on = true,
        .src_color = { 0, 0, 0 },
        .dst_color = { 144, 144, 144 },
        .color = { 0, 0, 0 },
        .fade_to_black = 2,
        .draw_type = DRAW_BLEND_ADD,
        .col_fade_speed = 4,
        .life = 128,
        .s_life = 128,
        .flags =
            SPARK_F_ATTACHED_POS | SPARK_F_FX | SPARK_F_SPRITE | SPARK_F_SCALE,
        .effect_num = effect_num,
        .sprite_idx = bubble_obj->mesh_idx,
        .pos = { .x = 0, .y = 0, .z = 0 },
        .vel = { .x = 0, .y = 0, .z = 0 },
        .gravity = 0,
        .max_y_vel = 0,
        .friction = 0,
        .scalar = 0,
        .dynamic = -1,
    };

    const int32_t safe_range = size_range > 0 ? size_range : 1;
    int32_t full_size = (Random_GetControl() % safe_range) + size;
    CLAMP(full_size, 0, 255);

    const uint8_t base = (uint8_t)full_size;
    const uint8_t dst = (uint8_t)(base << 3);
    spark->src_size.width = base;
    spark->src_size.height = base;
    spark->dst_size.width = dst;
    spark->dst_size.height = dst;
    spark->size = spark->src_size;
}

void Sparks_TriggerWaterfallMist(
    const int32_t x, const int32_t y, const int32_t z, const int32_t angle)
{
    const OBJECT *const explosion = Object_Get(O_EXPLOSION_1);
    if (explosion == nullptr || !explosion->loaded) {
        return;
    }

    static const int32_t offsets[] = { 576, 203, -203, -576 };

    for (int32_t i = 0; i < (int32_t)ARRAY_SIZE(offsets); i++) {
        SPARK *const spark = Sparks_GetFreeSpark();

        const int32_t offset = (Random_GetControl() & 0x1F) + offsets[i] - 16;
        const int32_t c = Math_Cos(angle) >> W2V_SHIFT;
        const int32_t s = Math_Sin(angle) >> W2V_SHIFT;

        *spark = (SPARK) {
            .on = true,
            .color = { 128, 128, 128 },
            .src_color = { 128, 128, 128 },
            .dst_color = { 192, 192, 192 },
            .col_fade_speed = 2,
            .fade_to_black = 4,
            .draw_type = DRAW_BLEND_ADD,
            .extras = 0,
            .life = (uint8_t)((Random_GetControl() & 3) + 6),
            .dynamic = -1,
            .sprite_idx = explosion->mesh_idx,
            .pos = {
                .x = x + (Random_GetControl() % 16) - 8 + c * offset,
                .y = y + (Random_GetControl() % 16) - 8,
                .z = z + (Random_GetControl() % 16) - 8 + s * offset,
            },
            .vel = {
                .x = s,
                .y = 0,
                .z = c,
            },
            .gravity = 0,
            .max_y_vel = 0,
            .friction = 3,
            .flags = SPARK_F_SPRITE | SPARK_F_ALT_SPRITE | SPARK_F_SCALE,
            .scalar = 6,
        };
        spark->s_life = spark->life;

        if ((Random_GetControl() & 1) != 0) {
            spark->flags |= SPARK_F_ROTATE;
            spark->rot_angle = (uint16_t)(Random_GetControl() & 0xFFF);
            if ((Random_GetControl() & 1) != 0) {
                spark->rot_add = -16 - (Random_GetControl() % 16);
            } else {
                spark->rot_add = 16 + (Random_GetControl() % 16);
            }
        }

        const uint8_t dst_size = (uint8_t)((Random_GetControl() & 7) + 12);
        const uint8_t src_size = (uint8_t)(dst_size >> 1);
        spark->src_size.width = src_size;
        spark->src_size.height = src_size;
        spark->dst_size.width = dst_size;
        spark->dst_size.height = dst_size;
        spark->size = spark->src_size;
    }
}

void Sparks_TriggerBreath(
    const XYZ_32 pos, const XYZ_32 vel, const int16_t room_num)
{
    const OBJECT *const object = Object_Get(O_EXPLOSION_1);
    if (object == nullptr || !object->loaded) {
        return;
    }

    SPARK *const spark = Sparks_GetFreeSpark();
    const int32_t jitter_x = (Random_GetControl() & 0xF) - 8;
    const int32_t jitter_y = (Random_GetControl() & 0xF) - 8;
    const int32_t jitter_z = (Random_GetControl() & 0xF) - 8;

    *spark = (SPARK) {
        .on = true,
        .src_color = { 0, 0, 0 },
        .dst_color = { 32, 32, 32 },
        .color = { 0, 0, 0 },
        .col_fade_speed = 4,
        .fade_to_black = 32,
        .draw_type = DRAW_BLEND_ADD,
        .extras = 0,
        .life = (uint8_t)((Random_GetControl() & 3) + 37),
        .dynamic = -1,
        .sprite_idx = object->mesh_idx,
        .pos = {
            .x = pos.x + jitter_x,
            .y = pos.y + jitter_y,
            .z = pos.z + jitter_z,
        },
        .vel = vel,
        .gravity = 0,
        .max_y_vel = 0,
        .friction = 0,
        .flags = SPARK_F_SPRITE | SPARK_F_ALT_SPRITE | SPARK_F_SCALE,
        .scalar = 3,
        .room_num = room_num,
    };
    spark->s_life = spark->life;

    const ROOM *const room = Room_Get(room_num);
    if (room != nullptr && room->flags.wind) {
        spark->flags |= SPARK_F_OUTSIDE;
    }

    const uint8_t dst_size = ((Random_GetControl() & 7) + 32);
    const uint8_t src_size = (dst_size >> 3);
    spark->src_size.width = src_size;
    spark->src_size.height = src_size;
    spark->dst_size.width = dst_size;
    spark->dst_size.height = dst_size;
    spark->size = spark->src_size;
}

void Sparks_TriggerFireFlame(
    const XYZ_32 pos, const int32_t body_part, const int32_t type)
{
    const ITEM *const lara_item = Lara_GetItem();
    const int32_t dx = lara_item->pos.x - pos.x;
    const int32_t dz = lara_item->pos.z - pos.z;
    const int32_t max_dist = 20 * WALL_L;
    if (dx < -max_dist || dx > max_dist || dz < -max_dist || dz > max_dist) {
        return;
    }

    SPARK *const spark = Sparks_GetFreeSpark();
    spark->on = true;

    if (type == 2) {
        spark->src_color.r = (Random_GetControl() & 0x1F) + 48;
        spark->src_color.g = spark->src_color.r;
        spark->src_color.b = (Random_GetControl() & 0x3F) - 64;
    } else if (type == 254) {
        spark->src_color.r = 48;
        spark->src_color.g = 255;
        spark->src_color.b = (Random_GetControl() & 0x1F) + 48;
        spark->dst_color.r = 32;
        spark->dst_color.g = (Random_GetControl() & 0x3F) - 64;
        spark->dst_color.b = (Random_GetControl() & 0x3F) + 128;
    } else {
        spark->src_color.r = 255;
        spark->src_color.g = (Random_GetControl() & 0x1F) + 48;
        spark->src_color.b = 48;
    }

    if (type != 254) {
        spark->dst_color.r = (Random_GetControl() & 0x3F) - 64;
        spark->dst_color.b = 32;
        spark->dst_color.g = (Random_GetControl() & 0x3F) + 128;
    }

    if (body_part == -1) {
        if (type == 2 || type == 255 || type == 254) {
            spark->fade_to_black = 6;
            spark->col_fade_speed = (Random_GetControl() & 3) + 5;
            spark->life = (type < 254 ? 0 : 8) + (Random_GetControl() & 3) + 16;
            spark->s_life = spark->life;
        } else {
            spark->fade_to_black = 8;
            spark->col_fade_speed = (Random_GetControl() & 3) + 20;
            spark->life = (Random_GetControl() & 7) + 40;
            spark->s_life = spark->life;
        }
    } else {
        spark->fade_to_black = 16;
        spark->col_fade_speed = (Random_GetControl() & 3) + 8;
        spark->life = (Random_GetControl() & 3) + 28;
        spark->s_life = spark->life;
    }

    spark->draw_type = DRAW_BLEND_ADD;
    spark->extras = 0;
    spark->dynamic = -1;

    if (body_part != -1) {
        spark->pos.x = (Random_GetControl() & 0x1F) - 16;
        spark->pos.y = 0;
        spark->pos.z = (Random_GetControl() & 0x1F) - 16;
    } else {
        spark->pos = pos;
        if (type == 0 || type == 1) {
            spark->pos.x += (Random_GetControl() & 0x1F) - 16;
            spark->pos.z += (Random_GetControl() & 0x1F) - 16;
        } else if (type >= 254) {
            spark->pos.x += (Random_GetControl() & 0x3F) - 32;
            spark->pos.z += (Random_GetControl() & 0x3F) - 32;
        } else {
            spark->pos.x += (Random_GetControl() & 0xF) - 8;
            spark->pos.z += (Random_GetControl() & 0xF) - 8;
        }
    }

    if (type == 2) {
        spark->vel.x = (Random_GetControl() & 0x1F) - 16;
        spark->vel.y = -1024 - (Random_GetControl() & 0x1FF);
        spark->vel.z = (Random_GetControl() & 0x1F) - 16;
        spark->friction = 68;
    } else {
        spark->vel.x = (Random_GetControl() & 0xFF) - 128;
        spark->vel.y = -16 - (Random_GetControl() & 0xF);
        spark->vel.z = (Random_GetControl() & 0xFF) - 128;

        if (type == 1) {
            spark->friction = 51;
        } else {
            spark->friction = 5;
        }
    }

    if (Random_GetControl() & 1) {
        if (body_part == -1) {
            spark->gravity = -16 - (Random_GetControl() & 0x1F);
            spark->flags = SPARK_F_ALT_SPRITE | SPARK_F_ROTATE | SPARK_F_SPRITE
                | SPARK_F_SCALE;
            spark->max_y_vel = -16 - (Random_GetControl() & 7);
        } else {
            spark->flags = SPARK_F_ALT_SPRITE | SPARK_F_FX | SPARK_F_ROTATE
                | SPARK_F_SPRITE | SPARK_F_SCALE;
            spark->item_num = body_part;
            spark->gravity = -32 - (Random_GetControl() & 0x3F);
            spark->max_y_vel = -24 - (Random_GetControl() & 7);
        }

        spark->rot_angle = Random_GetControl() & 0xFFF;

        if (Random_GetControl() & 1) {
            spark->rot_add = -16 - (Random_GetControl() & 0xF);
        } else {
            spark->rot_add = (Random_GetControl() & 0xF) + 16;
        }
    } else if (body_part == -1) {
        spark->flags = SPARK_F_ALT_SPRITE | SPARK_F_SPRITE | SPARK_F_SCALE;
        spark->gravity = -16 - (Random_GetControl() & 0x1F);
        spark->max_y_vel = -16 - (Random_GetControl() & 7);
    } else {
        spark->flags =
            SPARK_F_ALT_SPRITE | SPARK_F_FX | SPARK_F_SPRITE | SPARK_F_SCALE;
        spark->item_num = body_part;
        spark->gravity = -32 - (Random_GetControl() & 0x3F);
        spark->max_y_vel = -24 - (Random_GetControl() & 7);
    }

    spark->scalar = 2;
    spark->sprite_idx = Object_Get(O_EXPLOSION_1)->mesh_idx;

    uint8_t size;
    if (type == 0) {
        size = (Random_GetControl() & 0x1F) + 128;
    } else if (type == 1) {
        size = (Random_GetControl() & 0x1F) + 64;
    } else if (type < 254) {
        spark->max_y_vel = 0;
        spark->gravity = 0;
        size = (Random_GetControl() & 0x1F) + 32;
    } else {
        size = (Random_GetControl() & 0xF) + 48;
    }

    spark->src_size.width = size;
    spark->src_size.height = size;
    spark->size.width = size;
    spark->size.height = size;

    if (type == 2) {
        spark->dst_size.width = size >> 2;
        spark->dst_size.height = size >> 2;
    } else {
        spark->dst_size.width = size >> 4;
        spark->dst_size.height = size >> 4;
    }
}

void Sparks_TriggerFireSmoke(
    const XYZ_32 pos, const int32_t body_part, const int32_t type)
{
    const ITEM *const lara_item = Lara_GetItem();
    const int32_t dx = lara_item->pos.x - pos.x;
    const int32_t dz = lara_item->pos.z - pos.z;
    const int32_t max_dist = 20 * WALL_L;
    if (dx < -max_dist || dx > max_dist || dz < -max_dist || dz > max_dist) {
        return;
    }

    SPARK *const spark = Sparks_GetFreeSpark();
    spark->on = true;
    spark->src_color.r = 0;
    spark->src_color.g = 0;
    spark->src_color.b = 0;
    spark->dst_color.r = 32;
    spark->dst_color.g = 32;
    spark->dst_color.b = 32;

    if (body_part == -1) {
        if (type == 255) {
            spark->fade_to_black = 8;
            spark->col_fade_speed = (Random_GetControl() & 3) + 16;
            spark->life = (Random_GetControl() & 7) + 28;
            spark->s_life = spark->life;
        } else {
            spark->fade_to_black = 16;
            spark->col_fade_speed = (Random_GetControl() & 7) + 32;
            spark->life = (Random_GetControl() & 0xF) + 57;
            spark->s_life = spark->life;
        }
    } else {
        spark->fade_to_black = 12;
        spark->col_fade_speed = (Random_GetControl() & 3) + 4;
        spark->life = (Random_GetControl() & 3) + 20;
        spark->s_life = spark->life;
    }

    spark->draw_type = DRAW_BLEND_ADD;
    spark->extras = 0;
    spark->dynamic = -1;
    spark->pos.x = pos.x + (Random_GetControl() & 0xF) - 8;
    spark->pos.y = pos.y - (Random_GetControl() & 0x7F) - 256;
    spark->pos.z = pos.z + (Random_GetControl() & 0xF) - 8;
    spark->vel.x = (Random_GetControl() & 0xFF) - 128;
    spark->vel.y = -16 - (Random_GetControl() & 0xF);
    spark->vel.z = (Random_GetControl() & 0xFF) - 128;
    spark->friction = 4;

    if (Random_GetControl() & 1) {
        spark->flags = SPARK_F_ALT_SPRITE | SPARK_F_ROTATE | SPARK_F_SPRITE
            | SPARK_F_SCALE;
        spark->rot_angle = Random_GetControl() & 0xFFF;

        if (Random_GetControl() & 1) {
            spark->rot_add = -16 - (Random_GetControl() & 0xF);
        } else {
            spark->rot_add = (Random_GetControl() & 0xF) + 16;
        }
    } else {
        spark->flags = SPARK_F_ALT_SPRITE | SPARK_F_SPRITE | SPARK_F_SCALE;
    }

    spark->scalar = 3;
    spark->sprite_idx = Object_Get(O_EXPLOSION_1)->mesh_idx;
    spark->gravity = -16 - (Random_GetControl() & 0xF);
    spark->max_y_vel = -8 - (Random_GetControl() & 7);
    spark->dst_size.width = (Random_GetControl() & 0x3F) + 64;
    spark->src_size.width = spark->dst_size.width >> 2;
    spark->size.width = spark->src_size.width;
    spark->src_size.height = spark->src_size.width;
    spark->size.height = spark->src_size.width;
    spark->dst_size.height = spark->dst_size.width;
}

void Sparks_TriggerStaticFlame(const XYZ_32 pos, const int32_t size)
{
    const ITEM *const lara_item = Lara_GetItem();
    const int32_t dx = lara_item->pos.x - pos.x;
    const int32_t dz = lara_item->pos.z - pos.z;
    const int32_t max_dist = 20 * WALL_L;
    if (dx < -max_dist || dx > max_dist || dz < -max_dist || dz > max_dist) {
        return;
    }

    SPARK *const spark = Sparks_GetFreeSpark();
    spark->on = true;
    spark->src_color.r = (Random_GetControl() & 0x3F) - 64;
    spark->src_color.g = (Random_GetControl() & 0x3F) + 128;
    spark->src_color.b = 64;
    spark->dst_color.r = spark->src_color.r;
    spark->dst_color.g = spark->src_color.g;
    spark->dst_color.b = 64;
    spark->col_fade_speed = 1;
    spark->fade_to_black = 0;
    spark->life = 2;
    spark->s_life = 2;
    spark->draw_type = DRAW_BLEND_ADD;
    spark->extras = 0;
    spark->dynamic = -1;
    spark->pos.x = pos.x + (Random_GetControl() & 7) - 4;
    spark->pos.y = pos.y;
    spark->pos.z = pos.z + (Random_GetControl() & 7) - 4;
    spark->max_y_vel = 0;
    spark->gravity = 0;
    spark->friction = 0;
    spark->vel.z = 0;
    spark->vel.y = 0;
    spark->vel.x = 0;
    spark->flags = SPARK_F_ALT_SPRITE | SPARK_F_SPRITE | SPARK_F_SCALE;
    spark->sprite_idx = Object_Get(O_EXPLOSION_1)->mesh_idx;
    spark->scalar = 2;
    spark->dst_size.width = size;
    spark->dst_size.height = size;
    spark->src_size.height = size;
    spark->src_size.width = size;
    spark->size.height = size;
    spark->size.width = size;
}

void Sparks_TriggerSideFlame(
    const XYZ_32 pos, const int32_t angle, const int32_t speed,
    const bool pilot)
{
    const ITEM *const lara_item = Lara_GetItem();
    const int32_t dx = lara_item->pos.x - pos.x;
    const int32_t dz = lara_item->pos.z - pos.z;
    const int32_t max_dist = 20 * WALL_L;
    if (dx < -max_dist || dx > max_dist || dz < -max_dist || dz > max_dist) {
        return;
    }

    SPARK *const spark = Sparks_GetFreeSpark();
    spark->on = true;
    spark->src_color.r = (Random_GetControl() & 0x1F) + 48;
    spark->src_color.g = spark->src_color.r;
    spark->src_color.b = (Random_GetControl() & 0x3F) - 64;
    spark->dst_color.r = (Random_GetControl() & 0x3F) - 64;
    spark->dst_color.g = (Random_GetControl() & 0x3F) + 0x80;
    spark->dst_color.b = 32;
    spark->fade_to_black = 8;
    spark->col_fade_speed = (Random_GetControl() & 3) + 12;
    spark->draw_type = DRAW_BLEND_ADD;
    spark->extras = 0;
    spark->life = (Random_GetControl() & 7) + 28;
    spark->s_life = spark->life;
    spark->dynamic = -1;
    spark->pos.x = pos.x + (Random_GetControl() & 0x1F) - 16;
    spark->pos.y = pos.y + (Random_GetControl() & 0x1F) - 16;
    spark->pos.z = pos.z + (Random_GetControl() & 0x1F) - 16;

    int32_t dist;
    if (pilot) {
        dist = (speed << 7) + (Random_GetControl() & 0x1F);
    } else {
        dist = (speed << 8) + (Random_GetControl() & 0x1FF);
    }
    dist <<= 1;

    const int32_t s = (dist * Math_Sin(angle)) >> W2V_SHIFT;
    const int32_t c = (dist * Math_Cos(angle)) >> W2V_SHIFT;
    spark->vel.x = (int16_t)((Random_GetControl() & 0x7F) + s - 64);
    spark->vel.y = -6 - (Random_GetControl() & 7);
    spark->vel.z = (int16_t)((Random_GetControl() & 0x7F) + c - 64);
    spark->friction = 4;
    spark->flags = SPARK_F_ALT_SPRITE | SPARK_F_SPRITE | SPARK_F_SCALE;
    spark->gravity = -8 - (Random_GetControl() & 0xF);
    spark->max_y_vel = -8 - (Random_GetControl() & 7);
    spark->sprite_idx = Object_Get(O_EXPLOSION_1)->mesh_idx;
    spark->scalar = 3;

    int32_t size = (Random_GetControl() & 0x1F) + 128;
    if (pilot) {
        size >>= 2;
    }

    spark->dst_size.width = size;
    spark->dst_size.height = size;
    spark->src_size.width = size >> 1;
    spark->src_size.height = size >> 1;
    spark->size.width = size >> 1;
    spark->size.height = size >> 1;
}

void Sparks_TriggerUnderwaterExplosion(const ITEM *item)
{
    if (item == nullptr) {
        return;
    }

    Sparks_TriggerExplosionBubble(item->pos, item->room_num);
    Sparks_TriggerExplosionSparks(item->pos, 2, -2, 1, item->room_num);

    for (int32_t i = 0; i < 3; i++) {
        Sparks_TriggerExplosionSparks(item->pos, 2, -1, 1, item->room_num);
    }

    const int32_t water_height = Room_GetWaterHeight(
        item->pos.x, item->pos.y, item->pos.z, item->room_num);
    if (water_height == NO_HEIGHT) {
        return;
    }

    int32_t y = item->pos.y - water_height;
    if (y >= 2048) {
        return;
    }

    const int32_t wh = 2048 - y;
    y = wh >> 6;

    const ROOM *const room = Room_Get(item->room_num);
    WaterFX_SetupSplash(&(WATER_FX_SPLASH_SETUP) {
        .x = item->pos.x,
        .y = room->max_ceiling,
        .z = item->pos.z,
        .inner_y_size = -96,
        .inner_xz_vel = 160,
        .inner_gravity = 96,
        .inner_xz_off = y + 16,
        .inner_xz_size = y + 12,
        .inner_friction = 7,
        .inner_y_vel = (-512 - wh) << 3,
        .middle_xz_off = y + 24,
        .middle_xz_size = y + 24,
        .middle_y_size = -64,
        .middle_xz_vel = 224,
        .middle_y_vel = (-768 - wh) << 2,
        .middle_gravity = 56,
        .middle_friction = 8,
        .outer_xz_off = y + 32,
        .outer_xz_size = y + 32,
        .outer_xz_vel = 272,
        .outer_friction = 9,
    });
}

void Sparks_TriggerExplosionSparks(
    XYZ_32 pos, int32_t extras, int32_t dynamic, int32_t uw, int16_t room_num)
{
    const ITEM *const lara_item = Lara_GetItem();
    const int32_t dx = lara_item->pos.x - pos.x;
    const int32_t dz = lara_item->pos.z - pos.z;
    const int32_t max_dist = 30 * WALL_L;
    if (dx < -max_dist || dx > max_dist || dz < -max_dist || dz > max_dist) {
        return;
    }

    const OBJECT *const explosion = Object_Get(O_EXPLOSION_1);
    if (explosion == nullptr || !explosion->loaded) {
        return;
    }

    int32_t safe_extras = extras;
    CLAMP(safe_extras, 0, 3);
    static const uint8_t extras_table[4] = { 0, 4, 7, 10 };

    SPARK *const spark = Sparks_GetFreeSpark();
    *spark = (SPARK) {
        .on = true,
        .src_color = { 255, 0, 0 },
        .dst_color = { 0, 0, 0 },
        .color = { 0, 0, 0 },
        .draw_type = DRAW_BLEND_ADD,
        .extras = (uint8_t)(
            safe_extras
            | ((extras_table[safe_extras] + (Random_GetControl() & 7) - 4)
               << 3)),
        .life = 0,
        .dynamic = (int8_t)dynamic,
        .sprite_idx = explosion->mesh_idx,
        .pos = pos,
        .vel = {
            .x = (Random_GetControl() & 0xFFF) - 2048,
            .y = (Random_GetControl() & 0xFFF) - 2048,
            .z = (Random_GetControl() & 0xFFF) - 2048,
        },
        .gravity = 0,
        .max_y_vel = 0,
        .friction = 0,
        .flags = SPARK_F_SPRITE | SPARK_F_SCALE,
        .scalar = 3,
        .room_num = (uint8_t)room_num,
    };

    if (uw == 1) {
        spark->src_color.g = (uint8_t)((Random_GetControl() & 0x3F) + 128);
        spark->src_color.b = 32;
        spark->dst_color.r = 192;
        spark->dst_color.g = (uint8_t)((Random_GetControl() & 0x1F) + 64);
        spark->dst_color.b = 0;
        spark->col_fade_speed = 7;
        spark->fade_to_black = 8;
        spark->life = (uint8_t)((Random_GetControl() & 7) + 16);
        spark->flags |= SPARK_F_UNDERWATER;
    } else {
        spark->src_color.g = (uint8_t)((Random_GetControl() & 0xF) + 32);
        spark->src_color.b = 0;
        spark->dst_color.r = (uint8_t)((Random_GetControl() & 0x3F) + 192);
        spark->dst_color.g = (uint8_t)((Random_GetControl() & 0x3F) + 128);
        spark->dst_color.b = 32;
        spark->col_fade_speed = 8;
        spark->fade_to_black = 16;
        spark->life = (uint8_t)((Random_GetControl() & 7) + 24);
    }
    spark->s_life = spark->life;

    if (dynamic == -2) {
        spark->dynamic = Sparks_AllocDynamic(uw == 1 ? 2 : 1);
    }

    if (dynamic != -2 || uw == 1) {
        spark->pos.x = pos.x + (Random_GetControl() & 0x1F) - 16;
        spark->pos.y = pos.y + (Random_GetControl() & 0x1F) - 16;
        spark->pos.z = pos.z + (Random_GetControl() & 0x1F) - 16;
    } else {
        spark->pos.x = pos.x + (Random_GetControl() & 0x1FF) - 256;
        spark->pos.y = pos.y + (Random_GetControl() & 0x1FF) - 256;
        spark->pos.z = pos.z + (Random_GetControl() & 0x1FF) - 256;
    }

    spark->friction = (uint8_t)(uw == 1 ? 0x11 : 0x33);

    spark->flags |= SPARK_F_ALT_SPRITE;
    if ((Random_GetControl() & 1) != 0) {
        spark->flags |= SPARK_F_ROTATE;
        spark->rot_angle = (uint16_t)(Random_GetControl() & 0xFFF);
        const int32_t rot_add = (Random_GetControl() & 0x7F) + 32;
        spark->rot_add = (int8_t)MIN(rot_add, 127);
    }

    spark->src_size.width = (uint8_t)((Random_GetControl() & 0xF) + 40);
    spark->src_size.height =
        (uint8_t)(spark->src_size.width + (Random_GetControl() & 7) + 8);
    spark->dst_size.width = (uint8_t)(spark->src_size.width << 1);
    spark->dst_size.height = (uint8_t)(spark->src_size.height << 1);
    spark->size = spark->src_size;

    if (uw == 2) {
        const RGB_888 src = spark->src_color;
        const RGB_888 dst = spark->dst_color;
        spark->src_color = (RGB_888) { src.b, src.r, src.g };
        spark->dst_color = (RGB_888) { dst.b, dst.r, dst.g };
        spark->flags |= SPARK_F_GREEN;
    } else if (extras != 0) {
        Sparks_TriggerExplosionSmoke(pos, uw, room_num);
    } else {
        Sparks_TriggerExplosionSmokeEnd(pos, uw, room_num);
    }
}

void Sparks_TriggerExplosionBubble(const XYZ_32 pos, const int16_t room_num)
{
    const ITEM *const lara_item = Lara_GetItem();
    const int32_t dx = lara_item->pos.x - pos.x;
    const int32_t dz = lara_item->pos.z - pos.z;
    const int32_t max_dist = 30 * WALL_L;
    if (dx < -max_dist || dx > max_dist || dz < -max_dist || dz > max_dist) {
        return;
    }

    const OBJECT *const explosion = Object_Get(O_EXPLOSION_1);
    if (explosion == nullptr || !explosion->loaded) {
        return;
    }

    SPARK *const spark = Sparks_GetFreeSpark();
    *spark = (SPARK) {
        .on = true,
        .src_color = { 128, 64, 0 },
        .dst_color = { 128, 128, 128 },
        .color = { 0, 0, 0 },
        .col_fade_speed = 8,
        .fade_to_black = 12,
        .life = 24,
        .s_life = 24,
        .draw_type = DRAW_BLEND_ADD,
        .extras = 0,
        .dynamic = -1,
        .sprite_idx = explosion->mesh_idx,
        .pos = pos,
        .vel = { .x = 0, .y = 0, .z = 0 },
        .gravity = 0,
        .max_y_vel = 0,
        .friction = 0,
        .flags = SPARK_F_UNDERWATER | SPARK_F_SPRITE | SPARK_F_SCALE,
        .scalar = 3,
        .room_num = (uint8_t)room_num,
    };

    const uint8_t size = (uint8_t)((Random_GetControl() & 7) + 63);
    spark->src_size.width = (uint8_t)(size >> 1);
    spark->src_size.height = spark->src_size.width;
    spark->dst_size.width = (uint8_t)(size << 1);
    spark->dst_size.height = spark->dst_size.width;
    spark->size = spark->src_size;

    for (int32_t i = 0; i < 7; i++) {
        const XYZ_32 bubble_pos = {
            .x = (Random_GetControl() & 0x1FF) + pos.x - 256,
            .y = (Random_GetControl() & 0x7F) + pos.y - 64,
            .z = (Random_GetControl() & 0x1FF) + pos.z - 256,
        };
        Spawn_BubbleEx(&bubble_pos, room_num, 6, 15);
    }
}

void Sparks_TriggerExplosionSmoke(
    const XYZ_32 pos, const bool uw, const int16_t room_num)
{
    const ITEM *const lara_item = Lara_GetItem();
    const int32_t dx = lara_item->pos.x - pos.x;
    const int32_t dz = lara_item->pos.z - pos.z;
    const int32_t max_dist = 30 * WALL_L;
    if (dx < -max_dist || dx > max_dist || dz < -max_dist || dz > max_dist) {
        return;
    }

    SPARK *const spark = Sparks_GetFreeSpark();
    spark->on = true;
    spark->src_color.r = 144;
    spark->src_color.g = 144;
    spark->src_color.b = 144;
    spark->dst_color.r = 64;
    spark->dst_color.g = 64;
    spark->dst_color.b = 64;
    spark->col_fade_speed = 2;
    spark->fade_to_black = 8;
    spark->draw_type = DRAW_BLEND_SUB;
    spark->extras = 0;
    spark->life = (uint8_t)((Random_GetControl() & 3) + 10);
    spark->s_life = spark->life;
    spark->dynamic = -1;
    spark->pos.x = (Random_GetControl() & 0x1FF) + pos.x - 256;
    spark->pos.y = (Random_GetControl() & 0x1FF) + pos.y - 256;
    spark->pos.z = (Random_GetControl() & 0x1FF) + pos.z - 256;
    spark->vel.x = ((Random_GetControl() & 0xFFF) - 2048) >> 2;
    spark->vel.y = (Random_GetControl() & 0xFF) - 128;
    spark->vel.z = ((Random_GetControl() & 0xFFF) - 2048) >> 2;

    if (uw) {
        spark->friction = 2;
    } else {
        spark->friction = 6;
    }

    spark->flags =
        SPARK_F_ALT_SPRITE | SPARK_F_ROTATE | SPARK_F_SPRITE | SPARK_F_SCALE;
    spark->rot_angle = Random_GetControl() & 0xFFF;
    spark->rot_add = (Random_GetControl() & 0xF) + 16;
    spark->sprite_idx = Object_Get(O_EXPLOSION_1)->mesh_idx;
    spark->scalar = 1;
    spark->gravity = -3 - (Random_GetControl() & 3);
    spark->max_y_vel = -4 - (Random_GetControl() & 3);
    spark->dst_size.width = (Random_GetControl() & 0x1F) + 128;
    spark->size.width = spark->dst_size.width >> 2;
    spark->src_size.width = spark->size.width;
    spark->dst_size.height =
        spark->dst_size.width + (Random_GetControl() & 0x1F) + 32;
    spark->size.height = spark->dst_size.height >> 3;
    spark->src_size.height = spark->size.height;
}

void Sparks_TriggerExplosionSmokeEnd(
    const XYZ_32 pos, const bool uw, const int16_t room_num)
{
    const ITEM *const lara_item = Lara_GetItem();
    const int32_t dx = lara_item->pos.x - pos.x;
    const int32_t dz = lara_item->pos.z - pos.z;
    const int32_t max_dist = 30 * WALL_L;
    if (dx < -max_dist || dx > max_dist || dz < -max_dist || dz > max_dist) {
        return;
    }

    SPARK *const spark = Sparks_GetFreeSpark();
    spark->on = true;

    if (uw) {
        spark->src_color.r = 0;
        spark->src_color.g = 0;
        spark->src_color.b = 0;
        spark->dst_color.r = 192;
        spark->dst_color.g = 192;
        spark->dst_color.b = 208;
    } else {
        spark->src_color.r = 144;
        spark->src_color.g = 144;
        spark->src_color.b = 144;
        spark->dst_color.r = 64;
        spark->dst_color.g = 64;
        spark->dst_color.b = 64;
    }

    spark->col_fade_speed = 8;
    spark->fade_to_black = 64;
    spark->life = (uint8_t)((Random_GetControl() & 0x1F) + 96);
    spark->s_life = spark->life;

    spark->draw_type = uw ? DRAW_BLEND_ADD : DRAW_BLEND_SUB;

    spark->extras = 0;
    spark->dynamic = -1;
    spark->pos.x = (Random_GetControl() & 0x1F) + pos.x - 16;
    spark->pos.y = (Random_GetControl() & 0x1F) + pos.y - 16;
    spark->pos.z = (Random_GetControl() & 0x1F) + pos.z - 16;
    spark->vel.x = ((Random_GetControl() & 0xFFF) - 2048) >> 2;
    spark->vel.y = (Random_GetControl() & 0xFF) - 128;
    spark->vel.z = ((Random_GetControl() & 0xFFF) - 2048) >> 2;

    if (uw) {
        spark->friction = 20;
        spark->vel.y = (int16_t)(spark->vel.y >> 4);
        spark->pos.y += 32;
    } else {
        spark->friction = 6;
    }

    spark->flags =
        SPARK_F_ALT_SPRITE | SPARK_F_ROTATE | SPARK_F_SPRITE | SPARK_F_SCALE;
    spark->rot_angle = (uint16_t)(Random_GetControl() & 0xFFF);
    if ((Random_GetControl() & 1) != 0) {
        spark->rot_add = (int8_t)(-16 - (Random_GetControl() & 0xF));
    } else {
        spark->rot_add = (int8_t)((Random_GetControl() & 0xF) + 16);
    }

    spark->sprite_idx = Object_Get(O_EXPLOSION_1)->mesh_idx;
    spark->scalar = 3;

    if (uw) {
        spark->max_y_vel = 0;
        spark->gravity = 0;
    } else {
        spark->gravity = (int16_t)(-3 - (Random_GetControl() & 3));
        spark->max_y_vel = (int8_t)(-4 - (Random_GetControl() & 3));
    }

    spark->dst_size.width = (uint8_t)((Random_GetControl() & 0x1F) + 128);
    spark->size.width = (uint8_t)(spark->dst_size.width >> 2);
    spark->src_size.width = spark->size.width;
    spark->dst_size.height =
        (uint8_t)(spark->dst_size.width + (Random_GetControl() & 0x1F) + 32);
    spark->size.height = (uint8_t)(spark->dst_size.height >> 3);
    spark->src_size.height = spark->size.height;
    spark->room_num = (uint8_t)room_num;
}

void Sparks_TriggerFlareSparks(
    const XYZ_32 pos, const XYZ_32 vel, const bool smoke)
{
    const ITEM *const lara_item = Lara_GetItem();
    const int32_t dx = lara_item->pos.x - pos.x;
    const int32_t dz = lara_item->pos.z - pos.z;
    const int32_t max_dist = 16 * WALL_L;
    if (dx < -max_dist || dx > max_dist || dz < -max_dist || dz > max_dist) {
        return;
    }

    SPARK *const spark = Sparks_GetFreeSpark();
    spark->on = true;
    spark->src_color.r = 255;
    spark->src_color.g = 255;
    spark->src_color.b = 255;
    spark->dst_color.r = 255;
    spark->dst_color.g = (Random_GetDraw() & 0x7F) + 64;
    spark->dst_color.b = 192 - spark->dst_color.g;
    spark->col_fade_speed = 3;
    spark->fade_to_black = 5;
    spark->life = 10;
    spark->s_life = 10;
    spark->draw_type = DRAW_BLEND_ADD;
    spark->dynamic = -1;
    spark->pos.x = pos.x + (Random_GetDraw() & 7) - 3;
    spark->pos.y = pos.y + (Random_GetDraw() & 7) - 3;
    spark->pos.z = pos.z + (Random_GetDraw() & 7) - 3;
    spark->vel.x = (int16_t)((Random_GetDraw() & 0xFF) + vel.x - 128);
    spark->vel.y = (int16_t)((Random_GetDraw() & 0xFF) + vel.y - 128);
    spark->vel.z = (int16_t)((Random_GetDraw() & 0xFF) + vel.z - 128);
    spark->friction = 34;
    spark->scalar = 1;
    spark->size.width = (Random_GetDraw() & 3) + 4;
    spark->src_size.width = spark->size.width;
    spark->dst_size.width = (Random_GetDraw() & 1) + 1;
    spark->size.height = (Random_GetDraw() & 3) + 4;
    spark->src_size.height = spark->size.height;
    spark->dst_size.height = (Random_GetDraw() & 1) + 1;
    spark->max_y_vel = 0;
    spark->gravity = 0;
    spark->flags = SPARK_F_SCALE;

    if (!smoke) {
        return;
    }

    SPARK *const smoke_spark = Sparks_GetFreeSpark();
    smoke_spark->on = true;
    smoke_spark->src_color.r = spark->dst_color.r >> 1;
    smoke_spark->src_color.g = spark->dst_color.g >> 1;
    smoke_spark->src_color.b = spark->dst_color.b >> 1;
    smoke_spark->dst_color.r = 32;
    smoke_spark->dst_color.g = 32;
    smoke_spark->dst_color.b = 32;
    smoke_spark->col_fade_speed = (Random_GetDraw() & 3) + 8;
    smoke_spark->fade_to_black = 4;
    smoke_spark->draw_type = DRAW_BLEND_ADD;
    smoke_spark->life = (Random_GetDraw() & 7) + 13;
    smoke_spark->s_life = smoke_spark->life;
    smoke_spark->pos.x = pos.x + (vel.x >> 5);
    smoke_spark->pos.y = pos.y + (vel.y >> 5);
    smoke_spark->pos.z = pos.z + (vel.z >> 5);
    smoke_spark->extras = 0;
    smoke_spark->dynamic = -1;
    smoke_spark->vel.x = (int16_t)((Random_GetDraw() & 0x3F) + vel.x - 32);
    smoke_spark->vel.y = (int16_t)vel.y;
    smoke_spark->vel.z = (int16_t)((Random_GetDraw() & 0x3F) + vel.z - 32);
    smoke_spark->friction = 4;

    if (Random_GetDraw() & 1) {
        smoke_spark->flags = SPARK_F_ALT_SPRITE | SPARK_F_ROTATE
            | SPARK_F_SPRITE | SPARK_F_SCALE;
        smoke_spark->rot_angle = Random_GetDraw() & 0xFFF;

        if (Random_GetDraw() & 1) {
            smoke_spark->rot_add = -16 - (Random_GetDraw() & 0xF);
        } else {
            smoke_spark->rot_add = (Random_GetDraw() & 0xF) + 16;
        }
    } else {
        smoke_spark->flags =
            SPARK_F_ALT_SPRITE | SPARK_F_SPRITE | SPARK_F_SCALE;
    }

    smoke_spark->sprite_idx = Object_Get(O_EXPLOSION_1)->mesh_idx;
    smoke_spark->scalar = 2;
    smoke_spark->gravity = -8 - (Random_GetDraw() & 3);
    smoke_spark->max_y_vel = -4 - (Random_GetDraw() & 3);
    smoke_spark->dst_size.width = (Random_GetDraw() & 0xF) + 24;
    smoke_spark->src_size.width = smoke_spark->dst_size.width >> 3;
    smoke_spark->size.width = smoke_spark->dst_size.width >> 3;
    smoke_spark->dst_size.height = smoke_spark->dst_size.width;
    smoke_spark->src_size.height = smoke_spark->dst_size.height >> 3;
    smoke_spark->size.height = smoke_spark->dst_size.height >> 3;
}

void Sparks_TriggerRicochet(
    const GAME_VECTOR pos, const int32_t angle, const int32_t size)
{
    SPARK *spark = Sparks_GetFreeSpark();
    spark->on = true;
    spark->src_color.r = 255;
    spark->src_color.g = (Random_GetControl() & 0x1F) + 32;
    spark->src_color.b = 0;
    spark->color = spark->src_color;
    spark->dst_color.r = 192;
    spark->dst_color.g = (Random_GetControl() & 0x3F) + 96;
    spark->dst_color.b = 0;
    spark->col_fade_speed = 8;
    spark->fade_to_black = 8;
    spark->life = 24;
    spark->s_life = 24;
    spark->draw_type = DRAW_BLEND_ADD;
    spark->dynamic = -1;
    spark->pos.x = pos.x;
    spark->pos.y = pos.y;
    spark->pos.z = pos.z;
    int32_t ang = ((Random_GetControl() & 0x7FF) + angle - 1024) & 0xFFF;
    spark->vel.x = -Math_Sin(ang << 4) >> 3;
    spark->vel.y = 2 * (Random_GetControl() & 0x1FF) - 768;
    spark->vel.z = Math_Cos(ang << 4) >> 3;
    spark->friction = 1;
    spark->flags = SPARK_F_SCALE;
    spark->scalar = 3;
    spark->gravity =
        (int16_t)(ABS(spark->vel.y >> 6) + (Random_GetControl() & 0x1F));
    spark->size.width = (Random_GetControl() & 3) + 4;
    spark->src_size.width = spark->size.width;
    spark->dst_size.width = (Random_GetControl() & 1) + 1;
    spark->size.height = (Random_GetControl() & 3) + 4;
    spark->src_size.height = spark->size.height;
    spark->dst_size.height = (Random_GetControl() & 1) + 1;
    spark->max_y_vel = 0;

    spark = Sparks_GetFreeSpark();
    spark->on = true;
    uint8_t c = (uint8_t)((Random_GetControl() & 0x3F) + 128);
    spark->src_color.r = c;
    spark->src_color.g = c;
    spark->src_color.b = c;
    spark->color = spark->src_color;
    c >>= 1;
    spark->dst_color.r = c;
    spark->dst_color.g = c;
    spark->dst_color.b = c;
    spark->draw_type = DRAW_BLEND_SUB;
    spark->col_fade_speed = 8;
    spark->fade_to_black = 16;
    spark->life = 28;
    spark->s_life = 28;
    spark->dynamic = -1;
    spark->pos.x = pos.x;
    spark->pos.y = pos.y;
    spark->pos.z = pos.z;
    ang = ((Random_GetControl() & 0x7FF) + angle - 1023) & 0xFFF;
    spark->vel.x = -Math_Sin(ang << 4) >> 3;
    spark->vel.y = (Random_GetControl() & 0x1FF) - 384;
    spark->vel.z = Math_Cos(ang << 4) >> 3;
    spark->friction = 33;
    spark->flags = SPARK_F_SCALE;
    spark->scalar = 3;
    spark->gravity = (Random_GetControl() & 7) + 4;
    spark->size.width = (Random_GetControl() & 3) + 4;
    spark->src_size.width = spark->size.width;
    spark->dst_size.width = (Random_GetControl() & 1) + 1;
    spark->size.height = (Random_GetControl() & 3) + 4;
    spark->src_size.height = spark->size.height;
    spark->dst_size.height = (Random_GetControl() & 1) + 1;
    spark->max_y_vel = 0;
}

void Sparks_TriggerGunSmoke(
    const GAME_VECTOR pos, const bool initial, const LARA_GUN_TYPE weapon,
    const int32_t shade)
{
    Sparks_TriggerGunSmokeDirected(pos, (XYZ_32) {}, initial, weapon, shade);
}

void Sparks_TriggerGunSmokeDirected(
    const GAME_VECTOR pos, const XYZ_32 vel, const bool initial,
    const LARA_GUN_TYPE weapon, const int32_t shade)
{

    SPARK *const spark = Sparks_GetFreeSpark();
    spark->on = true;
    spark->src_color.r = 0;
    spark->src_color.g = 0;
    spark->src_color.b = 0;
    spark->dst_color.r = shade << 2;
    spark->dst_color.g = shade << 2;
    spark->dst_color.b = shade << 2;
    spark->col_fade_speed = 4;
    spark->fade_to_black = 32 - (initial << 4);
    spark->life = (Random_GetControl() & 3) + 40;
    spark->s_life = spark->life;

    if ((weapon == LGT_PISTOLS || weapon == LGT_MAGNUMS || weapon == LGT_UZIS)
        && spark->dst_color.r > 64) {
        spark->dst_color.r = 64;
        spark->dst_color.g = 64;
        spark->dst_color.b = 64;
    }

    spark->draw_type = 2;
    spark->extras = 0;
    spark->dynamic = -1;
    spark->pos.x = pos.x + (Random_GetControl() & 0x1F) - 16;
    spark->pos.y = pos.y + (Random_GetControl() & 0x1F) - 16;
    spark->pos.z = pos.z + (Random_GetControl() & 0x1F) - 16;

    if (initial) {
        spark->vel.x = vel.x + (Random_GetControl() & 0x3FF) - 512;
        spark->vel.y = vel.y + (Random_GetControl() & 0x3FF) - 512;
        spark->vel.z = vel.z + (Random_GetControl() & 0x3FF) - 512;
    } else {
        spark->vel.x = ((Random_GetControl() & 0x1FF) - 256) >> 1;
        spark->vel.y = ((Random_GetControl() & 0x1FF) - 256) >> 1;
        spark->vel.z = ((Random_GetControl() & 0x1FF) - 256) >> 1;
    }

    spark->friction = 4;

    if (Random_GetControl() & 1) {
        if (Room_Get(Lara_GetItem()->room_num)->flags.wind) {
            spark->flags = SPARK_F_ALT_SPRITE | SPARK_F_OUTSIDE | SPARK_F_ROTATE
                | SPARK_F_SPRITE | SPARK_F_SCALE;
        } else {
            spark->flags = SPARK_F_ALT_SPRITE | SPARK_F_ROTATE | SPARK_F_SPRITE
                | SPARK_F_SCALE;
        }

        spark->rot_angle = Random_GetControl() & 0xFFF;

        if (Random_GetControl() & 1) {
            spark->rot_add = -16 - (Random_GetControl() & 0xF);
        } else {
            spark->rot_add = (Random_GetControl() & 0xF) + 16;
        }
    } else if (Room_Get(Lara_GetItem()->room_num)->flags.wind) {
        spark->flags = SPARK_F_ALT_SPRITE | SPARK_F_OUTSIDE | SPARK_F_SPRITE
            | SPARK_F_SCALE;
    } else {
        spark->flags = SPARK_F_ALT_SPRITE | SPARK_F_SPRITE | SPARK_F_SCALE;
    }

    spark->sprite_idx = Object_Get(O_EXPLOSION_1)->mesh_idx;
    spark->scalar = 3;
    spark->gravity = -2 - (Random_GetControl() & 1);
    spark->max_y_vel = -2 - (Random_GetControl() & 1);

    uint8_t size = (Random_GetControl() & 7)
        - ((weapon == LGT_ROCKET || weapon == LGT_GRENADE) ? 0 : 12) + 24;

    if (initial) {
        spark->size.width = size >> 1;
        spark->src_size.width = spark->size.width;
        spark->dst_size.width = (size + 4) << 1;
    } else {
        spark->size.width = size >> 2;
        spark->src_size.width = spark->size.width;
        spark->dst_size.width = size;
    }

    if (initial) {
        spark->size.height = size >> 1;
        spark->src_size.height = spark->size.width;
        spark->dst_size.height = (size + 4) << 1;
    } else {
        spark->size.height = size >> 2;
        spark->src_size.height = spark->size.width;
        spark->dst_size.height = size;
    }
}

void Sparks_TriggerShotgunSparks(const XYZ_32 pos, const XYZ_32 vel)
{
    SPARK *const spark = Sparks_GetFreeSpark();
    spark->on = true;
    spark->src_color.r = 255;
    spark->src_color.g = 255;
    spark->src_color.b = 0;
    spark->dst_color.r = 255;
    spark->dst_color.g = (Random_GetControl() & 0x7F) + 64;
    spark->dst_color.b = 0;
    spark->col_fade_speed = 3;
    spark->fade_to_black = 5;
    spark->life = 10;
    spark->s_life = 10;
    spark->draw_type = 2;
    spark->dynamic = -1;
    spark->pos.x = pos.x + (Random_GetControl() & 7) - 3;
    spark->pos.y = pos.y + (Random_GetControl() & 7) - 3;
    spark->pos.z = pos.z + (Random_GetControl() & 7) - 3;
    spark->vel.x = vel.x + (Random_GetControl() & 0x1FF) - 256;
    spark->vel.y = vel.y + (Random_GetControl() & 0x1FF) - 256;
    spark->vel.z = vel.z + (Random_GetControl() & 0x1FF) - 256;
    spark->friction = 0;
    spark->flags = SPARK_F_SCALE;
    spark->scalar = 2;
    spark->max_y_vel = 0;
    spark->gravity = 0;
    spark->size.width = (Random_GetControl() & 3) + 4;
    spark->src_size.width = spark->size.width;
    spark->dst_size.width = 1;
    spark->size.height = (Random_GetControl() & 3) + 4;
    spark->src_size.height = spark->size.height;
    spark->dst_size.height = 1;
}

void Sparks_TriggerRocketSmoke(
    const XYZ_32 pos, const int32_t c, const int16_t room_num)
{
    SPARK *const spark = Sparks_GetFreeSpark();
    spark->on = true;

    spark->src_color.r = 0;
    spark->src_color.g = 0;
    spark->src_color.b = 0;
    spark->dst_color.r = c + 64;
    spark->dst_color.g = c + 64;
    spark->dst_color.b = c + 64;
    spark->fade_to_black = 12;
    spark->col_fade_speed = (Random_GetControl() & 3) + 4;
    spark->draw_type = 2;
    spark->extras = 0;
    spark->life = (Random_GetControl() & 3) + 20;
    spark->s_life = spark->life;
    spark->dynamic = -1;
    spark->pos.x = pos.x + (Random_GetControl() & 0xF) - 8;
    spark->pos.y = pos.y + (Random_GetControl() & 0xF) - 8;
    spark->pos.z = pos.z + (Random_GetControl() & 0xF) - 8;
    spark->vel.x = (Random_GetControl() & 0xFF) - 128;
    spark->vel.y = -4 - (Random_GetControl() & 3);
    spark->vel.z = (Random_GetControl() & 0xFF) - 128;
    spark->friction = 4;

    if (Random_GetControl() & 1) {
        spark->flags = SPARK_F_ALT_SPRITE | SPARK_F_ROTATE | SPARK_F_SPRITE
            | SPARK_F_SCALE;
        spark->rot_angle = Random_GetControl() & 0xFFF;

        if (Random_GetControl() & 1) {
            spark->rot_add = -16 - (Random_GetControl() & 0xF);
        } else {
            spark->rot_add = (Random_GetControl() & 0xF) + 16;
        }
    } else {
        spark->flags = SPARK_F_ALT_SPRITE | SPARK_F_SPRITE | SPARK_F_SCALE;
    }

    spark->scalar = 3;
    spark->sprite_idx = Object_Get(O_EXPLOSION_1)->mesh_idx;
    spark->gravity = -4 - (Random_GetControl() & 3);
    spark->max_y_vel = -4 - (Random_GetControl() & 3);
    const uint8_t size = (Random_GetControl() & 7) + 32;
    spark->dst_size.width = size;
    spark->src_size.width = size >> 2;
    spark->size.width = size >> 2;
    spark->src_size.height = size >> 2;
    spark->size.height = size >> 2;
    spark->dst_size.height = size;
}

void Sparks_TriggerRocketFlame(
    const XYZ_32 pos, const XYZ_32 vel, const int16_t item_num,
    const int16_t room_num)
{
    SPARK *const spark = Sparks_GetFreeSpark();
    spark->on = true;
    spark->src_color.r = (Random_GetControl() & 0x1F) + 48;
    spark->src_color.g = spark->src_color.r;
    spark->src_color.b = (Random_GetControl() & 0x3F) + 192;
    spark->dst_color.r = (Random_GetControl() & 0x3F) + 192;
    spark->dst_color.g = (Random_GetControl() & 0x3F) + 128;
    spark->dst_color.b = 32;
    spark->fade_to_black = 12;
    spark->col_fade_speed = (Random_GetControl() & 3) + 12;
    spark->draw_type = 2;
    spark->life = (Random_GetControl() & 3) + 28;
    spark->s_life = spark->life;
    spark->extras = 0;
    spark->dynamic = -1;
    spark->pos.x = pos.x + (Random_GetControl() & 0x1F) - 16;
    spark->pos.y = pos.y;
    spark->pos.z = pos.z + (Random_GetControl() & 0x1F) - 16;
    spark->vel.x = vel.x;
    spark->vel.y = vel.y;
    spark->vel.z = vel.z;
    spark->friction = 51;
    spark->item_num = item_num;

    if (Random_GetControl() & 1) {
        spark->flags = SPARK_F_ALT_SPRITE | SPARK_F_ITEM | SPARK_F_ROTATE
            | SPARK_F_SPRITE | SPARK_F_SCALE;
        spark->rot_angle = Random_GetControl() & 0xFFF;

        if (Random_GetControl() & 1) {
            spark->rot_add = -16 - (Random_GetControl() & 0xF);
        } else {
            spark->rot_add = (Random_GetControl() & 0xF) + 16;
        }
    } else {
        spark->flags =
            SPARK_F_ALT_SPRITE | SPARK_F_ITEM | SPARK_F_SPRITE | SPARK_F_SCALE;
    }

    spark->gravity = 0;
    spark->max_y_vel = 0;
    spark->scalar = 2;
    spark->sprite_idx = Object_Get(O_EXPLOSION_1)->mesh_idx;
    spark->size.width = (Random_GetControl() & 7) + 32;
    spark->src_size.width = spark->size.width;
    spark->dst_size.width = 2;
    spark->size.height = spark->size.width;
    spark->src_size.height = spark->size.height;
    spark->dst_size.height = 2;
}
