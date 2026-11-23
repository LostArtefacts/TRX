#include <trx/game/fx/lightning.h>

#include <trx/core/json/util/read_io.h>
#include <trx/core/json/util/write_io.h>
#include <trx/core/log.h>
#include <trx/core/math.h>
#include <trx/core/utils.h>
#include <trx/game/camera/spline.h>
#include <trx/game/fx/common.h>
#include <trx/game/interpolation.h>
#include <trx/game/lara.h>
#include <trx/game/matrix.h>
#include <trx/game/output/const.h>
#include <trx/game/output/sources/poly_fx.h>
#include <trx/game/random.h>
#include <trx/game/sparks.h>

#include <math.h>
#include <string.h>

// clang-format off
#define M_MAX_BOLTS        16
#define M_MAX_SEGMENTS     8
#define M_SEGMENT_POINTS   3
#define M_MAX_POINTS       (M_MAX_SEGMENTS * M_SEGMENT_POINTS)
#define M_KNOTS            4
#define M_SPLINE_KNOTS     (M_KNOTS + 2)
#define M_LIFE_STEP        2
#define M_FADE_LIFE        16
#define M_CORE_SHADE       64
#define M_CORE_HALF_WIDTH  4.0f
#define M_GLOW_MIN_WIDTH   4
#define M_GLOW_WIDTH_END   0x4000
#define M_GLOW_REF_PERSP   (REF_PERSP * 2)
#define M_GLOW_COARSE_SHIFT 8
#define M_FADE_START       (12 * WALL_L)
#define M_FADE_END         (20 * WALL_L)
#define M_FADE_SHIFT       13
#define M_DRAW_RANGE       0x6000
#define M_GLOW_RANGE       0x4000
#define M_GLOW_LIFE        4
#define M_GLOW_FADE_SPEED  2
#define M_GLOW_SCALAR      3
// clang-format on

typedef struct {
    // The two ends of the bolt and the two control points between them.
    XYZ_32 knots[M_KNOTS];
    XYZ_32 prev_knots[M_KNOTS];
    // The velocities of the knots past the first.
    XYZ_16 vel[M_KNOTS - 1];
    // The random offset of each point from the line or curve, picked anew
    // each tick.
    XYZ_16 jitter[M_MAX_POINTS];
    RGB_888 color;
    uint8_t life;
    uint8_t size;
    uint8_t variation;
    uint8_t segments;
    FX_LIGHTNING_FLAGS flags;
} M_BOLT;

static M_BOLT m_Bolts[M_MAX_BOLTS];

static void M_Reset(void)
{
    memset(m_Bolts, 0, sizeof(m_Bolts));
}

static int32_t M_GetPointCount(const M_BOLT *const bolt)
{
    return bolt->segments * M_SEGMENT_POINTS;
}

static int16_t M_GetVelocity(const int32_t variation)
{
    return (int16_t)((Random_GetControl() % variation) - (variation >> 1));
}

static int16_t M_GetStep(const int32_t variation)
{
    return (int16_t)((Random_GetDraw() % (variation * 2)) - variation);
}

static void M_Jitter(M_BOLT *const bolt)
{
    const int32_t count = M_GetPointCount(bolt);
    XYZ_16 total = {};
    for (int32_t i = 1; i < count - 1; i++) {
        if ((bolt->flags & FX_LIGHTNING_F_CURVED) != 0) {
            bolt->jitter[i] = (XYZ_16) {
                .x = (Random_GetDraw() & 0xF) - 8,
                .y = (Random_GetDraw() & 0xF) - 8,
                .z = (Random_GetDraw() & 0xF) - 8,
            };
        } else {
            total.x += M_GetStep(bolt->variation);
            total.y += M_GetStep(bolt->variation);
            total.z += M_GetStep(bolt->variation);
            bolt->jitter[i] = total;
        }
    }
}

