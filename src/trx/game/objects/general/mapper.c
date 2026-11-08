#include <trx/game/objects.h>
#include <trx/game/output.h>
#include <trx/game/random.h>
#include <trx/game/sound.h>
#include <trx/game/sparks.h>

// clang-format off
#define M_LASER_MESH        1
#define M_LASER_START_FRAME 200
// clang-format on

static void M_Initialise(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    Item_SetMeshVisible(item, M_LASER_MESH, false);
}

static void M_TriggerLight(const XYZ_32 pos)
{
    const int32_t shade = (Random_GetControl() & 0x1F) + 192;
    const int32_t falloff = (Random_GetControl() & 3) + 16;
    Output_AddDynamicLightRGB(pos, falloff, (RGB_888) { shade, shade, 0 });
}

static void M_TriggerSparks(const XYZ_32 pos, const int32_t height)
{
    for (int32_t i = 0; i < 2; i++) {
        SPARK *const spark = Sparks_InitialiseSpriteSpark(
            SPARK_TYPE_EXPLOSION, SPARK_CONTEXT_SMOKE);
        if (spark == nullptr) {
            continue;
        }

        spark->src_color.r = (Random_GetControl() & 0x7F) + 64;
        spark->src_color.g = spark->src_color.r;
        spark->src_color.b = 0;
        spark->dst_color.r = (Random_GetControl() & 0x7F) + 64;
        spark->dst_color.g = spark->dst_color.r - (Random_GetControl() & 0x1F);
        spark->dst_color.b = 0;
        spark->col_fade_speed = 4;
        spark->fade_to_black = 4;
        spark->draw_type = DRAW_BLEND_ADD;
        spark->life = 12;
        spark->s_life = 12;
        spark->pos.x = pos.x;
        spark->pos.y = height;
        spark->pos.z = pos.z;
        spark->vel.x = (Random_GetControl() & 0x3FF) - WALL_L / 2;
        spark->vel.y = -(Random_GetControl() & 0x7F) - STEP_L;
        spark->vel.z = (Random_GetControl() & 0x3FF) - WALL_L / 2;
        spark->max_y_vel = 0;
        spark->friction = 4;
        spark->scalar = 2;
        spark->src_size.width = (Random_GetControl() & 0xF) + 16;
        spark->src_size.height = spark->src_size.width;
        spark->size = spark->src_size;
        spark->dst_size.width = (Random_GetControl() & 1) + 3;
        spark->dst_size.height = spark->dst_size.width;
        spark->gravity = (Random_GetControl() & 0x1F) + 32;
        spark->flags = SPARK_F_SPRITE | SPARK_F_SCALE;

        Sparks_FinishSetup(spark);
    }
}

static void M_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    if (!Item_IsTriggerActive(item)) {
        return;
    }

    const int16_t frame = Item_GetRelativeFrame(item);
    if (frame >= M_LASER_START_FRAME) {
        if (frame == M_LASER_START_FRAME) {
            Item_SetMeshVisible(item, M_LASER_MESH, true);
        }
        Sound_Effect(SFX_MAPPER_LASER, &item->pos, SPM_NORMAL);

        XYZ_32 pos = {};
        Collide_GetJointAbsPosition(item, &pos, M_LASER_MESH);
        const int32_t height = Item_GetHeight(item);

        M_TriggerLight(pos);
        M_TriggerSparks(pos, height);
    }

    Item_Animate(item);
}

static void M_Setup(OBJECT *const obj)
{
    if (!obj->loaded) {
        return;
    }

    obj->initialise_func = M_Initialise;
    obj->control_func = M_Control;

    obj->save_anim = true;
    obj->save_flags = true;
}

REGISTER_OBJECT(O_MAPPER, M_Setup)
