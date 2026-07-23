#include <trx/game/fx/fire.h>

#include <trx/core/utils.h>
#include <trx/game/const.h>
#include <trx/game/matrix.h>
#include <trx/game/output/sources/poly_fx.h>
#include <trx/game/output/state.h>
#include <trx/game/random.h>
#include <trx/game/sparks.h>

#include <string.h>

#define M_MAX_SPARKS 20
#define M_MAX_FIRES 32
#define M_MAX_DRAW_DIST (20 * WALL_L)
#define M_FADE_DRAW_DIST (12 * WALL_L)
// The original's screen-space size clamp at its 640-wide, 80-degree FOV
// reference (phd_persp); expressed in world space here so the clamp onset
// does not shift with resolution or FOV.
#define M_CLAMP_PERSP 381

typedef struct {
    XYZ_32 pos;
    uint8_t size;
    uint8_t on; // 0 = free slot, otherwise the dim value (1 = full)
    int16_t room_num;
} M_FIRE;

static SPARK m_Sparks[M_MAX_SPARKS];
static M_FIRE m_Fires[M_MAX_FIRES];
static int32_t m_NextSpark = 1;

// Slots 1..19 form a shared pool; slot 0 is reserved for the static flame base.
static int32_t M_GetFreeSpark(void)
{
    int32_t free = m_NextSpark;
    int32_t min_life = INT16_MAX;
    int32_t min_idx = 1;
    for (int32_t i = 0; i < M_MAX_SPARKS - 1; i++) {
        SPARK *const spark = &m_Sparks[free];
        if (!spark->on) {
            m_NextSpark = free >= M_MAX_SPARKS - 1 ? 1 : free + 1;
            return free;
        }
        if ((int32_t)spark->life < min_life) {
            min_life = spark->life;
            min_idx = free;
        }
        free = free >= M_MAX_SPARKS - 1 ? 1 : free + 1;
    }
    m_NextSpark = min_idx >= M_MAX_SPARKS - 1 ? 1 : min_idx + 1;
    return min_idx;
}

static void M_SetSize(SPARK *const spark, const int32_t size)
{
    spark->src_size.width = size;
    spark->src_size.height = size;
    spark->size.width = size;
    spark->size.height = size;
}

static void M_SetRotation(SPARK *const spark)
{
    if (Random_GetControl() & 1) {
        spark->flags = SPARK_F_ROTATE;
        spark->rot_angle = Random_GetControl() & 0xFFF;
        if (Random_GetControl() & 1) {
            spark->rot_add = -16 - (Random_GetControl() & 0xF);
        } else {
            spark->rot_add = (Random_GetControl() & 0xF) + 16;
        }
    } else {
        spark->flags = 0;
    }
}

// The steady flame base, rewritten in slot 0 every frame.
static void M_TriggerStaticFlame(void)
{
    SPARK *const spark = &m_Sparks[0];
    spark->on = true;
    spark->dst_color.r = (Random_GetControl() & 0x3F) - 64;
    spark->dst_color.g = (Random_GetControl() & 0x3F) + 96;
    spark->dst_color.b = 64;
    spark->src_color = spark->dst_color;
    spark->col_fade_speed = 1;
    spark->fade_to_black = 0;
    spark->life = 8;
    spark->s_life = 8;
    spark->pos.x = (Random_GetControl() & 7) - 4;
    spark->pos.y = 0;
    spark->pos.z = (Random_GetControl() & 7) - 4;
    spark->max_y_vel = 0;
    spark->gravity = 0;
    spark->friction = 0;
    spark->vel = (XYZ_32) { 0, 0, 0 };
    spark->flags = 0;
    M_SetSize(spark, (Random_GetControl() & 0x1F) + 0x80);
    Sparks_FinishSetup(spark);
}

static void M_TriggerFireFlame(void)
{
    SPARK *const spark = &m_Sparks[M_GetFreeSpark()];
    spark->on = true;
    spark->src_color.r = 255;
    spark->src_color.g = (Random_GetControl() & 0x1F) + 48;
    spark->src_color.b = 48;
    spark->dst_color.r = (Random_GetControl() & 0x3F) + 192;
    spark->dst_color.g = (Random_GetControl() & 0x3F) + 128;
    spark->dst_color.b = 32;
    spark->fade_to_black = 8;
    spark->col_fade_speed = (Random_GetControl() & 3) + 8;
    spark->life = (Random_GetControl() & 7) + 32;
    spark->s_life = spark->life;
    spark->pos.x = 4 * (Random_GetControl() & 0x1F) - 64;
    spark->pos.y = 0;
    spark->pos.z = 4 * (Random_GetControl() & 0x1F) - 64;
    spark->vel.x = 2 * (Random_GetControl() & 0xFF) - 256;
    spark->vel.y = -16 - (Random_GetControl() & 0xF);
    spark->vel.z = 2 * (Random_GetControl() & 0xFF) - 256;
    spark->friction = 5;
    spark->gravity = -32 - (Random_GetControl() & 0x1F);
    spark->max_y_vel = -16 - (Random_GetControl() & 7);
    M_SetRotation(spark);
    const int32_t size = (Random_GetControl() & 0x1F) + 128;
    M_SetSize(spark, size);
    spark->dst_size.width = size >> 4;
    spark->dst_size.height = size >> 4;
    Sparks_FinishSetup(spark);
}

