#include <trx/core/json/util/read_io.h>
#include <trx/core/json/util/write_io.h>
#include <trx/core/math.h>
#include <trx/core/utils.h>
#include <trx/game/effects.h>
#include <trx/game/items/actions.h>
#include <trx/game/lara.h>
#include <trx/game/matrix.h>
#include <trx/game/objects.h>
#include <trx/game/objects/draw.h>
#include <trx/game/output.h>
#include <trx/game/output/lights.h>
#include <trx/game/random.h>
#include <trx/game/rooms.h>

#define M_MAX_BEETLES 128
#define M_HIT_RANGE 85
#define M_CHASE_RANGE 1024
#define M_MAX_DROP 1280
#define M_MAX_FALL_SPEED 500

typedef struct {
    int16_t count;
    int16_t slow_start;
    bool from_above;
} M_PRIV;

static int32_t M_GetOCB(const ITEM *const item)
{
    TRX_VALUE value;
    if (!ObjectProperty_GetItemValue(item, "ocb", &value)) {
        return 0;
    }
    return value.as_int;
}

static int32_t M_GetWibble(void)
{
    return ((int32_t)Output_GetTimeInGame() * 4) & 0xFC;
}

static int32_t M_CountBeetles(void)
{
    int32_t count = 0;
    for (int16_t effect_num = Effect_GetActiveNum(); effect_num != NO_EFFECT;
         effect_num = Effect_Get(effect_num)->next_active) {
        if (Effect_Get(effect_num)->object_id == O_SMALL_BEETLE) {
            count++;
        }
    }
    return count;
}

static RESULT M_LoadPriv(ITEM *const item, JSON_READ_IO *const io)
{
    M_PRIV *const p = item->priv;
    MUST(JSON_READ_OPT(io, "count", &p->count));
    MUST(JSON_READ_OPT(io, "slow_start", &p->slow_start));
    return OK;
}

static void M_SavePriv(const ITEM *const item, JSON_WRITE_IO *const io)
{
    const M_PRIV *const p = item->priv;
    JSONW_WRITE(io, "count", p->count);
    JSONW_WRITE(io, "slow_start", p->slow_start);
}

static void M_Initialise(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    M_PRIV *const p = item->priv;
    const int32_t ocb = M_GetOCB(item);
    const int32_t flags = ocb / 1000;
    p->from_above = (flags & 1) != 0;
    p->slow_start = flags & 4;
    p->count = ocb % 1000;

    if (p->from_above) {
        return;
    }

    const int16_t rot = item->rot.y;
    if (rot > 4096 && rot < 28672) {
        item->pos.x -= WALL_L / 2;
    } else if (rot < -4096 && rot > -28672) {
        item->pos.x += WALL_L / 2;
    }
    if (rot > -8192 && rot < 8192) {
        item->pos.z -= WALL_L / 2;
    } else if (rot < -20480 || rot > 20480) {
        item->pos.z += WALL_L / 2;
    }
}

static void M_Spawn(const ITEM *const item, const M_PRIV *const p)
{
    if (M_CountBeetles() >= M_MAX_BEETLES) {
        return;
    }

    const int16_t effect_num = Effect_Create(O_SMALL_BEETLE, item->room_num);
    if (effect_num == NO_EFFECT) {
        return;
    }

    EFFECT *const effect = Effect_Get(effect_num);
    effect->pos = item->pos;
    if (p->from_above) {
        effect->rot.y = Random_GetControl() << 1;
        effect->fall_speed = -16 - (Random_GetControl() & 0x1F);
    } else {
        effect->rot.y = (Random_GetControl() & 0x3FFF) + item->rot.y - DEG_45;
        effect->fall_speed = 0;
    }
    effect->rot.x = 0;
    effect->rot.z = 0;
    effect->flag1 = 0;
    effect->speed = (Random_GetControl() & 0x1F) + 1;
    effect->frame_num = 0;
    effect->counter = 0;
}

static void M_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    M_PRIV *const p = item->priv;
    if (p->count == 0
        || (p->slow_start != 0 && (Random_GetControl() & 0xF) != 0)) {
        return;
    }

    p->count--;
    if (p->slow_start != 0 && (Random_GetControl() & 1) != 0) {
        p->slow_start--;
    }
    M_Spawn(item, p);
}

