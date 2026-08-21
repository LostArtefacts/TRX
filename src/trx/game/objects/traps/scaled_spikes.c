#include <trx/config.h>
#include <trx/core/json/util/read_io.h>
#include <trx/core/json/util/write_io.h>
#include <trx/game/lara.h>
#include <trx/game/output.h>
#include <trx/game/random.h>
#include <trx/game/rooms.h>
#include <trx/game/sound.h>
#include <trx/game/sparks.h>

// clang-format off
#define M_DEFAULT_RADIUS 480
#define M_TILTED_RADIUS  300
#define M_DEFAULT_DAMAGE 8
#define M_COOLDOWN       64
#define M_MAX_Y_SCALE    5120
#define M_XZ_SCALE       (1 << W2V_SHIFT) // = 16384
#define M_ACCELERATION   (STEP_L / 2) // = 128
#define M_DEFAULT_SPEED  WALL_L
// clang-format on

typedef struct {
    int16_t speed;
    int16_t scale;
    int16_t cooldown;
    int32_t damage;
} M_PRIV;

// clang-format off
// TODO: improve OCB/property exposure
static const int16_t m_XZRots[8] = {
    -DEG_180, -DEG_135, -DEG_90, -DEG_45,
    +0,       +DEG_45,  +DEG_90, +DEG_135,
};
static const int16_t m_XZOffsets[8] = {
    0, 0, +WALL_L / 2, 0,
    0, 0, -WALL_L / 2, 0,
};
static const int16_t m_YOffsets[8] = {
    -WALL_L, 0, -WALL_L / 2, 0,
    0,       0, -WALL_L / 2, 0,
};
static const int16_t m_YDetOffsets[8] = {
    WALL_L, WALL_L / 2, WALL_L / 2, WALL_L / 2,
    0,      WALL_L / 2, WALL_L / 2, WALL_L / 2,
};
// clang-format on

static RESULT M_LoadPriv(ITEM *const item, JSON_READ_IO *const io)
{
    M_PRIV *const p = item->priv;
    MUST(JSON_READ_OPT(io, "speed", &p->speed));
    MUST(JSON_READ_OPT(io, "scale", &p->scale));
    MUST(JSON_READ_OPT(io, "cooldown", &p->cooldown));
    return OK;
}

static void M_SavePriv(const ITEM *const item, JSON_WRITE_IO *const io)
{
    const M_PRIV *const p = item->priv;
    JSONW_WRITE(io, "speed", p->speed);
    JSONW_WRITE(io, "scale", p->scale);
    JSONW_WRITE(io, "cooldown", p->cooldown);
}

static int32_t M_GetOCB(const ITEM *const item)
{
    TRX_VALUE value;
    if (!ObjectProperty_GetItemValue(item, "ocb", &value)) {
        return 0;
    }
    return value.as_int;
}

static void M_Initialise(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    Item_SetVisible(item, false);

    const int32_t ocb = M_GetOCB(item);
    if ((ocb & 8) != 0) {
        item->rot.x = m_XZRots[ocb & 7];
        item->rot.y = DEG_90;
        item->pos.z -= m_XZOffsets[ocb & 7];
    } else {
        item->rot.z = m_XZRots[ocb & 7];
        item->pos.x += m_XZOffsets[ocb & 7];
    }

    M_PRIV *const p = item->priv;
    p->speed = M_DEFAULT_SPEED;
    p->cooldown = 0;
    item->pos.y += m_YOffsets[ocb & 7];
}

