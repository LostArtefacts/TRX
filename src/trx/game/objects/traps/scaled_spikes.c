#include <trx/game/objects/traps/scaled_spikes.h>

#include <trx/config.h>
#include <trx/core/json/util/read_io.h>
#include <trx/core/json/util/write_io.h>
#include <trx/game/interpolation.h>
#include <trx/game/lara.h>
#include <trx/game/output.h>
#include <trx/game/random.h>
#include <trx/game/rooms.h>
#include <trx/game/sound.h>
#include <trx/game/sparks.h>

// clang-format off
#define M_DEFAULT_RADIUS      480
#define M_TILTED_RADIUS       300
#define M_DEFAULT_DAMAGE      8
#define M_DEFAULT_ORIENTATION 4
#define M_COOLDOWN            64
#define M_MAX_Y_SCALE         5120
#define M_XZ_SCALE            (1 << W2V_SHIFT) // = 16384
#define M_ACCELERATION        (STEP_L / 2) // = 128
#define M_DEFAULT_SPEED       WALL_L
// clang-format on

typedef struct {
    struct {
        XYZ_32 pos;
        XYZ_16 rot;
        bool is_set;
    } base;
    int16_t speed;
    int16_t scale;
    int16_t prev_scale;
    int16_t cooldown;
    int32_t damage;
    int32_t orientation;
    SCALED_SPIKES_MODE scaled_spikes_mode;
} M_PRIV;

typedef struct {
    int16_t xz_rot;
    int16_t xz_offset;
    int16_t y_offset;
    int16_t y_det_offset;
} M_SETUP;

static const M_SETUP m_Setups[8] = {
    // clang-format off
    { -DEG_180,       0,          -WALL_L,     WALL_L },
    { -DEG_135,       0,           0,          WALL_L / 2 },
    { -DEG_90,       +WALL_L / 2, -WALL_L / 2, WALL_L / 2 },
    { -DEG_45,        0,           0,          WALL_L / 2 },
    { +0,             0,           0,          0 },
    { +DEG_45,        0,           0,          WALL_L / 2 },
    { +DEG_90,       -WALL_L / 2, -WALL_L / 2, WALL_L / 2 },
    { +DEG_135,       0,           0,          WALL_L / 2 },
    // clang-format on
};

static RESULT M_LoadPriv(ITEM *const item, JSON_READ_IO *const io)
{
    M_PRIV *const p = item->priv;
    MUST(JSON_READ_OPT(io, "speed", &p->speed));
    MUST(JSON_READ_OPT(io, "scale", &p->scale));
    MUST(JSON_READ_OPT(io, "cooldown", &p->cooldown));
    p->prev_scale = p->scale;
    return OK;
}

static void M_SavePriv(const ITEM *const item, JSON_WRITE_IO *const io)
{
    const M_PRIV *const p = item->priv;
    JSONW_WRITE(io, "speed", p->speed);
    JSONW_WRITE(io, "scale", p->scale);
    JSONW_WRITE(io, "cooldown", p->cooldown);
}

static const char *M_CheckOrientation(const TRX_VALUE *const in)
{
    if (in->as_int < 0 || in->as_int > 0xF) {
        return "orientation is beyond known range";
    }
    return nullptr;
}

static const char *M_CheckMode(const TRX_VALUE *const in)
{
    return in->as_int < 0 || in->as_int >= SCALED_SPIKES_NUMBER_OF
        ? "no such scaled spikes mode"
        : nullptr;
}

static void M_EnsureBaseTransform(const ITEM *const item)
{
    M_PRIV *const p = item->priv;
    if (p->base.is_set) {
        return;
    }

    p->base.pos = item->pos;
    p->base.rot = item->rot;
    p->base.is_set = true;
}

