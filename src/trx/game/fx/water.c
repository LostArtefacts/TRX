#include <trx/game/fx/water.h>

#include <trx/config.h>
#include <trx/core/json/util/read_io.h>
#include <trx/core/json/util/write_io.h>
#include <trx/core/utils.h>
#include <trx/game/fx/common.h>
#include <trx/game/interpolation.h>
#include <trx/game/items.h>
#include <trx/game/lara/common.h>
#include <trx/game/objects.h>
#include <trx/game/output/sources/poly_fx.h>
#include <trx/game/output/state.h>
#include <trx/game/random.h>
#include <trx/game/rooms.h>
#include <trx/game/sound.h>
#include <trx/version.h>

#include <string.h>

#define M_SPLASH_Z_DEPTH_ADJUST -0.005f
#define M_RIPPLE_Z_DEPTH_ADJUST -0.005f

static const int16_t m_SplashRings[8][2] = {
    { 0, -24 }, { 17, -17 }, { 24, 0 },  { 17, 17 },
    { 0, 24 },  { -17, 17 }, { -24, 0 }, { -17, -17 },
};

static const uint8_t m_SplashQuadLinks[32] = {
    8,  9,  0, 1, 9,  10, 1, 2, 10, 11, 2, 3, 11, 12, 3, 4,
    12, 13, 4, 5, 13, 14, 5, 6, 14, 15, 6, 7, 15, 8,  7, 0,
};

static FX_WATER_SPLASH m_Splashes[4];
static FX_WATER_RIPPLE m_Ripples[16];
static int32_t m_SplashCount = 0;

static void M_Reset(void)
{
    memset(m_Splashes, 0, sizeof(m_Splashes));
    memset(m_Ripples, 0, sizeof(m_Ripples));
    m_SplashCount = 0;
}

static bool M_IsRoomUnderwater(const int16_t room_num)
{
    return Room_Get(room_num)->flags.underwater;
}

static void M_RememberRipple(FX_WATER_RIPPLE *const ripple)
{
    ripple->prev_size = ripple->size;
    ripple->prev_life = ripple->life;
    ripple->prev_init = ripple->init;
}

static void M_RememberSplash(FX_WATER_SPLASH *const splash)
{
    splash->prev_life = splash->life;
    for (int32_t i = 0; i < 48; i++) {
        FX_WATER_SPLASH_VERT *const v = &splash->v[i];
        v->prev_pos = v->pos;
    }
}

static bool M_GetUnderwaterBloodColor(
    RGBA_8888 *const color, const int32_t shade)
{
    switch (g_Config.visuals.blood_effects) {
    case BLOOD_EFFECTS_DISABLED:
        return false;
    case BLOOD_EFFECTS_PINK:
        *color = (RGBA_8888) { shade / 2, 0, shade, 255 };
        return true;
    case BLOOD_EFFECTS_RED:
        *color = g_TRVersion == 4
            ? (RGBA_8888) { (shade >> 1) << 1, 0, (shade >> 4) << 1, 255 }
            : (RGBA_8888) { shade, 0, 0, 255 };
        return true;
    case BLOOD_EFFECTS_NUMBER_OF:
        break;
    }
    return false;
}

static RGBA_8888 M_Gray(int32_t c)
{
    CLAMP(c, 0, 255);
    return (RGBA_8888) { c, c, c, 255 };
}

