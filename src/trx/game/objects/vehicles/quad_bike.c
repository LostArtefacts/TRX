#include <trx/game/objects/vehicles/quad_bike.h>

#include <trx/config.h>
#include <trx/core/json/util/read_io.h>
#include <trx/core/json/util/write_io.h>
#include <trx/game/anims.h>
#include <trx/game/camera.h>
#include <trx/game/game.h>
#include <trx/game/game_buf.h>
#include <trx/game/gun/flare.h>
#include <trx/game/gym.h>
#include <trx/game/input.h>
#include <trx/game/items.h>
#include <trx/game/lara.h>
#include <trx/game/music.h>
#include <trx/game/objects.h>
#include <trx/game/objects/vehicles/common.h>
#include <trx/game/random.h>
#include <trx/game/rooms.h>
#include <trx/game/sound.h>
#include <trx/game/sparks.h>
#include <trx/game/spawn.h>
#include <trx/version.h>

#define M_STATIC_RADIUS 100

typedef enum {
    M_STATE_EMPTY,
    M_STATE_DRIVE,
    M_STATE_TURN_L,
    M_STATE_3,
    M_STATE_4,
    M_STATE_SLOW,
    M_STATE_BRAKE,
    M_STATE_BIKE_DEATH,
    M_STATE_FALL,
    M_STATE_GET_ON_R,
    M_STATE_GET_OFF_R,
    M_STATE_HIT_BACK,
    M_STATE_HIT_FRONT,
    M_STATE_HIT_LEFT,
    M_STATE_HIT_RIGHT,
    M_STATE_STOP,
    M_STATE_16,
    M_STATE_LAND,
    M_STATE_STOP_SLOWLY,
    M_STATE_FALL_DEATH,
    M_STATE_FALL_OFF,
    M_STATE_WHEELIE,
    M_STATE_TURN_R,
    M_STATE_GET_ON_L,
    M_STATE_GET_OFF_L,
} M_STATE;

typedef enum {
    // clang-format off
    M_ANIM_TURN_LEFT   = 3,
    M_ANIM_FALL_FRONT  = 6,
    M_ANIM_MOUNT_RIGHT = 9,
    M_ANIM_HIT_BACK    = 11,
    M_ANIM_HIT_FRONT   = 12,
    M_ANIM_HIT_RIGHT   = 13,
    M_ANIM_HIT_LEFT    = 14,
    M_ANIM_TURN_RIGHT  = 20,
    M_ANIM_MOUNT_LEFT  = 23,
    M_ANIM_FALL_BACK   = 25,
    // clang-format on
} M_ANIM;

typedef struct {
    int32_t velocity;
    int16_t front_rot;
    int16_t rear_rot;
    int32_t revs;
    int32_t engine_revs;
    int16_t track_mesh;
    int32_t skidoo_turn;
    int32_t left_fall_speed;
    int32_t right_fall_speed;
    int16_t momentum_angle;
    int16_t extra_rotation;
    int32_t pitch;
    uint8_t flags;
} M_QUAD_BIKE_INFO;

typedef struct {
    M_QUAD_BIKE_INFO quad;
    int16_t *extra_rotation;
    int32_t extra_rotation_count;
    int32_t rear_rot_x_idx[2];
    int32_t front_rot_x_idx[2];
    bool test_static_collision;
} M_PRIV;

static const BITE m_QuadBites[6] = {
    { .pos = { .x = -56, .y = -32, .z = -380 }, .mesh_num = 0 },
    { .pos = { .x = 56, .y = -32, .z = -380 }, .mesh_num = 0 },
    { .pos = { .x = -8, .y = 180, .z = -48 }, .mesh_num = 3 },
    { .pos = { .x = 8, .y = 180, .z = -48 }, .mesh_num = 4 },
    { .pos = { .x = 90, .y = 180, .z = -32 }, .mesh_num = 6 },
    { .pos = { .x = -90, .y = 180, .z = -32 }, .mesh_num = 7 },
};

static bool m_DontExitQuad;
static bool m_HandbrakeStarting;
static bool m_CanHandbrakeStart;
static uint8_t m_ExhaustSmokeVel;

static RESULT M_LoadPriv(ITEM *const item, JSON_READ_IO *const io)
{
    M_PRIV *const p = item->priv;
    MUST(JSON_READ_OPT(io, "velocity", &p->quad.velocity));
    MUST(JSON_READ_OPT(io, "front_rot", &p->quad.front_rot));
    MUST(JSON_READ_OPT(io, "rear_rot", &p->quad.rear_rot));
    MUST(JSON_READ_OPT(io, "revs", &p->quad.revs));
    MUST(JSON_READ_OPT(io, "engine_revs", &p->quad.engine_revs));
    MUST(JSON_READ_OPT(io, "track_mesh", &p->quad.track_mesh));
    MUST(JSON_READ_OPT(io, "skidoo_turn", &p->quad.skidoo_turn));
    MUST(JSON_READ_OPT(io, "left_fall_speed", &p->quad.left_fall_speed));
    MUST(JSON_READ_OPT(io, "right_fall_speed", &p->quad.right_fall_speed));
    MUST(JSON_READ_OPT(io, "momentum_angle", &p->quad.momentum_angle));
    MUST(JSON_READ_OPT(io, "extra_rotation", &p->quad.extra_rotation));
    MUST(JSON_READ_OPT(io, "pitch", &p->quad.pitch));
    MUST(JSON_READ_OPT(io, "flags", &p->quad.flags));
    return OK;
}

static void M_SavePriv(const ITEM *const item, JSON_WRITE_IO *const io)
{
    const M_PRIV *const p = item->priv;
    JSONW_WRITE(io, "velocity", p->quad.velocity);
    JSONW_WRITE(io, "front_rot", p->quad.front_rot);
    JSONW_WRITE(io, "rear_rot", p->quad.rear_rot);
    JSONW_WRITE(io, "revs", p->quad.revs);
    JSONW_WRITE(io, "engine_revs", p->quad.engine_revs);
    JSONW_WRITE(io, "track_mesh", p->quad.track_mesh);
    JSONW_WRITE(io, "skidoo_turn", p->quad.skidoo_turn);
    JSONW_WRITE(io, "left_fall_speed", p->quad.left_fall_speed);
    JSONW_WRITE(io, "right_fall_speed", p->quad.right_fall_speed);
    JSONW_WRITE(io, "momentum_angle", p->quad.momentum_angle);
    JSONW_WRITE(io, "extra_rotation", p->quad.extra_rotation);
    JSONW_WRITE(io, "pitch", p->quad.pitch);
    JSONW_WRITE(io, "flags", p->quad.flags);
}

static int32_t M_CountExtraRotationValues(const XYZ_BOOL flags)
{
    return (flags.y ? 1 : 0) + (flags.x ? 1 : 0) + (flags.z ? 1 : 0);
}

static void M_EnableWheelExtraRotations(OBJECT *const obj)
{
    // TR3 rotates wheels around X at meshes 3/4 (rear) and 6/7 (front).
    if (obj->mesh_count > 3) {
        Object_GetBone(obj, 2)->rot.x = true;
    }
    if (obj->mesh_count > 4) {
        Object_GetBone(obj, 3)->rot.x = true;
    }
    if (obj->mesh_count > 6) {
        Object_GetBone(obj, 5)->rot.x = true;
    }
    if (obj->mesh_count > 7) {
        Object_GetBone(obj, 6)->rot.x = true;
    }
}

