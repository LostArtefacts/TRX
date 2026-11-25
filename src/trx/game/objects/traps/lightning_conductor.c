#include <trx/core/json/util/read_io.h>
#include <trx/core/json/util/write_io.h>
#include <trx/game/collision.h>
#include <trx/game/fx/debris.h>
#include <trx/game/fx/lightning.h>
#include <trx/game/lara.h>
#include <trx/game/objects.h>
#include <trx/game/objects/property.h>
#include <trx/game/output.h>
#include <trx/game/random.h>
#include <trx/game/rooms.h>
#include <trx/game/sound.h>

// clang-format off
#define M_FLIP_GROUP     1
#define M_STRIKE_DELAY   120
#define M_SKY_HEIGHT     (WALL_L * 4)
#define M_DEBRIS_FACES   (-128)
#define M_GLOW_SIZE      64
#define M_MAX_SPHERES    34
// clang-format on

// The OCB selects the behaviour of the conductor. A plain one strikes at
// random forever. A deadly one also kills Lara on its sector, and goes away
// once flip group 1 is showing. A destroying one waits for flip group 1,
// strikes once, then breaks the first two animating 8 items and goes
// away.
typedef enum {
    M_MODE_PLAIN = 0,
    M_MODE_DEADLY = 1,
    M_MODE_DESTROYING = 2,
} M_MODE;

typedef struct {
    int16_t strikes;
    int16_t offset;
    int16_t delay;
    int16_t targets[2];
} M_PRIV;

static int32_t M_GetOCB(const ITEM *const item)
{
    TRX_VALUE value = {};
    if (!ObjectProperty_GetItemValue(item, "ocb", &value)
        || value.type != TVT_S32) {
        return 0;
    }
    return value.as_int;
}

static RESULT M_LoadPriv(ITEM *const item, JSON_READ_IO *const io)
{
    M_PRIV *const p = item->priv;
    MUST(JSON_READ_OPT(io, "strikes", &p->strikes));
    MUST(JSON_READ_OPT(io, "offset", &p->offset));
    MUST(JSON_READ_OPT(io, "delay", &p->delay));
    return OK;
}

static void M_SavePriv(const ITEM *const item, JSON_WRITE_IO *const io)
{
    const M_PRIV *const p = item->priv;
    JSONW_WRITE(io, "strikes", p->strikes);
    JSONW_WRITE(io, "offset", p->offset);
    JSONW_WRITE(io, "delay", p->delay);
}

static void M_Initialise(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    M_PRIV *const p = item->priv;
    p->targets[0] = NO_ITEM;
    p->targets[1] = NO_ITEM;
    if (M_GetOCB(item) != M_MODE_DESTROYING) {
        return;
    }

    int32_t count = 0;
    for (int32_t i = 0; i < Item_GetLevelCount(); i++) {
        if (Item_Get(i)->object_id != O_ANIMATING_8) {
            continue;
        }
        if (count < 2) {
            p->targets[count] = i;
        }
        count++;
    }
}

static void M_StartStrike(M_PRIV *const p)
{
    p->strikes = (Random_GetControl() & 3) + 4;
    p->offset = (Random_GetControl() & 0x3FF) - 512;
}

static bool M_IsInFlashRoom(const ITEM *const item)
{
    TRX_VALUE value;
    if (!ObjectProperty_GetItemValue(item, "flash_room_nums", &value)) {
        return false;
    }
    const int16_t room_num = Lara_GetItem()->room_num;
    for (int32_t i = 0; i < value.as_list.count; i++) {
        if (value.as_list.values[i] == room_num) {
            return true;
        }
    }
    return false;
}

static bool M_IsLaraUnder(const ITEM *const item)
{
    const ITEM *const lara_item = Lara_GetItem();
    return (lara_item->pos.x >> WALL_SHIFT) == (item->pos.x >> WALL_SHIFT)
        && (lara_item->pos.z >> WALL_SHIFT) == (item->pos.z >> WALL_SHIFT)
        && lara_item->pos.y <= item->pos.y;
}

static void M_Shatter(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    if ((item->mesh_bits & 1) == 0) {
        return;
    }

    Sound_Effect(SFX_HIT_ROCK, &item->pos, SPM_NORMAL);
    SPHERE spheres[M_MAX_SPHERES];
    Collide_GetSpheres(item, spheres, true);
    const OBJECT *const obj = Object_Get(item->object_id);
    const SHATTER_ITEM shatter_item = {
        .pos = spheres[0].pos,
        .mesh = Object_GetMesh(obj->mesh_idx),
        .bit = 1,
        .yaw = item->rot.y,
    };
    FX_Debris_ShatterItem(&shatter_item, M_DEBRIS_FACES, item->room_num, 0);
    item->mesh_bits &= ~1;
}