static void M_DrawSplash(const FX_WATER_SPLASH *s)
{
    const int32_t big_splash_idx = Sparks_GetSpriteIndex(SPARK_TYPE_BIG_SPLASH);
    const int32_t small_splash_idx =
        Sparks_GetSpriteIndex(SPARK_TYPE_SMALL_SPLASH);
    if (big_splash_idx == NO_ITEM || small_splash_idx == NO_ITEM) {
        return;
    }

    const double ratio = Interpolation_GetWorldRate();
    const bool do_interp =
        Interpolation_IsActive() && ratio > 0.0 && ratio < 1.0;
    const int32_t time4 = Output_GetTimeInGame() * 4;

    XYZ_32 points[48];
    for (int32_t i = 0; i < 48; i++) {
        const FX_WATER_SPLASH_VERT *const v = &s->v[i];
        points[i] = (XYZ_32) {
            .x = s->pos.x
                + ((do_interp ? (int16_t)LERP(v->prev_pos.x, v->pos.x, ratio)
                              : v->pos.x)
                   >> 4),
            .y = s->pos.y
                + (do_interp ? (int16_t)LERP(v->prev_pos.y, v->pos.y, ratio)
                             : v->pos.y),
            .z = s->pos.z
                + ((do_interp ? (int16_t)LERP(v->prev_pos.z, v->pos.z, ratio)
                              : v->pos.z)
                   >> 4),
        };
    }

    for (int32_t ring = 0; ring < 3; ring++) {
        int32_t sprite_idx = big_splash_idx;
        if (ring == 2 || (ring == 0 && (s->flags & 4U) != 0U)
            || (ring == 1 && (s->flags & 8U) != 0U)) {
            sprite_idx = small_splash_idx + ((time4 >> 4) & 3);
        }

        const int32_t life = do_interp
            ? (int32_t)LERP(s->prev_life, s->life, ratio)
            : (int32_t)s->life;

        int32_t c = life << 1;
        CLAMPG(c, 255);
        const RGBA_8888 c1 = M_Gray(c);

        c = (life - (life >> 2)) << 1;
        CLAMPG(c, 255);
        const RGBA_8888 c2 = M_Gray(c);

        const int32_t base = ring * 16;
        for (int32_t quad = 0; quad < 8; quad++) {
            const int32_t i0 = m_SplashQuadLinks[quad * 4 + 0] + base;
            const int32_t i1 = m_SplashQuadLinks[quad * 4 + 1] + base;
            const int32_t i2 = m_SplashQuadLinks[quad * 4 + 2] + base;
            const int32_t i3 = m_SplashQuadLinks[quad * 4 + 3] + base;

            const XYZ_32 quad_pos[4] = {
                points[i0],
                points[i1],
                points[i3],
                points[i2],
            };
            const RGBA_8888 quad_color[4] = { c1, c1, c2, c2 };
            OutputSource_PolyFX_StageSpriteQuadWorldDepth(
                sprite_idx, quad_pos, quad_color, M_SPLASH_Z_DEPTH_ADJUST,
                DRAW_BLEND_ADD);
        }
    }
}

static void M_DrawRipple(const FX_WATER_RIPPLE *r)
{
    const double ratio = Interpolation_GetWorldRate();
    const bool do_interp =
        Interpolation_IsActive() && ratio > 0.0 && ratio < 1.0;
    const int32_t size = do_interp ? (int32_t)LERP(r->prev_size, r->size, ratio)
                                   : (int32_t)r->size;
    const int32_t init = do_interp ? (int32_t)LERP(r->prev_init, r->init, ratio)
                                   : (int32_t)r->init;
    const int32_t life = do_interp ? (int32_t)LERP(r->prev_life, r->life, ratio)
                                   : (int32_t)r->life;
    const int32_t n = g_TRVersion == 4 ? size : size << 2;
    int32_t sprite_idx = Sparks_GetSpriteIndex(SPARK_TYPE_RIPPLE);
    RGBA_8888 color;

    const int32_t fade = init != 0 ? init : life;
    if (g_TRVersion == 4) {
        if ((r->flags & FX_RIPPLE_BLOOD) != 0U) {
            sprite_idx = Sparks_GetSpriteIndex(SPARK_TYPE_EXPLOSION);
        }
        if ((r->flags & FX_RIPPLE_DARK) != 0U) {
            if ((r->flags & FX_RIPPLE_BLOOD) != 0U) {
                color = (RGBA_8888) {
                    .r = MIN((fade >> 1) << 1, 255),
                    .g = 0,
                    .b = MIN((fade >> 4) << 1, 255),
                    .a = 255,
                };
            } else {
                color = M_Gray(MIN(fade << 1, 255));
            }
        } else {
            color = M_Gray(MIN(fade << 2, 255));
        }
    } else if ((r->flags & FX_RIPPLE_DARK) != 0U) {
        if ((r->flags & FX_RIPPLE_BLOOD) != 0U) {
            sprite_idx = Sparks_GetSpriteIndex(SPARK_TYPE_EXPLOSION);
            const int32_t shade = g_TRVersion == 4 && init != 0 ? init : life;
            if (!M_GetUnderwaterBloodColor(&color, shade)) {
                return;
            }
        } else {
            int32_t c1 = (fade >> 2) << 3;
            CLAMPG(c1, 255);
            color = M_Gray(c1);
        }
    } else {
        int32_t c1 = (fade >> 1) << 3;
        CLAMPG(c1, 255);
        color = M_Gray(c1);
    }

    if (sprite_idx == NO_ITEM) {
        return;
    }

    const XYZ_32 quad_pos[4] = {
        { r->pos.x - n, r->pos.y, r->pos.z - n },
        { r->pos.x + n, r->pos.y, r->pos.z - n },
        { r->pos.x + n, r->pos.y, r->pos.z + n },
        { r->pos.x - n, r->pos.y, r->pos.z + n },
    };
    const RGBA_8888 quad_color[4] = { color, color, color, color };
    OutputSource_PolyFX_StageSpriteQuadWorldDepth(
        sprite_idx, quad_pos, quad_color, M_RIPPLE_Z_DEPTH_ADJUST,
        DRAW_BLEND_ADD);
}

