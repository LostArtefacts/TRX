#include <trx/core/json/util/read_io.h>
#include <trx/core/json/util/write_io.h>
#include <trx/game/lara.h>
#include <trx/game/objects.h>
#include <trx/game/output.h>
#include <trx/game/random.h>
#include <trx/game/sound.h>
#include <trx/game/sparks.h>

// clang-format off
#define M_MIN_FALLOFF 8
#define M_MAX_RANGE   (WALL_L * 2) // = 2048
#define M_RANGE_STEP  (STEP_L / 8) // = 32
#define M_SPEED_STEP  (STEP_L / 4) // = 64
// clang-format on

typedef enum {
    M_STATE_IDLE,
    M_STATE_REAR,
    M_STATE_BLOW,
} M_STATE;

typedef enum {
    M_ANIM_REAR = 1,
} M_ANIM;

typedef struct {
    struct {
        int32_t max;
        int32_t current;
    } blow_loops;
    int32_t speed;
    int32_t deadly_range;
    bool stop;
} M_PRIV;

static const BITE m_Mouth = {
    .pos = { .x = 0, .y = 128, .z = 0 },
    .mesh_num = 7,
};

static RESULT M_LoadPriv(ITEM *const item, JSON_READ_IO *const io)
{
    M_PRIV *const p = item->priv;
    if (SHOULD(JSON_PUSH(io, "blow_loops"))) {
        MUST(JSON_READ_OPT(io, "max", &p->blow_loops.max));
        MUST(JSON_READ_OPT(io, "current", &p->blow_loops.current));
        MUST(JSON_POP(io));
    }
    MUST(JSON_READ_OPT(io, "speed", &p->speed));
    MUST(JSON_READ_OPT(io, "deadly_range", &p->deadly_range));
    MUST(JSON_READ_OPT(io, "stop", &p->stop));
    return OK;
}

static void M_SavePriv(const ITEM *const item, JSON_WRITE_IO *const io)
{
    const M_PRIV *const p = item->priv;
    JSONW_PUSH_OBJECT(io);
    JSONW_WRITE(io, "max", p->blow_loops.max);
    JSONW_WRITE(io, "current", p->blow_loops.current);
    JSONW_POP_AND_SET(io, "blow_loops");

    JSONW_WRITE(io, "speed", p->speed);
    JSONW_WRITE(io, "deadly_range", p->deadly_range);
    JSONW_WRITE(io, "stop", p->stop);
}

static void M_TriggerFlame(
    const XYZ_32 pos, const int32_t angle, const int32_t speed)
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

    spark->src_color.r = (Random_GetControl() & 0x1F) + 48;
    spark->src_color.g = spark->src_color.r;
    spark->src_color.b = (Random_GetControl() & 0x3F) + 192;
    spark->dst_color.r = (Random_GetControl() & 0x3F) + 192;
    spark->dst_color.g = (Random_GetControl() & 0x3F) + 128;
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

    const int32_t dist = speed - (Random_GetControl() % ((speed >> 3) + 1));
    const XYZ_32 dir = XYZ_32_RotateYaw((XYZ_32) { .z = dist * 2 }, angle);
    spark->vel.x = dir.x + (Random_GetControl() & 0x7F) - 64;
    spark->vel.y = (Random_GetControl() & 7) + 6;
    spark->vel.z = dir.z + (Random_GetControl() & 0x7F) - 64;

    spark->friction = 4;
    spark->flags = SPARK_F_ALT_SPRITE | SPARK_F_SPRITE | SPARK_F_SCALE;

    if ((Random_GetControl() & 1) != 0) {
        spark->flags |= SPARK_F_ROTATE;
        spark->rot_angle = Random_GetControl() & 0xFFF;
        spark->rot_add = (Random_GetControl() & 0x3F) - 32;
    }

    spark->gravity = -8 - (Random_GetControl() & 0xF);
    spark->max_y_vel = -8 - (Random_GetControl() & 7);
    spark->scalar = 3;
    spark->dst_size.width = (speed >> 4) + (Random_GetControl() & 0xF);
    spark->size.width = spark->dst_size.width >> 2;
    spark->src_size.width = spark->size.width;
    spark->dst_size.height = spark->dst_size.width;
    spark->size.height = spark->size.width;
    spark->src_size.height = spark->size.height;
    Sparks_FinishSetup(spark);
}

static void M_Initialise(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    Item_SwitchToAnim(item, M_ANIM_REAR, 0);
    item->current_anim_state = M_STATE_REAR;
    item->goal_anim_state = M_STATE_REAR;
}

static bool M_Trigger(ITEM *const item, const ITEM_TRIGGER *const trigger)
{
    M_PRIV *const p = item->priv;
    if (p == nullptr) {
        return true;
    }

    if (trigger->kind == ITEM_TRIGGER_ANTI) {
        return true;
    }

    item->timer = 0;
    p->blow_loops.max = (int32_t)trigger->timer;
    return true;
}

