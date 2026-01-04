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
#include <trx/utils.h>
#include <trx/version.h>

#define M_MAX_SPARKS 192
#define M_MAX_SPARK_DYNAMICS 32
#define M_LERP(a, b, ratio) ((a) + ((b) - (a)) * (ratio))

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
static SPARKS_CALLBACKS m_Callbacks = {};

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
        const SPARK *const sptr = &m_Sparks[i];
        if ((int32_t)sptr->life < min_life && sptr->dynamic == -1
            && ((sptr->flags & SPARK_F_BLOOD) == 0U || (i & 1) != 0)) {
            free = i;
            min_life = (int32_t)sptr->life;
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
    m_Dynamics[(uint8_t)idx].on = false;
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
    m_Callbacks = (SPARKS_CALLBACKS) {};
}

void Sparks_SetCallbacks(const SPARKS_CALLBACKS *const callbacks)
{
    if (callbacks != nullptr) {
        m_Callbacks = *callbacks;
    } else {
        m_Callbacks = (SPARKS_CALLBACKS) {};
    }
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
        SPARK *const sptr = &m_Sparks[i];
        if (!sptr->on) {
            continue;
        }

        if ((sptr->flags & SPARK_F_ATTACHED_POS) == 0U || sptr->life > 16) {
            if (sptr->life > 0) {
                sptr->life--;
            }
        }

        if (sptr->life == 0) {
            if (sptr->dynamic != -1) {
                m_Dynamics[(uint8_t)sptr->dynamic].on = false;
                sptr->dynamic = -1;
            }

            sptr->on = false;
            continue;
        }

        const int32_t lived = (int32_t)sptr->s_life - (int32_t)sptr->life;

        // Color fade: src -> dst, then fade-to-black.
        if (lived < (int32_t)sptr->col_fade_speed
            && sptr->col_fade_speed != 0U) {
            const float fade = lived / (float)sptr->col_fade_speed;
            sptr->color.r = M_LERP(
                (int32_t)sptr->src_color.r, (int32_t)sptr->dst_color.r, fade);
            sptr->color.g = M_LERP(
                (int32_t)sptr->src_color.g, (int32_t)sptr->dst_color.g, fade);
            sptr->color.b = M_LERP(
                (int32_t)sptr->src_color.b, (int32_t)sptr->dst_color.b, fade);
        } else if (
            sptr->life < sptr->fade_to_black && sptr->fade_to_black != 0U) {
            const float fade =
                ((int32_t)sptr->life - (int32_t)sptr->fade_to_black)
                / (float)sptr->fade_to_black;
            sptr->color.r = sptr->dst_color.r * fade;
            sptr->color.g = sptr->dst_color.g * fade;
            sptr->color.b = sptr->dst_color.b * fade;
        } else {
            sptr->color = sptr->dst_color;
        }

        if (sptr->life == sptr->fade_to_black
            && (sptr->flags & SPARK_F_UNDERWATER) != 0U) {
            sptr->dst_size.width >>= 2;
            sptr->dst_size.height >>= 2;
        }

        if ((sptr->flags & SPARK_F_ROTATE) != 0U) {
            sptr->rot_angle = (sptr->rot_angle + sptr->rot_add) & 0xFFF;
        }

        if ((sptr->flags & SPARK_F_ALT_SPRITE) != 0U) {
            const OBJECT *const explosion = Object_Get(O_EXPLOSION_1);
            if (explosion->loaded) {
                const int32_t base = explosion->mesh_idx;
                if (sptr->color.r < 16 && sptr->color.g < 16
                    && sptr->color.b < 16) {
                    sptr->sprite_idx = base + 3;
                } else if (
                    sptr->color.r < 64 && sptr->color.g < 64
                    && sptr->color.b < 64) {
                    sptr->sprite_idx = base + 2;
                } else if (
                    sptr->color.r < 96 && sptr->color.g < 96
                    && sptr->color.b < 96) {
                    sptr->sprite_idx = base + 1;
                } else {
                    sptr->sprite_idx = base;
                }
            }
        }

        if (lived == (int32_t)(sptr->extras >> 3)
            && (sptr->extras & 7U) != 0U) {
            int32_t uw = 0;
            if ((sptr->flags & SPARK_F_UNDERWATER) != 0U) {
                uw = 1;
            } else if ((sptr->flags & SPARK_F_GREEN) != 0U) {
                uw = 2;
            }

            const XYZ_32 spark_pos = Sparks_GetWorldPos(sptr);

            if (m_Callbacks.trigger_explosion_sparks == nullptr) {
                ASSERT_FAIL(); // TODO: hook TriggerExplosionSparks equivalent
            } else {
                for (int32_t j = 0; j < (int32_t)(sptr->extras & 7U); j++) {
                    m_Callbacks.trigger_explosion_sparks(
                        spark_pos, (int32_t)(sptr->extras & 7U) - 1,
                        sptr->dynamic, uw, sptr->room_num);
                    sptr->dynamic = -1;
                }
            }

            if ((sptr->flags & SPARK_F_UNDERWATER) != 0U) {
                if (m_Callbacks.trigger_explosion_bubble == nullptr) {
                    ASSERT_FAIL(); // TODO: hook TriggerExplosionBubble
                } else {
                    m_Callbacks.trigger_explosion_bubble(
                        spark_pos, sptr->room_num);
                }
            }

            sptr->extras = 0;
        }

        // Physics
        sptr->vel.y += sptr->gravity;
        if (sptr->max_y_vel != 0) {
            const int32_t limit = (int32_t)sptr->max_y_vel << 5;
            if ((sptr->vel.y < 0 && sptr->vel.y < limit)
                || (sptr->vel.y > 0 && sptr->vel.y > limit)) {
                sptr->vel.y = limit;
            }
        }

        if ((sptr->friction & 0x0FU) != 0U) {
            sptr->vel.x -= sptr->vel.x >> (sptr->friction & 0x0FU);
            sptr->vel.z -= sptr->vel.z >> (sptr->friction & 0x0FU);
        }

        if ((sptr->friction & 0xF0U) != 0U) {
            sptr->vel.y -= sptr->vel.y >> (sptr->friction >> 4);
        }

        sptr->pos.x += sptr->vel.x >> 5;
        sptr->pos.y += sptr->vel.y >> 5;
        sptr->pos.z += sptr->vel.z >> 5;

        if ((sptr->flags & SPARK_F_OUTSIDE) != 0U) {
            sptr->pos.x += m_SmokeWind.x >> 1;
            sptr->pos.z += m_SmokeWind.z >> 1;
        }

        // Size lerp across lifetime.
        if (sptr->s_life != 0U) {
            const float fade = lived / (float)sptr->s_life;
            sptr->size.width = M_LERP(
                (int32_t)sptr->src_size.width, (int32_t)sptr->dst_size.width,
                fade);
            sptr->size.height = M_LERP(
                (int32_t)sptr->src_size.height, (int32_t)sptr->dst_size.height,
                fade);
        } else {
            sptr->size = sptr->src_size;
        }

        // If attached to a node, detach after a short random delay for some
        // node types.
        if ((sptr->flags & (SPARK_F_ITEM | SPARK_F_ATTACHED_NODE))
                == (SPARK_F_ITEM | SPARK_F_ATTACHED_NODE)
            && (sptr->node_num == 2 || sptr->node_num == 3)) {
            const int32_t b = sptr->node_num == 3 ? (Random_GetDraw() & 3) + 12
                                                  : (Random_GetDraw() & 3) + 8;
            if (lived > b) {
                sptr->pos = Sparks_GetWorldPos(sptr);
                sptr->flags &= ~(SPARK_F_ATTACHED_NODE | SPARK_F_ITEM);
            }
        }
    }

    // Dynamic light pass.
    for (int32_t i = 0; i < M_MAX_SPARKS; i++) {
        const SPARK *const sptr = &m_Sparks[i];
        if (!sptr->on || sptr->dynamic == -1) {
            continue;
        }

        M_SPARK_DYNAMIC *const dl = &m_Dynamics[(uint8_t)sptr->dynamic];
        if (!dl->on) {
            continue;
        }

        const XYZ_32 world_pos = Sparks_GetWorldPos(sptr);
        const int32_t rnd = Random_GetControl();
        XYZ_32 pos = {
            .x = world_pos.x + ((rnd & 0xF) << 4),
            .y = world_pos.y + (rnd & 0xF0),
            .z = world_pos.z + ((rnd >> 4) & 0xF0),
        };

        int32_t falloff = (int32_t)sptr->s_life - (int32_t)sptr->life - 1;
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
        OutputSource_PolyFX_StageSpark(&m_Sparks[i]);
    }
}