static void M_Draw(void)
{
    for (int32_t i = 0; i < (int32_t)ARRAY_SIZE(m_Splashes); i++) {
        const FX_WATER_SPLASH *const splash = &m_Splashes[i];
        if ((splash->flags & 1U) == 0U) {
            continue;
        }
        M_DrawSplash(splash);
    }

    for (int32_t i = 0; i < (int32_t)ARRAY_SIZE(m_Ripples); i++) {
        const FX_WATER_RIPPLE *const ripple = &m_Ripples[i];
        if ((ripple->flags & FX_RIPPLE_ACTIVE) == 0U) {
            continue;
        }
        M_DrawRipple(ripple);
    }
}

static void M_Control(void)
{
    if (m_SplashCount > 0) {
        m_SplashCount--;
    }

    for (int32_t i = 0; i < (int32_t)ARRAY_SIZE(m_Splashes); i++) {
        FX_WATER_SPLASH *const splash = &m_Splashes[i];
        if ((splash->flags & 1U) == 0U) {
            continue;
        }

        M_RememberSplash(splash);

        bool set = false;
        for (int32_t j = 0; j < 48; j++) {
            FX_WATER_SPLASH_VERT *const v = &splash->v[j];
            v->pos.x += v->vel.x >> 2;
            v->pos.y += (int16_t)(v->vel.y >> 6);
            v->pos.z += v->vel.z >> 2;
            v->vel.x -= v->vel.x >> v->friction;
            v->vel.z -= v->vel.z >> v->friction;

            if ((v->min_vel.x < 0 && v->vel.x > v->min_vel.x)
                || (v->min_vel.x > 0 && v->vel.x < v->min_vel.x)) {
                v->vel.x = v->min_vel.x;
            } else if (
                (v->min_vel.z < 0 && v->vel.z > v->min_vel.z)
                || (v->min_vel.z > 0 && v->vel.z < v->min_vel.z)) {
                v->vel.z = v->min_vel.z;
            }

            v->vel.y += (int32_t)v->gravity << 3;
            CLAMPG(v->vel.y, 0x10000);

            if (v->pos.y > 0) {
                if (j < 16) {
                    splash->flags |= 4U;
                } else if (j < 32) {
                    splash->flags |= 8U;
                }

                v->pos.y = 0;
                set = true;
            }
        }

        if (set) {
            splash->life--;
            if (splash->life == 0U) {
                splash->flags = 0U;
            }
        }
    }

    for (int32_t i = 0; i < (int32_t)ARRAY_SIZE(m_Ripples); i++) {
        FX_WATER_RIPPLE *const ripple = &m_Ripples[i];
        if ((ripple->flags & FX_RIPPLE_ACTIVE) == 0U) {
            continue;
        }

        M_RememberRipple(ripple);

        const bool is_slow = (ripple->flags & FX_RIPPLE_SLOW) != 0U;
        const bool is_tr4 = g_TRVersion == 4;

        if (ripple->size < (is_tr4 ? 252U : 254U)) {
            ripple->size += is_tr4 && !is_slow ? 4U : 2U;
        }

        if (ripple->init == 0U) {
            ripple->life -= is_tr4 ? 3U : 2U;
            if (ripple->life > 250U) {
                ripple->flags = 0U;
            }
        } else if (ripple->init < ripple->life) {
            ripple->init += is_tr4 && is_slow ? 8U : 4U;
            if (ripple->init >= ripple->life) {
                ripple->init = 0U;
            }
        }
    }
}