static void M_Reset(M_PRIV *const p)
{
    p->blow_loops.current = p->blow_loops.max;
    p->speed = 0;
    p->deadly_range = 0;
    p->stop = false;
}

static bool M_TestFireRange(const ITEM *const item, const XYZ_32 pos)
{
    // Originally, item->pos.y was used as the baseline here, but this fails for
    // the alternate model used in the gold expansion. The following produces
    // the same baseline as the original base game.
    const XYZ_32 lara_pos = Lara_GetItem()->pos;
    const int32_t y_pos = ROUND_TO_CLICK_UP(pos.y);
    if (lara_pos.y <= y_pos - (WALL_L / 2)
        || lara_pos.y >= y_pos + (STEP_L * 3)) {
        return false;
    }

    const M_PRIV *const p = item->priv;
    const int32_t forward_range = p->deadly_range;
    const int32_t side_range = WALL_L / 2;
    XZ_32 min = { pos.x, pos.z };
    XZ_32 max = { pos.x, pos.z };

    const DIRECTION dir = Math_GetDirection(item->rot.y);
    switch (dir) {
    case DIR_NORTH:
        max.x += forward_range;
        min.z -= side_range;
        max.z += side_range;
        break;
    case DIR_EAST:
        min.x -= side_range;
        max.x += side_range;
        min.z -= forward_range;
        break;
    case DIR_SOUTH:
        min.x -= forward_range;
        min.z -= side_range;
        max.z += side_range;
        break;
    case DIR_WEST:
        min.x -= side_range;
        max.x += side_range;
        max.z += forward_range;
        break;
    default:
        break;
    }

    return lara_pos.x > min.x && lara_pos.x < max.x && lara_pos.z > min.z
        && lara_pos.z < max.z;
}

static void M_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    if (!Item_IsTriggerActive(item)) {
        return;
    }

    if (item->current_anim_state == M_STATE_IDLE) {
        item->goal_anim_state = M_STATE_REAR;
        Item_Animate(item);
        return;
    }

    M_PRIV *const p = item->priv;
    const RGB_888 light_color = {
        .r = (Random_GetControl() & 0x3F) + 192,
        .g = (Random_GetControl() & 0x1F) + 96,
        .b = 0,
    };

    if (item->current_anim_state == M_STATE_REAR) {
        XYZ_32 pos = m_Mouth.pos;
        Collide_GetJointAbsPosition(item, &pos, m_Mouth.mesh_num);
        Output_AddDynamicLightRGB(pos, M_MIN_FALLOFF, light_color);
        M_Reset(p);
    } else {
        XYZ_32 pos = {};
        Collide_GetJointAbsPosition(item, &pos, m_Mouth.mesh_num);

        if (p->stop) {
            if (p->speed != 0) {
                p->speed -= M_SPEED_STEP;
            }

            p->deadly_range -= M_RANGE_STEP;
            CLAMPL(p->deadly_range, 0);
        } else {
            if (p->speed < M_MAX_RANGE) {
                p->speed += M_SPEED_STEP;
            }

            if (p->deadly_range < M_MAX_RANGE) {
                p->deadly_range += M_RANGE_STEP;
            }

            Sound_Effect(SFX_FLAME_THROWER_LOOP, &item->pos, SPM_NORMAL);
        }

        p->blow_loops.current--;
        CLAMPL(p->blow_loops.current, 0);

        const int32_t time4 = Output_GetTimeInGame() * 4;
        if ((time4 & 4) != 0) {
            M_TriggerFlame(pos, item->rot.y + DEG_90, p->speed);
        }

        if (p->blow_loops.current == 0 && !p->stop
            && Item_TestFrameEqual(item, 0)) {
            p->stop = true;
            item->goal_anim_state = M_STATE_REAR;
        }

        Output_AddDynamicLightRGB(
            pos, (p->speed >> 7) + M_MIN_FALLOFF, light_color);

        const LARA_INFO *const lara = Lara_GetLaraInfo();
        if (!lara->burn && M_TestFireRange(item, pos)) {
            Lara_CatchFire();
        }
    }

    Item_Animate(item);
}

static void M_Setup(OBJECT *const obj)
{
    if (!obj->loaded) {
        return;
    }

    obj->collision_func = Object_Collision;
    obj->initialise_func = M_Initialise;
    obj->control_func = M_Control;
    obj->trigger_func = M_Trigger;

    obj->priv_size = sizeof(M_PRIV);
    obj->priv_load_func = M_LoadPriv;
    obj->priv_save_func = M_SavePriv;

    obj->save_flags = true;
    obj->save_anim = true;
}

REGISTER_OBJECT(O_FIRE_HEAD, M_Setup)