static void M_ControlEffect(const int16_t effect_num)
{
    EFFECT *const effect = Effect_Get(effect_num);
    const int32_t idx = effect_num & (M_MAX_BEETLES - 1);
    const XYZ_32 old_pos = effect->pos;

    effect->pos.x += (Math_Sin(effect->rot.y) * effect->speed) >> W2V_SHIFT;
    effect->pos.y += effect->fall_speed;
    effect->pos.z += (Math_Cos(effect->rot.y) * effect->speed) >> W2V_SHIFT;
    effect->fall_speed += 6;

    const XYZ_32 lara_pos = Lara_GetItem()->pos;
    const int32_t dx = lara_pos.x - effect->pos.x;
    const int32_t dy = lara_pos.y - effect->pos.y;
    const int32_t dz = lara_pos.z - effect->pos.z;
    const int16_t angle = (int16_t)Math_Atan(dz, dx) - effect->rot.y;
    if (ABS(dz) < M_HIT_RANGE && ABS(dy) < M_HIT_RANGE
        && ABS(dx) < M_HIT_RANGE) {
        Lara_TakeDamage(1, true);
    }

    if (effect->flag1 != 0) {
        if (ABS(dx) + ABS(dz) > M_CHASE_RANGE) {
            if (effect->speed < (idx & 0x1F) + 24) {
                effect->speed++;
            }
            if (ABS(angle) < 4096) {
                effect->rot.y += (int16_t)((M_GetWibble() - idx) << 3);
            } else if (angle < 0) {
                effect->rot.y -= 1024;
            } else {
                effect->rot.y += 1024;
            }
        } else {
            if ((effect->speed & 1) != 0) {
                effect->rot.y += 512;
            } else {
                effect->rot.y -= 512;
            }
            effect->speed = 48 - (ABS(angle) >> 7);
            if (effect->speed < -16) {
                effect->speed = idx & 0xF;
            }
        }
    }

    int16_t room_num = effect->room_num;
    const SECTOR *const sector = Room_GetSector(effect->pos, &room_num);
    const int32_t height = Room_GetHeight(sector, effect->pos);
    if (room_num != effect->room_num) {
        Effect_UpdateRoom(effect_num, room_num);
    }

    if (height == NO_HEIGHT || height < effect->pos.y - M_MAX_DROP) {
        effect->rot.y += angle > 0 ? DEG_90 : -DEG_90;
        effect->pos = old_pos;
        effect->fall_speed = 0;
    } else if (effect->pos.y > height) {
        effect->pos.y = height;
        effect->fall_speed = 0;
        effect->flag1 = 1;
    }

    if (effect->fall_speed >= M_MAX_FALL_SPEED) {
        Effect_Destroy(effect_num);
        return;
    }
    effect->rot.x = -64 * effect->fall_speed;
}

static bool M_DrawEffect(const EFFECT *const effect)
{
    const OBJECT *const obj = Object_Get(effect->object_id);
    Matrix_Push();
    Matrix_TranslateAbs32(effect->interp.result.pos);
    Matrix_Rot16(effect->interp.result.rot);
    Output_CalculateStaticLightRGB_F((RGB_F) { 1.0f, 1.0f, 1.0f });
    Object_DrawMesh(obj->mesh_idx + ((M_GetWibble() >> 3) & 1), -1, false);
    Matrix_Pop();
    return true;
}

static void M_ClearAll(ITEM *const item)
{
    int16_t effect_num = Effect_GetActiveNum();
    while (effect_num != NO_EFFECT) {
        const int16_t next = Effect_Get(effect_num)->next_active;
        if (Effect_Get(effect_num)->object_id == O_SMALL_BEETLE) {
            Effect_Destroy(effect_num);
        }
        effect_num = next;
    }
    Room_SetFlipEffect(-1);
}

static void M_Setup(OBJECT *const obj)
{
    obj->initialise_func = M_Initialise;
    obj->control_func = M_Control;
    obj->draw_func = nullptr;
    obj->effect_control_func = M_ControlEffect;
    obj->effect_draw_func = M_DrawEffect;
    obj->priv_size = sizeof(M_PRIV);
    obj->priv_load_func = M_LoadPriv;
    obj->priv_save_func = M_SavePriv;
    obj->save_flags = true;
}

REGISTER_OBJECT(O_SMALL_BEETLE, M_Setup)
REGISTER_ITEM_ACTION(ITEM_ACTION_CLEAR_SMALL_BEETLES, M_ClearAll)
