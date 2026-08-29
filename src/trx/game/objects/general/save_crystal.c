#include <trx/game/objects/general/save_crystal.h>

#include <trx/config.h>
#include <trx/core/json/util/read_io.h>
#include <trx/core/json/util/write_io.h>
#include <trx/game/catalog/manager.h>
#include <trx/game/input.h>
#include <trx/game/inventory.h>
#include <trx/game/lara.h>
#include <trx/game/lara/poison.h>
#include <trx/game/objects/common.h>
#include <trx/game/objects/general/pickup.h>
#include <trx/game/objects/property.h>
#include <trx/game/output.h>
#include <trx/game/rooms.h>
#include <trx/game/sound.h>
#include <trx/game/stats.h>
#include <trx/version.h>

// The crystal injection gives TR1 and TR2 one mesh per tint: the PC crystal,
// the PS1 tint, and the heal crystal. TR3's own crystal is green, so its
// injection keeps that as mesh 0 and adds a blue copy for the other modes.
#define M_MESH_PC 0
#define M_MESH_PS1 1
#define M_MESH_HEAL 2
#define M_MESH_TR3_HEAL 0
#define M_MESH_TR3_SAVE 1

typedef struct {
    int32_t mesh_index;
    int32_t heal_amount;
    RGB_888 glow_color;
    bool bob;
    bool initialised;
    bool counted_for_stats;
    bool used_for_save;
    int16_t initial_angle;
} M_PRIV;

static int32_t m_ConfigListener = -1;

static const OBJECT_BOUNDS m_SaveCrystal_Bounds = {
    .shift = {
        .min = { .x = -STEP_L*3/2, .y = -100, .z = -STEP_L*3/2, },
        .max = { .x = +STEP_L*3/2, .y = +WALL_L, .z = +STEP_L*3/2, },
    },
    .rot = {
        .min = { .x = -DEG_45, .y = 0, .z = 0, },
        .max = { .x = +DEG_45, .y = 0, .z = 0, },
    },
};

static const OBJECT_BOUNDS m_UW_Bounds = {
    .shift = {
        .min = { .x = -STEP_L*3/2, .y = -WALL_L, .z = -STEP_L*3/2, },
        .max = { .x = +STEP_L*3/2, .y = +WALL_L, .z = +STEP_L*3/2, },
    },
    .rot = {
        .min = { .x = -DEG_90, .y = 0, .z = 0, },
        .max = { .x = +DEG_90, .y = 0, .z = 0, },
    },
};

static const LARA_STATE_ID m_StopStates[] = {
    // clang-format off
    LS_STOP,
    LS_TREAD,
    LS_SURF_TREAD,
    NO_CATALOG_ID,
    // clang-format on
};

static RESULT M_LoadPriv(ITEM *const item, JSON_READ_IO *const io)
{
    M_PRIV *const p = item->priv;
    MUST(JSON_READ_OPT(io, "counted_for_stats", &p->counted_for_stats));
    MUST(JSON_READ_OPT(io, "used_for_save", &p->used_for_save));
    MUST(JSON_READ_OPT(io, "initialised", &p->initialised));
    MUST(JSON_READ_OPT(io, "initial_angle", &p->initial_angle));
    return OK;
}

static void M_SavePriv(const ITEM *const item, JSON_WRITE_IO *const io)
{
    const M_PRIV *const p = item->priv;
    JSONW_WRITE(io, "counted_for_stats", p->counted_for_stats);
    JSONW_WRITE(io, "used_for_save", p->used_for_save);
    JSONW_WRITE(io, "initialised", p->initialised);
    JSONW_WRITE(io, "initial_angle", p->initial_angle);
}

static void M_CountCrystal(M_PRIV *const p)
{
    if (p->counted_for_stats) {
        return;
    }
    p->counted_for_stats = true;
    Stats_AddCrystal();
}

static const OBJECT_BOUNDS *M_Bounds(void)
{
    const LARA_INFO *const lara = Lara_GetLaraInfo();
    return lara->water_status == LWS_ABOVE_WATER
            || lara->water_status == LWS_WADE
        ? &m_SaveCrystal_Bounds
        : &m_UW_Bounds;
}

// Every crystal needs this from the start: a crystal that is never simulated
// would otherwise keep the default mesh_bits and draw all of its tints at once.
static void M_ApplyMesh(ITEM *const item)
{
    const M_PRIV *const p = item->priv;
    item->mesh_bits = SaveCrystal_GetMeshBits(item->object_id, p->mesh_index);
}

static void M_Initialise(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    M_PRIV *const p = item->priv;
    p->initialised = false;
    p->counted_for_stats = false;
    p->used_for_save = false;
    p->initial_angle = 0;

    M_ApplyMesh(item);

    if (g_Config.gameplay.save_crystal_mode != SAVE_CRYSTAL_OFF
        && g_TRVersion != 3) {
        // TR3 places its crystals with the code bits already set, so the item
        // manager activates them; the injected TR1/TR2 ones need it done here.
        Item_AddSimulated(item_num);
    }
}

