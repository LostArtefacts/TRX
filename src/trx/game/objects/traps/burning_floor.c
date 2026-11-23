#include <trx/core/json/util/read_io.h>
#include <trx/core/json/util/write_io.h>
#include <trx/core/utils.h>
#include <trx/game/collision.h>
#include <trx/game/fx/debris.h>
#include <trx/game/fx/fire.h>
#include <trx/game/items.h>
#include <trx/game/lara.h>
#include <trx/game/objects.h>
#include <trx/game/objects/general/burning_torch.h>
#include <trx/game/output/flash.h>
#include <trx/game/rooms.h>
#include <trx/game/sound.h>

// clang-format off
#define M_COLLAPSE_TIME   (LOGIC_FPS * 15)
#define M_MEDIUM_TIME     LOGIC_FPS
#define M_BIG_TIME        (LOGIC_FPS * 2)
#define M_FADE_MIN        2
#define M_FADE_MAX        255
#define M_DEADLY_RANGE    256
#define M_BURN_HIT_POINTS 100
#define M_TORCH_REACH     32
#define M_MAX_SPHERES     34
#define M_LAST_MESH       4
#define M_DEBRIS          (-24)
#define M_LAST_DEBRIS     (-32)
#define M_LIGHT_START     127
#define M_LIGHT_STEP_TIME 6
// clang-format on

typedef enum {
    M_FIRE_SMALL = 0,
    M_FIRE_MEDIUM = 1,
    M_FIRE_BIG = 2,
    M_FIRE_SIZES = 3,
} M_FIRE_SIZE;

typedef struct {
    int16_t x;
    int16_t z;
    M_FIRE_SIZE size;
} M_FIRE;

typedef struct {
    int32_t timer;
    int32_t fades[M_FIRE_SIZES];
    // How bright the floor stays while it burns, from 127 when it catches
    // fire down to 52. 0 keeps its full light.
    int32_t light;
} M_PRIV;

static const M_FIRE m_Fires[] = {
    // clang-format off
    { -96,   1216, M_FIRE_BIG },
    { 560,   736,  M_FIRE_BIG },
    { -432,  -976, M_FIRE_BIG },
    { -64,   -128, M_FIRE_BIG },
    { 824,   64,   M_FIRE_BIG },
    { 456,   -352, M_FIRE_MEDIUM },
    { 392,   352,  M_FIRE_MEDIUM },
    { 1096,  608,  M_FIRE_MEDIUM },
    { -424,  -416, M_FIRE_MEDIUM },
    { 520,   1152, M_FIRE_MEDIUM },
    { -248,  516,  M_FIRE_MEDIUM },
    { -808,  80,   M_FIRE_MEDIUM },
    { -1192, -384, M_FIRE_SMALL },
    { -904,  -864, M_FIRE_SMALL },
    { -136,  -912, M_FIRE_SMALL },
    { 184,   608,  M_FIRE_SMALL },
    // clang-format on
};

// Lists the places where Lara catches fire. The original game lists the last
// one at (0, 768), but measures the z distance with the x offset, so it lies
// at (0, 0).
static const XZ_16 m_DeadlyFires[] = {
    { .x = -512, .z = -512 },
    { .x = 0, .z = 0 },
    { .x = 512, .z = 512 },
    { .x = 0, .z = 0 },
};

static RESULT M_LoadPriv(ITEM *const item, JSON_READ_IO *const io)
{
    M_PRIV *const p = item->priv;
    MUST(JSON_READ_OPT(io, "timer", &p->timer));
    MUST(JSON_READ_OPT(io, "fade_small", &p->fades[M_FIRE_SMALL]));
    MUST(JSON_READ_OPT(io, "fade_medium", &p->fades[M_FIRE_MEDIUM]));
    MUST(JSON_READ_OPT(io, "fade_big", &p->fades[M_FIRE_BIG]));
    MUST(JSON_READ_OPT(io, "light", &p->light));
    return OK;
}

static void M_SavePriv(const ITEM *const item, JSON_WRITE_IO *const io)
{
    const M_PRIV *const p = item->priv;
    JSONW_WRITE(io, "timer", p->timer);
    JSONW_WRITE(io, "fade_small", p->fades[M_FIRE_SMALL]);
    JSONW_WRITE(io, "fade_medium", p->fades[M_FIRE_MEDIUM]);
    JSONW_WRITE(io, "fade_big", p->fades[M_FIRE_BIG]);
    JSONW_WRITE(io, "light", p->light);
}

static bool M_IsTorchOnFloor(const ITEM *const item, const ITEM *const torch)
{
    SPHERE spheres[M_MAX_SPHERES];
    const int32_t count = Collide_GetSpheres(item, spheres, true);
    for (int32_t i = 0; i < count; i++) {
        const SPHERE *const sphere = &spheres[i];
        if (sphere->r <= 0) {
            continue;
        }
        const XYZ_32 diff = XYZ_32_Subtract(sphere->pos, torch->pos);
        if (SQUARE(diff.x) + SQUARE(diff.y) + SQUARE(diff.z)
            < SQUARE(sphere->r + M_TORCH_REACH)) {
            return true;
        }
    }
    return false;
}

