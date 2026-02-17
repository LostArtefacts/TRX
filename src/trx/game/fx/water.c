#include <trx/game/fx/water.h>

#include <trx/game/items.h>
#include <trx/game/lara/common.h>
#include <trx/game/objects.h>
#include <trx/game/output/sources/poly_fx.h>
#include <trx/game/random.h>
#include <trx/game/rooms.h>
#include <trx/game/sound.h>
#include <trx/utils.h>

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
static int32_t m_Wibble = 0;

void FX_Water_Init(void)
{
    memset(m_Splashes, 0, sizeof(m_Splashes));
    memset(m_Ripples, 0, sizeof(m_Ripples));
    m_SplashCount = 0;
    m_Wibble = 0;
}

static bool M_IsRoomUnderwater(const int16_t room_num)
{
    return Room_Get(room_num)->flags.underwater;
}

FX_WATER_RIPPLE *FX_Water_SetupRipple(
    const int32_t x, const int32_t y, const int32_t z, int32_t size,
    const bool is_still)
{
    int32_t idx = -1;
    for (int32_t i = 0; i < (int32_t)ARRAY_SIZE(m_Ripples); i++) {
        if ((m_Ripples[i].flags & 1U) == 0U) {
            idx = i;
            break;
        }
    }
    if (idx < 0) {
        return nullptr;
    }

    FX_WATER_RIPPLE *const ripple = &m_Ripples[idx];

    if (size < 0) {
        ripple->flags = is_still ? 19U : 3U;
        size = -size;
    } else {
        ripple->flags = 1U;
    }

    ripple->init = 1U;
    ripple->size = (uint8_t)size;
    ripple->life = (uint8_t)((Random_GetControl() & 0xF) + 48);
    ripple->x = (Random_GetControl() & 0x7F) + x - 64;
    ripple->y = y;
    ripple->z = (Random_GetControl() & 0x7F) + z - 64;
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
            SFX_LARA_SPLASH, &(XYZ_32) { setup.x, setup.y, setup.z },
            SPM_NORMAL);
        return;
    }

    FX_WATER_SPLASH *const splash = &m_Splashes[idx];
    splash->flags = 3U;

    if (setup.outer_friction == -9) {
        splash->flags = 67U;
        setup.outer_friction = 9;
    }

    splash->x = setup.x;
    splash->y = setup.y;
    splash->z = setup.z;
    splash->life = 63U;

    FX_WATER_SPLASH_VERT *v = splash->v;

    for (int32_t i = 0; i < 8; i++) {
        v->wx = (setup.inner_xz_off * m_SplashRings[i][0]) << 1;
        v->wy = 0;
        v->wz = (setup.inner_xz_off * m_SplashRings[i][1]) << 1;
        v->xv = (setup.inner_xz_vel * m_SplashRings[i][0]) / 12;
        v->yv = 0;
        v->zv = (setup.inner_xz_vel * m_SplashRings[i][1]) / 12;
        v->oxv = v->xv >> 3;
        v->ozv = v->zv >> 3;
        v->gravity = 0;
        v->friction = (uint8_t)(setup.inner_friction - 2);
        v++;
    }

    for (int32_t i = 0; i < 8; i++) {
        v->wx =
            ((setup.inner_xz_off + setup.inner_xz_size) * m_SplashRings[i][0])
            << 1;
        v->wy = setup.inner_y_size;
        v->wz =
            ((setup.inner_xz_off + setup.inner_xz_size) * m_SplashRings[i][1])
            << 1;
        v->xv = (setup.inner_xz_vel * m_SplashRings[i][0]) >> 3;
        v->yv = setup.inner_y_vel;
        v->zv = (setup.inner_xz_vel * m_SplashRings[i][1]) >> 3;
        v->oxv = v->xv >> 3;
        v->ozv = v->zv >> 3;
        v->gravity = (uint8_t)setup.inner_gravity;
        v->friction = (uint8_t)setup.inner_friction;
        v++;
    }

    for (int32_t i = 0; i < 8; i++) {
        v->wx = (setup.middle_xz_off * m_SplashRings[i][0]) << 1;
        v->wy = 0;
        v->wz = (setup.middle_xz_off * m_SplashRings[i][1]) << 1;
        v->xv = (setup.middle_xz_vel * m_SplashRings[i][0]) / 12;
        v->yv = 0;
        v->zv = (setup.middle_xz_vel * m_SplashRings[i][1]) / 12;
        v->oxv = v->xv >> 3;
        v->ozv = v->zv >> 3;
        v->gravity = 0;
        v->friction = (uint8_t)(setup.middle_friction - 2);
        v++;
    }

    for (int32_t i = 0; i < 8; i++) {
        v->wx =
            ((setup.middle_xz_off + setup.middle_xz_size) * m_SplashRings[i][0])
            << 1;
        v->wy = setup.middle_y_size;
        v->wz =
            ((setup.middle_xz_off + setup.middle_xz_size) * m_SplashRings[i][1])
            << 1;
        v->xv = (setup.middle_xz_vel * m_SplashRings[i][0]) >> 3;
        v->yv = setup.middle_y_vel;
        v->zv = (setup.middle_xz_vel * m_SplashRings[i][1]) >> 3;
        v->oxv = v->xv >> 3;
        v->ozv = v->zv >> 3;
        v->gravity = (uint8_t)setup.middle_gravity;
        v->friction = (uint8_t)setup.middle_friction;
        v++;
    }

    for (int32_t i = 0; i < 8; i++) {
        v->wx = (setup.outer_xz_off * m_SplashRings[i][0]) << 1;
        v->wy = 0;
        v->wz = (setup.outer_xz_off * m_SplashRings[i][1]) << 1;
        v->xv = (setup.outer_xz_vel * m_SplashRings[i][0]) / 12;
        v->yv = 0;
        v->zv = (setup.outer_xz_vel * m_SplashRings[i][1]) / 12;
        v->oxv = v->xv >> 3;
        v->ozv = v->zv >> 3;
        v->gravity = 0;
        v->friction = (uint8_t)(setup.outer_friction - 2);
        v++;
    }

    for (int32_t i = 0; i < 8; i++) {
        v->wx =
            ((setup.outer_xz_off + setup.outer_xz_size) * m_SplashRings[i][0])
            << 1;
        v->wy = 0;
        v->wz =
            ((setup.outer_xz_off + setup.outer_xz_size) * m_SplashRings[i][1])
            << 1;
        v->xv = (setup.outer_xz_vel * m_SplashRings[i][0]) >> 3;
        v->yv = 0;
        v->zv = (setup.outer_xz_vel * m_SplashRings[i][1]) >> 3;
        v->oxv = v->xv >> 3;
        v->ozv = v->zv >> 3;
        v->gravity = 0;
        v->friction = (uint8_t)setup.outer_friction;
        v++;
    }

    Sound_Effect(
        SFX_LARA_SPLASH, &(XYZ_32) { setup.x, setup.y, setup.z }, SPM_NORMAL);
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
        .x = item->pos.x,
        .y = water_height,
        .z = item->pos.z,
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