static void M_CalcExtraRotationLayout(const OBJECT *const obj, M_PRIV *const p)
{
    p->extra_rotation_count = 0;
    p->rear_rot_x_idx[0] = -1;
    p->rear_rot_x_idx[1] = -1;
    p->front_rot_x_idx[0] = -1;
    p->front_rot_x_idx[1] = -1;

    if (obj == nullptr || !obj->loaded) {
        return;
    }

    int32_t cursor = 0;
    cursor += M_CountExtraRotationValues(obj->base_rot);

    for (int32_t mesh_idx = 1; mesh_idx < obj->mesh_count; mesh_idx++) {
        const ANIM_BONE *const bone = Object_GetBone(obj, mesh_idx - 1);
        const XYZ_BOOL flags = bone->rot;

        if (flags.x) {
            const int32_t x_idx = cursor + (flags.y ? 1 : 0);
            switch (mesh_idx) {
            case 3:
                p->rear_rot_x_idx[0] = x_idx;
                break;
            case 4:
                p->rear_rot_x_idx[1] = x_idx;
                break;
            case 6:
                p->front_rot_x_idx[0] = x_idx;
                break;
            case 7:
                p->front_rot_x_idx[1] = x_idx;
                break;
            default:
                break;
            }
        }

        cursor += M_CountExtraRotationValues(flags);
    }

    p->extra_rotation_count = cursor;
}

static void M_Initialise(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    M_PRIV *const p = item->priv;

    p->quad.momentum_angle = item->rot.y;

    const OBJECT *const obj = Object_Get(item->object_id);
    M_CalcExtraRotationLayout(obj, p);
    if (p->extra_rotation_count > 0) {
        p->extra_rotation = GameBuf_Alloc(
            sizeof(int16_t) * p->extra_rotation_count, GBUF_ITEM_DATA);
    } else {
        p->extra_rotation = nullptr;
    }

    item->extra_rotations = p->extra_rotation;
}

static int32_t M_GetOnQuadBike(
    const int16_t item_num, const COLL_INFO *const coll)
{
    ITEM *const item = Item_Get(item_num);
    ITEM *const lara_item = Lara_GetItem();
    LARA_INFO *const lara = Lara_GetLaraInfo();

    if (!g_Input.action || item->trigger.spent
        || lara->gun_status != LGS_ARMLESS || lara_item->gravity) {
        return 0;
    }

    const int32_t dx = lara_item->pos.x - item->pos.x;
    const int32_t dy = ABS(item->pos.y - lara_item->pos.y);
    const int32_t dz = lara_item->pos.z - item->pos.z;
    const int32_t dist = SQUARE(dx) + SQUARE(dz);

    if (dy > 256 || dist > 170000) {
        return 0;
    }

    int16_t room_num = item->room_num;
    SECTOR *const sector = Room_GetSector(item->pos, &room_num);

    const int32_t h = Room_GetHeight(sector, item->pos);
    if (h < -MAX_HEIGHT) {
        return 0;
    }

    const int16_t ang =
        Math_Atan(
            item->pos.z - lara_item->pos.z, item->pos.x - lara_item->pos.x)
        - item->rot.y;
    uint16_t uang = lara_item->rot.y - item->rot.y;

    if (ang > -0x1FFE && ang < 0x5FFA) {
        if (uang <= 0x1FFE || uang >= 0x5FFA) {
            return 0;
        }
    } else {
        if (uang <= 0x9FF6 || uang >= 0xDFF2) {
            return 0;
        }
    }

    return 1;
}

static void M_Collision(
    const int16_t item_num, ITEM *const lara_item, COLL_INFO *const coll)
{
    LARA_INFO *const lara = Lara_GetLaraInfo();
    if (lara_item->hit_points < 0 || Lara_Vehicle_GetItem() != nullptr) {
        return;
    }

    if (!M_GetOnQuadBike(item_num, coll)) {
        Object_Collision(item_num, lara_item, coll);
        return;
    }

    Lara_Vehicle_SetIndex(item_num);

    if (lara->gun_type == LGT_FLARE) {
        Gun_Flare_Dispose(false);
        lara->gun_type = LGT_UNARMED;
        lara->request_gun_type = LGT_UNARMED;
    }

    lara->gun_status = LGS_HANDS_BUSY;
    ITEM *const item = Item_Get(item_num);
    M_PRIV *const p = item->priv;
    M_QUAD_BIKE_INFO *const quad = &p->quad;

    const int16_t angle =
        (int16_t)Math_Atan(
            item->pos.z - lara_item->pos.z, item->pos.x - lara_item->pos.x)
        - item->rot.y;

    if (angle > -0x1FFE && angle < 0x5FFA) {
        Lara_Vehicle_SwitchToAnim(M_ANIM_MOUNT_LEFT, 0);
        lara_item->current_anim_state = M_STATE_GET_ON_L;
        lara_item->goal_anim_state = M_STATE_GET_ON_L;
    } else {
        Lara_Vehicle_SwitchToAnim(M_ANIM_MOUNT_RIGHT, 0);
        lara_item->current_anim_state = M_STATE_GET_ON_R;
        lara_item->goal_anim_state = M_STATE_GET_ON_R;
    }

    lara_item->pos.x = item->pos.x;
    lara_item->pos.y = item->pos.y;
    lara_item->pos.z = item->pos.z;
    lara_item->rot.y = item->rot.y;
    lara->head_rot.y = 0;
    lara->head_rot.x = 0;
    lara->torso_rot.y = 0;
    lara->torso_rot.x = 0;
    lara->hit_direction = DIR_UNKNOWN;
    Item_Animate(lara_item);

    {
        const bool is_ambient =
            Music_GetCurrentPlayingTrack() == Music_GetCurrentLoopedTrack();
        if (is_ambient) {
            Vehicle_PlayTrackPool(item, "track", MPM_ONCE);
        }
    }

    quad->revs = 0;
}

static void M_Explode(ITEM *const item)
{
    if (Room_Get(item->room_num)->flags.underwater) {
        Sparks_TriggerUnderwaterExplosion(item);
    } else {
        Sparks_TriggerExplosionSparks(item->pos, 3, -2, 0, item->room_num);

        for (int32_t i = 0; i < 3; i++) {
            Sparks_TriggerExplosionSparks(item->pos, 3, -1, 0, item->room_num);
        }
    }

    const int16_t vehicle_item_num = Lara_Vehicle_GetIndex();
    Item_Shatter(vehicle_item_num, -2, 0);
    Item_Destroy(vehicle_item_num);
    Item_SetFinished(item, true);
    Sound_Effect(SFX_EXPLOSION_1, nullptr, SPM_NORMAL);
    Sound_Effect(SFX_EXPLOSION_2, nullptr, SPM_NORMAL);
    Lara_Vehicle_SetIndex(NO_ITEM);
}