void Sparks_DetachEffect(const int16_t effect_num)
{
    for (int32_t i = 0; i < M_MAX_SPARKS; i++) {
        SPARK *const sptr = &m_Sparks[i];
        if (!sptr->on) {
            continue;
        }

        if ((sptr->flags & SPARK_F_FX) != 0U
            && sptr->effect_num == effect_num) {
            if ((sptr->flags & SPARK_F_ATTACHED_POS) != 0U) {
                sptr->on = false;
                continue;
            }

            const EFFECT *const effect = Effect_Get(effect_num);
            sptr->pos.x += effect->pos.x;
            sptr->pos.y += effect->pos.y;
            sptr->pos.z += effect->pos.z;
            sptr->flags &= ~SPARK_F_FX;
        }
    }
}

void Sparks_DetachItem(const int16_t item_num)
{
    for (int32_t i = 0; i < M_MAX_SPARKS; i++) {
        SPARK *const sptr = &m_Sparks[i];
        if (!sptr->on) {
            continue;
        }

        if ((sptr->flags & SPARK_F_ITEM) != 0U && sptr->item_num == item_num) {
            if ((sptr->flags & SPARK_F_ATTACHED_POS) != 0U) {
                sptr->on = false;
                continue;
            }

            const ITEM *const item = Item_Get(item_num);
            sptr->pos.x += item->pos.x;
            sptr->pos.y += item->pos.y;
            sptr->pos.z += item->pos.z;
            sptr->flags &= ~SPARK_F_ITEM;
            sptr->flags &= ~SPARK_F_ATTACHED_NODE;
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
    if (dx < -0x4000 || dx > 0x4000 || dz < -0x4000 || dz > 0x4000) {
        return;
    }

    const OBJECT *const bubble_obj = Object_Get(O_BUBBLE_1);
    if (!bubble_obj->loaded) {
        return;
    }

    const int32_t idx = M_GetFreeSpark();
    SPARK *const sptr = &m_Sparks[idx];
    *sptr = (SPARK) {
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
    sptr->src_size.width = base;
    sptr->src_size.height = base;
    sptr->dst_size.width = dst;
    sptr->dst_size.height = dst;
    sptr->size = sptr->src_size;
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
        SPARK *const sptr = Sparks_GetFreeSpark();

        const int32_t offset = (Random_GetControl() & 0x1F) + offsets[i] - 16;
        const int32_t c = Math_Cos(angle) >> W2V_SHIFT;
        const int32_t s = Math_Sin(angle) >> W2V_SHIFT;

        *sptr = (SPARK) {
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
        sptr->s_life = sptr->life;

        if ((Random_GetControl() & 1) != 0) {
            sptr->flags |= SPARK_F_ROTATE;
            sptr->rot_angle = (uint16_t)(Random_GetControl() & 0xFFF);
            if ((Random_GetControl() & 1) != 0) {
                sptr->rot_add = -16 - (Random_GetControl() % 16);
            } else {
                sptr->rot_add = 16 + (Random_GetControl() % 16);
            }
        }

        const uint8_t dst_size = (uint8_t)((Random_GetControl() & 7) + 12);
        const uint8_t src_size = (uint8_t)(dst_size >> 1);
        sptr->src_size.width = src_size;
        sptr->src_size.height = src_size;
        sptr->dst_size.width = dst_size;
        sptr->dst_size.height = dst_size;
        sptr->size = sptr->src_size;
    }
}
