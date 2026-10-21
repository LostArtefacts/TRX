#include <trx/game/fx/shockwave.h>

#include <trx/core/math.h>
#include <trx/core/utils.h>
#include <trx/game/fx/common.h>
#include <trx/game/interpolation.h>
#include <trx/game/items/anim.h>
#include <trx/game/lara.h>
#include <trx/game/output/sources/poly_fx.h>
#include <trx/game/random.h>
#include <trx/game/sound.h>
#include <trx/game/sparks.h>

#include <string.h>

#define M_MAX_SHOCKWAVES 16
#define M_SEGMENTS 16
#define M_SEGMENT_ANGLE (DEG_360 / M_SEGMENTS)
#define M_FADE_TICKS 8
// How far Lara can stand from where the ring hits her for the hit to throw
// sparks, along either axis.
#define M_HIT_EFFECT_RANGE 0x4000

typedef struct {
    XYZ_32 pos;
    int16_t inner_radius;
    int16_t outer_radius;
    int16_t prev_inner_radius;
    int16_t prev_outer_radius;
    int16_t speed;
    int16_t x_rot;
    RGB_888 color;
    uint8_t life;
    FX_SHOCKWAVE_FLAGS flags;
} M_SHOCKWAVE;

static M_SHOCKWAVE m_Shockwaves[M_MAX_SHOCKWAVES];

static void M_Reset(void)
{
    memset(m_Shockwaves, 0, sizeof(m_Shockwaves));
}

// Throws sparks off Lara where the ring passes through her, flying the way
// the ring travels.
static void M_TriggerHitEffect(
    XYZ_32 pos, const RGB_888 color, int16_t dir, int32_t speed)
{
    const ITEM *const lara_item = Lara_GetItem();
    const int32_t dx = lara_item->pos.x - pos.x;
    const int32_t dz = lara_item->pos.z - pos.z;
    if (ABS(dx) > M_HIT_EFFECT_RANGE || ABS(dz) > M_HIT_EFFECT_RANGE) {
        return;
    }

    const int32_t sprite_idx = Sparks_GetSpriteIndex(SPARK_TYPE_LASER_SIGHT);
    if (sprite_idx == NO_ITEM) {
        return;
    }

    SPARK *const spark = Sparks_InitialiseSpark(SPARK_CONTEXT_DEFAULT);
    speed += Random_GetControl() & 0xF;
    const XYZ_32 vel = {
        .x = (speed * Math_Sin(dir)) >> (W2V_SHIFT - 4),
        .y = -512 - (Random_GetControl() & 0x1FF),
        .z = (speed * Math_Cos(dir)) >> (W2V_SHIFT - 4),
    };

    dir += (Random_GetControl() & 1) != 0 ? DEG_90 : -DEG_90;
    const int32_t spread = (Random_GetControl() & 0x1FF) - 256;
    pos.x += (spread * Math_Sin(dir)) >> W2V_SHIFT;
    pos.z += (spread * Math_Cos(dir)) >> W2V_SHIFT;

    *spark = (SPARK) {
        .on = true,
        .context = SPARK_CONTEXT_DEFAULT,
        .src_color = { 0, 0, 0 },
        .dst_color = color,
        .col_fade_speed = 4,
        .fade_to_black = 8,
        .draw_type = DRAW_BLEND_ADD,
        .life = (uint8_t)((Random_GetControl() & 3) + 16),
        .dynamic = -1,
        .sprite_idx = sprite_idx,
        .sprite_obj_id = O_SPARKS_GFX,
        .pos = {
            .x = pos.x + (Random_GetControl() & 0x1F) - 16,
            .y = pos.y + (Random_GetControl() & 0x1F) - 16,
            .z = pos.z + (Random_GetControl() & 0x1F) - 16,
        },
        .vel = vel,
        .friction = 3,
        .flags = SPARK_F_SCALE | SPARK_F_SPRITE | SPARK_F_ROTATE
            | SPARK_F_ALT_SPRITE,
        .rot_angle = (uint16_t)(Random_GetControl() & 0xFFF),
        .scalar = 1,
        .max_y_vel = 0,
        .gravity = (int16_t)((Random_GetControl() & 0x3F) + 64),
    };
    if ((Random_GetControl() & 1) != 0) {
        spark->rot_add = (int8_t)(-16 - (Random_GetControl() & 0xF));
    } else {
        spark->rot_add = (int8_t)((Random_GetControl() & 0xF) + 16);
    }
    spark->s_life = spark->life;

    const uint8_t size = (uint8_t)((Random_GetControl() & 0x1F) + 32);
    spark->src_size.width = size;
    spark->src_size.height = size;
    spark->dst_size.width = size >> 2;
    spark->dst_size.height = size >> 2;
    spark->size = spark->src_size;
    Sparks_FinishSetup(spark);
}