static void M_SaveSplashes(JSON_WRITE_IO *const io)
{
    JSONW_PUSH_ARRAY(io);
    for (int32_t i = 0; i < (int32_t)ARRAY_SIZE(m_Splashes); i++) {
        const FX_WATER_SPLASH *const splash = &m_Splashes[i];
        if ((splash->flags & 1U) == 0U) {
            continue;
        }
        JSONW_PUSH_OBJECT(io);
        JSONW_WRITE(io, "pos", splash->pos);
        JSONW_WRITE(io, "flags", splash->flags);
        JSONW_WRITE(io, "life", splash->life);
        JSONW_PUSH_ARRAY(io);
        for (int32_t j = 0; j < (int32_t)ARRAY_SIZE(splash->v); j++) {
            const FX_WATER_SPLASH_VERT *const v = &splash->v[j];
            JSONW_PUSH_OBJECT(io);
            JSONW_WRITE(io, "pos", v->pos);
            JSONW_WRITE(io, "vel", v->vel);
            JSONW_WRITE(io, "min_vel", v->min_vel);
            JSONW_WRITE(io, "friction", v->friction);
            JSONW_WRITE(io, "gravity", v->gravity);
            JSONW_POP_AND_APPEND(io);
        }
        JSONW_POP_AND_SET(io, "verts");
        JSONW_POP_AND_APPEND(io);
    }
    JSONW_POP_AND_SET_NZ(io, "splashes");
}

static void M_SaveRipples(JSON_WRITE_IO *const io)
{
    JSONW_PUSH_ARRAY(io);
    for (int32_t i = 0; i < (int32_t)ARRAY_SIZE(m_Ripples); i++) {
        const FX_WATER_RIPPLE *const ripple = &m_Ripples[i];
        if ((ripple->flags & FX_RIPPLE_ACTIVE) == 0U) {
            continue;
        }
        JSONW_PUSH_OBJECT(io);
        JSONW_WRITE(io, "pos", ripple->pos);
        JSONW_WRITE(io, "flags", ripple->flags);
        JSONW_WRITE(io, "life", ripple->life);
        JSONW_WRITE(io, "size", ripple->size);
        JSONW_WRITE(io, "init", ripple->init);
        JSONW_POP_AND_APPEND(io);
    }
    JSONW_POP_AND_SET_NZ(io, "ripples");
}

static void M_Save(JSON_WRITE_IO *const io)
{
    JSONW_WRITE_NZ(io, "splash_count", m_SplashCount);
    M_SaveSplashes(io);
    M_SaveRipples(io);
}