void FX_Water_WadeSplash(
    const ITEM *const item, const int32_t water_height, const int32_t depth)
{
    int16_t room_num = item->room_num;
    Room_GetSector(item->pos, &room_num);
    if (!M_IsRoomUnderwater(room_num)) {
        return;
    }

    const BOUNDS_16 *const bounds = &Item_GetBestFrame(item)->bounds;

    if (item->pos.y + bounds->min.y > water_height
        || item->pos.y + bounds->max.y < water_height) {
        return;
    }

    if (item->fall_speed > 0 && depth < 474 && m_SplashCount == 0) {
        const FX_WATER_SPLASH_SETUP setup = {
            .x = item->pos.x,
            .y = water_height,
            .z = item->pos.z,
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
        (m_Wibble & 0xF) == 0
        && (((Random_GetControl() & 0xF) == 0)
            || item->current_anim_state != LS(LS_STOP))) {
        FX_Water_SetupRipple(
            item->pos.x, water_height, item->pos.z,
            -16 - (Random_GetControl() & 0xF),
            item->current_anim_state == LS(LS_STOP));
    }
}

void FX_Water_TriggerUnderwaterBlood(const XYZ_32 pos, const int32_t size)
{
    int32_t idx = -1;
    for (int32_t i = 0; i < (int32_t)ARRAY_SIZE(m_Ripples); i++) {
        if ((m_Ripples[i].flags & 1U) == 0U) {
            idx = i;
            break;
        }
    }
    if (idx < 0) {
        return;
    }

    FX_WATER_RIPPLE *const ripple = &m_Ripples[idx];
    ripple->flags = 0x33U;
    ripple->init = 1U;
    ripple->life = (Random_GetControl() & 7) - 16;
    ripple->size = size;
    ripple->x = pos.x + (Random_GetControl() & 0x3F) - 32;
    ripple->y = pos.y;
    ripple->z = pos.z + (Random_GetControl() & 0x3F) - 32;
}

void FX_Water_TriggerUnderwaterBloodD(const XYZ_32 pos, const int32_t size)
{
    int32_t idx = -1;
    for (int32_t i = 0; i < (int32_t)ARRAY_SIZE(m_Ripples); i++) {
        if ((m_Ripples[i].flags & 1U) == 0U) {
            idx = i;
            break;
        }
    }
    if (idx < 0) {
        return;
    }

    FX_WATER_RIPPLE *const ripple = &m_Ripples[idx];
    ripple->flags = 0x33U;
    ripple->init = 1U;
    ripple->life = (Random_GetDraw() & 7) - 16;
    ripple->size = size;
    ripple->x = pos.x + (Random_GetDraw() & 0x3F) - 32;
    ripple->y = pos.y;
    ripple->z = pos.z + (Random_GetDraw() & 0x3F) - 32;
}

static RGBA_8888 M_Gray(int32_t c)
{
    CLAMP(c, 0, 255);
    return (RGBA_8888) { c, c, c, 255 };
}

static void M_DrawSplash(
    const int32_t base_sprite_idx, const FX_WATER_SPLASH *s)
{
    XYZ_32 points[48];
    for (int32_t i = 0; i < 48; i++) {
        const FX_WATER_SPLASH_VERT *const v = &s->v[i];
        points[i] = (XYZ_32) {
            .x = s->x + (v->wx >> 4),
            .y = s->y + v->wy,
            .z = s->z + (v->wz >> 4),
        };
    }

    for (int32_t ring = 0; ring < 3; ring++) {
        int32_t sprite_idx = base_sprite_idx + 8;
        if (ring == 2 || (ring == 0 && (s->flags & 4U) != 0U)
            || (ring == 1 && (s->flags & 8U) != 0U)) {
            sprite_idx = base_sprite_idx + 4 + ((m_Wibble >> 4) & 3);
        }

        int32_t c = (int32_t)s->life << 1;
        CLAMPG(c, 255);
        const RGBA_8888 c1 = M_Gray(c);

        c = ((int32_t)s->life - ((int32_t)s->life >> 2)) << 1;
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

static void M_DrawRipple(
    const int32_t base_sprite_idx, const FX_WATER_RIPPLE *r)
{
    const int32_t n = (int32_t)r->size << 2;
    int32_t sprite_idx = base_sprite_idx + 9;
    RGBA_8888 color;

    const bool censored = false;

    if ((r->flags & 0x10U) != 0U) {
        if ((r->flags & 0x20U) != 0U) {
            sprite_idx = base_sprite_idx;
            if (censored) {
                color = (RGBA_8888) { r->life / 2, 0, r->life, 255 };
            } else {
                color = (RGBA_8888) { r->life, 0, 0, 255 };
            }
        } else {
            int32_t c1 = r->init != 0U ? (r->init >> 2) : (r->life >> 2);
            c1 <<= 3;
            CLAMPG(c1, 255);
            color = M_Gray(c1);
        }
    } else {
        int32_t c1 = r->init != 0U ? (r->init >> 1) : (r->life >> 1);
        c1 <<= 3;
        CLAMPG(c1, 255);
        color = M_Gray(c1);
    }

    // double-sided
    const XYZ_32 quad_pos[2][4] = {
        {
            { r->x - n, r->y, r->z - n },
            { r->x + n, r->y, r->z - n },
            { r->x + n, r->y, r->z + n },
            { r->x - n, r->y, r->z + n },
        },
        {
            { r->x - n, r->y, r->z - n },
            { r->x - n, r->y, r->z + n },
            { r->x + n, r->y, r->z + n },
            { r->x + n, r->y, r->z - n },
        },
    };
    const RGBA_8888 quad_color[4] = { color, color, color, color };
    OutputSource_PolyFX_StageSpriteQuadWorldDepth(
        sprite_idx, quad_pos[0], quad_color, M_RIPPLE_Z_DEPTH_ADJUST,
        DRAW_BLEND_ADD);
    OutputSource_PolyFX_StageSpriteQuadWorldDepth(
        sprite_idx, quad_pos[1], quad_color, M_RIPPLE_Z_DEPTH_ADJUST,
        DRAW_BLEND_ADD);
}

void FX_Water_Draw(void)
{
    const OBJECT *const explosion = Object_Get(O_EXPLOSION_1);
    if (!explosion->loaded) {
        return;
    }

    const int32_t base_sprite_idx = explosion->mesh_idx;

    for (int32_t i = 0; i < (int32_t)ARRAY_SIZE(m_Splashes); i++) {
        const FX_WATER_SPLASH *const splash = &m_Splashes[i];
        if ((splash->flags & 1U) == 0U) {
            continue;
        }
        M_DrawSplash(base_sprite_idx, splash);
    }

    for (int32_t i = 0; i < (int32_t)ARRAY_SIZE(m_Ripples); i++) {
        const FX_WATER_RIPPLE *const ripple = &m_Ripples[i];
        if ((ripple->flags & 1U) == 0U) {
            continue;
        }
        M_DrawRipple(base_sprite_idx, ripple);
    }
}

void FX_Water_Update(void)
{
    if (m_SplashCount > 0) {
        m_SplashCount--;
    }

    for (int32_t i = 0; i < (int32_t)ARRAY_SIZE(m_Splashes); i++) {
        FX_WATER_SPLASH *const splash = &m_Splashes[i];
        if ((splash->flags & 1U) == 0U) {
            continue;
        }

        bool set = false;
        for (int32_t j = 0; j < 48; j++) {
            FX_WATER_SPLASH_VERT *const v = &splash->v[j];
            v->wx += v->xv >> 2;
            v->wy += (int16_t)(v->yv >> 6);
            v->wz += v->zv >> 2;
            v->xv -= v->xv >> v->friction;
            v->zv -= v->zv >> v->friction;

            if ((v->oxv < 0 && v->xv > v->oxv)
                || (v->oxv > 0 && v->xv < v->oxv)) {
                v->xv = v->oxv;
            } else if (
                (v->ozv < 0 && v->zv > v->ozv)
                || (v->ozv > 0 && v->zv < v->ozv)) {
                v->zv = v->ozv;
            }

            v->yv += (int32_t)v->gravity << 3;
            CLAMPG(v->yv, 0x10000);

            if (v->wy > 0) {
                if (j < 16) {
                    splash->flags |= 4U;
                } else if (j < 32) {
                    splash->flags |= 8U;
                }

                v->wy = 0;
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
        if ((ripple->flags & 1U) == 0U) {
            continue;
        }

        if (ripple->size < 254U) {
            ripple->size += 2U;
        }

        if (ripple->init == 0U) {
            ripple->life -= 2U;
            if (ripple->life > 250U) {
                ripple->flags = 0U;
            }
        } else if (ripple->init < ripple->life) {
            ripple->init += 4U;
            if (ripple->init >= ripple->life) {
                ripple->init = 0U;
            }
        }
    }

    m_Wibble = (m_Wibble + 4) & 0xFC;
}