static void M_GetPoints(
    const M_BOLT *const bolt, const double ratio, XYZ_32 *const points)
{
    XYZ_32 knots[M_KNOTS];
    for (int32_t i = 0; i < M_KNOTS; i++) {
        const XYZ_32 prev = bolt->prev_knots[i];
        const XYZ_32 cur = bolt->knots[i];
        knots[i] = (XYZ_32) {
            .x = (int32_t)LERP(prev.x, cur.x, ratio),
            .y = (int32_t)LERP(prev.y, cur.y, ratio),
            .z = (int32_t)LERP(prev.z, cur.z, ratio),
        };
    }

    const int32_t count = M_GetPointCount(bolt);
    const XYZ_32 src = knots[0];
    const XYZ_32 dst = knots[M_KNOTS - 1];
    points[0] = src;
    points[count - 1] = dst;

    if ((bolt->flags & FX_LIGHTNING_F_CURVED) != 0) {
        // The curve passes through each end twice, so that it starts and
        // stops there.
        int32_t kx[M_SPLINE_KNOTS];
        int32_t ky[M_SPLINE_KNOTS];
        int32_t kz[M_SPLINE_KNOTS];
        for (int32_t i = 0; i < M_SPLINE_KNOTS; i++) {
            int32_t knot_idx = i - 1;
            CLAMP(knot_idx, 0, M_KNOTS - 1);
            const XYZ_32 knot = knots[knot_idx];
            kx[i] = knot.x;
            ky[i] = knot.y;
            kz[i] = knot.z;
        }

        const int32_t step = SPLINE_ONE / (count - 1);
        for (int32_t i = 1; i < count - 1; i++) {
            const XYZ_16 jitter = bolt->jitter[i];
            points[i] = (XYZ_32) {
                .x = Spline_Calculate(i * step, kx, M_SPLINE_KNOTS) + jitter.x,
                .y = Spline_Calculate(i * step, ky, M_SPLINE_KNOTS) + jitter.y,
                .z = Spline_Calculate(i * step, kz, M_SPLINE_KNOTS) + jitter.z,
            };
        }
    } else {
        const XYZ_32 step = {
            .x = (dst.x - src.x) / (count - 1),
            .y = (dst.y - src.y) / (count - 1),
            .z = (dst.z - src.z) / (count - 1),
        };
        for (int32_t i = 1; i < count - 1; i++) {
            const XYZ_16 jitter = bolt->jitter[i];
            points[i] = (XYZ_32) {
                .x = src.x + step.x * i + jitter.x,
                .y = src.y + step.y * i + jitter.y,
                .z = src.z + step.z * i + jitter.z,
            };
        }
    }
}

static int32_t M_GetViewDepth(const XYZ_32 pos)
{
    // clang-format off
    return (int32_t)((
        g_ViewMatrix._20 * pos.x +
        g_ViewMatrix._21 * pos.y +
        g_ViewMatrix._22 * pos.z +
        g_ViewMatrix._23) >> W2V_SHIFT);
    // clang-format on
}

static RGBA_8888 M_FadeColor(
    const RGB_888 color, const int32_t keep, const int32_t shift)
{
    return (RGBA_8888) {
        .r = (color.r * keep) >> shift,
        .g = (color.g * keep) >> shift,
        .b = (color.b * keep) >> shift,
        .a = 0xFF,
    };
}

static void M_GetGlowScreenPos(
    const XYZ_32 pos, const int32_t depth, int32_t *const out_x,
    int32_t *const out_y)
{
    // clang-format off
    const int64_t x = (
        g_ViewMatrix._00 * pos.x +
        g_ViewMatrix._01 * pos.y +
        g_ViewMatrix._02 * pos.z +
        g_ViewMatrix._03) >> W2V_SHIFT;
    const int64_t y = (
        g_ViewMatrix._10 * pos.x +
        g_ViewMatrix._11 * pos.y +
        g_ViewMatrix._12 * pos.z +
        g_ViewMatrix._13) >> W2V_SHIFT;
    // clang-format on
    *out_x = (int32_t)(x * M_GLOW_REF_PERSP / depth);
    *out_y = (int32_t)(y * M_GLOW_REF_PERSP / depth);
}