static RESULT M_LoadSplashes(JSON_READ_IO *const io)
{
    const int32_t count = JSON_ARRAY_LEN(io);
    for (int32_t i = 0; i < count; i++) {
        if (i >= (int32_t)ARRAY_SIZE(m_Splashes)) {
            LOG_WARNING(
                "Malformed save: too many splashes. Extra splashes will be "
                "ignored.");
            break;
        }

        FX_WATER_SPLASH *const splash = &m_Splashes[i];
        MUST(JSON_PUSH_INDEX(io, i));
        MUST(JSON_READ(io, "pos", &splash->pos));
        MUST(JSON_READ(io, "flags", &splash->flags));
        MUST(JSON_READ(io, "life", &splash->life));
        MUST(JSON_PUSH(io, "verts"));
        const int32_t vert_count = JSON_ARRAY_LEN(io);
        if (vert_count != (int32_t)ARRAY_SIZE(splash->v)) {
            return JSON_ReadIO_Fail(
                io, "a splash holds %d points, not %d", vert_count,
                (int32_t)ARRAY_SIZE(splash->v));
        }
        for (int32_t j = 0; j < vert_count; j++) {
            FX_WATER_SPLASH_VERT *const v = &splash->v[j];
            MUST(JSON_PUSH_INDEX(io, j));
            MUST(JSON_READ(io, "pos", &v->pos));
            MUST(JSON_READ(io, "vel", &v->vel));
            MUST(JSON_READ(io, "min_vel", &v->min_vel));
            MUST(JSON_READ(io, "friction", &v->friction));
            MUST(JSON_READ(io, "gravity", &v->gravity));
            MUST(JSON_POP(io));
        }
        MUST(JSON_POP(io));
        MUST(JSON_POP(io));
        M_RememberSplash(splash);
    }
    return OK;
}

static RESULT M_LoadRipples(JSON_READ_IO *const io)
{
    const int32_t count = JSON_ARRAY_LEN(io);
    for (int32_t i = 0; i < count; i++) {
        if (i >= (int32_t)ARRAY_SIZE(m_Ripples)) {
            LOG_WARNING(
                "Malformed save: too many ripples. Extra ripples will be "
                "ignored.");
            break;
        }

        FX_WATER_RIPPLE *const ripple = &m_Ripples[i];
        MUST(JSON_PUSH_INDEX(io, i));
        MUST(JSON_READ(io, "pos", &ripple->pos));
        MUST(JSON_READ(io, "flags", &ripple->flags));
        MUST(JSON_READ(io, "life", &ripple->life));
        MUST(JSON_READ(io, "size", &ripple->size));
        MUST(JSON_READ(io, "init", &ripple->init));
        MUST(JSON_POP(io));
        M_RememberRipple(ripple);
    }
    return OK;
}

static RESULT M_Load(JSON_READ_IO *const io)
{
    MUST(JSON_READ_D(io, "splash_count", &m_SplashCount, 0));
    if (JSON_ReadIO_HasKey(io, "splashes")) {
        MUST(JSON_PUSH(io, "splashes"));
        MUST(M_LoadSplashes(io));
        MUST(JSON_POP(io));
    }
    if (JSON_ReadIO_HasKey(io, "ripples")) {
        MUST(JSON_PUSH(io, "ripples"));
        MUST(M_LoadRipples(io));
        MUST(JSON_POP(io));
    }
    return OK;
}

FX_WATER_RIPPLE *FX_Water_SetupRipple(
    const XYZ_32 pos, const int32_t size, const uint32_t flags)
{
    int32_t idx = -1;
    for (int32_t i = 0; i < (int32_t)ARRAY_SIZE(m_Ripples); i++) {
        if ((m_Ripples[i].flags & FX_RIPPLE_ACTIVE) == 0U) {
            idx = i;
            break;
        }
    }
    if (idx < 0) {
        return nullptr;
    }

    FX_WATER_RIPPLE *const ripple = &m_Ripples[idx];
    ripple->flags = (uint8_t)(flags | FX_RIPPLE_ACTIVE);
    ripple->init = 1U;
    ripple->size = (uint8_t)size;
    ripple->life = (uint8_t)((Random_GetControl() & 0xF) + 48);
    ripple->pos = pos;
    if ((flags & FX_RIPPLE_JITTER) != 0U) {
        ripple->pos.x += (Random_GetControl() & 0x7F) - 64;
        ripple->pos.z += (Random_GetControl() & 0x7F) - 64;
    }
    M_RememberRipple(ripple);
    return ripple;
}