static bool M_CheckGetOff(void)
{
    ITEM *const item = Lara_Vehicle_GetItem();
    ITEM *const lara_item = Lara_GetItem();
    LARA_INFO *const lara = Lara_GetLaraInfo();

    if ((lara_item->current_anim_state == M_STATE_GET_OFF_R
         || lara_item->current_anim_state == M_STATE_GET_OFF_L)
        && lara_item->frame_num == Item_GetAnim(lara_item)->frame_end) {
        if (lara_item->current_anim_state == M_STATE_GET_OFF_L) {
            lara_item->rot.y += DEG_90;
        } else {
            lara_item->rot.y -= DEG_90;
        }

        Item_SwitchToAnim(lara_item, LA(LA_STAND_STILL), 0);
        lara_item->current_anim_state = LS_STOP;
        lara_item->goal_anim_state = LS_STOP;
        lara_item->pos = XYZ_32_Subtract(
            lara_item->pos,
            XYZ_32_RotateYaw((XYZ_32) { .z = 512 }, lara_item->rot.y));
        lara_item->rot.x = 0;
        lara_item->rot.z = 0;
        Lara_Vehicle_SetIndex(NO_ITEM);
        lara->gun_status = LGS_ARMLESS;
    } else if (lara_item->frame_num == Item_GetAnim(lara_item)->frame_end) {
        M_PRIV *const p = item->priv;
        M_QUAD_BIKE_INFO *const quad = &p->quad;

        if (lara_item->current_anim_state == M_STATE_FALL_OFF) {
            Item_SwitchToAnim(lara_item, LA(LA_FREEFALL), 0);
            lara_item->current_anim_state = LS_FAST_FALL;

            XYZ_32 pos = {};
            Lara_GetMeshPos(LM_HIPS, &pos);

            lara_item->pos.x = pos.x;
            lara_item->pos.y = pos.y;
            lara_item->pos.z = pos.z;
            lara_item->gravity = true;
            lara_item->fall_speed = item->fall_speed;
            lara_item->rot.x = 0;
            lara_item->rot.z = 0;
            lara->gun_status = LGS_ARMLESS;
            Lara_Kill();
            item->trigger.spent = true;
            return false;
        }

        if (lara_item->current_anim_state == M_STATE_FALL_DEATH) {
            lara_item->goal_anim_state = M_STATE_FALL;
            lara_item->fall_speed = 154;
            lara_item->speed = 0;
            quad->flags |= 0x80;
            return false;
        }
    }

    return true;
}

static int32_t M_TestHeight(
    const ITEM *const item, const int32_t z_off, const int32_t x_off,
    XYZ_32 *const pos)
{
    *pos = XYZ_32_OffsetLocalYaw(
        item->pos, (XYZ_32) { .x = x_off, .z = z_off }, item->rot.y);

    // The height a wheel sits at follows the quad's pitch and roll, which the
    // yaw turn above says nothing about.
    pos->y = item->pos.y + ((x_off * Math_Sin(item->rot.z)) >> W2V_SHIFT)
        - ((z_off * Math_Sin(item->rot.x)) >> W2V_SHIFT);

    int16_t room_num = item->room_num;
    SECTOR *const sector = Room_GetSector(*pos, &room_num);
    const int32_t ceiling = Room_GetCeiling(sector, *pos);

    if (pos->y < ceiling || ceiling == NO_HEIGHT) {
        return NO_HEIGHT;
    }

    const M_PRIV *const p = item->priv;
    const int32_t height = Room_GetHeight(sector, *pos);
    if (height == NO_HEIGHT || !p->test_static_collision) {
        return height;
    }

    COLL_INFO coll = {};
    coll.radius = M_STATIC_RADIUS;
    if (Collide_CollideStaticObjects(
            &coll, *pos, item->room_num, LARA_HEIGHT)) {
        return NO_HEIGHT;
    }
    return height;
}

static void M_TriggerExhaustSmoke(
    XYZ_32 pos, int16_t angle, int32_t speed, const bool moving)
{
    SPARK *const spark = Sparks_InitialiseSpriteSpark(SPARK_TYPE_EXPLOSION);
    if (spark == nullptr) {
        return;
    }

    spark->src_color.r = 0;
    spark->src_color.g = 0;
    spark->src_color.b = 0;

    if (moving) {
#if 0
        // OG
        spark->dst_color.r = MINMAX((96 * speed) >> 5, 0, 255);
        spark->dst_color.g = MINMAX((96 * speed) >> 5, 0, 255);
        spark->dst_color.b = MINMAX((128 * speed) >> 5, 0, 255);
#else
        spark->dst_color.r = 96 >> 1;
        spark->dst_color.g = 96 >> 1;
        spark->dst_color.b = 128 >> 1;
#endif
    } else {
        spark->dst_color.r = 96;
        spark->dst_color.g = 96;
        spark->dst_color.b = 128;
    }

    spark->col_fade_speed = 4;
    spark->fade_to_black = 4;
    spark->life = (Random_GetControl() & 3) - (speed >> 12) + 20;
    CLAMPL(spark->life, 9);
    spark->s_life = spark->life;

    spark->draw_type = DRAW_BLEND_ADD;
    spark->extras = 0;
    spark->dynamic = -1;
    spark->pos = (XYZ_32) {
        .x = pos.x + (Random_GetControl() & 0xF) - 8,
        .y = pos.y + (Random_GetControl() & 0xF) - 8,
        .z = pos.z + (Random_GetControl() & 0xF) - 8,
    };
    const XYZ_32 dir = XYZ_32_RotateYaw((XYZ_32) { .z = speed }, angle);
    spark->vel = (XYZ_32) {
        .x = (Random_GetControl() & 0xFF) + (dir.x >> 2) - 128,
        .y = -8 - (Random_GetControl() & 7),
        .z = (Random_GetControl() & 0xFF) + (dir.z >> 2) - 128,
    };
    spark->friction = 4;

    if (Random_GetControl() & 1) {
        spark->flags = SPARK_F_ALT_SPRITE | SPARK_F_ROTATE | SPARK_F_SPRITE
            | SPARK_F_SCALE;
        spark->rot_angle = Random_GetControl() & 0xFFF;

        if (Random_GetControl() & 1) {
            spark->rot_add = -24 - (Random_GetControl() & 7);
        } else {
            spark->rot_add = (Random_GetControl() & 7) + 24;
        }
    } else {
        spark->flags = SPARK_F_ALT_SPRITE | SPARK_F_SPRITE | SPARK_F_SCALE;
    }

    spark->scalar = 2;
    spark->gravity = -4 - (Random_GetControl() & 3);
    spark->max_y_vel = -8 - (Random_GetControl() & 7);
    spark->dst_size.width = (Random_GetControl() & 7) + (speed >> 7) + 32;
    spark->src_size.width = spark->dst_size.width >> 1;
    spark->size.width = spark->dst_size.width >> 1;
    spark->dst_size.height = spark->dst_size.width;
    spark->src_size.height = spark->dst_size.height >> 1;
    spark->size.height = spark->dst_size.height >> 1;
    Sparks_FinishSetup(spark);
}

static bool M_SkidooCanGetOff(const int32_t lr)
{
    ITEM *const item = Lara_Vehicle_GetItem();

    const int16_t angle = item->rot.y + DEG_90 * lr;
    const XYZ_32 pos = XYZ_32_OffsetYaw(item->pos, angle, 512);

    int16_t room_num = item->room_num;
    SECTOR *const sector = Room_GetSector(pos, &room_num);
    const int32_t h = Room_GetHeight(sector, pos);
    const int32_t c = Room_GetCeiling(sector, pos);

    const HEIGHT_TYPE height_type = Room_GetHeightType();
    if (height_type != HT_BIG_SLOPE && height_type != HT_DIAGONAL
        && h != NO_HEIGHT && ABS(h - item->pos.y) <= 512
        && c - item->pos.y <= -LARA_HEIGHT && h - c >= LARA_HEIGHT) {
        return true;
    }

    return false;
}