static void M_HandleSave(ITEM *const item, const SAVEGAME_STAGE stage)
{
    switch (stage) {
    case SAVEGAME_STAGE_AFTER_LOAD:
        // The save carries the mesh the crystal had when it was written, which
        // the mode may since have changed.
        M_ApplyMesh(item);
        if (item->is_finished) {
            const int16_t item_num = Item_GetIndex(item);
            Item_DetachFromRoom(item_num);
        }
        break;

    case SAVEGAME_STAGE_BEFORE_SAVE:
        M_PRIV *const p = item->priv;
        if (p->used_for_save) {
            // need to reset the crystal status
            Item_SetFinished(item, true);
            p->used_for_save = false;
            const int16_t item_num = Item_GetIndex(item);
            Item_DetachFromRoom(item_num);
        }

    default:
        break;
    }
}

// The light matches the crystal: green for healing, blue otherwise. TR1 and
// TR2 crystals cast no light, as in the original games.
static RGB_888 M_GetModeGlow(void)
{
    if (g_TRVersion != 3) {
        return (RGB_888) { 0, 0, 0 };
    }
    return g_Config.gameplay.save_crystal_mode == SAVE_CRYSTAL_HEAL
        ? (RGB_888) { 0, 0xFF, 0 }
        : (RGB_888) { 0, 0x40, 0xFF };
}

static void M_AddGlow(const ITEM *const item, const int32_t intensity)
{
    const M_PRIV *const p = item->priv;
    RGB_888 color = p->glow_color;
    if (color.r == 0 && color.g == 0 && color.b == 0) {
        color = M_GetModeGlow();
    }
    if (color.r == 0 && color.g == 0 && color.b == 0) {
        return;
    }

    Output_AddDynamicLightRGB(
        item->pos, 8,
        (RGB_888) {
            .r = (color.r * intensity) >> 8,
            .g = (color.g * intensity) >> 8,
            .b = (color.b * intensity) >> 8,
        });
}

// TR1 and TR2 crystals rest on the floor and spin through the model's own
// animation. TR3 crystals hover and bob, and spin by hand.
static void M_Animate(ITEM *const item)
{
    M_PRIV *const p = item->priv;
    if (!p->bob) {
        Item_Animate(item);
        M_AddGlow(item, 0xFF);
        return;
    }

    if (!p->initialised) {
        p->initialised = true;
        p->initial_angle = item->pos.y;
    }

    item->rot.y += 1024;
    const int32_t timer = Output_GetTimeInGame();
    const int16_t angle = Math_Cos((timer & 0x3F) << 10);
    int32_t c = ABS(angle >> 9);
    CLAMPG(c, 31);
    c <<= 3;

    item->pos.y = p->initial_angle - ABS(angle >> 6) - 64;
    M_AddGlow(item, c);
}

// Heal and pickup crystals are taken as soon as Lara reaches them.
static bool M_TestProximity(const ITEM *const item, const ITEM *const lara_item)
{
    const int32_t dx = ABS(item->pos.x - lara_item->pos.x);
    const int32_t dy = ABS(item->pos.y - lara_item->pos.y);
    const int32_t dz = ABS(item->pos.z - lara_item->pos.z);
    return dx < STEP_L && dy < WALL_L && dz < STEP_L;
}

static void M_Heal(ITEM *const lara_item, const ITEM *const item)
{
    const M_PRIV *const p = item->priv;
    Lara_Poison_Cure();
    lara_item->hit_points += p->heal_amount;
    CLAMPG(lara_item->hit_points, LARA_MAX_HITPOINTS);

    // PS1: SFX_SAVE_CRYSTAL, PC: SFX_MENU_MEDI
    Sound_Effect(SFX_MENU_MEDI, &lara_item->pos, SPM_ALWAYS);
}

static void M_Collect(const ITEM *const lara_item)
{
    Inv_AddItem(O_SAVE_CRYSTAL_ITEM);
    Sound_Effect(SFX_SAVE_CRYSTAL, &lara_item->pos, SPM_ALWAYS);
}

static void M_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);

    // A crystal spent on a save stays simulated, so without this it would keep
    // casting its light from where it stood.
    if (!item->is_visible || item->is_finished || item->clear_body) {
        return;
    }

    M_ApplyMesh(item);
    M_Animate(item);

    const SAVE_CRYSTAL_MODE mode = g_Config.gameplay.save_crystal_mode;
    if (mode != SAVE_CRYSTAL_HEAL && mode != SAVE_CRYSTAL_PICKUP
        && mode != SAVE_CRYSTAL_SAVE_PICKUP) {
        return;
    }

    ITEM *const lara_item = Lara_GetItem();
    if (!M_TestProximity(item, lara_item)) {
        return;
    }

    M_PRIV *const p = item->priv;
    M_CountCrystal(p);
    if (mode == SAVE_CRYSTAL_HEAL) {
        M_Heal(lara_item, item);
    } else {
        M_Collect(lara_item);
    }
    Item_Destroy(item_num);
}