void FX_Water_SetupSplash(const FX_WATER_SPLASH_SETUP *const setup_)
{
    FX_WATER_SPLASH_SETUP setup = *setup_;

    int32_t idx = -1;
    for (int32_t i = 0; i < (int32_t)ARRAY_SIZE(m_Splashes); i++) {
        if ((m_Splashes[i].flags & 1U) == 0U) {
            idx = i;
            break;
        }
    }

    if (idx < 0) {
        Sound_Effect(
            SFX_LARA_SPLASH,
            &(XYZ_32) { setup.pos.x, setup.pos.y, setup.pos.z }, SPM_NORMAL);
        return;
    }

    FX_WATER_SPLASH *const splash = &m_Splashes[idx];
    splash->flags = 3U;

    if (setup.outer_friction == -9) {
        splash->flags = 67U;
        setup.outer_friction = 9;
    }

    splash->pos.x = setup.pos.x;
    splash->pos.y = setup.pos.y;
    splash->pos.z = setup.pos.z;
    splash->life = 63U;

    FX_WATER_SPLASH_VERT *v = splash->v;

    for (int32_t i = 0; i < 8; i++) {
        v->pos.x = (setup.inner_xz_off * m_SplashRings[i][0]) * 2;
        v->pos.y = 0;
        v->pos.z = (setup.inner_xz_off * m_SplashRings[i][1]) * 2;
        v->vel.x = (setup.inner_xz_vel * m_SplashRings[i][0]) / 12;
        v->vel.y = 0;
        v->vel.z = (setup.inner_xz_vel * m_SplashRings[i][1]) / 12;
        v->min_vel.x = v->vel.x >> 3;
        v->min_vel.z = v->vel.z >> 3;
        v->gravity = 0;
        v->friction = (uint8_t)(setup.inner_friction - 2);
        v->prev_pos = v->pos;
        v++;
    }

    for (int32_t i = 0; i < 8; i++) {
        v->pos.x =
            ((setup.inner_xz_off + setup.inner_xz_size) * m_SplashRings[i][0])
            * 2;
        v->pos.y = setup.inner_y_size;
        v->pos.z =
            ((setup.inner_xz_off + setup.inner_xz_size) * m_SplashRings[i][1])
            * 2;
        v->vel.x = (setup.inner_xz_vel * m_SplashRings[i][0]) >> 3;
        v->vel.y = setup.inner_y_vel;
        v->vel.z = (setup.inner_xz_vel * m_SplashRings[i][1]) >> 3;
        v->min_vel.x = v->vel.x >> 3;
        v->min_vel.z = v->vel.z >> 3;
        v->gravity = (uint8_t)setup.inner_gravity;
        v->friction = (uint8_t)setup.inner_friction;
        v->prev_pos = v->pos;
        v++;
    }

    for (int32_t i = 0; i < 8; i++) {
        v->pos.x = (setup.middle_xz_off * m_SplashRings[i][0]) * 2;
        v->pos.y = 0;
        v->pos.z = (setup.middle_xz_off * m_SplashRings[i][1]) * 2;
        v->vel.x = (setup.middle_xz_vel * m_SplashRings[i][0]) / 12;
        v->vel.y = 0;
        v->vel.z = (setup.middle_xz_vel * m_SplashRings[i][1]) / 12;
        v->min_vel.x = v->vel.x >> 3;
        v->min_vel.z = v->vel.z >> 3;
        v->gravity = 0;
        v->friction = (uint8_t)(setup.middle_friction - 2);
        v->prev_pos = v->pos;
        v++;
    }

    for (int32_t i = 0; i < 8; i++) {
        v->pos.x =
            ((setup.middle_xz_off + setup.middle_xz_size) * m_SplashRings[i][0])
            * 2;
        v->pos.y = setup.middle_y_size;
        v->pos.z =
            ((setup.middle_xz_off + setup.middle_xz_size) * m_SplashRings[i][1])
            * 2;
        v->vel.x = (setup.middle_xz_vel * m_SplashRings[i][0]) >> 3;
        v->vel.y = setup.middle_y_vel;
        v->vel.z = (setup.middle_xz_vel * m_SplashRings[i][1]) >> 3;
        v->min_vel.x = v->vel.x >> 3;
        v->min_vel.z = v->vel.z >> 3;
        v->gravity = (uint8_t)setup.middle_gravity;
        v->friction = (uint8_t)setup.middle_friction;
        v->prev_pos = v->pos;
        v++;
    }

    for (int32_t i = 0; i < 8; i++) {
        v->pos.x = (setup.outer_xz_off * m_SplashRings[i][0]) * 2;
        v->pos.y = 0;
        v->pos.z = (setup.outer_xz_off * m_SplashRings[i][1]) * 2;
        v->vel.x = (setup.outer_xz_vel * m_SplashRings[i][0]) / 12;
        v->vel.y = 0;
        v->vel.z = (setup.outer_xz_vel * m_SplashRings[i][1]) / 12;
        v->min_vel.x = v->vel.x >> 3;
        v->min_vel.z = v->vel.z >> 3;
        v->gravity = 0;
        v->friction = (uint8_t)(setup.outer_friction - 2);
        v->prev_pos = v->pos;
        v++;
    }

    for (int32_t i = 0; i < 8; i++) {
        v->pos.x =
            ((setup.outer_xz_off + setup.outer_xz_size) * m_SplashRings[i][0])
            * 2;
        v->pos.y = 0;
        v->pos.z =
            ((setup.outer_xz_off + setup.outer_xz_size) * m_SplashRings[i][1])
            * 2;
        v->vel.x = (setup.outer_xz_vel * m_SplashRings[i][0]) >> 3;
        v->vel.y = 0;
        v->vel.z = (setup.outer_xz_vel * m_SplashRings[i][1]) >> 3;
        v->min_vel.x = v->vel.x >> 3;
        v->min_vel.z = v->vel.z >> 3;
        v->gravity = 0;
        v->friction = (uint8_t)setup.outer_friction;
        v->prev_pos = v->pos;
        v++;
    }
    splash->prev_life = splash->life;

    Sound_Effect(
        SFX_LARA_SPLASH, &(XYZ_32) { setup.pos.x, setup.pos.y, setup.pos.z },
        SPM_NORMAL);
}