static void M_SetOrientation(ITEM *const item, const TRX_VALUE *const in)
{
    M_PRIV *const p = item->priv;
    p->orientation = in->as_int;
    M_EnsureBaseTransform(item);
    item->pos = p->base.pos;
    item->rot = p->base.rot;

    const M_SETUP *const setup = &m_Setups[p->orientation & 7];
    if ((p->orientation & 8) != 0) {
        item->rot.x = setup->xz_rot;
        item->rot.y = DEG_90;
        item->pos.z -= setup->xz_offset;
    } else {
        item->rot.z = setup->xz_rot;
        item->pos.x += setup->xz_offset;
    }
    item->pos.y += setup->y_offset;
}

static void M_Initialise(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    Item_SetVisible(item, false);

    M_PRIV *const p = item->priv;
    p->speed = M_DEFAULT_SPEED;
    p->cooldown = 0;
    M_EnsureBaseTransform(item);
}

static bool M_TestCollision(const ITEM *const item, const ITEM *const lara_item)
{
    if (lara_item->hit_points <= 0) {
        return false;
    }

    if (g_Config.debug.enable_invulnerability) {
        return false;
    }

    const M_PRIV *const p = item->priv;
    const M_SETUP *const setup = &m_Setups[p->orientation & 7];
    int32_t x;
    int32_t z;
    if ((p->orientation & 8) != 0) {
        x = ROUND_TO_SECTOR(item->pos.x) + WALL_L / 2;
        z = ROUND_TO_SECTOR(item->pos.z + setup->xz_offset) + WALL_L / 2;
    } else {
        x = ROUND_TO_SECTOR(item->pos.x - setup->xz_offset) + WALL_L / 2;
        z = ROUND_TO_SECTOR(item->pos.z) + WALL_L / 2;
    }

    const int32_t radius =
        (p->orientation & 1) != 0 ? M_TILTED_RADIUS : M_DEFAULT_RADIUS;
    const int32_t y = item->pos.y + setup->y_det_offset;

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
    p->prev_scale = p->scale;

    if (!Item_IsTriggerActive(item) || p->cooldown != 0) {
        if (Item_IsTriggerActive(item)) {
            p->speed += (p->speed >> 3) + 32;
            p->scale -= p->speed;

            if (p->scale < 0) {
                p->speed = M_DEFAULT_SPEED;
                p->scale = 0;
                Item_SetVisible(item, false);
            }

            if (p->scaled_spikes_mode == SCALED_SPIKES_MODE_ONE_SHOT) {
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
                && (p->orientation & 7) > 2 && (p->orientation & 7) < 6) {
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
            if (p->orientation == 8 || p->orientation == 0) {
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

            if ((p->orientation & 7) == 2 || (p->orientation & 7) == 6) {
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
                if (p->scaled_spikes_mode != SCALED_SPIKES_MODE_EXTENDED
                    && lara_item->hit_points > 0) {
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

    const double ratio = Interpolation_GetWorldRate();
    const bool do_interp =
        Interpolation_IsActive() && ratio > 0.0 && ratio < 1.0;
    const int32_t y_scale = do_interp
        ? LERP(p->prev_scale << 2, p->scale << 2, ratio)
        : p->scale << 2;

    Matrix_ScaleX(M_XZ_SCALE);
    Matrix_ScaleY(y_scale);
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
            "Damage dealt when Lara hits the spikes without dying instantly."),
        OBJECT_PROPERTY_SETTER(
            M_PRIV, orientation, M_DEFAULT_ORIENTATION, M_CheckOrientation,
            M_SetOrientation,
            "The orientation configuration of the spikes. Value range: "
            "minimum 0; maximum 15."),
        OBJECT_PROPERTY_CHECKED(
            M_PRIV, scaled_spikes_mode, SCALED_SPIKES_MODE_LOOPING, M_CheckMode,
            "The behavior of the spikes when triggered - 0: looping; 1: "
            "permanently extended; 2: one-shot."));
}

REGISTER_OBJECT(O_SCALED_SPIKES, M_Setup)