static void M_DestroyTargets(const M_PRIV *const p)
{
    for (size_t i = 0; i < ARRAY_SIZE(p->targets); i++) {
        if (p->targets[i] != NO_ITEM) {
            M_Shatter(p->targets[i]);
            Item_Destroy(p->targets[i]);
        }
    }
}

static void M_Strike(const int16_t item_num, const M_MODE mode)
{
    ITEM *const item = Item_Get(item_num);
    M_PRIV *const p = item->priv;

    Sound_Effect(SFX_ELECTRIC_ARCING_LOOP, &item->pos, SPM_NORMAL);
    p->strikes--;

    const int32_t b = (Random_GetControl() & 0x3F) + 192;
    const int32_t g = b - (Random_GetControl() & 0x1F);
    int32_t r = g - (Random_GetControl() & 0xFF);
    CLAMPL(r, 0);
    const RGB_888 color = { .r = (uint8_t)r, .g = (uint8_t)g, .b = (uint8_t)b };

    const XYZ_32 ground = {
        .x = (Random_GetControl() & 0x1FF) + item->pos.x - 256,
        .y = item->pos.y,
        .z = (Random_GetControl() & 0x1FF) + item->pos.z - 256,
    };
    FX_Lightning_Create(&(FX_LIGHTNING_SETUP) {
        .src = item->pos,
        .dst = ground,
        .variation = (Random_GetControl() & 0xF) + 16,
        .color = color,
        .life = 24,
        .size = 24,
        .segments = 3,
        .flags = FX_LIGHTNING_F_CURVED | FX_LIGHTNING_F_MOVING_END,
    });

    const XYZ_32 sky = {
        .x = item->pos.x + p->offset,
        .y = item->pos.y - M_SKY_HEIGHT,
        .z = item->pos.z + p->offset,
    };
    const bool is_fork = (Random_GetControl() & 7) != 0 && p->strikes != 0;
    FX_Lightning_Create(&(FX_LIGHTNING_SETUP) {
        .src = sky,
        .dst = item->pos,
        .variation = (Random_GetControl() & 0x1F) + (is_fork ? 16 : 48),
        .color = color,
        .life = 32,
        .size = 48,
        .segments = 3,
        .flags = is_fork ? FX_LIGHTNING_F_NONE : FX_LIGHTNING_F_CURVED,
    });

    if (M_IsInFlashRoom(item)) {
        const int32_t frame = (int32_t)Output_GetTimeInGame();
        const int32_t shift = (frame & 1) != 0 ? 1 : 2;
        Output_Flash_Trigger((RGB_888) {
            .r = color.r >> shift,
            .g = color.g >> shift,
            .b = color.b >> shift,
        });
    }

    FX_Lightning_CreateGlow(item->pos, color, M_GLOW_SIZE);

    if (mode == M_MODE_DESTROYING && p->strikes == 0) {
        M_DestroyTargets(p);
        Item_Destroy(item_num);
    } else if (
        mode == M_MODE_DEADLY && !Lara_GetLaraInfo()->burn
        && M_IsLaraUnder(item)) {
        Lara_CatchFire();
        Lara_GetItem()->hit_points = 0;
    }
}

static void M_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    M_PRIV *const p = item->priv;
    if (!Item_IsTriggerActive(item)) {
        return;
    }

    const M_MODE mode = M_GetOCB(item);
    const bool is_flipped = Room_GetFlipGroupStatus(M_FLIP_GROUP);
    if (mode == M_MODE_DESTROYING) {
        if (!is_flipped) {
            return;
        }
        if (p->delay < M_STRIKE_DELAY) {
            p->strikes = 0;
            p->delay++;
            return;
        }
        if (p->strikes == 0) {
            M_StartStrike(p);
        }
    } else if (mode == M_MODE_DEADLY && is_flipped) {
        Item_Destroy(item_num);
        return;
    }

    if (p->strikes != 0) {
        M_Strike(item_num, mode);
    } else if ((Random_GetControl() & 0x3F) == 0) {
        Sound_Effect(SFX_THUNDER_CRACK, &item->pos, SPM_NORMAL);
        M_StartStrike(p);
    }
}

static void M_Setup(OBJECT *const obj)
{
    obj->initialise_func = M_Initialise;
    obj->control_func = M_Control;
    obj->draw_func = nullptr;
    obj->priv_size = sizeof(M_PRIV);
    obj->priv_load_func = M_LoadPriv;
    obj->priv_save_func = M_SavePriv;
    obj->save_flags = true;

    OBJECT_PROPERTIES(
        obj,
        OBJECT_PROPERTY_STORED(
            "flash_room_nums", (S32_LIST) {},
            "Numbers of the rooms in which Lara sees the screen flash when "
            "the conductor strikes."));
}

REGISTER_OBJECT(O_LIGHTNING_CONDUCTOR, M_Setup)