static void M_Collision(
    const int16_t item_num, ITEM *const lara_item, COLL_INFO *const coll)
{
    ITEM *const item = Item_Get(item_num);
    const OBJECT *const obj = Object_Get(item->object_id);

    Object_Collision(item_num, lara_item, coll);

    const LARA_INFO *const lara = Lara_GetLaraInfo();
    if (!g_Input.action || lara->gun_status != LGS_ARMLESS
        || lara_item->gravity) {
        return;
    }

    if (!Lara_HasState(m_StopStates)) {
        return;
    }

    item->rot.y = lara_item->rot.y;
    item->rot.z = 0;
    item->rot.x = 0;
    if (!Lara_TestPosition(item, obj->bounds_func())) {
        return;
    }

    if (g_Config.flow.load_save_disabled) {
        Lara_RefuseInteraction();
        return;
    }

    int16_t room_num = lara_item->room_num;
    const XYZ_32 pos = lara_item->pos;
    const SECTOR *const sector = Room_GetSector(pos, &room_num);
    const int32_t ceiling = Room_GetCeiling(sector, pos);
    const int32_t floor = Room_GetHeight(sector, pos);
    if (ceiling >= item->pos.y || floor < item->pos.y) {
        return;
    }

    M_PRIV *const p = item->priv;
    M_CountCrystal(p);
    p->used_for_save = true;
    GF_ShowInventory(INV_SAVE_CRYSTAL_MODE);
}

static void M_ApplyMode(OBJECT *const obj)
{
    const SAVE_CRYSTAL_MODE mode = g_Config.gameplay.save_crystal_mode;
    const bool enabled = mode != SAVE_CRYSTAL_OFF;

    obj->draw_func = enabled ? Object_DrawAnimatingItem : nullptr;
    obj->control_func = enabled ? M_Control : nullptr;
    obj->collision_func = mode == SAVE_CRYSTAL_SAVE ? M_Collision : nullptr;
    obj->save_flags = enabled;
    if (g_TRVersion != 3) {
        Object_SetReflective(O_SAVE_CRYSTAL_ITEM, enabled);
        Object_SetReflective(O_SAVE_CRYSTAL_OPTION, enabled);
    }
}

static void M_ApplyModeToLevel(void)
{
    OBJECT *const obj = Object_Get(O_SAVE_CRYSTAL_ITEM);
    if (!obj->loaded) {
        return;
    }

    M_ApplyMode(obj);

    for (int32_t item_num = 0; item_num < Item_GetTotalCount(); item_num++) {
        ITEM *const item = Item_Get(item_num);
        if (item->object_id != O_SAVE_CRYSTAL_ITEM) {
            continue;
        }
        M_ApplyMesh(item);
        Item_AddSimulated(item_num);
    }
}

static void M_HandleConfigChange(const EVENT *const event, void *const data)
{
    if (Config_Change_HasMirror(
            event->data, &g_Config.gameplay.save_crystal_mode)) {
        M_ApplyModeToLevel();
    }
}

static void M_Setup(OBJECT *const obj)
{
    obj->initialise_func = M_Initialise;
    obj->handle_save_func = M_HandleSave;
    obj->priv_load_func = M_LoadPriv;
    obj->priv_save_func = M_SavePriv;
    obj->priv_size = sizeof(M_PRIV);
    obj->bounds_func = M_Bounds;

    OBJECT_PROPERTIES(
        obj,
        OBJECT_PROPERTY(
            M_PRIV, bob, (bool)(g_TRVersion == 3),
            "Whether the crystal hovers and bobs in place."),
        OBJECT_PROPERTY(
            M_PRIV, glow_color, ((RGB_888) { 0, 0, 0 }),
            "Color of the light the crystal casts. Black picks one from the "
            "save crystal mode."),
        OBJECT_PROPERTY(
            M_PRIV, mesh_index, -1,
            "Mesh the crystal is drawn with. -1 picks one from the save "
            "crystal mode."),
        OBJECT_PROPERTY(
            M_PRIV, heal_amount, LARA_MAX_HITPOINTS / 2,
            "Health restored by a healing crystal."));

    M_ApplyMode(obj);

    if (m_ConfigListener < 0) {
        m_ConfigListener =
            Config_SubscribeChanges(M_HandleConfigChange, nullptr);
    }
}

// The mesh count guard keeps older crystal injections working.
uint32_t SaveCrystal_GetMeshBits(const OBJECT_ID object_id, int32_t mesh_index)
{
    const SAVE_CRYSTAL_MODE mode = g_Config.gameplay.save_crystal_mode;
    if (mesh_index < 0) {
        if (g_TRVersion == 3) {
            mesh_index =
                mode == SAVE_CRYSTAL_HEAL ? M_MESH_TR3_HEAL : M_MESH_TR3_SAVE;
        } else if (mode == SAVE_CRYSTAL_HEAL) {
            mesh_index = M_MESH_HEAL;
        } else if (g_Config.visuals.enable_ps1_crystals) {
            mesh_index = M_MESH_PS1;
        } else {
            mesh_index = M_MESH_PC;
        }
    }

    if (mesh_index >= Object_Get(object_id)->mesh_count) {
        mesh_index = 0;
    }
    return 1 << mesh_index;
}

REGISTER_OBJECT(O_SAVE_CRYSTAL_ITEM, M_Setup)