static int32_t M_GetCollisionAnim(ITEM *const item, XYZ_32 *const pos)
{
    pos->x = item->pos.x - pos->x;
    pos->z = item->pos.z - pos->z;

    if (pos->x == 0 && pos->z == 0) {
        return 0;
    }

    const XYZ_32 local = XYZ_32_UnrotateYaw(*pos, item->rot.y);
    const int32_t fb = local.z;
    const int32_t lr = local.x;

    if (ABS(fb) > ABS(lr)) {
        return fb > 0 ? 14 : 13;
    } else {
        return lr > 0 ? 11 : 12;
    }
}

static int32_t M_DoDynamics(
    const int32_t height, int32_t fall_speed, int32_t *const y_pos)
{
    if (height <= *y_pos) {
        int32_t bounce = (height - *y_pos) << 2;
        CLAMPL(bounce, -80);
        fall_speed += (bounce - fall_speed) >> 3;
        CLAMPG(*y_pos, height);
    } else {
        *y_pos += fall_speed;

        if (*y_pos <= height - 80) {
            fall_speed += 6;
        } else {
            *y_pos = height;
            fall_speed = 0;
        }
    }

    return fall_speed;
}

static int32_t M_DoShift(
    ITEM *const item, const XYZ_32 *const new_pos, const XYZ_32 *const old_pos)
{
    const int32_t new_x = new_pos->x >> WALL_SHIFT;
    const int32_t new_z = new_pos->z >> WALL_SHIFT;
    const int32_t old_x = old_pos->x >> WALL_SHIFT;
    const int32_t old_z = old_pos->z >> WALL_SHIFT;
    const int32_t shift_x = new_pos->x & (WALL_L - 1);
    const int32_t shift_z = new_pos->z & (WALL_L - 1);

    if (new_x == old_x) {
        if (new_z == old_z) {
            item->pos.z += (old_pos->z - new_pos->z);
            item->pos.x += (old_pos->x - new_pos->x);
            return 0;
        } else if (new_z <= old_z) {
            item->pos.z += WALL_L - shift_z;
            return item->pos.x - new_pos->x;
        } else {
            item->pos.z -= 1 + shift_z;
            return new_pos->x - item->pos.x;
        }
    }

    if (new_z == old_z) {
        if (new_x <= old_x) {
            item->pos.x += WALL_L - shift_x;
            return new_pos->z - item->pos.z;
        } else {
            item->pos.x -= 1 + shift_x;
            return item->pos.z - new_pos->z;
        }
    }

    int32_t x = 0;
    int32_t z = 0;
    XYZ_32 test_pos = { old_pos->x, new_pos->y, new_pos->z };
    int16_t room_num = item->room_num;
    SECTOR *sector = Room_GetSector(test_pos, &room_num);
    const int32_t h = Room_GetHeight(sector, test_pos);

    if (h < old_pos->y - 256) {
        if (new_pos->z > old_pos->z) {
            z = -1 - shift_z;
        } else {
            z = WALL_L - shift_z;
        }
    }

    test_pos = (XYZ_32) { new_pos->x, new_pos->y, old_pos->z };
    room_num = item->room_num;
    sector = Room_GetSector(test_pos, &room_num);
    const int32_t h2 = Room_GetHeight(sector, test_pos);

    if (h2 < old_pos->y - 256) {
        if (new_pos->x > old_pos->x) {
            x = -1 - shift_x;
        } else {
            x = WALL_L - shift_x;
        }
    }

    if (x != 0 && z != 0) {
        item->pos.x += x;
        item->pos.z += z;
        return 0;
    }

    if (z != 0) {
        item->pos.z += z;

        if (z > 0) {
            return item->pos.x - new_pos->x;
        } else {
            return new_pos->x - item->pos.x;
        }
    }

    if (x != 0) {
        item->pos.x += x;

        if (x > 0) {
            return new_pos->z - item->pos.z;
        } else {
            return item->pos.z - new_pos->z;
        }
    }

    item->pos.x += old_pos->x - new_pos->x;
    item->pos.z += old_pos->z - new_pos->z;
    return 0;
}

static void M_SkidooBaddieCollision(ITEM *const quad)
{
    ITEM *const lara_item = Lara_GetItem();

    int16_t nearby_rooms[16] = { quad->room_num };
    int16_t nearby_room_count = 1;

    const PORTALS *const portals = Room_Get(quad->room_num)->portals;
    if (portals != nullptr) {
        for (int32_t i = 0; i < portals->count && i < 16; i++) {
            nearby_rooms[nearby_room_count] = portals->portal[i].room_num;
            nearby_room_count++;
        }
    }

    for (int32_t i = 0; i < nearby_room_count; i++) {
        int16_t item_num = Room_Get(nearby_rooms[i])->item_num;
        while (item_num != NO_ITEM) {
            ITEM *const item = Item_Get(item_num);

            if (!item->is_collidable || !item->is_visible || item == lara_item
                || item == quad) {
                goto loop_end;
            }

            const OBJECT *const obj = Object_Get(item->object_id);
            if (obj->collision_func == nullptr
                || (!obj->intelligent && item->object_id != O_ROLLING_BALL_2)) {
                goto loop_end;
            }

            int32_t dx = quad->pos.x - item->pos.x;
            int32_t dy = quad->pos.y - item->pos.y;
            int32_t dz = quad->pos.z - item->pos.z;

            if (dx <= -2048 || dx >= 2048 || dz <= -2048 || dz >= 2048
                || dy <= -2048 || dy >= 2048
                || !Item_TestBoundsCollide(item, quad, 500)) {
                goto loop_end;
            }

            if (item->object_id == O_ROLLING_BALL_2) {
                if (item->current_anim_state == 1) {
                    Lara_TakeDamage(100, true);
                }
            } else {
                if (Item_ShouldSpawnBlood(item)) {
                    Spawn_BloodBath(
                        item->pos.x, quad->pos.y - 256, item->pos.z,
                        quad->speed, quad->rot.y, item->room_num, 3);
                }
                if (item->hit_points > 0) {
                    Item_TakeFatalDamage(item, quad);
                }
            }

        loop_end:
            item_num = item->next_item;
        }
    }
}