void FX_Water_Splash(const ITEM *const item)
{
    int16_t room_num = item->room_num;
    Room_GetSector(item->pos, &room_num);
    if (!M_IsRoomUnderwater(room_num)) {
        return;
    }

    const int32_t water_height = Room_GetWaterHeight(item->pos, room_num);
    FX_WATER_SPLASH_SETUP setup = {
        .pos = { .x = item->pos.x, .y = water_height, .z = item->pos.z },
        .inner_xz_off = 32,
        .inner_xz_size = 8,
        .inner_y_size = -128,
        .inner_xz_vel = 320,
        .inner_y_vel = (int16_t)(-40 * item->fall_speed),
        .inner_gravity = 160,
        .inner_friction = 7,
        .middle_xz_off = 48,
        .middle_xz_size = 32,
        .middle_y_size = -64,
        .middle_xz_vel = 480,
        .middle_y_vel = (int16_t)(-20 * item->fall_speed),
        .middle_gravity = 96,
        .middle_friction = 8,
        .outer_xz_off = 32,
        .outer_xz_size = 128,
        .outer_xz_vel = 544,
        .outer_friction = 9,
    };
    FX_Water_SetupSplash(&setup);
}

void FX_Water_WadeSplash(const ITEM *const item, const int32_t depth)
{
    int16_t room_num = item->room_num;
    Room_GetSector(item->pos, &room_num);
    if (!M_IsRoomUnderwater(room_num)) {
        return;
    }

    // Lara swimming below a submerged ceiling has no surface to break, so she
    // leaves nothing behind on it.
    const int32_t water_height = Room_GetWaterHeightEx(
        item->pos, item->room_num,
        (ROOM_WATER_HEIGHT_ARGS) {
            .fix_tilts = true,
            .require_air_above = true,
        });
    if (water_height == NO_HEIGHT) {
        return;
    }

    const BOUNDS_16 *const bounds = &Item_GetBestFrame(item)->bounds;

    if (item->pos.y + bounds->min.y > water_height
        || item->pos.y + bounds->max.y < water_height) {
        return;
    }

    const int32_t time4 = Output_GetTimeInGame() * 4;
    if (item->fall_speed > 0 && depth < 474 && m_SplashCount == 0) {
        const FX_WATER_SPLASH_SETUP setup = {
            .pos = { .x = item->pos.x, .y = water_height, .z = item->pos.z },
            .inner_xz_off = 16,
            .inner_xz_size = 12,
            .inner_y_size = -96,
            .inner_xz_vel = 160,
            .inner_y_vel = (int16_t)(-72 * item->fall_speed),
            .inner_gravity = 128,
            .inner_friction = 7,
            .middle_xz_off = 24,
            .middle_xz_size = 24,
            .middle_y_size = -64,
            .middle_xz_vel = 224,
            .middle_y_vel = (int16_t)(-36 * item->fall_speed),
            .middle_gravity = 72,
            .middle_friction = 8,
            .outer_xz_off = 32,
            .outer_xz_size = 32,
            .outer_xz_vel = 272,
            .outer_friction = 9,
        };
        FX_Water_SetupSplash(&setup);
        m_SplashCount = 16;
    } else if (
        (time4 & 0xF) == 0
        && (((Random_GetControl() & 0xF) == 0)
            || item->current_anim_state != LS(LS_STOP))) {
        const bool is_still = item->current_anim_state == LS(LS_STOP);
        const XYZ_32 pos = { item->pos.x, water_height, item->pos.z };
        if (g_TRVersion == 4) {
            FX_Water_SetupRipple(
                pos, (Random_GetControl() & 0xF) + 112,
                is_still ? FX_RIPPLE_DARK : FX_RIPPLE_DARK | FX_RIPPLE_SLOW);
        } else {
            FX_Water_SetupRipple(
                pos, 16 + (Random_GetControl() & 0xF),
                is_still ? FX_RIPPLE_SLOW | FX_RIPPLE_DARK | FX_RIPPLE_JITTER
                         : FX_RIPPLE_SLOW | FX_RIPPLE_JITTER);
        }
    }
}