// Catches fire from a burning torch that lies still on the floor, and takes
// the torch away.
static void M_CatchTorch(const ITEM *const item, M_PRIV *const p)
{
    int16_t torch_num = Room_Get(item->room_num)->item_num;
    while (torch_num != NO_ITEM) {
        const ITEM *const torch = Item_Get(torch_num);
        if (torch->object_id == O_BURNING_TORCH_ITEM && torch->speed == 0
            && torch->fall_speed == 0 && BurningTorch_IsLit(torch)
            && M_IsTorchOnFloor(item, torch)) {
            p->timer = 1;
            Item_Destroy(torch_num);
            return;
        }
        torch_num = torch->next_item;
    }
}

static void M_AddFires(const ITEM *const item, const M_PRIV *const p)
{
    for (int32_t i = 0; i < (int32_t)ARRAY_SIZE(m_Fires); i++) {
        const M_FIRE *const fire = &m_Fires[i];
        const int32_t fade = p->fades[fire->size];
        if (fade == 0) {
            continue;
        }
        const XYZ_32 pos = {
            .x = item->pos.x + fire->x,
            .y = item->pos.y - (fire->size << 6) - 64,
            .z = item->pos.z + fire->z,
        };
        FX_Fire_Add(pos, fire->size, item->room_num, fade);
    }
}

// Sets Lara on fire where she stands in the flames, and makes the floor give
// way at once.
static void M_BurnLara(const ITEM *const item, M_PRIV *const p)
{
    if (Lara_GetLaraInfo()->burn) {
        return;
    }

    ITEM *const lara_item = Lara_GetItem();
    for (int32_t i = 0; i < (int32_t)ARRAY_SIZE(m_DeadlyFires); i++) {
        const XZ_16 *const fire = &m_DeadlyFires[i];
        const int32_t dx = ABS(item->pos.x + fire->x - lara_item->pos.x);
        const int32_t dy = ABS(item->pos.y - lara_item->pos.y);
        const int32_t dz = ABS(item->pos.z + fire->z - lara_item->pos.z);
        if (dx < M_DEADLY_RANGE && dy < M_DEADLY_RANGE && dz < M_DEADLY_RANGE) {
            Lara_CatchFire();
            lara_item->hit_points = M_BURN_HIT_POINTS;
            p->timer = M_COLLAPSE_TIME;
            for (int32_t j = 0; j < M_FIRE_SIZES; j++) {
                p->fades[j] = M_FADE_MIN;
            }
            Output_Flash_Trigger((RGB_888) { .r = 255, .g = 64, .b = 0 });
            return;
        }
    }
}

static void M_Spread(M_PRIV *const p)
{
    p->fades[M_FIRE_SMALL] += 4;
    if (p->timer > M_MEDIUM_TIME) {
        p->fades[M_FIRE_MEDIUM] += 4;
    }
    if (p->timer > M_BIG_TIME) {
        p->fades[M_FIRE_BIG] += 8;
    }
    for (int32_t i = 0; i < M_FIRE_SIZES; i++) {
        CLAMPG(p->fades[i], M_FADE_MAX);
    }
    p->timer++;
    p->light = M_LIGHT_START - p->timer / M_LIGHT_STEP_TIME;
}

// Returns whether every fire has died down.
static bool M_DieDown(M_PRIV *const p)
{
    p->fades[M_FIRE_SMALL] -= 4;
    p->fades[M_FIRE_MEDIUM] -= 3;
    p->fades[M_FIRE_BIG] -= 2;
    bool is_out = true;
    for (int32_t i = 0; i < M_FIRE_SIZES; i++) {
        CLAMPL(p->fades[i], M_FADE_MIN);
        is_out &= p->fades[i] == M_FADE_MIN;
    }
    return is_out;
}

static void M_Collapse(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    Room_FlipMap(0);
    for (int32_t i = 0; i <= M_LAST_MESH; i++) {
        const int16_t faces = i == M_LAST_MESH ? M_LAST_DEBRIS : M_DEBRIS;
        if (FX_Debris_ShatterItemMesh(item, i, faces, 1, false)) {
            Sound_Effect(SFX_HIT_ROCK, &item->pos, SPM_NORMAL);
        }
    }
    Item_Destroy(item_num);
}

static void M_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    M_PRIV *const p = item->priv;
    if (p->timer == 0) {
        M_CatchTorch(item, p);
        return;
    }

    M_AddFires(item, p);
    M_BurnLara(item, p);
    if (p->timer < M_COLLAPSE_TIME) {
        M_Spread(p);
    } else if (M_DieDown(p)) {
        M_Collapse(item_num);
    }
}

// Darkens the floor while it burns. The light that is left stays when Lara
// sets off the collapse herself, as in the original game.
static float M_GetAmbientScale(const ITEM *const item)
{
    const M_PRIV *const p = item->priv;
    if (p->light == 0) {
        return 1.0f;
    }
    return (p->light + 1) / 128.0f;
}

static void M_Setup(OBJECT *const obj)
{
    obj->control_func = M_Control;
    obj->get_ambient_scale_func = M_GetAmbientScale;
    obj->priv_size = sizeof(M_PRIV);
    obj->priv_load_func = M_LoadPriv;
    obj->priv_save_func = M_SavePriv;
    obj->save_position = true;
    obj->save_flags = true;
}

REGISTER_OBJECT(O_BURNING_FLOOR, M_Setup)
