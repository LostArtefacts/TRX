#include <trx/game/weather_fx.h>

#include <trx/game/camera.h>
#include <trx/game/game/state.h>
#include <trx/game/lara.h>
#include <trx/game/math.h>
#include <trx/game/objects.h>
#include <trx/game/output/shaders/mesh.h>
#include <trx/game/output/sources/poly_fx.h>
#include <trx/game/output/state.h>
#include <trx/game/output/vars.h>
#include <trx/game/random.h>
#include <trx/game/rooms.h>
#include <trx/game/viewport.h>
#include <trx/utils.h>

#include <math.h>
#include <string.h>

#define M_MAX_WEATHER 256
#define M_MAX_WEATHER_ALIVE 16

#define M_RAIN_MAX_DISTANCE 6000
#define M_RAIN_BASE_Y_OFF (-1024)
#define M_RAIN_BASE_YV 16
#define M_RAIN_YV_RND_MASK 7
#define M_RAIN_SPAWN_DIST_MASK 0xFFF
#define M_RAIN_SPAWN_ANGLE_MASK 0x1FFE
#define M_RAIN_Y_RND_MASK 0x7FF
#define M_RAIN_LIFE_BASE 88

#define M_SNOW_LIFE_BASE 96
#define M_SNOW_YV_MIN 8
#define M_SNOW_YV_RANGE 24

typedef struct {
    XYZ_32 pos;
    int8_t xv;
    uint8_t yv;
    int8_t zv;
    uint8_t life;
} M_RAINDROP;

typedef struct {
    XYZ_32 pos;
    bool stopped;
    int8_t xv;
    uint8_t yv;
    int8_t zv;
    uint8_t life;
} M_SNOWFLAKE;

static M_RAINDROP m_Raindrops[M_MAX_WEATHER];
static M_SNOWFLAKE m_Snowflakes[M_MAX_WEATHER];
static WEATHER_TYPE m_WeatherType = WEATHER_NONE;

static void M_ClearWeather(void)
{
    memset(m_Raindrops, 0, sizeof(m_Raindrops));
    memset(m_Snowflakes, 0, sizeof(m_Snowflakes));
}

static int64_t M_GetViewDepth(const XYZ_32 pos)
{
    // clang-format off
    return
        g_ViewMatrix._20 * pos.x +
        g_ViewMatrix._21 * pos.y +
        g_ViewMatrix._22 * pos.z +
        g_ViewMatrix._23;
    // clang-format on
}

static void M_StageQuadWorldTransparent(
    const XYZ_32 world_pos[4], const RGBA_8888 color[4])
{
    const float disp[4][2] = {};
    const int64_t near_z = Output_GetNearZ();
    const int64_t far_z = Output_GetFarZ();
    const int64_t zv_0 = M_GetViewDepth(world_pos[0]);
    const int64_t zv_1 = M_GetViewDepth(world_pos[2]);
    const int64_t zv = (zv_0 + zv_1) / 2;
    if (zv <= near_z || zv >= far_z) {
        return;
    }

    OutputSource_PolyFX_StageQuadTransparentExt(
        -1, world_pos, disp, color,
        VERT_FLAT_SHADED | VERT_NO_LIGHTING | VERT_NO_WIBBLE);
}