// Returns the half width of the glow in world units. The original game sets
// a base width in screen pixels, narrowing it with depth down to a floor, and
// scales it by the screen length of the segment divided by 16. Its
// normalisation of the side vector measures that length only in steps of 256
// pixels, so it divides by those steps only from 512 pixels up. The screen is
// taken as a 1280x960 viewport, so that the glow matches the original game at
// that resolution and does not depend on the render resolution.
static float M_GetGlowHalfWidth(
    const M_BOLT *const bolt, const XYZ_32 from, const XYZ_32 to,
    const int32_t depth)
{
    const int32_t to_depth = M_GetViewDepth(to);
    if (depth <= 0 || to_depth <= 0) {
        return 0.0f;
    }

    int32_t pixels = depth < M_GLOW_WIDTH_END
        ? ((M_GLOW_WIDTH_END - depth) * bolt->size) >> 16
        : 0;
    CLAMPL(pixels, M_GLOW_MIN_WIDTH);

    int32_t from_x;
    int32_t from_y;
    int32_t to_x;
    int32_t to_y;
    M_GetGlowScreenPos(from, depth, &from_x, &from_y);
    M_GetGlowScreenPos(to, to_depth, &to_x, &to_y);
    const int32_t dx = from_x - to_x;
    const int32_t dy = from_y - to_y;
    const int32_t coarse_x = dx >> M_GLOW_COARSE_SHIFT;
    const int32_t coarse_y = dy >> M_GLOW_COARSE_SHIFT;
    int32_t steps =
        (int32_t)sqrtf((float)(coarse_x * coarse_x + coarse_y * coarse_y));
    CLAMPL(steps, 1);

    const float length = sqrtf((float)dx * dx + (float)dy * dy);
    const float half_width = length * pixels / (16.0f * steps);
    return half_width * depth / (float)M_GLOW_REF_PERSP;
}

static bool M_IsInDrawRange(const M_BOLT *const bolt)
{
    const XYZ_32 lara_pos = Lara_GetItem()->pos;
    const XYZ_32 src = bolt->knots[0];
    return ABS(src.x - lara_pos.x) <= M_DRAW_RANGE
        && ABS(src.y - lara_pos.y) <= M_DRAW_RANGE
        && ABS(src.z - lara_pos.z) <= M_DRAW_RANGE;
}

static void M_DrawBolt(
    const M_BOLT *const bolt, const int32_t sprite_idx, const double ratio)
{
    XYZ_32 points[M_MAX_POINTS];
    M_GetPoints(bolt, ratio, points);

    RGB_888 core = { M_CORE_SHADE, M_CORE_SHADE, M_CORE_SHADE };
    RGB_888 glow = bolt->color;
    if (bolt->life < M_FADE_LIFE) {
        const RGBA_8888 faded_core = M_FadeColor(core, bolt->life, 4);
        const RGBA_8888 faded_glow = M_FadeColor(glow, bolt->life, 4);
        core = (RGB_888) { faded_core.r, faded_core.g, faded_core.b };
        glow = (RGB_888) { faded_glow.r, faded_glow.g, faded_glow.b };
    }

    const int32_t count = M_GetPointCount(bolt);
    for (int32_t i = 0; i < count - 1; i++) {
        const int32_t depth = M_GetViewDepth(points[i]);
        if (depth >= M_FADE_END) {
            continue;
        }

        const int32_t keep = depth > M_FADE_START ? M_FADE_END - depth : 1;
        const int32_t shift = depth > M_FADE_START ? M_FADE_SHIFT : 0;
        const RGBA_8888 core_color = M_FadeColor(core, keep, shift);
        OutputSource_PolyFX_StageLineSegment(
            points[i], core_color, points[i + 1], core_color, M_CORE_HALF_WIDTH,
            DRAW_BLEND_ADD);

        if (sprite_idx != NO_ITEM) {
            const RGBA_8888 glow_color = M_FadeColor(glow, keep, shift);
            OutputSource_PolyFX_StageSpriteLineSegment(
                sprite_idx, points[i], glow_color, points[i + 1], glow_color,
                M_GetGlowHalfWidth(bolt, points[i], points[i + 1], depth),
                DRAW_BLEND_ADD);
        }
    }
}