static int32_t M_SkidooDynamics(ITEM *const item)
{
    m_DontExitQuad = false;
    M_PRIV *const p = item->priv;
    M_QUAD_BIKE_INFO *const quad = &p->quad;

    XYZ_32 old_pos;
    old_pos.x = item->pos.x;
    old_pos.y = item->pos.y;
    old_pos.z = item->pos.z;

    XYZ_32 new_pos = {};

    XYZ_32 front_left_pos = {};
    XYZ_32 front_right_pos = {};
    XYZ_32 back_left_pos = {};
    XYZ_32 back_right_pos = {};
    XYZ_32 mid_left_pos = {};
    XYZ_32 mid_right_pos = {};
    XYZ_32 bm_left_pos = {};
    XYZ_32 bm_right_pos = {};
    XYZ_32 fm_left_pos = {};
    XYZ_32 fm_right_pos = {};

    // clang-format off
    const int32_t front_left_height  = M_TestHeight(item, 550,  -260, &front_left_pos);
    const int32_t front_right_height = M_TestHeight(item, 550,  260,  &front_right_pos);
    const int32_t back_left_height   = M_TestHeight(item, -550, -260, &back_left_pos);
    const int32_t back_right_height  = M_TestHeight(item, -550, 260,  &back_right_pos);
    const int32_t mid_left_height    = M_TestHeight(item, 0,    -260, &mid_left_pos);
    const int32_t mid_right_height   = M_TestHeight(item, 0,    260,  &mid_right_pos);
    const int32_t bm_left_height     = M_TestHeight(item, 275,  -260, &bm_left_pos);
    const int32_t bm_right_height    = M_TestHeight(item, 275,  260,  &bm_right_pos);
    const int32_t fm_left_height     = M_TestHeight(item, -275, -260, &fm_left_pos);
    const int32_t fm_right_height    = M_TestHeight(item, -275, 260,  &fm_right_pos);
    // clang-format on

    CLAMPG(back_left_pos.y, back_left_height);
    CLAMPG(back_right_pos.y, back_right_height);
    CLAMPG(front_left_pos.y, front_left_height);
    CLAMPG(front_right_pos.y, front_right_height);
    CLAMPG(fm_left_pos.y, fm_left_height);
    CLAMPG(fm_right_pos.y, fm_right_height);
    CLAMPG(bm_left_pos.y, bm_left_height);
    CLAMPG(bm_right_pos.y, bm_right_height);
    CLAMPG(mid_left_pos.y, mid_left_height);
    CLAMPG(mid_right_pos.y, mid_right_height);

    if (item->pos.y <= item->floor - 256) {
        item->rot.y += quad->extra_rotation + quad->skidoo_turn;
    } else {
        if (quad->skidoo_turn < -364) {
            quad->skidoo_turn += 364;
        } else if (quad->skidoo_turn > 364) {
            quad->skidoo_turn -= 364;
        } else {
            quad->skidoo_turn = 0;
        }

        item->rot.y += quad->extra_rotation + quad->skidoo_turn;

        int16_t vel = 546 - (quad->velocity >> 8);
        const int16_t ang = item->rot.y - quad->momentum_angle;
        if (!g_Input.action && quad->velocity > 0) {
            vel += vel >> 2;
        }

        if (ang < -273) {
            if (ang >= -27300) {
                quad->momentum_angle -= vel;
            } else {
                quad->momentum_angle = item->rot.y + 27300;
            }
        } else if (ang > 273) {
            if (ang <= 27300) {
                quad->momentum_angle += vel;
            } else {
                quad->momentum_angle = item->rot.y - 27300;
            }
        } else {
            quad->momentum_angle = item->rot.y;
        }
    }

    int16_t room_num = item->room_num;
    SECTOR *const sector = Room_GetSector(item->pos, &room_num);
    const int32_t height = Room_GetHeight(sector, item->pos);

    int32_t speed = item->pos.y < height
        ? item->speed
        : (item->speed * Math_Cos(item->rot.x)) >> W2V_SHIFT;

    item->pos = XYZ_32_OffsetYaw(item->pos, quad->momentum_angle, speed);

    int32_t slip = (100 * Math_Sin(item->rot.x)) >> W2V_SHIFT;
    if (ABS(slip) > 50) {
        m_DontExitQuad = true;

        if (slip > 0) {
            slip -= 10;
        } else {
            slip += 10;
        }

        item->pos = XYZ_32_Subtract(
            item->pos, XYZ_32_RotateYaw((XYZ_32) { .z = slip }, item->rot.y));
    }

    slip = (50 * Math_Sin(item->rot.z)) >> W2V_SHIFT;

    if (ABS(slip) > 25) {
        m_DontExitQuad = true;
        // Sideways, along the quad's own x, read off a forward turn so that
        // each component rounds as it did.
        const XYZ_32 roll_slip =
            XYZ_32_RotateYaw((XYZ_32) { .z = slip }, item->rot.y);
        item->pos.x += roll_slip.z;
        item->pos.z -= roll_slip.x;
    }

    new_pos.x = item->pos.x;
    new_pos.z = item->pos.z;

    if (!item->trigger.spent) {
        M_SkidooBaddieCollision(item);
    }

    int16_t shift = 0;
    int16_t shift2 = 0;

    XYZ_32 front_left_pos2 = {};
    XYZ_32 bm_left_pos2 = {};
    XYZ_32 mid_left_pos2 = {};
    XYZ_32 fm_left_pos2 = {};
    XYZ_32 back_left_pos2 = {};
    XYZ_32 front_right_pos2 = {};
    XYZ_32 bm_right_pos2 = {};
    XYZ_32 mid_right_pos2 = {};
    XYZ_32 fm_right_pos2 = {};
    XYZ_32 back_right_pos2 = {};

    const int32_t front_left_height2 =
        M_TestHeight(item, 550, -260, &front_left_pos2);
    if (front_left_height2 < front_left_pos.y - 256) {
        shift = (int16_t)M_DoShift(item, &front_left_pos2, &front_left_pos);
    }

    const int32_t bm_left_height2 =
        M_TestHeight(item, 275, -260, &bm_left_pos2);
    if (bm_left_height2 < bm_left_pos.y - 256) {
        M_DoShift(item, &bm_left_pos2, &bm_left_pos);
    }

    const int32_t mid_left_height2 =
        M_TestHeight(item, 0, -260, &mid_left_pos2);
    if (mid_left_height2 < mid_left_pos.y - 256) {
        M_DoShift(item, &mid_left_pos2, &mid_left_pos);
    }

    const int32_t fm_left_height2 =
        M_TestHeight(item, -275, -260, &fm_left_pos2);
    if (fm_left_height2 < fm_left_pos.y - 256) {
        M_DoShift(item, &fm_left_pos2, &fm_left_pos);
    }

    const int32_t back_left_height2 =
        M_TestHeight(item, -550, -260, &back_left_pos2);
    if (back_left_height2 < back_left_pos.y - 256) {
        shift2 = M_DoShift(item, &back_left_pos2, &back_left_pos);
        if ((shift2 > 0 && shift >= 0) || (shift2 < 0 && shift <= 0)) {
            shift += shift2;
        }
    }

    const int32_t front_right_height2 =
        M_TestHeight(item, 550, 260, &front_right_pos2);
    if (front_right_height2 < front_right_pos.y - 256) {
        shift2 = M_DoShift(item, &front_right_pos2, &front_right_pos);
        if ((shift2 > 0 && shift >= 0) || (shift2 < 0 && shift <= 0)) {
            shift += shift2;
        }
    }

    const int32_t bm_right_height2 =
        M_TestHeight(item, 275, 260, &bm_right_pos2);
    if (bm_right_height2 < bm_right_pos.y - 256) {
        M_DoShift(item, &bm_right_pos2, &bm_right_pos);
    }

    const int32_t mid_right_height2 =
        M_TestHeight(item, 0, 260, &mid_right_pos2);
    if (mid_right_height2 < mid_right_pos.y - 256) {
        M_DoShift(item, &mid_right_pos2, &mid_right_pos);
    }

    const int32_t fm_right_height2 =
        M_TestHeight(item, -275, 260, &fm_right_pos2);
    if (fm_right_height2 < fm_right_pos.y - 256) {
        M_DoShift(item, &fm_right_pos2, &fm_right_pos);
    }

    const int32_t back_right_height2 =
        M_TestHeight(item, -550, 260, &back_right_pos2);
    if (back_right_height2 < back_right_pos.y - 256) {
        shift2 = M_DoShift(item, &back_right_pos2, &back_right_pos);
        if ((shift2 > 0 && shift >= 0) || (shift2 < 0 && shift <= 0)) {
            shift += shift2;
        }
    }

    room_num = item->room_num;
    SECTOR *const sector2 = Room_GetSector(item->pos, &room_num);
    const int32_t height2 = Room_GetHeight(sector2, item->pos);

    if (height2 < item->pos.y - 256) {
        M_DoShift(item, &item->pos, &old_pos);
    }

    quad->extra_rotation = shift;
    const int32_t anim = M_GetCollisionAnim(item, &new_pos);

    if (anim != 0) {
        const XYZ_32 delta = XYZ_32_Subtract(item->pos, old_pos);
        int32_t speed2 = XYZ_32_UnrotateYaw(delta, quad->momentum_angle).z;
        speed2 <<= 8;

        if (Lara_Vehicle_GetItem() == item && quad->velocity == 0xA000
            && speed2 < 0x9FF6) {
            Lara_TakeDamage((0xA000 - speed2) >> 7, true);
        }

        if (quad->velocity > 0 && speed2 < quad->velocity) {
            quad->velocity = speed2 < 0 ? 0 : speed2;
        } else if (quad->velocity < 0 && speed2 > quad->velocity) {
            quad->velocity = speed2 > 0 ? 0 : speed2;
        }

        if (quad->velocity < -0x3000) {
            quad->velocity = -0x3000;
        }
    }

    return anim;
}