static void M_StageRainDropCrossQuads(
    const XYZ_32 from, const RGBA_8888 from_color, const XYZ_32 to,
    const RGBA_8888 to_color)
{
    const int64_t zv_mid = (M_GetViewDepth(from) + M_GetViewDepth(to)) / 2;
    const int64_t near_z = Output_GetNearZ();
    const int64_t far_z = Output_GetFarZ();
    if (zv_mid <= near_z || zv_mid >= far_z) {
        return;
    }

    const float dx = (float)(to.x - from.x);
    const float dy = (float)(to.y - from.y);
    const float dz = (float)(to.z - from.z);
    float d_len = sqrtf(dx * dx + dy * dy + dz * dz);
    if (d_len <= 0.00001f) {
        return;
    }
    d_len = 1.0f / d_len;

    const float dir_x = dx * d_len;
    const float dir_y = dy * d_len;
    const float dir_z = dz * d_len;

    const XYZ_32 mid = { (from.x + to.x) / 2, (from.y + to.y) / 2,
                         (from.z + to.z) / 2 };
    const float vx = (float)(mid.x - g_Camera.pos.x);
    const float vy = (float)(mid.y - g_Camera.pos.y);
    const float vz = (float)(mid.z - g_Camera.pos.z);
    float v_len = sqrtf(vx * vx + vy * vy + vz * vz);
    if (v_len <= 0.00001f) {
        return;
    }
    v_len = 1.0f / v_len;
    const float view_x = vx * v_len;
    const float view_y = vy * v_len;
    const float view_z = vz * v_len;

    // Two intersecting quads (a "cross") around the segment.
    const float half_w = 1.0f;

    // side_0 = segment_dir x view_dir
    float side0_x = dir_y * view_z - dir_z * view_y;
    float side0_y = dir_z * view_x - dir_x * view_z;
    float side0_z = dir_x * view_y - dir_y * view_x;
    float side0_len =
        sqrtf(side0_x * side0_x + side0_y * side0_y + side0_z * side0_z);
    if (side0_len <= 0.00001f) {
        // Degenerate: use world up for view direction.
        side0_x = dir_y * 0.0f - dir_z * 1.0f;
        side0_y = dir_z * 0.0f - dir_x * 0.0f;
        side0_z = dir_x * 1.0f - dir_y * 0.0f;
        side0_len =
            sqrtf(side0_x * side0_x + side0_y * side0_y + side0_z * side0_z);
        if (side0_len <= 0.00001f) {
            return;
        }
    }
    side0_len = 1.0f / side0_len;
    side0_x *= side0_len;
    side0_y *= side0_len;
    side0_z *= side0_len;

    // side_1 = segment_dir x side_0 (guaranteed perpendicular to segment_dir).
    float side1_x = dir_y * side0_z - dir_z * side0_y;
    float side1_y = dir_z * side0_x - dir_x * side0_z;
    float side1_z = dir_x * side0_y - dir_y * side0_x;
    float side1_len =
        sqrtf(side1_x * side1_x + side1_y * side1_y + side1_z * side1_z);
    if (side1_len <= 0.00001f) {
        return;
    }
    side1_len = 1.0f / side1_len;
    side1_x *= side1_len;
    side1_y *= side1_len;
    side1_z *= side1_len;

    const RGBA_8888 color[4] = { from_color, from_color, to_color, to_color };

    {
        const XYZ_32 quad0[4] = {
            {
                from.x - (int32_t)lrintf(side0_x * half_w),
                from.y - (int32_t)lrintf(side0_y * half_w),
                from.z - (int32_t)lrintf(side0_z * half_w),
            },
            {
                from.x + (int32_t)lrintf(side0_x * half_w),
                from.y + (int32_t)lrintf(side0_y * half_w),
                from.z + (int32_t)lrintf(side0_z * half_w),
            },
            {
                to.x + (int32_t)lrintf(side0_x * half_w),
                to.y + (int32_t)lrintf(side0_y * half_w),
                to.z + (int32_t)lrintf(side0_z * half_w),
            },
            {
                to.x - (int32_t)lrintf(side0_x * half_w),
                to.y - (int32_t)lrintf(side0_y * half_w),
                to.z - (int32_t)lrintf(side0_z * half_w),
            },
        };
        M_StageQuadWorldTransparent(quad0, color);
    }

    {
        const XYZ_32 quad1[4] = {
            {
                from.x - (int32_t)lrintf(side1_x * half_w),
                from.y - (int32_t)lrintf(side1_y * half_w),
                from.z - (int32_t)lrintf(side1_z * half_w),
            },
            {
                from.x + (int32_t)lrintf(side1_x * half_w),
                from.y + (int32_t)lrintf(side1_y * half_w),
                from.z + (int32_t)lrintf(side1_z * half_w),
            },
            {
                to.x + (int32_t)lrintf(side1_x * half_w),
                to.y + (int32_t)lrintf(side1_y * half_w),
                to.z + (int32_t)lrintf(side1_z * half_w),
            },
            {
                to.x - (int32_t)lrintf(side1_x * half_w),
                to.y - (int32_t)lrintf(side1_y * half_w),
                to.z - (int32_t)lrintf(side1_z * half_w),
            },
        };
        M_StageQuadWorldTransparent(quad1, color);
    }
}