static void M_TriggerSmoke(void)
{
    SPARK *const spark = &m_Sparks[M_GetFreeSpark()];
    spark->on = true;
    spark->src_color = COLOR_RGB_888_BLACK;
    spark->dst_color = (RGB_888) { 32, 32, 32 };
    spark->fade_to_black = 16;
    spark->col_fade_speed = (Random_GetControl() & 7) + 32;
    spark->life = (Random_GetControl() & 0xF) + 57;
    spark->s_life = spark->life;
    spark->pos.x = (Random_GetControl() & 0xF) - 8;
    spark->pos.y = -256 - (Random_GetControl() & 0x7F);
    spark->pos.z = (Random_GetControl() & 0xF) - 8;
    spark->vel.x = (Random_GetControl() & 0xFF) - 128;
    spark->vel.y = -16 - (Random_GetControl() & 0xF);
    spark->vel.z = (Random_GetControl() & 0xFF) - 128;
    spark->friction = 4;
    M_SetRotation(spark);
    spark->gravity = -16 - (Random_GetControl() & 0xF);
    spark->max_y_vel = -8 - (Random_GetControl() & 7);
    const int32_t size = (Random_GetControl() & 0x7F) + 128;
    M_SetSize(spark, size >> 2);
    spark->dst_size.width = size;
    spark->dst_size.height = size;
    Sparks_FinishSetup(spark);
}

static void M_KeepBurning(void)
{
    const int32_t time4 = Output_GetTimeInGame() * 4;
    M_TriggerStaticFlame();
    if ((time4 & 0xF) == 0) {
        M_TriggerFireFlame();
        if ((time4 & 0x1F) == 0) {
            M_TriggerSmoke();
        }
    }
}

