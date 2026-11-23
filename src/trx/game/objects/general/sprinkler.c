#include <trx/core/json/util/read_io.h>
#include <trx/core/json/util/write_io.h>
#include <trx/game/fx/droplets.h>
#include <trx/game/gun.h>
#include <trx/game/items.h>
#include <trx/game/objects.h>
#include <trx/game/output.h>
#include <trx/game/random.h>
#include <trx/game/sound.h>
#include <trx/game/sparks.h>

// clang-format off
#define M_SPRAY_TIME  600
#define M_DRIP_TIME   1200
#define M_HEAD_HEIGHT 944
#define M_SPRAY_COUNT 3
// clang-format on

typedef struct {
    int32_t age;
} M_PRIV;

static RESULT M_LoadPriv(ITEM *const item, JSON_READ_IO *const io)
{
    M_PRIV *const p = item->priv;
    MUST(JSON_READ_OPT(io, "age", &p->age));
    return OK;
}

static void M_SavePriv(const ITEM *const item, JSON_WRITE_IO *const io)
{
    const M_PRIV *const p = item->priv;
    JSONW_WRITE(io, "age", p->age);
}

static void M_TriggerDrip(const ITEM *const item)
{
    const XYZ_32 pos = {
        .x = item->pos.x + (Random_GetControl() & 0x1F) - 16,
        .y = item->pos.y + (Random_GetControl() & 0x1F) - M_HEAD_HEIGHT,
        .z = item->pos.z + (Random_GetControl() & 0x1F) - 16,
    };
    const RGB_888 color = {
        .r = 112,
        .g = (Random_GetControl() & 0x1F) + 128,
        .b = (Random_GetControl() & 0x1F) + 128,
    };
    const int16_t y_vel = (Random_GetControl() & 0xF) + 16;
    const uint8_t gravity = (Random_GetControl() & 0x1F) + 32;
    const uint8_t life = (Random_GetControl() & 0x1F) + 16;
    FX_Droplet_Create(pos, item->room_num, color, y_vel, gravity, life);
}

static void M_TriggerSpray(const ITEM *const item)
{
    SPARK *const spark = Sparks_InitialiseSpark(SPARK_CONTEXT_SPLASH);
    if (spark == nullptr) {
        return;
    }

    spark->src_color.r = 112;
    spark->src_color.g = (Random_GetControl() & 0x1F) + 128;
    spark->src_color.b = (Random_GetControl() & 0x1F) + 128;
    spark->dst_color.r = spark->src_color.r >> 1;
    spark->dst_color.g = spark->src_color.g >> 1;
    spark->dst_color.b = spark->src_color.b >> 1;
    spark->col_fade_speed = 4;
    spark->fade_to_black = 8;
    spark->life = 20;
    spark->s_life = 20;
    spark->draw_type = DRAW_BLEND_ADD;
    spark->extras = 0;
    spark->dynamic = -1;

    const int32_t quadrant = (int32_t)Output_GetTimeInGame() & 3;
    const int32_t angle = ((quadrant << 10) + (Random_GetControl() & 0x3FF))
        << 4;
    spark->vel.z = Math_Cos(angle) >> 4;
    spark->vel.y = -32 - (Random_GetControl() & 0x1F);
    spark->vel.x = (-Math_Sin(angle)) >> 4;
    spark->vel.x = (spark->vel.x * (Random_GetControl() & 0xFF)) >> 8;
    spark->vel.z = (spark->vel.z * (Random_GetControl() & 0xFF)) >> 8;
    spark->pos.x = item->pos.x + (spark->vel.x >> 3);
    spark->pos.y = item->pos.y + (spark->vel.y >> 5) - 928;
    spark->pos.z = item->pos.z + (spark->vel.z >> 3);
    spark->friction = 4;
    spark->flags = SPARK_F_SCALE;
    spark->scalar = 3;
    spark->gravity = (Random_GetControl() & 0x3F) + 64;
    spark->max_y_vel = 0;
    spark->size.width = 2;
    spark->size.height = 2;
    spark->src_size = spark->size;
    spark->dst_size = spark->size;
    Sparks_FinishSetup(spark);
}