static bool M_AnimateQuadBike(
    ITEM *const item, const int32_t hit_wall, const bool killed)
{
    int16_t state;

    ITEM *const lara_item = Lara_GetItem();
    M_PRIV *const p = item->priv;
    M_QUAD_BIKE_INFO *const quad = &p->quad;
    state = lara_item->current_anim_state;

    if (item->pos.y != item->floor && state != M_STATE_FALL
        && state != M_STATE_LAND && state != M_STATE_FALL_OFF && !killed) {
        const int16_t anim_idx =
            quad->velocity < 0 ? M_ANIM_FALL_FRONT : M_ANIM_FALL_BACK;
        Lara_Vehicle_SwitchToAnim(anim_idx, 0);
        lara_item->current_anim_state = M_STATE_FALL;
        lara_item->goal_anim_state = M_STATE_FALL;
    } else if (
        hit_wall != 0 && state != M_STATE_HIT_FRONT && state != M_STATE_HIT_BACK
        && state != M_STATE_HIT_LEFT && state != M_STATE_HIT_RIGHT
        && state != M_STATE_FALL_OFF && quad->velocity > 0x3555 && !killed) {
        switch (hit_wall) {
        case 13:
            Lara_Vehicle_SwitchToAnim(M_ANIM_HIT_FRONT, 0);
            lara_item->current_anim_state = M_STATE_HIT_FRONT;
            lara_item->goal_anim_state = M_STATE_HIT_FRONT;
            break;

        case 14:
            Lara_Vehicle_SwitchToAnim(M_ANIM_HIT_BACK, 0);
            lara_item->current_anim_state = M_STATE_HIT_BACK;
            lara_item->goal_anim_state = M_STATE_HIT_BACK;
            break;

        case 11:
            Lara_Vehicle_SwitchToAnim(M_ANIM_HIT_LEFT, 0);
            lara_item->current_anim_state = M_STATE_HIT_LEFT;
            lara_item->goal_anim_state = M_STATE_HIT_LEFT;
            break;

        default:
            Lara_Vehicle_SwitchToAnim(M_ANIM_HIT_RIGHT, 0);
            lara_item->current_anim_state = M_STATE_HIT_RIGHT;
            lara_item->goal_anim_state = M_STATE_HIT_RIGHT;
            break;
        }

        Sound_Effect(SFX_QUAD_FRONT_IMPACT, &item->pos, SPM_NORMAL);
    } else {
        switch (lara_item->current_anim_state) {
        case M_STATE_DRIVE:
            if (killed) {
                if (quad->velocity <= 0x5000) {
                    lara_item->goal_anim_state = M_STATE_BIKE_DEATH;
                } else {
                    lara_item->goal_anim_state = M_STATE_FALL_DEATH;
                }
            } else if (
                !(quad->velocity & 0xFFFFFF00) && !g_Input.jump
                && !g_Input.action) {
                lara_item->goal_anim_state = M_STATE_STOP;
            } else if (g_Input.left && !m_HandbrakeStarting) {
                lara_item->goal_anim_state = M_STATE_TURN_L;
            } else if (g_Input.right && !m_HandbrakeStarting) {
                lara_item->goal_anim_state = M_STATE_TURN_R;
            } else if (g_Input.jump) {
                if (quad->velocity <= 0x6AAA) {
                    lara_item->goal_anim_state = M_STATE_SLOW;
                } else {
                    lara_item->goal_anim_state = M_STATE_BRAKE;
                }
            }
            break;

        case M_STATE_TURN_L:
            if (!(quad->velocity & 0xFFFFFF00)) {
                lara_item->goal_anim_state = M_STATE_STOP;
            } else if (g_Input.right) {
                Lara_Vehicle_SwitchToAnim(M_ANIM_TURN_RIGHT, 0);
                lara_item->current_anim_state = M_STATE_TURN_R;
                lara_item->goal_anim_state = M_STATE_TURN_R;
            } else if (!g_Input.left) {
                lara_item->goal_anim_state = M_STATE_DRIVE;
            }
            break;

        case M_STATE_SLOW:
        case M_STATE_BRAKE:
        case M_STATE_STOP_SLOWLY:
            if (!(quad->velocity & 0xFFFFFF00)) {
                lara_item->goal_anim_state = M_STATE_STOP;
            } else if (g_Input.left) {
                lara_item->goal_anim_state = M_STATE_TURN_L;
            } else if (g_Input.right) {
                lara_item->goal_anim_state = M_STATE_TURN_R;
            }
            break;

        case M_STATE_FALL:
            if (item->pos.y == item->floor) {
                lara_item->goal_anim_state = M_STATE_LAND;
            } else if (item->fall_speed > 240 && !Game_IsInGym()) {
                quad->flags |= 0x40;
            }
            break;

        case M_STATE_HIT_BACK:
        case M_STATE_HIT_FRONT:
        case M_STATE_HIT_LEFT:
        case M_STATE_HIT_RIGHT:
            if (g_Input.jump || g_Input.action) {
                lara_item->goal_anim_state = M_STATE_DRIVE;
            }
            break;

        case M_STATE_STOP:
            if (killed) {
                lara_item->goal_anim_state = M_STATE_BIKE_DEATH;
                break;
            }
            if (g_Input.roll && !quad->velocity && !m_DontExitQuad) {
                if (g_Input.right && M_SkidooCanGetOff(1)) {
                    lara_item->goal_anim_state = M_STATE_GET_OFF_R;
                } else if (g_Input.left && M_SkidooCanGetOff(-1)) {
                    lara_item->goal_anim_state = M_STATE_GET_OFF_L;
                }
            } else if (g_Input.jump || g_Input.action) {
                lara_item->goal_anim_state = M_STATE_DRIVE;
            }
            break;

        case M_STATE_TURN_R:
            if (!(quad->velocity & 0xFFFFFF00)) {
                lara_item->goal_anim_state = M_STATE_STOP;
            } else if (g_Input.left) {
                Lara_Vehicle_SwitchToAnim(M_ANIM_TURN_LEFT, 0);
                lara_item->current_anim_state = M_STATE_TURN_L;
                lara_item->goal_anim_state = M_STATE_TURN_L;
            } else if (!g_Input.right) {
                lara_item->goal_anim_state = M_STATE_DRIVE;
            }
            break;
        }
    }

    if (Room_Get(item->room_num)->flags.underwater
        || Room_Get(item->room_num)->flags.swamp) {
        lara_item->goal_anim_state = M_STATE_FALL_OFF;
        Lara_Kill();
        M_Explode(item);
        return false;
    }

    return true;
}