void FX_Water_TriggerUnderwaterBlood(const XYZ_32 pos, const int32_t size)
{
    int32_t idx = -1;
    for (int32_t i = 0; i < (int32_t)ARRAY_SIZE(m_Ripples); i++) {
        if ((m_Ripples[i].flags & FX_RIPPLE_ACTIVE) == 0U) {
            idx = i;
            break;
        }
    }
    if (idx < 0) {
        return;
    }

    FX_WATER_RIPPLE *const ripple = &m_Ripples[idx];
    ripple->flags = FX_RIPPLE_ACTIVE | FX_RIPPLE_DARK | FX_RIPPLE_BLOOD
        | (g_TRVersion == 4 ? 0U : FX_RIPPLE_SLOW);
    ripple->init = 1U;
    ripple->life = (Random_GetControl() & 7) - 16;
    ripple->size = size;
    ripple->pos.x = pos.x + (Random_GetControl() & 0x3F) - 32;
    ripple->pos.y = pos.y;
    ripple->pos.z = pos.z + (Random_GetControl() & 0x3F) - 32;
    M_RememberRipple(ripple);
}

void FX_Water_TriggerUnderwaterBloodD(const XYZ_32 pos, const int32_t size)
{
    int32_t idx = -1;
    for (int32_t i = 0; i < (int32_t)ARRAY_SIZE(m_Ripples); i++) {
        if ((m_Ripples[i].flags & FX_RIPPLE_ACTIVE) == 0U) {
            idx = i;
            break;
        }
    }
    if (idx < 0) {
        return;
    }

    FX_WATER_RIPPLE *const ripple = &m_Ripples[idx];
    ripple->flags =
        FX_RIPPLE_ACTIVE | FX_RIPPLE_SLOW | FX_RIPPLE_DARK | FX_RIPPLE_BLOOD;
    ripple->init = 1U;
    ripple->life = (Random_GetDraw() & 7) - 16;
    ripple->size = size;
    ripple->pos.x = pos.x + (Random_GetDraw() & 0x3F) - 32;
    ripple->pos.y = pos.y;
    ripple->pos.z = pos.z + (Random_GetDraw() & 0x3F) - 32;
    M_RememberRipple(ripple);
}

static const FX_MODULE m_Module = {
    .control_func = M_Control,
    .draw_func = M_Draw,
    .reset_func = M_Reset,
    .save_key = "water",
    .save_func = M_Save,
    .load_func = M_Load,
};

REGISTER_FX(m_Module)