static void M_SpawnRainDrop(M_RAINDROP *const drop)
{
    const ITEM *const lara_item = Lara_GetItem();
    if (lara_item == nullptr) {
        return;
    }

    const int32_t dist = Random_GetDraw() & M_RAIN_SPAWN_DIST_MASK;
    const int32_t angle = (Random_GetDraw() & M_RAIN_SPAWN_ANGLE_MASK) << 3;

    drop->pos = (XYZ_32) {
        .x = lara_item->pos.x + ((dist * Math_Sin(angle)) >> W2V_SHIFT),
        .z = lara_item->pos.z + ((dist * Math_Cos(angle)) >> W2V_SHIFT),
        .y = lara_item->pos.y + M_RAIN_BASE_Y_OFF
            - (Random_GetDraw() & M_RAIN_Y_RND_MASK),
    };

    int16_t room_num = NO_ROOM;
    if (Room_GetOutsideStatus(drop->pos, &room_num) != 1) {
        drop->pos.x = 0;
        return;
    }

    drop->yv =
        (uint8_t)((Random_GetDraw() & M_RAIN_YV_RND_MASK) + M_RAIN_BASE_YV);
    drop->xv = (int8_t)((Random_GetDraw() & 7) - 4);
    drop->zv = (int8_t)((Random_GetDraw() & 7) - 4);
    drop->life = (uint8_t)(M_RAIN_LIFE_BASE - ((int32_t)drop->yv << 1));
}

static void M_UpdateRain(void)
{
    const ITEM *const lara_item = Lara_GetItem();
    if (lara_item == nullptr) {
        return;
    }

    const XZ_32 wind = Sparks_GetSmokeWind();

    int32_t num_alive = 0;
    for (int32_t i = 0; i < M_MAX_WEATHER; i++) {
        M_RAINDROP *const drop = &m_Raindrops[i];

        if (drop->pos.x == 0 && num_alive < M_MAX_WEATHER_ALIVE) {
            num_alive++;
            M_SpawnRainDrop(drop);
        }

        if (drop->pos.x == 0) {
            continue;
        }

        int16_t room_num = NO_ROOM;
        const int32_t outside = Room_GetOutsideStatus(drop->pos, &room_num);
        if (outside == -2
            || (room_num != NO_ROOM && Room_Get(room_num)->flags.underwater)
            || drop->life > 240
            || ABS(g_Camera.pos.x - drop->pos.x) > M_RAIN_MAX_DISTANCE
            || ABS(g_Camera.pos.z - drop->pos.z) > M_RAIN_MAX_DISTANCE) {
            drop->pos.x = 0;
            continue;
        }

        drop->pos.x += (int32_t)drop->xv + 4 * wind.x;
        drop->pos.y += (int32_t)drop->yv << 3;
        drop->pos.z += (int32_t)drop->zv + 4 * wind.z;

        int32_t rnd = Random_GetDraw();
        if ((rnd & 3) != 3) {
            drop->xv += (int8_t)((rnd & 3) - 1);
            if (drop->xv < -4) {
                drop->xv = -4;
            } else if (drop->xv > 4) {
                drop->xv = 4;
            }
        }

        rnd = (rnd >> 2) & 3;
        if (rnd != 3) {
            drop->zv += (int8_t)(rnd - 1);
            if (drop->zv < -4) {
                drop->zv = -4;
            } else if (drop->zv > 4) {
                drop->zv = 4;
            }
        }

        drop->life -= 2;
        if (drop->life > 240) {
            drop->pos.x = 0;
        }
    }
}

