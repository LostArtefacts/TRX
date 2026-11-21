#include <trx/core/json/util/read_io.h>
#include <trx/core/json/util/write_io.h>
#include <trx/game/items.h>
#include <trx/game/objects.h>
#include <trx/game/output.h>

// clang-format off
#define M_FIXED_SCALE (1 << W2V_SHIFT) // = 16384
#define M_MAX_SCALE   (WALL_L * 4) // = 4096
#define M_SPEED       128
#define M_HOLD_TIME   210
// clang-format on

typedef struct {
    int16_t scale;
    int16_t prev_scale;
    int16_t timer;
    bool is_shown;
} M_PRIV;

static RESULT M_LoadPriv(ITEM *const item, JSON_READ_IO *const io)
{
    M_PRIV *const p = item->priv;
    MUST(JSON_READ_OPT(io, "scale", &p->scale));
    MUST(JSON_READ_OPT(io, "timer", &p->timer));
    MUST(JSON_READ_OPT(io, "is_shown", &p->is_shown));
    p->prev_scale = p->scale;
    return OK;
}

static void M_SavePriv(const ITEM *const item, JSON_WRITE_IO *const io)
{
    const M_PRIV *const p = item->priv;
    JSONW_WRITE(io, "scale", p->scale);
    JSONW_WRITE(io, "timer", p->timer);
    JSONW_WRITE(io, "is_shown", p->is_shown);
}

static void M_AlignToWall(ITEM *const item)
{
    switch (item->rot.y) {
    case 0:
        item->pos.z &= ~(WALL_L - 1);
        break;
    case DEG_90:
        item->pos.x &= ~(WALL_L - 1);
        break;
    case -DEG_180:
        item->pos.z |= WALL_L - 1;
        break;
    case -DEG_90:
        item->pos.x |= WALL_L - 1;
        break;
    }
}

static void M_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    M_PRIV *const p = item->priv;
    p->prev_scale = p->scale;
    if (!Item_IsTriggerActive(item)) {
        return;
    }

    M_AlignToWall(item);

    if (!p->is_shown) {
        if (p->scale < M_MAX_SCALE) {
            p->scale += M_SPEED;
        } else {
            p->is_shown = true;
            p->scale = M_MAX_SCALE;
            p->timer = M_HOLD_TIME;
        }
    } else if (p->timer > 0) {
        p->timer--;
    } else if (p->scale >= M_SPEED) {
        p->scale -= M_SPEED;
    } else {
        Item_Destroy(item_num);
    }
}

static inline XYZ_32 M_GetDrawScale(const int32_t value)
{
    return (XYZ_32) {
        .x = M_FIXED_SCALE,
        .y = M_FIXED_SCALE,
        .z = value << 2,
    };
}

static float M_GetOpacity(const int32_t scale)
{
    const int32_t alpha = scale >> 5;
    return alpha >= 128 ? 1.0f : (alpha << 1) / 255.0f;
}

static bool M_Draw(const ITEM *const item)
{
    const M_PRIV *const p = item->priv;
    if (p->scale == 0) {
        return false;
    }

    RGBA_F tint = Output_GetTint();
    tint.a *= M_GetOpacity(p->scale);
    Output_PushTintOverride(tint);
    const bool result = Item_DrawScaled(
        item, M_GetDrawScale(p->scale), M_GetDrawScale(p->prev_scale));
    Output_PopTintOverride();
    return result;
}

static void M_Setup(OBJECT *const obj)
{
    if (!obj->loaded) {
        return;
    }

    obj->control_func = M_Control;
    obj->draw_func = M_Draw;

    obj->priv_size = sizeof(M_PRIV);
    obj->priv_load_func = M_LoadPriv;
    obj->priv_save_func = M_SavePriv;
    obj->save_flags = true;
}

REGISTER_OBJECT(O_GOD_HEAD, M_Setup)