static bool M_TestCollision(const ITEM *const item, const ITEM *const lara_item)
{
    if (lara_item->hit_points <= 0) {
        return false;
    }

    if (g_Config.debug.enable_invulnerability) {
        return false;
    }

    const int32_t ocb = M_GetOCB(item);
    int32_t x;
    int32_t z;
    if ((ocb & 8) != 0) {
        x = ROUND_TO_SECTOR(item->pos.x) + WALL_L / 2;
        z = ROUND_TO_SECTOR(item->pos.z + m_XZOffsets[ocb & 7]) + WALL_L / 2;
    } else {
        x = ROUND_TO_SECTOR(item->pos.x + m_XZOffsets[ocb & 7]) + WALL_L / 2;
        z = ROUND_TO_SECTOR(item->pos.z) + WALL_L / 2;
    }

    const int32_t radius = (ocb & 1) != 0 ? M_TILTED_RADIUS : M_DEFAULT_RADIUS;
    const int32_t y = item->pos.y + m_YDetOffsets[ocb & 7];

    const ANIM_FRAME *const frame = Item_GetBestFrame(lara_item);
    if (lara_item->pos.y + frame->bounds.min.y > y
        || lara_item->pos.y + frame->bounds.max.y < y - 900) {
        return false;
    }

    const int32_t x_min = lara_item->pos.x + frame->bounds.min.x;
    const int32_t x_max = lara_item->pos.x + frame->bounds.max.x;
    const int32_t z_min = lara_item->pos.z + frame->bounds.min.z;
    const int32_t z_max = lara_item->pos.z + frame->bounds.max.z;
    return x_min <= x + radius && x_max >= x - radius && z_min <= z + radius
        && z_max >= z - radius;
}

static void M_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    M_PRIV *const p = item->priv;
    ITEM *const lara_item = Lara_GetItem();

    const int32_t ocb = M_GetOCB(item);
    if (!Item_IsTriggerActive(item) || p->cooldown != 0) {
        if (Item_IsTriggerActive(item)) {
            p->speed += (p->speed >> 3) + 32;
            p->scale -= p->speed;

            if (p->scale < 0) {
                p->speed = M_DEFAULT_SPEED;
                p->scale = 0;
                Item_SetVisible(item, false);
            }

            if ((ocb & 32) != 0) {
                p->cooldown = 1;
            } else if (p->cooldown != 0) {
                p->cooldown--;
            }
        } else if (item->timer == 0) {
            p->speed += (p->speed >> 3) + 32;
            if (p->scale > 0) {
                p->scale -= p->speed;
                CLAMPL(p->scale, 0);
            }
        }
    } else {
        if (p->speed == M_DEFAULT_SPEED) {
            Sound_Effect(SFX_SPIKES, &item->pos, SPM_NORMAL);
        }
        Item_SetVisible(item, true);

        if (M_TestCollision(item, lara_item)) {
            const ANIM_FRAME *const item_frame = Item_GetBestFrame(item);
            const ANIM_FRAME *const lara_frame = Item_GetBestFrame(lara_item);
            int32_t blood_count = 0;

            if ((p->speed > M_DEFAULT_SPEED || lara_item->gravity)
                && (ocb & 7) > 2 && (ocb & 7) < 6) {
                if (lara_item->fall_speed > 6 || p->speed > M_DEFAULT_SPEED) {
                    Lara_Kill();
                    blood_count = 20;
                }
            } else {
                Lara_TakeDamage(p->damage, false);
                blood_count = (Random_GetControl() & 3) + 2;
            }

            int32_t y_top = lara_item->pos.y + lara_frame->bounds.min.y;
            int32_t y_bottom = lara_item->pos.y + lara_frame->bounds.max.y;

            int32_t y_bounds_1;
            int32_t y_bounds_2;
            if ((ocb & 0xF) == 8 || (ocb & 0xF) == 0) {
                y_bounds_1 = -item_frame->bounds.max.y;
                y_bounds_2 = -item_frame->bounds.min.y;
            } else {
                y_bounds_1 = item_frame->bounds.min.y;
                y_bounds_2 = item_frame->bounds.max.y;
            }

            if (y_top < item->pos.y + y_bounds_1) {
                y_top = y_bounds_1 + item->pos.y;
            }

            if (y_bottom > item->pos.y + y_bounds_2) {
                y_bottom = y_bounds_2 + item->pos.y;
            }

            if ((ocb & 7) == 2 || (ocb & 7) == 6) {
                blood_count >>= 1;
            }

            const int32_t dy = ABS(y_top - y_bottom) + 1;
            do {
                const XYZ_32 pos = {
                    .x = (Random_GetControl() & 0x7F) + lara_item->pos.x - 64,
                    .y = y_bottom - Random_GetControl() % dy,
                    .z = (Random_GetControl() & 0x7F) + lara_item->pos.z - 64,
                };
                Sparks_TriggerBloodTR4(pos, Random_GetControl() << 1, 1);
                blood_count--;
            } while (blood_count > 0);

            if (lara_item->hit_points <= 0) {
                int16_t room_num = lara_item->room_num;
                const SECTOR *const sector =
                    Room_GetSector(lara_item->pos, &room_num);
                const int32_t height = Room_GetHeight(sector, lara_item->pos);

                if (item->pos.y >= lara_item->pos.y
                    && height - lara_item->pos.y < 50) {
                    Item_SwitchToAnim(lara_item, LA(LA_SPIKE_DEATH), 0);
                    lara_item->current_anim_state = LS(LS_DEATH);
                    lara_item->goal_anim_state = LS(LS_DEATH);
                    lara_item->gravity = false;
                }
            }
        }

        p->speed += M_ACCELERATION;
        p->scale += p->speed;

        if (p->scale >= M_MAX_Y_SCALE) {
            p->scale = M_MAX_Y_SCALE;
            if (p->speed <= M_DEFAULT_SPEED) {
                p->speed = 0;
                if ((ocb & 16) == 0 && lara_item->hit_points > 0) {
                    p->cooldown = M_COOLDOWN;
                }
            } else {
                p->speed = -p->speed >> 1;
            }
        }
    }
}