static bool M_UserControl(ITEM *item, int32_t height, int32_t *pitch)
{
    M_QUAD_BIKE_INFO *quad;

    M_PRIV *const p = item->priv;
    quad = &p->quad;

    if (!quad->velocity && !g_Input.sprint && !m_CanHandbrakeStart) {
        m_CanHandbrakeStart = true;
    } else if (quad->velocity) {
        m_CanHandbrakeStart = false;
    }

    if (!g_Input.sprint) {
        m_HandbrakeStarting = 0;
    }

    if (!m_HandbrakeStarting) {
        if (quad->revs > 16) {
            quad->velocity += quad->revs >> 4;
            quad->revs -= quad->revs >> 3;
        } else {
            quad->revs = 0;
        }
    }

    if (item->pos.y < height - 256) {
        if (quad->engine_revs < 0xA000) {
            quad->engine_revs += (0xA000 - quad->engine_revs) >> 3;
        }
    } else {
        if (!quad->velocity && g_Input.look) {
            Lara_Look_UpDown();
        }

        if (quad->velocity > 0) {
            if (g_Input.sprint && !m_HandbrakeStarting
                && quad->velocity > 0x3000) {
                if (g_Input.left) {
                    quad->skidoo_turn -= 500;

                    if (quad->skidoo_turn < -0x5B0) {
                        quad->skidoo_turn = -0x5B0;
                    }
                } else if (g_Input.right) {
                    quad->skidoo_turn += 500;

                    if (quad->skidoo_turn > 0x5B0) {
                        quad->skidoo_turn = 0x5B0;
                    }
                }
            } else {
                if (g_Input.left) {
                    quad->skidoo_turn -= 455;

                    if (quad->skidoo_turn < -910) {
                        quad->skidoo_turn = -910;
                    }
                } else if (g_Input.right) {
                    quad->skidoo_turn += 455;

                    if (quad->skidoo_turn > 910) {
                        quad->skidoo_turn = 910;
                    }
                }
            }
        } else if (quad->velocity < 0) {
            if (g_Input.sprint && !m_HandbrakeStarting
                && quad->velocity < -0x2800) {
                if (g_Input.right) {
                    quad->skidoo_turn -= 500;

                    if (quad->skidoo_turn < -0x5B0) {
                        quad->skidoo_turn = -0x5B0;
                    }
                } else if (g_Input.left) {
                    quad->skidoo_turn += 500;

                    if (quad->skidoo_turn > 0x5B0) {
                        quad->skidoo_turn = 0x5B0;
                    }
                }
            } else {
                if (g_Input.right) {
                    quad->skidoo_turn -= 455;

                    if (quad->skidoo_turn < -910) {
                        quad->skidoo_turn = -910;
                    }
                } else if (g_Input.left) {
                    quad->skidoo_turn += 455;

                    if (quad->skidoo_turn > 910) {
                        quad->skidoo_turn = 910;
                    }
                }
            }
        }

        if (g_Input.jump) {
            if (g_Input.sprint
                && (m_CanHandbrakeStart || m_HandbrakeStarting)) {
                m_HandbrakeStarting = 1;
                quad->revs -= 512;

                if (quad->revs < -0x3000) {
                    quad->revs = -0x3000;
                }
            } else if (quad->velocity > 0) {
                quad->velocity -= 640;
            } else if (quad->velocity > -0x3000) {
                quad->velocity -= 768;
            }
        } else if (g_Input.action) {
            if (g_Input.sprint
                && (m_CanHandbrakeStart || m_HandbrakeStarting)) {
                m_HandbrakeStarting = 1;
                quad->revs += 512;

                if (quad->revs >= 0xA000) {
                    quad->revs = 0xA000;
                }
            } else if (quad->velocity < 0xA000) {
                if (quad->velocity < 0x4000) {
                    quad->velocity += ((0x4800 - quad->velocity) >> 3) + 8;
                } else if (quad->velocity < 0x7000) {
                    quad->velocity += ((0x7800 - quad->velocity) >> 4) + 4;
                } else {
                    quad->velocity += ((0xA000 - quad->velocity) >> 3) + 2;
                }
            } else {
                quad->velocity = 0xA000;
            }
        } else if (quad->velocity > 256) {
            quad->velocity -= 256;
        } else if (quad->velocity < -256) {
            quad->velocity += 256;
        } else {
            quad->velocity = 0;
        }

        if (m_HandbrakeStarting && quad->revs && !g_Input.jump
            && !g_Input.action) {
            if (quad->revs > 8) {
                quad->revs -= quad->revs >> 3;
            } else {
                quad->revs = 0;
            }
        }

        item->speed = quad->velocity >> 8;

        if (quad->engine_revs > 0x7000) {
            quad->engine_revs = -0x2000;
        }

        int32_t revs = 0;
        if (quad->velocity < 0) {
            revs = ABS(quad->revs) + ABS(quad->velocity >> 1);
        } else if (quad->velocity < 0x7000) {
            revs = ABS(quad->revs) + 0x8800 * quad->velocity / 0x7000 - 0x2000;
        } else if (quad->velocity <= 0xA000) {
            revs = ABS(quad->revs) + 0x9800 * (quad->velocity - 0x7000) / 0x3000
                - 0x2800;
        } else {
            revs += ABS(quad->revs);
        }

        quad->engine_revs += (revs - quad->engine_revs) >> 3;
    }

    *pitch = quad->engine_revs;
    return false;
}

static void M_Setup(OBJECT *const obj)
{
    if (!obj->loaded) {
        return;
    }

    obj->initialise_func = M_Initialise;
    obj->priv_size = sizeof(M_PRIV);
    obj->priv_load_func = M_LoadPriv;
    obj->priv_save_func = M_SavePriv;
    obj->collision_func = M_Collision;
    obj->draw_func = Object_DrawAnimatingItem;
    obj->event_func = Vehicle_HandleEvent;
    obj->save_position = true;
    obj->save_flags = true;
    obj->save_anim = true;

    M_EnableWheelExtraRotations(obj);

    OBJECT_PROPERTIES(
        obj,
        OBJECT_PROPERTY_STORED(
            "track_1", -1, "Random music track pool, slot 1. -1 = disabled."),
        OBJECT_PROPERTY_STORED(
            "track_2", -1, "Random music track pool, slot 2. -1 = disabled."),
        OBJECT_PROPERTY_STORED(
            "track_3", -1, "Random music track pool, slot 3. -1 = disabled."),
        OBJECT_PROPERTY_STORED(
            "track_4", -1, "Random music track pool, slot 4. -1 = disabled."),
        OBJECT_PROPERTY_STORED(
            "is_heavy", true,
            "Whether or not this vehicle can activate heavy triggers."),
        OBJECT_PROPERTY(
            M_PRIV, test_static_collision, false,
            "Whether or not this vehicle can collide with static meshes."));
}