// Hurts Lara where the ring passes through her, as tall as she stands.
static void M_HurtLara(const M_SHOCKWAVE *const wave)
{
    ITEM *const lara_item = Lara_GetItem();
    const BOUNDS_16 *const bounds = &Item_GetBestFrame(lara_item)->bounds;
    const int32_t dx = lara_item->pos.x - wave->pos.x;
    const int32_t dz = lara_item->pos.z - wave->pos.z;
    const int32_t dist = Math_Sqrt(SQUARE(dx) + SQUARE(dz));
    if (wave->pos.y <= lara_item->pos.y + bounds->min.y
        || wave->pos.y >= lara_item->pos.y + bounds->max.y + STEP_L
        || dist <= wave->inner_radius || dist >= wave->outer_radius) {
        return;
    }

    const int16_t dir = Math_Atan(dz, dx);
    M_TriggerHitEffect(
        (XYZ_32) {
            .x = lara_item->pos.x, .y = wave->pos.y, .z = lara_item->pos.z },
        wave->color, dir, wave->speed);
    const int32_t shift = (wave->flags & FX_SHOCKWAVE_F_HURT_LESS) != 0 ? 3 : 2;
    Lara_TakeDamage(wave->speed >> shift, false);
}

static void M_Control(void)
{
    for (int32_t i = 0; i < M_MAX_SHOCKWAVES; i++) {
        M_SHOCKWAVE *const wave = &m_Shockwaves[i];
        if (wave->life == 0) {
            continue;
        }

        wave->life--;
        if (wave->life == 0) {
            continue;
        }

        wave->prev_outer_radius = wave->outer_radius;
        wave->prev_inner_radius = wave->inner_radius;
        wave->outer_radius += wave->speed;
        wave->inner_radius += wave->speed >> 1;
        wave->speed -= wave->speed >> 4;

        if (Lara_GetItem()->hit_points >= 0
            && (wave->flags & (FX_SHOCKWAVE_F_HURT | FX_SHOCKWAVE_F_HURT_LESS))
                != 0) {
            M_HurtLara(wave);
        }
    }
}

// Returns a point of the ring's rim, tilted and placed in the world.
static XYZ_32 M_GetRimPoint(
    const M_SHOCKWAVE *const wave, const int32_t radius, const int32_t angle)
{
    const XYZ_32 local = {
        .x = (radius * Math_Sin(angle)) >> W2V_SHIFT,
        .z = (radius * Math_Cos(angle)) >> W2V_SHIFT,
    };
    return XYZ_32_Add(
        wave->pos, XYZ_32_Rotate(local, (XYZ_16) { .x = wave->x_rot }));
}

static void M_Draw(void)
{
    const int32_t sprite_idx = Sparks_GetSpriteIndex(SPARK_TYPE_SHOCKWAVE);
    if (sprite_idx == NO_ITEM) {
        return;
    }

    const double ratio = Interpolation_GetWorldRate();
    for (int32_t i = 0; i < M_MAX_SHOCKWAVES; i++) {
        const M_SHOCKWAVE *const wave = &m_Shockwaves[i];
        if (wave->life == 0) {
            continue;
        }

        const int32_t outer =
            (int32_t)LERP(wave->prev_outer_radius, wave->outer_radius, ratio);
        const int32_t inner =
            (int32_t)LERP(wave->prev_inner_radius, wave->inner_radius, ratio);

        RGBA_8888 color = {
            .r = wave->color.r,
            .g = wave->color.g,
            .b = wave->color.b,
            .a = 255,
        };
        if (wave->life < M_FADE_TICKS) {
            color.r = (color.r * wave->life) / M_FADE_TICKS;
            color.g = (color.g * wave->life) / M_FADE_TICKS;
            color.b = (color.b * wave->life) / M_FADE_TICKS;
        }
        const RGBA_8888 colors[4] = { color, color, color, color };

        // The sprite runs across each segment, from the inner edge at its
        // top to the outer edge at its bottom.
        for (int32_t j = 0; j < M_SEGMENTS; j++) {
            const int32_t angle_0 = j * M_SEGMENT_ANGLE;
            const int32_t angle_1 = angle_0 + M_SEGMENT_ANGLE;
            const XYZ_32 points[4] = {
                M_GetRimPoint(wave, inner, angle_0),
                M_GetRimPoint(wave, inner, angle_1),
                M_GetRimPoint(wave, outer, angle_1),
                M_GetRimPoint(wave, outer, angle_0),
            };
            OutputSource_PolyFX_StageSpriteQuadWorld(
                sprite_idx, points, colors, DRAW_BLEND_ADD);
        }
    }
}

void FX_Shockwave_Create(const FX_SHOCKWAVE_SETUP *const setup)
{
    M_SHOCKWAVE *wave = nullptr;
    for (int32_t i = 0; i < M_MAX_SHOCKWAVES; i++) {
        if (m_Shockwaves[i].life == 0) {
            wave = &m_Shockwaves[i];
            break;
        }
    }
    if (wave == nullptr) {
        return;
    }

    *wave = (M_SHOCKWAVE) {
        .pos = setup->pos,
        .inner_radius = setup->inner_radius,
        .outer_radius = setup->outer_radius,
        .prev_inner_radius = setup->inner_radius,
        .prev_outer_radius = setup->outer_radius,
        .speed = setup->speed,
        .x_rot = setup->x_rot,
        .color = setup->color,
        .life = setup->life,
        .flags = setup->flags,
    };
    Sound_Effect(SFX_SHOCKWAVE, &wave->pos, SPM_NORMAL);
}

static const FX_MODULE m_Module = {
    .control_func = M_Control,
    .draw_func = M_Draw,
    .reset_func = M_Reset,
};

REGISTER_FX(m_Module)