static bool M_Draw(const ITEM *const item)
{
    const M_PRIV *const p = item->priv;
    if (p->scale == 0) {
        return false;
    }

    ANIM_FRAME *frames[2];
    int32_t rate;
    const int32_t frac = Item_GetFrames(item, frames, &rate);
    const BOUNDS_16 *const bounds = Item_GetBoundsAccurate(item);
    const OBJECT *const obj = Object_Get(item->object_id);

    Matrix_Push();
    Matrix_TranslateAbs32(item->interp.result.pos);
    Matrix_RotX(item->interp.result.rot.x);
    Matrix_RotZ(item->interp.result.rot.z);
    Matrix_RotY(item->interp.result.rot.y);

    const CLIP clip = Output_CheckBoundsClip(bounds);
    if (clip == CLIP_NOT_VISIBLE) {
        Matrix_Pop();
        return false;
    }

    Matrix_ScaleX(M_XZ_SCALE);
    Matrix_ScaleY(p->scale << 2);
    Matrix_ScaleZ(M_XZ_SCALE);

    Output_CalculateObjectLighting(
        item, frames[0] != nullptr ? &frames[0]->bounds : bounds);
    Object_DrawMesh(obj->mesh_idx, clip, false);

    if (g_Config.debug.enable_debug_bounding_boxes) {
        Output_DrawCuboid(bounds);
    }
    Matrix_Pop();
    return true;
}

static void M_Setup(OBJECT *const obj)
{
    obj->initialise_func = M_Initialise;
    obj->control_func = M_Control;
    obj->draw_func = M_Draw;

    obj->priv_size = sizeof(M_PRIV);
    obj->priv_load_func = M_LoadPriv;
    obj->priv_save_func = M_SavePriv;
    obj->save_flags = true;

    OBJECT_PROPERTIES(
        obj,
        OBJECT_PROPERTY(
            M_PRIV, damage, M_DEFAULT_DAMAGE,
            "Damage dealt when Lara hits the spikes without dying instantly."));
}

REGISTER_OBJECT(O_SCALED_SPIKES, M_Setup)