static void M_TriggerMist(const ITEM *const item)
{
    SPARK *const spark =
        Sparks_InitialiseSpriteSpark(SPARK_TYPE_EXPLOSION, SPARK_CONTEXT_SMOKE);
    if (spark == nullptr) {
        return;
    }

    const uint8_t shade = (Random_GetControl() & 0x1F) + 32;
    spark->src_color = (RGB_888) { 0, 0, 0 };
    spark->dst_color = (RGB_888) { shade, shade, shade };
    // TR4 makes this a smoke spark, which loses two points of life per tick
    // and draws at a scalar of 2. The life and the fades are therefore half
    // of the original values.
    spark->col_fade_speed = 2;
    spark->fade_to_black = (8 - (Random_GetControl() & 3)) >> 1;
    spark->life = ((Random_GetControl() & 3) + 24 + 1) >> 1;
    spark->s_life = spark->life;
    spark->draw_type = DRAW_BLEND_ADD;
    spark->extras = 0;
    spark->dynamic = -1;
    spark->pos.x = item->pos.x + (Random_GetControl() & 0x1F) - 16;
    spark->pos.y = item->pos.y + (Random_GetControl() & 0x1F) - M_HEAD_HEIGHT;
    spark->pos.z = item->pos.z + (Random_GetControl() & 0x1F) - 16;
    spark->vel.x = 2 * (Random_GetControl() & 0x1FF) - 512;
    spark->vel.y = 2 * (Random_GetControl() & 0x1FF) + 512;
    spark->vel.z = 2 * (Random_GetControl() & 0x1FF) - 512;
    spark->friction = 3;
    spark->flags = SPARK_F_ALT_SPRITE | SPARK_F_SMOKE | SPARK_F_ROTATE
        | SPARK_F_SPRITE | SPARK_F_SCALE;
    spark->rot_angle = Random_GetControl() & 0xFFF;
    if ((Random_GetControl() & 1) != 0) {
        spark->rot_add = -64 - (Random_GetControl() & 0x3F);
    } else {
        spark->rot_add = (Random_GetControl() & 0x3F) + 64;
    }
    spark->scalar = 2;
    spark->gravity = -4 - (Random_GetControl() & 3);
    spark->max_y_vel = 0;
    const int32_t size = ((Random_GetControl() & 0xF) + 16) << 2;
    spark->size.width = size >> 3;
    spark->size.height = size >> 3;
    spark->src_size = spark->size;
    spark->dst_size.width = size;
    spark->dst_size.height = size;
    Sparks_FinishSetup(spark);
}

static void M_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    M_PRIV *const p = item->priv;

    if (p->age < M_DRIP_TIME) {
        p->age++;
        const int32_t wibble = (int32_t)Output_GetTimeInGame() * 4;
        if ((wibble & 0xF) == 0
            && (p->age <= M_SPRAY_TIME
                || Random_GetControl() % (p->age - M_SPRAY_TIME) < 100)) {
            M_TriggerDrip(item);
        }
    }

    if (p->age <= M_SPRAY_TIME) {
        Sound_Effect(SFX_SPRINKLER, &item->pos, SPM_NORMAL);
        for (int32_t i = 0; i < M_SPRAY_COUNT; i++) {
            M_TriggerSpray(item);
        }
        M_TriggerMist(item);
    }
}

static void M_Setup(OBJECT *const obj)
{
    obj->control_func = M_Control;
    obj->collision_func = Gun_Torch_FireCollision;
    obj->priv_size = sizeof(M_PRIV);
    obj->priv_load_func = M_LoadPriv;
    obj->priv_save_func = M_SavePriv;
    obj->save_flags = true;
}

REGISTER_OBJECT(O_SPRINKLER, M_Setup)