static void M_Control(void)
{
    for (int32_t i = 0; i < M_MAX_BOLTS; i++) {
        M_BOLT *const bolt = &m_Bolts[i];
        if (bolt->life == 0) {
            continue;
        }

        memcpy(bolt->prev_knots, bolt->knots, sizeof(bolt->knots));
        bolt->life = bolt->life > M_LIFE_STEP ? bolt->life - M_LIFE_STEP : 0;
        if (bolt->life == 0) {
            continue;
        }

        for (int32_t j = 0; j < M_KNOTS - 1; j++) {
            XYZ_32 *const knot = &bolt->knots[j + 1];
            XYZ_16 *const vel = &bolt->vel[j];
            knot->x += vel->x * 2;
            knot->y += vel->y * 2;
            knot->z += vel->z * 2;
            vel->x -= vel->x >> 4;
            vel->y -= vel->y >> 4;
            vel->z -= vel->z >> 4;
        }
        M_Jitter(bolt);
    }
}

static void M_Draw(void)
{
    if (Lara_GetItem() == nullptr) {
        return;
    }

    const int32_t sprite_idx = Sparks_GetSpriteIndex(SPARK_TYPE_LIGHTNING);
    const double ratio = Interpolation_GetWorldRate();
    for (int32_t i = 0; i < M_MAX_BOLTS; i++) {
        const M_BOLT *const bolt = &m_Bolts[i];
        if (bolt->life != 0 && M_IsInDrawRange(bolt)) {
            M_DrawBolt(bolt, sprite_idx, ratio);
        }
    }
}

static void M_Save(JSON_WRITE_IO *const io)
{
    JSONW_PUSH_ARRAY(io);
    for (int32_t i = 0; i < M_MAX_BOLTS; i++) {
        const M_BOLT *const bolt = &m_Bolts[i];
        if (bolt->life == 0) {
            continue;
        }
        JSONW_PUSH_OBJECT(io);
        JSONW_PUSH_ARRAY(io);
        for (int32_t j = 0; j < M_KNOTS; j++) {
            JSONW_PUSH_VALUE(io, bolt->knots[j]);
            JSONW_POP_AND_APPEND(io);
        }
        JSONW_POP_AND_SET(io, "knots");
        JSONW_PUSH_ARRAY(io);
        for (int32_t j = 0; j < M_KNOTS - 1; j++) {
            JSONW_PUSH_VALUE(io, bolt->vel[j]);
            JSONW_POP_AND_APPEND(io);
        }
        JSONW_POP_AND_SET(io, "vel");
        JSONW_WRITE(io, "color", bolt->color);
        JSONW_WRITE(io, "life", bolt->life);
        JSONW_WRITE(io, "size", bolt->size);
        JSONW_WRITE(io, "variation", bolt->variation);
        JSONW_WRITE(io, "segments", bolt->segments);
        JSONW_WRITE(io, "flags", (int32_t)bolt->flags);
        JSONW_POP_AND_APPEND(io);
    }
    JSONW_POP_AND_SET_NZ(io, "lightning");
}

static RESULT M_LoadBolt(JSON_READ_IO *const io, M_BOLT *const bolt)
{
    MUST(JSON_PUSH(io, "knots"));
    for (int32_t i = 0; i < M_KNOTS; i++) {
        MUST(JSON_READ_A(io, i, &bolt->knots[i]));
    }
    MUST(JSON_POP(io));
    MUST(JSON_PUSH(io, "vel"));
    for (int32_t i = 0; i < M_KNOTS - 1; i++) {
        MUST(JSON_READ_A(io, i, &bolt->vel[i]));
    }
    MUST(JSON_POP(io));

    int32_t flags = 0;
    MUST(JSON_READ(io, "color", &bolt->color));
    MUST(JSON_READ(io, "life", &bolt->life));
    MUST(JSON_READ(io, "size", &bolt->size));
    MUST(JSON_READ(io, "variation", &bolt->variation));
    MUST(JSON_READ(io, "segments", &bolt->segments));
    MUST(JSON_READ(io, "flags", &flags));
    bolt->flags = flags;
    bolt->variation = MAX(bolt->variation, 1);
    CLAMP(bolt->segments, 1, M_MAX_SEGMENTS);
    memcpy(bolt->prev_knots, bolt->knots, sizeof(bolt->knots));
    M_Jitter(bolt);
    return OK;
}