bool QuadBike_Control(void)
{
    ITEM *const item = Lara_Vehicle_GetItem();
    ITEM *const lara_item = Lara_GetItem();
    M_PRIV *const p = item->priv;
    M_QUAD_BIKE_INFO *const quad = &p->quad;

    int32_t hit_wall = M_SkidooDynamics(item);
    bool killed = false;
    int32_t pitch = 0;

    XYZ_32 front_left_pos = { 0 };
    XYZ_32 front_right_pos = { 0 };
    const int32_t front_left_height =
        M_TestHeight(item, 550, -260, &front_left_pos);
    const int32_t front_right_height =
        M_TestHeight(item, 550, 260, &front_right_pos);

    int16_t room_num = item->room_num;
    SECTOR *const sector = Room_GetSector(item->pos, &room_num);
    const int32_t height = Room_GetHeight(sector, item->pos);
    Vehicle_TestTriggers(lara_item, item);

    if (lara_item->hit_points <= 0) {
        killed = true;
        g_Input.forward = 0;
        g_Input.back = 0;
        g_Input.left = 0;
        g_Input.right = 0;
    }

    int32_t driving = 0;
    if (quad->flags != 0) {
        driving = front_right_height; // what
        hit_wall = 0;
    } else {
        switch (lara_item->current_anim_state) {
        case M_STATE_GET_ON_R:
        case M_STATE_GET_OFF_R:
        case M_STATE_GET_ON_L:
        case M_STATE_GET_OFF_L:
            driving = -1;
            hit_wall = 0;
            break;

        default:
            driving = M_UserControl(item, height, &pitch);
            break;
        }
    }

    if (quad->velocity != 0 || quad->revs != 0) {
        quad->pitch = pitch;

        if (quad->pitch < -0x8000) {
            quad->pitch = -0x8000;
        } else if (quad->pitch > 0xA000) {
            quad->pitch = 0xA000;
        }

        Sound_Effect(
            SFX_QUAD_MOVE, &item->pos,
            SPM_PITCH | ((SOUND_DEFAULT_PITCH + quad->pitch) << 8));
    } else {
        if (driving != -1) {
            Sound_Effect(SFX_QUAD_IDLE, &item->pos, SPM_NORMAL);
        }

        quad->pitch = 0;
    }

    item->floor = height;
    // Cap per-frame wheel rotation delta to avoid large int16 angle jumps.
    // Large jumps can make interpolation pick the "short way" and appear to
    // spin backwards at high speed.
    int32_t wheel_delta = quad->velocity >> 2;
    CLAMP(wheel_delta, -0x1000, 0x1000);
    quad->front_rot = quad->front_rot - wheel_delta;

    int32_t rear_delta = wheel_delta + (quad->revs >> 3);
    CLAMP(rear_delta, -0x1000, 0x1000);
    quad->rear_rot = quad->rear_rot - rear_delta;

    if (p->extra_rotation != nullptr) {
        if (p->rear_rot_x_idx[0] >= 0) {
            p->extra_rotation[p->rear_rot_x_idx[0]] = quad->rear_rot;
        }
        if (p->rear_rot_x_idx[1] >= 0) {
            p->extra_rotation[p->rear_rot_x_idx[1]] = quad->rear_rot;
        }
        if (p->front_rot_x_idx[0] >= 0) {
            p->extra_rotation[p->front_rot_x_idx[0]] = quad->front_rot;
        }
        if (p->front_rot_x_idx[1] >= 0) {
            p->extra_rotation[p->front_rot_x_idx[1]] = quad->front_rot;
        }
    }

    quad->left_fall_speed = M_DoDynamics(
        front_left_height, quad->left_fall_speed, &front_left_pos.y);
    quad->right_fall_speed = M_DoDynamics(
        front_right_height, quad->right_fall_speed, &front_right_pos.y);
    item->fall_speed =
        (int16_t)M_DoDynamics(height, item->fall_speed, &item->pos.y);

    const int32_t avg_front_height =
        (front_left_pos.y + front_right_pos.y) >> 1;
    const int16_t x_rot =
        (int16_t)Math_Atan(550, item->pos.y - avg_front_height);
    const int16_t z_rot =
        (int16_t)Math_Atan(260, avg_front_height - front_left_pos.y);
    item->rot.x += (x_rot - item->rot.x) >> 1;
    item->rot.z += (z_rot - item->rot.z) >> 1;

    const LARA_INFO *const lara = Lara_GetLaraInfo();
    if (!(quad->flags & 0x80)) {
        Room_GetSector(
            (XYZ_32) { item->pos.x, item->pos.y - 16, item->pos.z }, &room_num);
        if (room_num != item->room_num) {
            Item_UpdateRoom(Lara_Vehicle_GetIndex(), room_num);
            Item_UpdateRoom(lara->item_num, room_num);
        }

        lara_item->pos = item->pos;
        lara_item->rot = item->rot;
        if (!M_AnimateQuadBike(item, hit_wall, killed)) {
            return false;
        }
        Item_Animate(lara_item);
        Lara_Vehicle_SyncItemAnim();
        g_Camera.target_elevation = -5460;

        if (quad->flags & 0x40 && item->pos.y == item->floor) {
            if (g_Config.debug.enable_invulnerability) {
                lara_item->goal_anim_state = LS(LS_STOP);
                lara_item->current_anim_state = LS(LS_STOP);
                Item_SwitchToAnim(lara_item, LA(LA_FREEFALL_LAND), 0);
            } else {
                Item_Shatter(lara->item_num, -1, 0);
                Lara_Kill();
                lara_item->trigger.spent = true;
            }
            M_Explode(item);
            return false;
        }
    }

    const int16_t state = lara_item->current_anim_state;
    if (state != M_STATE_GET_ON_R && state != M_STATE_GET_ON_L
        && state != M_STATE_GET_OFF_R && state != M_STATE_GET_OFF_L) {
        XYZ_32 pos = { 0 };
        for (int32_t i = 0; i < 2; i++) {
            pos.x = m_QuadBites[i].pos.x;
            pos.y = m_QuadBites[i].pos.y;
            pos.z = m_QuadBites[i].pos.z;
            Collide_GetJointAbsPosition(item, &pos, m_QuadBites[i].mesh_num);
            const int16_t smoke_rot = item->rot.y + (i == 0 ? 0x9000 : 0x7000);

            if (item->speed > 32) {
                const int32_t smoke_vel = MINMAX(96 - item->speed, 8, 64);
                M_TriggerExhaustSmoke(pos, smoke_rot, smoke_vel, true);
            } else {
                int32_t smoke_vel = 0;
                if (m_ExhaustSmokeVel < 16) {
                    smoke_vel =
                        ((Random_GetControl() & 7)
                         + (Random_GetControl() & 0x10) + 2 * m_ExhaustSmokeVel)
                        << 7;
                    m_ExhaustSmokeVel++;
                } else if (m_HandbrakeStarting) {
                    smoke_vel = (ABS(quad->revs) >> 2)
                        + ((Random_GetControl() & 7) << 7);
                } else if (Random_GetControl() & 3) {
                    smoke_vel = 0;
                } else {
                    smoke_vel = ((Random_GetControl() & 0xF)
                                 + (Random_GetControl() & 0x10))
                        << 7;
                }

                M_TriggerExhaustSmoke(pos, smoke_rot, smoke_vel, false);
            }
        }
    } else {
        if (Game_IsInGym()) {
            Gym_TrackManager_Reset(GYM_TRACK_ASSAULT);
        }

        m_ExhaustSmokeVel = 0;
    }

    return M_CheckGetOff();
}

REGISTER_OBJECT(O_QUAD_BIKE, M_Setup)