static void M_AdvanceSpark(SPARK *const spark, const int32_t base_sprite)
{
    spark->life--;
    if (spark->life == 0) {
        spark->on = false;
        return;
    }

    Sparks_Sync(spark);

    const int32_t lived = (int32_t)spark->s_life - (int32_t)spark->life;
    if (lived < (int32_t)spark->col_fade_speed) {
        const int32_t fade = (lived << 16) / spark->col_fade_speed;
        spark->color.r = spark->src_color.r
            + ((fade * (spark->dst_color.r - spark->src_color.r)) >> 16);
        spark->color.g = spark->src_color.g
            + ((fade * (spark->dst_color.g - spark->src_color.g)) >> 16);
        spark->color.b = spark->src_color.b
            + ((fade * (spark->dst_color.b - spark->src_color.b)) >> 16);
    } else if (
        spark->life < spark->fade_to_black && spark->fade_to_black != 0) {
        const int32_t fade =
            (((int32_t)spark->life - (int32_t)spark->fade_to_black) << 16)
                / spark->fade_to_black
            + 0x10000;
        spark->color.r = (fade * spark->dst_color.r) >> 16;
        spark->color.g = (fade * spark->dst_color.g) >> 16;
        spark->color.b = (fade * spark->dst_color.b) >> 16;
        if (spark->color.r < 8 && spark->color.g < 8 && spark->color.b < 8) {
            spark->on = false;
            return;
        }
    } else {
        spark->color = spark->dst_color;
    }

    if ((spark->flags & SPARK_F_ROTATE) != 0U) {
        spark->rot_angle = (spark->rot_angle + spark->rot_add) & 0xFFF;
    }

    if (base_sprite != NO_ITEM) {
        if (spark->color.r < 24 && spark->color.g < 24 && spark->color.b < 24) {
            spark->sprite_idx = base_sprite + 2;
        } else if (
            spark->color.r < 80 && spark->color.g < 80 && spark->color.b < 80) {
            spark->sprite_idx = base_sprite + 1;
        } else {
            spark->sprite_idx = base_sprite;
        }
    }

    spark->vel.y += spark->gravity;
    if (spark->max_y_vel != 0) {
        const int32_t limit = (int32_t)spark->max_y_vel << 5;
        if ((spark->vel.y < 0 && spark->vel.y < limit)
            || (spark->vel.y > 0 && spark->vel.y > limit)) {
            spark->vel.y = limit;
        }
    }

    if (spark->friction != 0U) {
        spark->vel.x -= spark->vel.x >> spark->friction;
        spark->vel.z -= spark->vel.z >> spark->friction;
    }

    spark->pos.x += spark->vel.x >> 5;
    spark->pos.y += spark->vel.y >> 5;
    spark->pos.z += spark->vel.z >> 5;

    const int32_t size_fade = (lived << 16) / spark->s_life;
    const int32_t width = spark->src_size.width
        + ((size_fade
            * ((int32_t)spark->dst_size.width - (int32_t)spark->src_size.width))
           >> 16);
    const int32_t height = spark->src_size.height
        + ((size_fade
            * ((int32_t)spark->dst_size.height
               - (int32_t)spark->src_size.height))
           >> 16);
    spark->size.width = width;
    spark->size.height = height;
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

// Caps the projected growth of close-up sparks like the original, which
// clamps the screen size at 4 * size (S_DrawFireSparks). Its lower clamp of
// 4 px at the render width is negligible and skipped.
static int32_t M_ScaleSize(
    const int32_t size, const int32_t shift, const int32_t depth)
{
    return MIN(size >> shift, size * depth / M_CLAMP_PERSP);
}

static void M_DrawFire(const M_FIRE *const fire)
{
    const int32_t shift = 2 - fire->size;
    const int32_t dim = fire->on == 1 ? 256 : fire->on;

    for (int32_t i = 0; i < M_MAX_SPARKS; i++) {
        const SPARK *const src = &m_Sparks[i];
        if (!src->on) {
            continue;
        }

        SPARK spark = *src;
        spark.pos.x = fire->pos.x + (src->pos.x >> shift);
        spark.pos.y = fire->pos.y + (src->pos.y >> shift);
        spark.pos.z = fire->pos.z + (src->pos.z >> shift);
        spark.prev_pos.x = fire->pos.x + (src->prev_pos.x >> shift);
        spark.prev_pos.y = fire->pos.y + (src->prev_pos.y >> shift);
        spark.prev_pos.z = fire->pos.z + (src->prev_pos.z >> shift);
        spark.prev_world_pos = spark.prev_pos;

        const int32_t depth = M_GetViewDepth(spark.pos);
        if (depth <= 0 || depth >= M_MAX_DRAW_DIST) {
            continue;
        }

        spark.size.width = M_ScaleSize(src->size.width, shift, depth);
        spark.size.height = M_ScaleSize(src->size.height, shift, depth);
        spark.prev_size.width = M_ScaleSize(src->prev_size.width, shift, depth);
        spark.prev_size.height =
            M_ScaleSize(src->prev_size.height, shift, depth);

        // Fade out with distance like the original.
        int32_t bright = dim;
        if (depth > M_FADE_DRAW_DIST) {
            bright = (bright * (M_MAX_DRAW_DIST - depth)) >> 13;
        }

        // The spark stager compensates for the 128-neutral scale itself.
        spark.color.r = (src->color.r * bright) >> 8;
        spark.color.g = (src->color.g * bright) >> 8;
        spark.color.b = (src->color.b * bright) >> 8;
        spark.prev_color.r = (src->prev_color.r * bright) >> 8;
        spark.prev_color.g = (src->prev_color.g * bright) >> 8;
        spark.prev_color.b = (src->prev_color.b * bright) >> 8;

        spark.flags =
            SPARK_F_SPRITE | SPARK_F_SCALE | (src->flags & SPARK_F_ROTATE);
        spark.scalar = 2;
        spark.draw_type = DRAW_BLEND_ADD;
        // The original submits each fire spark twice (S_DrawFireSparks).
        OutputSource_PolyFX_StageSpark(&spark);
        OutputSource_PolyFX_StageSpark(&spark);
    }
}

void FX_Fire_Reset(void)
{
    memset(m_Sparks, 0, sizeof(m_Sparks));
    memset(m_Fires, 0, sizeof(m_Fires));
    m_NextSpark = 1;
}

void FX_Fire_NewFrame(void)
{
    for (int32_t i = 0; i < M_MAX_FIRES; i++) {
        m_Fires[i].on = 0;
    }
}

void FX_Fire_Add(
    const XYZ_32 pos, const int32_t size, const int16_t room_num,
    const int32_t fade)
{
    for (int32_t i = 0; i < M_MAX_FIRES; i++) {
        M_FIRE *const fire = &m_Fires[i];
        if (fire->on) {
            continue;
        }
        uint8_t on = fade & 0xFF;
        if (on == 0) {
            on = 1;
        }
        fire->pos = pos;
        fire->size = size;
        fire->room_num = room_num;
        fire->on = on;
        break;
    }
}

static bool M_AnyFireActive(void)
{
    for (int32_t i = 0; i < M_MAX_FIRES; i++) {
        if (m_Fires[i].on) {
            return true;
        }
    }
    return false;
}

void FX_Fire_Control(void)
{
    // Spawning draws from the control RNG that demo playback is locked to, so
    // only run the pool while a fire is registered this frame. Existing sparks
    // still advance to fade out; that path draws no RNG.
    if (M_AnyFireActive()) {
        M_KeepBurning();
    }

    const int32_t base_sprite = Sparks_GetSpriteIndex(SPARK_TYPE_EXPLOSION);
    for (int32_t i = 0; i < M_MAX_SPARKS; i++) {
        SPARK *const spark = &m_Sparks[i];
        if (spark->on) {
            M_AdvanceSpark(spark, base_sprite);
        }
    }
}

void FX_Fire_Draw(void)
{
    if (Sparks_GetSpriteIndex(SPARK_TYPE_EXPLOSION) == NO_ITEM) {
        return;
    }

    for (int32_t i = 0; i < M_MAX_FIRES; i++) {
        const M_FIRE *const fire = &m_Fires[i];
        if (fire->on) {
            M_DrawFire(fire);
        }
    }
}