static RESULT M_Load(JSON_READ_IO *const io)
{
    if (!JSON_ReadIO_HasKey(io, "lightning")) {
        return OK;
    }
    MUST(JSON_PUSH(io, "lightning"));

    const int32_t count = JSON_ARRAY_LEN(io);
    for (int32_t i = 0; i < count; i++) {
        if (i >= M_MAX_BOLTS) {
            LOG_WARNING(
                "Malformed save: too many lightning bolts. Extra bolts will "
                "be ignored.");
            break;
        }
        MUST(JSON_PUSH_INDEX(io, i));
        MUST(M_LoadBolt(io, &m_Bolts[i]));
        MUST(JSON_POP(io));
    }

    MUST(JSON_POP(io));
    return OK;
}

void FX_Lightning_Create(const FX_LIGHTNING_SETUP *const setup)
{
    M_BOLT *bolt = nullptr;
    for (int32_t i = 0; i < M_MAX_BOLTS; i++) {
        if (m_Bolts[i].life == 0) {
            bolt = &m_Bolts[i];
            break;
        }
    }
    if (bolt == nullptr) {
        return;
    }

    const XYZ_32 src = setup->src;
    const XYZ_32 dst = setup->dst;
    uint8_t segments = setup->segments;
    CLAMP(segments, 1, M_MAX_SEGMENTS);
    *bolt = (M_BOLT) {
        .knots = {
            src,
            {
                .x = (src.x * 3 + dst.x) >> 2,
                .y = (src.y * 3 + dst.y) >> 2,
                .z = (src.z * 3 + dst.z) >> 2,
            },
            {
                .x = (dst.x * 3 + src.x) >> 2,
                .y = (dst.y * 3 + src.y) >> 2,
                .z = (dst.z * 3 + src.z) >> 2,
            },
            dst,
        },
        .color = setup->color,
        .life = setup->life,
        .size = setup->size,
        .variation = MAX(setup->variation, 1),
        .segments = segments,
        .flags = setup->flags,
    };

    for (int32_t i = 0; i < M_KNOTS - 1; i++) {
        const bool is_end = i == M_KNOTS - 2;
        if (is_end && (bolt->flags & FX_LIGHTNING_F_MOVING_END) == 0) {
            continue;
        }
        bolt->vel[i].x = M_GetVelocity(bolt->variation);
        bolt->vel[i].y = M_GetVelocity(bolt->variation);
        bolt->vel[i].z = M_GetVelocity(bolt->variation);
    }

    memcpy(bolt->prev_knots, bolt->knots, sizeof(bolt->knots));
    M_Jitter(bolt);
}

void FX_Lightning_CreateGlow(
    const XYZ_32 pos, const RGB_888 color, const uint8_t size)
{
    const XYZ_32 lara_pos = Lara_GetItem()->pos;
    if (ABS(lara_pos.x - pos.x) > M_GLOW_RANGE
        || ABS(lara_pos.z - pos.z) > M_GLOW_RANGE) {
        return;
    }

    const int32_t sprite_idx = Sparks_GetSpriteIndex(SPARK_TYPE_LENS_FLARE_1);
    if (sprite_idx == NO_ITEM) {
        return;
    }

    SPARK *const spark = Sparks_InitialiseSpark(SPARK_CONTEXT_ELECTRICITY);
    if (spark == nullptr) {
        return;
    }

    const uint8_t glow_size = size + (Random_GetControl() & 3);
    *spark = (SPARK) {
        .on = true,
        .context = SPARK_CONTEXT_ELECTRICITY,
        .src_color = color,
        .dst_color = color,
        .col_fade_speed = M_GLOW_FADE_SPEED,
        .draw_type = DRAW_BLEND_ADD,
        .life = M_GLOW_LIFE,
        .s_life = M_GLOW_LIFE,
        .dynamic = -1,
        .sprite_idx = sprite_idx,
        .sprite_obj_id = O_SPARKS_GFX,
        .pos = pos,
        .flags = SPARK_F_SCALE | SPARK_F_SPRITE,
        .scalar = M_GLOW_SCALAR,
        .src_size = { glow_size, glow_size },
        .dst_size = { glow_size, glow_size },
        .size = { glow_size, glow_size },
    };
    Sparks_FinishSetup(spark);
}

static const FX_MODULE m_Module = {
    .control_func = M_Control,
    .draw_func = M_Draw,
    .reset_func = M_Reset,
    .save_key = "lightning",
    .save_func = M_Save,
    .load_func = M_Load,
};

REGISTER_FX(m_Module)