static void M_DrawRain(void)
{
    const XZ_32 wind = Sparks_GetSmokeWind();

    for (int32_t i = 0; i < M_MAX_WEATHER; i++) {
        const M_RAINDROP *const drop = &m_Raindrops[i];
        if (drop->pos.x == 0) {
            continue;
        }

        const XYZ_32 to = drop->pos;
        const XYZ_32 from = {
            drop->pos.x - (wind.x << 2),
            drop->pos.y - ((int32_t)drop->yv << 3),
            drop->pos.z - (wind.z << 2),
        };

        const RGBA_8888 from_color = { 0, 0, 0x20, 0x00 };
        const RGBA_8888 to_color = { 0x30, 0x40, 0x60, 0x80 };
        M_StageRainDropCrossQuads(from, from_color, to, to_color);
    }
}

static void M_SpawnSnowflake(M_SNOWFLAKE *const snow)
{
    const ITEM *const lara_item = Lara_GetItem();
    if (lara_item == nullptr) {
        return;
    }

    const int32_t dist = Random_GetDraw() & M_RAIN_SPAWN_DIST_MASK;
    const int32_t angle = (Random_GetDraw() & M_RAIN_SPAWN_ANGLE_MASK) << 3;

    snow->pos = (XYZ_32) {
        .x = lara_item->pos.x + ((dist * Math_Sin(angle)) >> W2V_SHIFT),
        .z = lara_item->pos.z + ((dist * Math_Cos(angle)) >> W2V_SHIFT),
        .y = lara_item->pos.y + M_RAIN_BASE_Y_OFF
            - (Random_GetDraw() & M_RAIN_Y_RND_MASK),
    };

    int16_t room_num = NO_ROOM;
    if (Room_GetOutsideStatus(snow->pos, &room_num) != 1) {
        snow->pos.x = 0;
        return;
    }

    snow->stopped = false;
    snow->xv = (int8_t)((Random_GetDraw() & 7) - 4);
    snow->yv =
        (uint8_t)(((Random_GetDraw() % M_SNOW_YV_RANGE) + M_SNOW_YV_MIN) << 3);
    snow->zv = (int8_t)((Random_GetDraw() & 7) - 4);
    snow->life = (uint8_t)(M_SNOW_LIFE_BASE - ((int32_t)snow->yv << 1));
}

static void M_UpdateSnow(void)
{
    const ITEM *const lara_item = Lara_GetItem();
    if (lara_item == nullptr) {
        return;
    }

    int32_t num_alive = 0;
    for (int32_t i = 0; i < M_MAX_WEATHER; i++) {
        M_SNOWFLAKE *const snow = &m_Snowflakes[i];

        if (snow->pos.x == 0 && num_alive < M_MAX_WEATHER_ALIVE) {
            num_alive++;
            M_SpawnSnowflake(snow);
        }

        if (snow->pos.x == 0) {
            continue;
        }

        const XYZ_32 old_pos = snow->pos;

        int16_t room_num = NO_ROOM;
        int32_t outside = 1;
        if (!snow->stopped) {
            snow->pos.x += snow->xv;
            snow->pos.y += (snow->yv & 0xF8) >> 2;
            snow->pos.z += snow->zv;

            outside = Room_GetOutsideStatus(snow->pos, &room_num);
            if (outside == -3) {
                snow->pos.x = 0;
                continue;
            }

            if (outside == -2
                || (room_num != NO_ROOM
                    && Room_Get(room_num)->flags.underwater)) {
                snow->stopped = true;
                snow->pos = old_pos;

                if (snow->life > 16) {
                    snow->life = 16;
                }
                if (snow->yv > 16) {
                    snow->yv -= 16;
                }
            }
        }

        if (snow->life == 0) {
            snow->pos.x = 0;
            continue;
        }

        if ((ABS(g_Camera.pos.x - snow->pos.x) > M_RAIN_MAX_DISTANCE
             || ABS(g_Camera.pos.z - snow->pos.z) > M_RAIN_MAX_DISTANCE)
            && snow->life > 16) {
            snow->life = 16;
        }

        const XZ_32 wind = Sparks_GetSmokeWind();

        if (snow->xv < (wind.x << 1)) {
            snow->xv++;
        } else if (snow->xv > (wind.x << 1)) {
            snow->xv--;
        }

        if (snow->zv < (wind.z << 1)) {
            snow->zv++;
        } else if (snow->zv > (wind.z << 1)) {
            snow->zv--;
        }

        snow->life -= 2;
        if ((snow->yv & 7) != 7) {
            snow->yv++;
        }
    }
}

