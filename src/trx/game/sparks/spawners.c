#include <trx/game/sparks/spawners.h>

#include <trx/config.h>
#include <trx/game/fx/water.h>
#include <trx/game/gun/registry.h>
#include <trx/game/lara.h>
#include <trx/game/output/sources/poly_fx.h>
#include <trx/game/random.h>
#include <trx/game/rooms.h>
#include <trx/game/spawn.h>
#include <trx/version.h>

static bool M_GetBloodSparkColors(RGB_888 *const src, RGB_888 *const dst)
{
    switch (g_Config.visuals.blood_effects) {
    case BLOOD_EFFECTS_DISABLED:
        return false;
    case BLOOD_EFFECTS_PINK:
        *src = (RGB_888) { 112, 0, 224 };
        *dst = (RGB_888) { 96, 0, 192 };
        return true;
    case BLOOD_EFFECTS_RED:
        *src = (RGB_888) { 224, 0, 32 };
        *dst = (RGB_888) { 192, 0, 24 };
        return true;
    case BLOOD_EFFECTS_NUMBER_OF:
        break;
    }
    return false;
}

static RGB_888 M_GetBloodShadeColor(const int32_t shade)
{
    if (g_Config.visuals.blood_effects == BLOOD_EFFECTS_PINK) {
        return (RGB_888) { shade >> 1, 0, shade };
    }
    return (RGB_888) { shade, 0, 0 };
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

    SPARK *const spark = Sparks_GetFreeSpark();
    *spark = (SPARK) {
        .on = true,
        .src_color = { 0, 0, 0 },
        .dst_color = { 144, 144, 144 },
        .fade_to_black = 2,
        .draw_type = DRAW_BLEND_ADD,
        .col_fade_speed = 4,
        .life = 128,
        .s_life = 128,
        .flags =
            SPARK_F_ATTACHED_POS | SPARK_F_FX | SPARK_F_SPRITE | SPARK_F_SCALE,
        .effect_num = effect_num,
        .sprite_idx = bubble_obj->mesh_idx,
        .sprite_obj_id = O_BUBBLE_1,
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
    Sparks_FinishSetup(spark);
}

void Sparks_TriggerWaterfallMist(
    const int32_t x, const int32_t y, const int32_t z, const int32_t angle)
{
    const int32_t sprite_idx = Sparks_GetSpriteIndex(SPARK_TYPE_EXPLOSION);
    if (sprite_idx == NO_ITEM) {
        return;
    }

    static const int32_t offsets[] = { 576, 203, -203, -576 };

    for (int32_t i = 0; i < (int32_t)ARRAY_SIZE(offsets); i++) {
        SPARK *const spark = Sparks_GetFreeSpark();

        // the sparks are spread along the line perpendicular to the flow
        const int32_t offset = (Random_GetControl() & 0x1F) + offsets[i] - 16;
        const XYZ_32 spread =
            XYZ_32_RotateYaw((XYZ_32) { .z = offset }, angle + DEG_90);

        *spark = (SPARK) {
            .on = true,
            .src_color = { 128, 128, 128 },
            .dst_color = { 192, 192, 192 },
            .col_fade_speed = 2,
            .fade_to_black = 4,
            .draw_type = DRAW_BLEND_ADD,
            .extras = 0,
            .life = (uint8_t)((Random_GetControl() & 3) + 6),
            .dynamic = -1,
            .sprite_idx = sprite_idx,
            .sprite_obj_id = O_SPARKS_GFX,
            .pos = {
                .x = x + (Random_GetControl() % 16) - 8 + spread.x,
                .y = y + (Random_GetControl() % 16) - 8,
                .z = z + (Random_GetControl() % 16) - 8 + spread.z,
            },
            .vel = XYZ_32_RotateYaw((XYZ_32) { .z = 1 }, angle),
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
        Sparks_FinishSetup(spark);
    }
}

void Sparks_TriggerSmallSplash(const XYZ_32 pos, const int32_t count)
{
    if (!g_Config.visuals.enable_droplets) {
        return;
    }

    for (int32_t i = 0; i < count; i++) {
        SPARK *const spark = Sparks_GetFreeSpark();
        spark->on = true;
        spark->src_color.r = 112;
        spark->src_color.g = (Random_GetDraw() & 0x1F) + 128;
        spark->src_color.b = (Random_GetDraw() & 0x1F) + 128;
        spark->dst_color.r = spark->src_color.r >> 1;
        spark->dst_color.g = spark->src_color.g >> 1;
        spark->dst_color.b = spark->src_color.b >> 1;
        spark->col_fade_speed = 4;
        spark->fade_to_black = 8;
        spark->life = 24;
        spark->s_life = 24;
        spark->draw_type = DRAW_BLEND_ADD;
        spark->extras = 0;
        spark->dynamic = -1;
        const int32_t ang = Random_GetDraw() & 0xFFF;
        const XYZ_32 dir = XYZ_32_RotateYaw((XYZ_32) { .z = 128 }, -(ang << 4));
        spark->vel.x = dir.x;
        spark->vel.y = -640 - (Random_GetDraw() & 0xFF);
        spark->vel.z = dir.z;
        spark->pos.x = pos.x + (spark->vel.x >> 3);
        spark->pos.y = pos.y - (spark->vel.y >> 5);
        spark->pos.z = pos.z + (spark->vel.z >> 3);
        spark->friction = 5;
        // The original leaves the size unset and draws at a fixed screen size;
        // scale in world space like the blood pixels so the splash stays
        // visible at higher resolutions.
        spark->flags = SPARK_F_SCALE;
        spark->scalar = 3;
        spark->max_y_vel = 0;
        spark->gravity = (Random_GetDraw() & 0xF) + 64;
        spark->size.width = 2;
        spark->src_size.width = 2;
        spark->size.height = 2;
        spark->src_size.height = 2;
        spark->dst_size.width = 2;
        spark->dst_size.height = 2;
        Sparks_FinishSetup(spark);
    }
}

void Sparks_TriggerBreath(
    const XYZ_32 pos, const XYZ_32 vel, const int16_t room_num)
{
    const int32_t sprite_idx = Sparks_GetSpriteIndex(SPARK_TYPE_EXPLOSION);
    if (sprite_idx == NO_ITEM) {
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
        .col_fade_speed = 4,
        .fade_to_black = 32,
        .draw_type = DRAW_BLEND_ADD,
        .extras = 0,
        .life = (uint8_t)((Random_GetControl() & 3) + 37),
        .dynamic = -1,
        .sprite_idx = sprite_idx,
        .sprite_obj_id = O_SPARKS_GFX,
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
    Sparks_FinishSetup(spark);
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

    SPARK *const spark = Sparks_InitialiseSpriteSpark(SPARK_TYPE_EXPLOSION);
    if (spark == nullptr) {
        return;
    }

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

    // TR4 gives the slow 16-frame fade to the free sparks and the
    // type-based recipes to the attached ones; TR3 and TR5 arrange it the
    // other way around.
    const bool use_slow_fade =
        g_TRVersion == 4 ? body_part == -1 : body_part != -1;
    if (use_slow_fade) {
        spark->fade_to_black = 16;
        spark->col_fade_speed = (Random_GetControl() & 3) + 8;
        spark->life = (Random_GetControl() & 3) + 28;
        spark->s_life = spark->life;
    } else if (type == 2 || type == 255 || type == 254) {
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
    Sparks_FinishSetup(spark);
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

    SPARK *const spark = Sparks_InitialiseSpriteSpark(SPARK_TYPE_EXPLOSION);
    if (spark == nullptr) {
        return;
    }

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
    spark->gravity = -16 - (Random_GetControl() & 0xF);
    spark->max_y_vel = -8 - (Random_GetControl() & 7);
    spark->dst_size.width = (Random_GetControl() & 0x3F) + 64;
    spark->src_size.width = spark->dst_size.width >> 2;
    spark->size.width = spark->src_size.width;
    spark->src_size.height = spark->src_size.width;
    spark->size.height = spark->src_size.width;
    spark->dst_size.height = spark->dst_size.width;
    Sparks_FinishSetup(spark);
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

    SPARK *const spark = Sparks_InitialiseSpriteSpark(SPARK_TYPE_EXPLOSION);
    if (spark == nullptr) {
        return;
    }

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
    spark->scalar = 2;
    spark->dst_size.width = size;
    spark->dst_size.height = size;
    spark->src_size.height = size;
    spark->src_size.width = size;
    spark->size.height = size;
    spark->size.width = size;
    Sparks_FinishSetup(spark);
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

    SPARK *const spark = Sparks_InitialiseSpriteSpark(SPARK_TYPE_EXPLOSION);
    if (spark == nullptr) {
        return;
    }

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

    const XYZ_32 dir = XYZ_32_RotateYaw((XYZ_32) { .z = dist }, angle);
    spark->vel.x = (int16_t)((Random_GetControl() & 0x7F) + dir.x - 64);
    spark->vel.y = -6 - (Random_GetControl() & 7);
    spark->vel.z = (int16_t)((Random_GetControl() & 0x7F) + dir.z - 64);
    spark->friction = 4;
    spark->flags = SPARK_F_ALT_SPRITE | SPARK_F_SPRITE | SPARK_F_SCALE;
    spark->gravity = -8 - (Random_GetControl() & 0xF);
    spark->max_y_vel = -8 - (Random_GetControl() & 7);
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
    Sparks_FinishSetup(spark);
}

void Sparks_TriggerBloodTR3(
    const XYZ_32 pos, int32_t angle_12, const int32_t count)
{
    RGB_888 src_color;
    RGB_888 dst_color;
    if (!M_GetBloodSparkColors(&src_color, &dst_color)) {
        return;
    }

    for (int32_t i = 0; i < count; i++) {
        SPARK *const spark = Sparks_GetFreeSpark();
        spark->on = true;
        spark->src_color = src_color;
        spark->dst_color = dst_color;

        spark->col_fade_speed = 8;
        spark->fade_to_black = 8;
        spark->life = 24;
        spark->s_life = 24;
        spark->draw_type = DRAW_BLEND;
        spark->extras = 0;
        spark->dynamic = -1;
        spark->pos.x = pos.x + (Random_GetControl() & 0x1F) - 16;
        spark->pos.y = pos.y + (Random_GetControl() & 0x1F) - 16;
        spark->pos.z = pos.z + (Random_GetControl() & 0x1F) - 16;
        const int16_t dist = Random_GetControl() & 0xF;
        const int32_t ang =
            ((Random_GetControl() & 0x1F) + angle_12 - 16) & 0xFFF;
        const XYZ_32 dir =
            XYZ_32_RotateYaw((XYZ_32) { .z = dist * 128 }, -(ang << 4));
        spark->vel.x = dir.x;
        spark->vel.y = -128 - (Random_GetControl() & 0xFF);
        spark->vel.z = dir.z;
        spark->friction = 4;
        spark->flags = SPARK_F_BLOOD | SPARK_F_SCALE;
        spark->scalar = 3;
        spark->max_y_vel = 0;
        spark->gravity = (Random_GetControl() & 0x1F) + 31;
        spark->size.width = 2;
        spark->src_size.width = 2;
        spark->size.height = 2;
        spark->src_size.height = 2;
        spark->dst_size.width = 2 - (Random_GetControl() & 1);
        spark->dst_size.height = 2 - (Random_GetControl() & 1);
        Sparks_FinishSetup(spark);
    }
}

void Sparks_TriggerBloodTR3D(
    const XYZ_32 pos, int32_t angle_12, const int32_t count)
{
    RGB_888 src_color;
    RGB_888 dst_color;
    if (!M_GetBloodSparkColors(&src_color, &dst_color)) {
        return;
    }

    for (int32_t i = 0; i < count; i++) {
        SPARK *const spark = Sparks_GetFreeSpark();
        spark->on = true;
        spark->src_color = src_color;
        spark->dst_color = dst_color;

        spark->col_fade_speed = 8;
        spark->fade_to_black = 8;
        spark->life = 24;
        spark->s_life = 24;
        spark->draw_type = DRAW_BLEND;
        spark->extras = 0;
        spark->dynamic = -1;
        spark->pos.x = pos.x + (Random_GetDraw() & 0x1F) - 16;
        spark->pos.y = pos.y + (Random_GetDraw() & 0x1F) - 16;
        spark->pos.z = pos.z + (Random_GetDraw() & 0x1F) - 16;
        const int16_t dist = Random_GetDraw() & 0xF;
        const int32_t ang = ((Random_GetDraw() & 0x1F) + angle_12 - 16) & 0xFFF;
        const XYZ_32 dir =
            XYZ_32_RotateYaw((XYZ_32) { .z = dist * 128 }, -(ang << 4));
        spark->vel.x = dir.x;
        spark->vel.y = -128 - (Random_GetDraw() & 0xFF);
        spark->vel.z = dir.z;
        spark->friction = 4;
        spark->flags = SPARK_F_SCALE;
        spark->scalar = 3;
        spark->max_y_vel = 0;
        spark->gravity = (Random_GetDraw() & 0x1F) + 31;
        spark->size.width = 2;
        spark->src_size.width = 2;
        spark->size.height = 2;
        spark->src_size.height = 2;
        spark->dst_size.width = 2 - (Random_GetDraw() & 1);
        spark->dst_size.height = 2 - (Random_GetDraw() & 1);
        Sparks_FinishSetup(spark);
    }
}

void Sparks_TriggerBloodTR4(
    const XYZ_32 pos, const int32_t angle_12, const int32_t count)
{
    if (g_Config.visuals.blood_effects == BLOOD_EFFECTS_DISABLED) {
        return;
    }

    for (int32_t i = 0; i < count; i++) {
        const int32_t shade = (Random_GetControl() & 0x3F) + 48;
        SPARK *const spark = Sparks_InitialiseSpriteSpark(SPARK_TYPE_BLOOD);
        if (spark == nullptr) {
            return;
        }

        spark->src_color = COLOR_RGB_888_BLACK;
        spark->dst_color = M_GetBloodShadeColor(shade);
        spark->col_fade_speed = 4;
        spark->fade_to_black = 8;
        spark->life = (Random_GetControl() & 7) + 24;
        spark->s_life = spark->life;
        spark->draw_type = DRAW_BLEND_ADD;
        spark->extras = 0;
        spark->dynamic = -1;
        spark->pos.x = pos.x + (Random_GetControl() & 0x1F) - 16;
        spark->pos.y = pos.y + (Random_GetControl() & 0x1F) - 16;
        spark->pos.z = pos.z + (Random_GetControl() & 0x1F) - 16;
        const int32_t dist = Random_GetControl() & 0xF;
        const int32_t ang = angle_12 == -1
            ? Random_GetControl() & 0xFFF
            : (((Random_GetControl() & 0x1F) + angle_12 - 16) & 0xFFF);
        spark->vel.x = -(dist * Math_Sin(ang << 4)) >> 9;
        spark->vel.y = -128 - (Random_GetControl() & 0xFF);
        spark->vel.z = (dist * Math_Cos(ang << 4)) >> 9;
        spark->friction = 4;
        spark->flags =
            SPARK_F_BLOOD | SPARK_F_SCALE | SPARK_F_SPRITE | SPARK_F_ROTATE;
        spark->rot_angle = Random_GetControl() & 0xFFF;
        spark->rot_add = (Random_GetControl() & 1) != 0
            ? -64 - (Random_GetControl() & 0x3F)
            : (Random_GetControl() & 0x3F) + 64;
        spark->scalar = 3;
        spark->max_y_vel = 0;
        spark->gravity = (Random_GetControl() & 0x1F) + 31;
        const int32_t size = (Random_GetControl() & 7) + 8;
        spark->size.width = size;
        spark->src_size.width = size;
        spark->dst_size.width = size >> 2;
        spark->size.height = size;
        spark->src_size.height = size;
        spark->dst_size.height = size >> 2;
        Sparks_FinishSetup(spark);
    }
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

    const int32_t water_height = Room_GetWaterHeight(item->pos, item->room_num);
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
    FX_Water_SetupSplash(&(FX_WATER_SPLASH_SETUP) {
        .pos = { .x = item->pos.x, .y = room->max_ceiling, .z = item->pos.z },
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

    const int32_t sprite_idx = Sparks_GetSpriteIndex(SPARK_TYPE_EXPLOSION);
    if (sprite_idx == NO_ITEM) {
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
        .draw_type = DRAW_BLEND_ADD,
        .extras = (uint8_t)(
            safe_extras
            | ((extras_table[safe_extras] + (Random_GetControl() & 7) - 4)
               << 3)),
        .life = 0,
        .dynamic = (int8_t)dynamic,
        .sprite_idx = sprite_idx,
        .sprite_obj_id = O_SPARKS_GFX,
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
        // TR4 spins either way; TR3 only clockwise and slower.
        const int32_t rot_add = g_TRVersion == 4
            ? (Random_GetControl() & 0xFF) + 128
            : MIN((Random_GetControl() & 0x7F) + 32, 127);
        spark->rot_add = (int8_t)rot_add;
    }

    spark->src_size.width = (uint8_t)((Random_GetControl() & 0xF) + 40);
    // TR4 keeps the puff square; TR3 stretches it vertically.
    spark->src_size.height = g_TRVersion == 4
        ? spark->src_size.width
        : (uint8_t)(spark->src_size.width + (Random_GetControl() & 7) + 8);
    spark->dst_size.width = (uint8_t)(spark->src_size.width << 1);
    spark->dst_size.height = (uint8_t)(spark->src_size.height << 1);
    spark->size = spark->src_size;

    if (uw == 2) {
        const RGB_888 src = spark->src_color;
        const RGB_888 dst = spark->dst_color;
        spark->src_color = (RGB_888) { src.b, src.r, src.g };
        spark->dst_color = (RGB_888) { dst.b, dst.r, dst.g };
        spark->color = spark->src_color;
        spark->flags |= SPARK_F_GREEN;
    } else if (extras != 0) {
        Sparks_TriggerExplosionSmoke(pos, uw, room_num);
    } else {
        Sparks_TriggerExplosionSmokeEnd(pos, uw, room_num);
    }
    Sparks_FinishSetup(spark);
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

    const int32_t sprite_idx = Sparks_GetSpriteIndex(SPARK_TYPE_EXPLOSION);
    if (sprite_idx == NO_ITEM) {
        return;
    }

    SPARK *const spark = Sparks_GetFreeSpark();
    *spark = (SPARK) {
        .on = true,
        .src_color = { 128, 64, 0 },
        .dst_color = { 128, 128, 128 },
        .col_fade_speed = 8,
        .fade_to_black = 12,
        .life = 24,
        .s_life = 24,
        .draw_type = DRAW_BLEND_ADD,
        .extras = 0,
        .dynamic = -1,
        .sprite_idx = sprite_idx,
        .sprite_obj_id = O_SPARKS_GFX,
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
    Sparks_FinishSetup(spark);

    for (int32_t i = 0; i < 7; i++) {
        const XYZ_32 bubble_pos = {
            .x = pos.x + (Random_GetControl() & 0x1FF) - 256,
            .y = pos.y + (Random_GetControl() & 0x7F) - 64,
            .z = pos.z + (Random_GetControl() & 0x1FF) - 256,
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

    SPARK *const spark = Sparks_InitialiseSpriteSpark(SPARK_TYPE_EXPLOSION);
    if (spark == nullptr) {
        return;
    }

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
    spark->pos.x = pos.x + (Random_GetControl() & 0x1FF) - 256;
    spark->pos.y = pos.y + (Random_GetControl() & 0x1FF) - 256;
    spark->pos.z = pos.z + (Random_GetControl() & 0x1FF) - 256;
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
    Sparks_FinishSetup(spark);
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

    SPARK *const spark = Sparks_InitialiseSpriteSpark(SPARK_TYPE_EXPLOSION);
    if (spark == nullptr) {
        return;
    }

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
    spark->pos.x = pos.x + (Random_GetControl() & 0x1F) - 16;
    spark->pos.y = pos.y + (Random_GetControl() & 0x1F) - 16;
    spark->pos.z = pos.z + (Random_GetControl() & 0x1F) - 16;
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
    Sparks_FinishSetup(spark);
}

void Sparks_TriggerDartSmoke(const XYZ_32 pos, const XZ_32 vel, const bool hit)
{
    const ITEM *const lara_item = Lara_GetItem();
    const int32_t dx = lara_item->pos.x - pos.x;
    const int32_t dz = lara_item->pos.z - pos.z;
    const int32_t max_dist = 16 * WALL_L;
    if (dx < -max_dist || dx > max_dist || dz < -max_dist || dz > max_dist) {
        return;
    }

    SPARK *const spark = Sparks_InitialiseSpriteSpark(SPARK_TYPE_EXPLOSION);
    if (spark == nullptr) {
        return;
    }

    spark->src_color.r = 16;
    spark->src_color.g = 8;
    spark->src_color.b = 4;
    spark->dst_color.r = 64;
    spark->dst_color.g = 48;
    spark->dst_color.b = 32;
    spark->col_fade_speed = 8;
    spark->fade_to_black = 4;
    spark->draw_type = DRAW_BLEND_ADD;
    spark->extras = 0;
    spark->life = (Random_GetControl() & 3) + 32;
    spark->s_life = spark->life;
    spark->dynamic = -1;
    spark->pos.x = pos.x + (Random_GetControl() & 0x1F) - 16;
    spark->pos.y = pos.y + (Random_GetControl() & 0x1F) - 16;
    spark->pos.z = pos.z + (Random_GetControl() & 0x1F) - 16;

    if (hit) {
        spark->vel.x = (Random_GetControl() & 0xFF) - vel.x - 128;
        spark->vel.y = -4 - (Random_GetControl() & 3);
        spark->vel.z = (Random_GetControl() & 0xFF) - vel.z - 128;
    } else {
        if (vel.x != 0) {
            spark->vel.x = -vel.x;
        } else {
            spark->vel.x = (Random_GetControl() & 0xFF) - 128;
        }

        spark->vel.y = -4 - (Random_GetControl() & 3);

        if (vel.z != 0) {
            spark->vel.z = -vel.z;
        } else {
            spark->vel.z = (Random_GetControl() & 0xFF) - 128;
        }
    }

    spark->friction = 3;

    if ((Random_GetControl() & 1) != 0) {
        spark->flags = SPARK_F_ALT_SPRITE | SPARK_F_ROTATE | SPARK_F_SPRITE
            | SPARK_F_SCALE;
        spark->rot_angle = Random_GetControl() & 0xFFF;

        if ((Random_GetControl() & 1) != 0) {
            spark->rot_add = -16 - (Random_GetControl() & 0xF);
        } else {
            spark->rot_add = (Random_GetControl() & 0xF) + 16;
        }
    } else {
        spark->flags = SPARK_F_ALT_SPRITE | SPARK_F_SPRITE | SPARK_F_SCALE;
    }

    spark->scalar = 1;
    int32_t rnd = (Random_GetControl() & 0x3F) + 72;
    if (hit) {
        rnd >>= 1;
        spark->dst_size.width = (uint8_t)rnd;
        spark->size.width = spark->dst_size.width >> 2;
        spark->src_size.width = spark->size.width;
        spark->dst_size.height = (uint8_t)rnd;
        spark->size.height = spark->dst_size.height >> 2;
        spark->src_size.height = spark->size.height;
        spark->max_y_vel = 0;
        spark->gravity = 0;
    } else {
        spark->dst_size.width = (uint8_t)rnd;
        spark->size.width = spark->dst_size.width >> 4;
        spark->src_size.width = spark->size.width;
        spark->dst_size.height = (uint8_t)rnd;
        spark->size.height = spark->dst_size.height >> 4;
        spark->src_size.height = spark->size.height;
        spark->gravity = -4 - (Random_GetControl() & 3);
        spark->max_y_vel = -4 - (Random_GetControl() & 3);
    }
    Sparks_FinishSetup(spark);
}

void Sparks_TriggerPickupAid(const XYZ_32 pos, const XZ_32 vel)
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
    spark->src_color.r = 48;
    spark->src_color.g = 40;
    spark->src_color.b = 36;
    spark->dst_color.r = (Random_GetDraw() & 0x20) + 96;
    spark->dst_color.g = spark->dst_color.r;
    spark->dst_color.b = 96;
    spark->col_fade_speed = 8;
    spark->fade_to_black = 2;
    spark->draw_type = DRAW_BLEND_ADD;
    spark->extras = 0;
    spark->life = (Random_GetControl() & 3) + 7;
    spark->s_life = spark->life;
    spark->dynamic = -1;
    spark->pos = pos;
    spark->pos.y -= Random_GetControl() & 16;

    if (vel.x != 0) {
        spark->vel.x = -vel.x;
    } else {
        spark->vel.x = (Random_GetControl() & 0x20) - 16;
    }

    spark->vel.y = -4 - (Random_GetControl() & 3);

    if (vel.z != 0) {
        spark->vel.z = -vel.z;
    } else {
        spark->vel.z = (Random_GetControl() & 0x20) - 16;
    }

    spark->friction = 3;

    if ((Random_GetControl() & 1) != 0) {
        spark->flags = SPARK_F_ROTATE | SPARK_F_SPRITE | SPARK_F_SCALE;
        spark->rot_angle = Random_GetControl() & 0xFFF;

        if ((Random_GetControl() & 1) != 0) {
            spark->rot_add = -16 - (Random_GetControl() & 0xF);
        } else {
            spark->rot_add = (Random_GetControl() & 0xF) + 16;
        }
    } else {
        spark->flags = SPARK_F_SPRITE | SPARK_F_SCALE;
    }

    const OBJECT *const obj = Object_Get(O_PICKUP_AID);
    const int32_t mesh_count = ABS(obj->mesh_count) - 1;
    spark->sprite_idx = obj->mesh_idx + (Random_GetControl() & mesh_count);
    spark->sprite_obj_id = O_PICKUP_AID;

    spark->scalar = 1;
    int32_t rnd = (Random_GetControl() & 0x3F) + 36;
    spark->dst_size.width = (uint8_t)rnd;
    spark->size.width = spark->dst_size.width >> 4;
    spark->src_size.width = spark->size.width;
    spark->dst_size.height = (uint8_t)rnd;
    spark->size.height = spark->dst_size.height >> 4;
    spark->src_size.height = spark->size.height;
    spark->gravity = -4 - (Random_GetControl() & 3);
    spark->max_y_vel = -4 - (Random_GetControl() & 3);

    Sparks_FinishSetup(spark);
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
    spark->extras = 0;
    spark->dynamic = -1;
    spark->pos.x = pos.x + (Random_GetDraw() & 7) - 3;
    spark->pos.y = pos.y + (Random_GetDraw() & 7) - 3;
    spark->pos.z = pos.z + (Random_GetDraw() & 7) - 3;
    spark->vel.x = (int16_t)(vel.x + (Random_GetDraw() & 0xFF) - 128);
    spark->vel.y = (int16_t)(vel.y + (Random_GetDraw() & 0xFF) - 128);
    spark->vel.z = (int16_t)(vel.z + (Random_GetDraw() & 0xFF) - 128);
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
    Sparks_FinishSetup(spark);

    if (!smoke) {
        return;
    }

    SPARK *const smoke_spark =
        Sparks_InitialiseSpriteSpark(SPARK_TYPE_EXPLOSION);
    if (smoke_spark == nullptr) {
        return;
    }

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
    smoke_spark->vel.x = (int16_t)(vel.x + (Random_GetDraw() & 0x3F) - 32);
    smoke_spark->vel.y = (int16_t)vel.y;
    smoke_spark->vel.z = (int16_t)(vel.z + (Random_GetDraw() & 0x3F) - 32);
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

    smoke_spark->scalar = 2;
    smoke_spark->gravity = -8 - (Random_GetDraw() & 3);
    smoke_spark->max_y_vel = -4 - (Random_GetDraw() & 3);
    smoke_spark->dst_size.width = (Random_GetDraw() & 0xF) + 24;
    smoke_spark->src_size.width = smoke_spark->dst_size.width >> 3;
    smoke_spark->size.width = smoke_spark->dst_size.width >> 3;
    smoke_spark->dst_size.height = smoke_spark->dst_size.width;
    smoke_spark->src_size.height = smoke_spark->dst_size.height >> 3;
    smoke_spark->size.height = smoke_spark->dst_size.height >> 3;
    Sparks_FinishSetup(smoke_spark);
}

void Sparks_TriggerRicochetTR3(
    const GAME_VECTOR pos, const int32_t angle, const int32_t size)
{
    SPARK *spark = Sparks_GetFreeSpark();
    spark->on = true;
    spark->src_color.r = 255;
    spark->src_color.g = (Random_GetControl() & 0x1F) + 32;
    spark->src_color.b = 0;
    spark->dst_color.r = 192;
    spark->dst_color.g = (Random_GetControl() & 0x3F) + 96;
    spark->dst_color.b = 0;
    spark->col_fade_speed = 8;
    spark->fade_to_black = 8;
    spark->life = 24;
    spark->s_life = 24;
    spark->draw_type = DRAW_BLEND_ADD;
    spark->extras = 0;
    spark->dynamic = -1;
    spark->pos.x = pos.x;
    spark->pos.y = pos.y;
    spark->pos.z = pos.z;
    int32_t ang = ((Random_GetControl() & 0x7FF) + angle - 1024) & 0xFFF;
    XYZ_32 dir = XYZ_32_RotateYaw((XYZ_32) { .z = 2048 }, -(ang << 4));
    spark->vel.x = dir.x;
    spark->vel.y = 2 * (Random_GetControl() & 0x1FF) - 768;
    spark->vel.z = dir.z;
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
    Sparks_FinishSetup(spark);

    spark = Sparks_GetFreeSpark();
    spark->on = true;
    uint8_t c = (uint8_t)((Random_GetControl() & 0x3F) + 128);
    spark->src_color.r = c;
    spark->src_color.g = c;
    spark->src_color.b = c;
    c >>= 1;
    spark->dst_color.r = c;
    spark->dst_color.g = c;
    spark->dst_color.b = c;
    spark->draw_type = DRAW_BLEND_SUB;
    spark->extras = 0;
    spark->col_fade_speed = 8;
    spark->fade_to_black = 16;
    spark->life = 28;
    spark->s_life = 28;
    spark->dynamic = -1;
    spark->pos.x = pos.x;
    spark->pos.y = pos.y;
    spark->pos.z = pos.z;
    ang = ((Random_GetControl() & 0x7FF) + angle - 1023) & 0xFFF;
    dir = XYZ_32_RotateYaw((XYZ_32) { .z = 2048 }, -(ang << 4));
    spark->vel.x = dir.x;
    spark->vel.y = (Random_GetControl() & 0x1FF) - 384;
    spark->vel.z = dir.z;
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
    Sparks_FinishSetup(spark);
}

void Sparks_TriggerRicochetTR4(
    const GAME_VECTOR pos, const int32_t angle, const int32_t count,
    const int32_t smoke_only)
{
    if (smoke_only == 0) {
        for (int32_t i = 0; i < count; i++) {
            const int32_t rnd = Random_GetControl();
            SPARK *const spark = Sparks_GetFreeSpark();
            spark->on = true;
            spark->src_color.r = 128;
            spark->src_color.g = (rnd & 0xF) + 16;
            spark->src_color.b = 0;
            spark->dst_color.r = 96;
            spark->dst_color.g = ((rnd >> 4) & 0x1F) + 48;
            spark->dst_color.b = 0;
            spark->col_fade_speed = 2;
            spark->fade_to_black = 4;
            spark->life = 9;
            spark->s_life = 9;
            spark->draw_type = DRAW_BLEND_ADD;
            spark->extras = 0;
            spark->dynamic = -1;
            spark->pos = pos.pos;
            spark->vel.y = (rnd & 0xFFF) - 2048;
            spark->gravity = (rnd >> 7) & 0x1F;
            const int32_t ang = (((rnd >> 3) & 0x7FF) + angle - 1024) & 0xFFF;
            const XYZ_32 dir =
                XYZ_32_RotateYaw((XYZ_32) { .z = 1024 }, -(ang << 4));
            spark->vel.x = dir.x;
            spark->vel.z = dir.z;
            spark->friction = 34;
            spark->flags = SPARK_F_NONE;
            spark->max_y_vel = 0;
            Sparks_FinishSetup(spark);
        }

        SPARK *const spark = Sparks_InitialiseSpriteSpark(SPARK_TYPE_RICOCHET);
        if (spark != nullptr) {
            const int32_t rnd = Random_GetControl();
            spark->src_color.r = 48;
            spark->src_color.g = (rnd & 0xF) + 32;
            spark->src_color.b = 0;
            spark->dst_color.r = 0;
            spark->dst_color.g = 0;
            spark->dst_color.b = 0;
            spark->col_fade_speed = 4;
            spark->fade_to_black = 0;
            spark->life = 4;
            spark->s_life = 4;
            spark->draw_type = DRAW_BLEND_ADD;
            spark->extras = 0;
            spark->dynamic = -1;
            spark->pos = pos.pos;
            spark->vel = (XYZ_32) {};
            spark->flags = SPARK_F_SCALE | SPARK_F_SPRITE | SPARK_F_ROTATE;
            spark->rot_angle = (rnd >> 2) & 0xFFF;
            spark->rot_add = (rnd & 1) != 0 ? -64 - ((rnd >> 1) & 0x3F)
                                            : ((rnd >> 1) & 0x3F) + 64;
            spark->scalar = 3;
            spark->size.width = ((rnd >> 10) & 7) + 8;
            spark->src_size.width = spark->size.width;
            spark->dst_size.width = 1;
            spark->size.height = spark->size.width;
            spark->src_size.height = spark->size.height;
            spark->dst_size.height = 1;
            spark->max_y_vel = 0;
            spark->gravity = 0;
            Sparks_FinishSetup(spark);
        }
    }

    // OG passes -5 rather than a flag, which both skips the streaks and turns
    // the single puff into six drifting ones.
    for (int32_t i = 0; i < 1 - smoke_only; i++) {
        SPARK *const spark = Sparks_InitialiseSpriteSpark(SPARK_TYPE_EXPLOSION);
        if (spark == nullptr) {
            return;
        }

        const int32_t rnd = Random_GetControl();
        spark->src_color.r = 0;
        spark->src_color.g = 0;
        spark->src_color.b = 0;
        spark->dst_color.r = 40;
        spark->dst_color.g = 40;
        spark->dst_color.b = 48;
        spark->col_fade_speed = (rnd & 3) + 4;
        spark->fade_to_black = 8;
        spark->life = ((rnd >> 2) & 7) + 16;
        spark->s_life = spark->life;
        spark->extras = 0;
        spark->dynamic = -1;
        spark->pos = pos.pos;

        if (smoke_only != 0) {
            spark->col_fade_speed >>= 1;
            spark->fade_to_black = 4;
            spark->life >>= 1;
            spark->s_life >>= 1;
            spark->vel.x = (rnd & 0x1FF) - 256;
            spark->vel.y = ((rnd >> 2) & 0x1FF) - 256;
            spark->vel.z = ((rnd >> 4) & 0x1FF) - 256;
        } else {
            spark->vel = (XYZ_32) {};
        }

        spark->draw_type = DRAW_BLEND_ADD;
        spark->friction = 0;
        spark->flags = SPARK_F_SCALE | SPARK_F_SPRITE | SPARK_F_ROTATE;
        spark->rot_angle = (rnd >> 3) & 0xFFF;
        spark->rot_add = (rnd & 1) != 0 ? -16 - (rnd & 0xF) : (rnd & 0xF) + 16;
        spark->scalar = 2;
        spark->gravity = -4 - ((rnd >> 9) & 3);
        spark->max_y_vel = -4 - ((rnd >> 6) & 3);
        spark->size.width = ((rnd >> 5) & 7) + 4;
        spark->src_size.width = spark->size.width;
        spark->dst_size.width = spark->size.width << 2;
        spark->size.height = spark->size.width;
        spark->src_size.height = spark->size.height;
        spark->dst_size.height = spark->dst_size.width;
        Sparks_FinishSetup(spark);
    }
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
    SPARK *const spark = Sparks_InitialiseSpriteSpark(SPARK_TYPE_EXPLOSION);
    if (spark == nullptr) {
        return;
    }

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

    spark->draw_type = DRAW_BLEND_ADD;
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

    spark->scalar = 3;
    spark->gravity = -2 - (Random_GetControl() & 1);
    spark->max_y_vel = -2 - (Random_GetControl() & 1);

    const WEAPON_INFO *const info = Gun_Registry_Get(weapon);
    const uint8_t size = info->smoke_size_func != nullptr
        ? info->smoke_size_func()
        : (Random_GetControl() & 7) + 12;

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
    Sparks_FinishSetup(spark);
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
    spark->draw_type = DRAW_BLEND_ADD;
    spark->extras = 0;
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
    Sparks_FinishSetup(spark);
}

void Sparks_TriggerRocketSmoke(
    const XYZ_32 pos, const int32_t c, const int16_t room_num)
{
    SPARK *const spark = Sparks_InitialiseSpriteSpark(SPARK_TYPE_EXPLOSION);
    if (spark == nullptr) {
        return;
    }

    spark->src_color.r = 0;
    spark->src_color.g = 0;
    spark->src_color.b = 0;
    spark->dst_color.r = c + 64;
    spark->dst_color.g = c + 64;
    spark->dst_color.b = c + 64;
    spark->fade_to_black = 12;
    spark->col_fade_speed = (Random_GetControl() & 3) + 4;
    spark->draw_type = DRAW_BLEND_ADD;
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
    spark->gravity = -4 - (Random_GetControl() & 3);
    spark->max_y_vel = -4 - (Random_GetControl() & 3);
    const uint8_t size = (Random_GetControl() & 7) + 32;
    spark->dst_size.width = size;
    spark->src_size.width = size >> 2;
    spark->size.width = size >> 2;
    spark->src_size.height = size >> 2;
    spark->size.height = size >> 2;
    spark->dst_size.height = size;
    Sparks_FinishSetup(spark);
}

void Sparks_TriggerRocketFlame(
    const XYZ_32 pos, const XYZ_32 vel, const int16_t item_num,
    const int16_t room_num)
{
    SPARK *const spark = Sparks_InitialiseSpriteSpark(SPARK_TYPE_EXPLOSION);
    if (spark == nullptr) {
        return;
    }

    spark->src_color.r = (Random_GetControl() & 0x1F) + 48;
    spark->src_color.g = spark->src_color.r;
    spark->src_color.b = (Random_GetControl() & 0x3F) + 192;
    spark->dst_color.r = (Random_GetControl() & 0x3F) + 192;
    spark->dst_color.g = (Random_GetControl() & 0x3F) + 128;
    spark->dst_color.b = 32;
    spark->fade_to_black = 12;
    spark->col_fade_speed = (Random_GetControl() & 3) + 12;
    spark->draw_type = DRAW_BLEND_ADD;
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
    spark->size.width = (Random_GetControl() & 7) + 32;
    spark->src_size.width = spark->size.width;
    spark->dst_size.width = 2;
    spark->size.height = spark->size.width;
    spark->src_size.height = spark->size.height;
    spark->dst_size.height = 2;
    Sparks_FinishSetup(spark);
}

void Sparks_TriggerFlamethrowerHitFlame(const XYZ_32 pos)
{
    const ITEM *const lara_item = Lara_GetItem();
    const int32_t dx = lara_item->pos.x - pos.x;
    const int32_t dz = lara_item->pos.z - pos.z;
    const int32_t max_dist = 16 * WALL_L;
    if (dx < -max_dist || dx > max_dist || dz < -max_dist || dz > max_dist) {
        return;
    }

    SPARK *const spark = Sparks_InitialiseSpriteSpark(SPARK_TYPE_EXPLOSION);
    if (spark == nullptr) {
        return;
    }

    spark->src_color.r = 255;
    spark->src_color.g = (Random_GetControl() & 0x1F) + 48;
    spark->src_color.b = 48;
    spark->dst_color.r = (Random_GetControl() & 0x3F) + 192;
    spark->dst_color.g = (Random_GetControl() & 0x3F) + 128;
    spark->dst_color.b = 32;

    spark->fade_to_black = 8;
    spark->col_fade_speed = (Random_GetControl() & 3) + 8;
    spark->draw_type = DRAW_BLEND_ADD;
    spark->extras = 0;
    spark->life = (Random_GetControl() & 7) + 20;
    spark->s_life = spark->life;
    spark->dynamic = -1;
    spark->pos.x = pos.x + (Random_GetControl() & 0x1F) - 16;
    spark->pos.y = pos.y;
    spark->pos.z = pos.z + (Random_GetControl() & 0x1F) - 16;
    spark->vel.x = (Random_GetControl() & 0xFF) - 128;
    spark->vel.y = -16 - (Random_GetControl() & 0xF);
    spark->vel.z = (Random_GetControl() & 0xFF) - 128;
    spark->friction = 5;

    if (Random_GetControl() & 1) {
        spark->gravity = -16 - (Random_GetControl() & 0x1F);
        spark->max_y_vel = -16 - (Random_GetControl() & 7);
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
        spark->gravity = -16 - (Random_GetControl() & 0x1F);
        spark->max_y_vel = -16 - (Random_GetControl() & 7);
    }

    spark->scalar = 2;
    spark->size.width = (Random_GetControl() & 0x1F) + 128;
    spark->src_size.width = spark->size.width;
    spark->dst_size.width = spark->size.width >> 4;
    spark->size.height = spark->size.width;
    spark->src_size.height = spark->size.height;
    spark->dst_size.height = spark->size.height >> 4;
    Sparks_FinishSetup(spark);
}

void Sparks_TriggerFlamethrowerSmoke(const XYZ_32 pos, const bool uw)
{
    const ITEM *const lara_item = Lara_GetItem();
    const int32_t dx = lara_item->pos.x - pos.x;
    const int32_t dz = lara_item->pos.z - pos.z;
    const int32_t max_dist = 16 * WALL_L;
    if (dx < -max_dist || dx > max_dist || dz < -max_dist || dz > max_dist) {
        return;
    }

    SPARK *const spark = Sparks_InitialiseSpriteSpark(SPARK_TYPE_EXPLOSION);
    if (spark == nullptr) {
        return;
    }

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
    spark->fade_to_black = 23;
    spark->life = (Random_GetControl() & 0xF) + 32;
    spark->s_life = spark->life;

    if (uw) {
        spark->draw_type = DRAW_BLEND_ADD;
    } else {
        spark->draw_type = DRAW_BLEND_SUB;
    }

    spark->extras = 0;
    spark->dynamic = -1;
    spark->pos.x = pos.x + (Random_GetControl() & 0x1F) - 16;
    spark->pos.y = pos.y + (Random_GetControl() & 0x1F) - 16;
    spark->pos.z = pos.z + (Random_GetControl() & 0x1F) - 16;
    spark->vel.x = ((Random_GetControl() & 0xFFF) - 2048) >> 2;
    spark->vel.y = (Random_GetControl() & 0xFF) - 128;
    spark->vel.z = ((Random_GetControl() & 0xFFF) - 2048) >> 2;

    if (uw) {
        spark->friction = 20;
        spark->vel.y >>= 4;
        spark->pos.y += 32;
    } else {
        spark->friction = 6;
    }

    spark->flags =
        SPARK_F_ALT_SPRITE | SPARK_F_ROTATE | SPARK_F_SPRITE | SPARK_F_SCALE;
    spark->rot_angle = Random_GetControl() & 0xFFF;

    if (Random_GetControl() & 1) {
        spark->rot_add = -16 - (Random_GetControl() & 0xF);
    } else {
        spark->rot_add = (Random_GetControl() & 0xF) + 16;
    }

    spark->scalar = 3;

    if (uw) {
        spark->max_y_vel = 0;
        spark->gravity = 0;
    } else {
        spark->gravity = -3 - (Random_GetControl() & 3);
        spark->max_y_vel = -4 - (Random_GetControl() & 3);
    }

    spark->dst_size.width = (Random_GetControl() & 0x1F) + 128;
    spark->size.width = spark->dst_size.width >> 2;
    spark->src_size.width = spark->size.width;

    spark->dst_size.height =
        spark->dst_size.width + (Random_GetControl() & 0x1F) + 32;
    spark->size.height = spark->dst_size.height >> 3;
    spark->src_size.height = spark->size.height;
    Sparks_FinishSetup(spark);
}