static void M_DrawSnow(void)
{
    const OBJECT *const obj = Object_Get(O_SNOWFLAKE);
    if (!obj->loaded) {
        return;
    }

    const int32_t sprite_idx = obj->mesh_idx;
    for (int32_t i = 0; i < M_MAX_WEATHER; i++) {
        M_SNOWFLAKE *const snow = &m_Snowflakes[i];
        if (snow->pos.x == 0) {
            continue;
        }

        const XYZ_32 center = snow->pos;
        const int64_t zv = M_GetViewDepth(center);
        const int64_t near_z = Output_GetNearZ();
        const int64_t far_z = Output_GetFarZ();
        const int32_t vpos_z = (int32_t)(zv >> W2V_SHIFT);

        if (vpos_z < 128 && snow->life > 16) {
            snow->life = 16;
        }

        if (zv <= near_z || zv >= far_z) {
            continue;
        }

        if (vpos_z < 128 || g_PhdPersp <= 0) {
            continue;
        }

        const int32_t game_w = Viewport_GetWidth(VIEWPORT_GAME);
        const int32_t game_h = Viewport_GetHeight(VIEWPORT_GAME);
        const int32_t ui_w = Viewport_GetWidth(VIEWPORT_UI);
        const int32_t ui_h = Viewport_GetHeight(VIEWPORT_UI);

        const XYZ_32 world_pos[4] = { center, center, center, center };
        const float s = 16.0f;
        const float disp[4][2] = {
            { -s, -s },
            { s, -s },
            { s, s },
            { -s, s },
        };

        uint32_t c;
        if ((snow->yv & 7) < 7) {
            c = (uint32_t)(snow->yv & 7);
        } else if (snow->life > 18) {
            c = 15;
        } else {
            c = snow->life;
        }
        c <<= 3;
        CLAMPG(c, 255);

        {
            const int32_t fog_start = Output_GetFogStart();
            const int32_t fog_end = Output_GetFogEnd();
            float fade = 1.0f;
            if (fog_end > fog_start && vpos_z > fog_start) {
                float t = (vpos_z - fog_start) / (float)(fog_end - fog_start);
                CLAMP(t, 0.0f, 1.0f);
                fade = 1.0f - t;
            }
            c = (uint32_t)(c * fade);
        }

        const RGBA_8888 color = { c, c, c, 255 };
        const RGBA_8888 colors[4] = { color, color, color, color };

        OutputSource_PolyFX_StageQuadBlendAddExt(
            sprite_idx, world_pos, disp, colors,
            VERT_NO_LIGHTING | VERT_NO_WIBBLE | VERT_BILLBOARD
                | VERT_ABS_SPRITE);
    }
}

void WeatherFX_Init(void)
{
    M_ClearWeather();
}

WEATHER_TYPE WeatherFX_GetWeather(void)
{
    return m_WeatherType;
}

void WeatherFX_SetWeather(const WEATHER_TYPE weather_type)
{
    m_WeatherType = weather_type;
}

void WeatherFX_Update(void)
{
    const WEATHER_TYPE weather_type = m_WeatherType;
    if (weather_type == WEATHER_RAIN) {
        M_UpdateRain();
    } else if (weather_type == WEATHER_SNOW) {
        M_UpdateSnow();
    }
}

void WeatherFX_Draw(void)
{
    switch (m_WeatherType) {
    case WEATHER_RAIN:
        M_DrawRain();
        break;
    case WEATHER_SNOW:
        M_DrawSnow();
        break;
    default:
        break;
    }
}
