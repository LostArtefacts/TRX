#include <trx/game/objects/general/pickup.h>

#include <trx/config.h>
#include <trx/core/json/util/read_io.h>
#include <trx/core/json/util/write_io.h>
#include <trx/game/camera.h>
#include <trx/game/effects.h>
#include <trx/game/game.h>
#include <trx/game/gun.h>
#include <trx/game/input.h>
#include <trx/game/inventory.h>
#include <trx/game/inventory_ring/control.h>
#include <trx/game/lara.h>
#include <trx/game/lua.h>
#include <trx/game/objects/families.h>
#include <trx/game/objects/general/flare_item.h>
#include <trx/game/objects/links.h>
#include <trx/game/output.h>
#include <trx/game/overlay.h>
#include <trx/game/random.h>
#include <trx/game/rooms.h>
#include <trx/game/savegame.h>
#include <trx/game/sparks.h>
#include <trx/game/stats.h>
#include <trx/version.h>

// clang-format off
#define M_LF_PICKUP_ERASE        42
#define M_LF_FAST_PICKUP_ERASE   15
#define M_LF_PICKUP_FLARE        58
#define M_LF_PICKUP_FLARE_UW     20
#define M_LF_PICKUP_UW           18
#define M_LF_PICKUP_CROUCH_1     20
#define M_LF_PICKUP_CROUCH_2     22
#define M_LF_PICKUP_CROUCH_FLARE 22
#define M_LF_PICKUP_CRAWL        20
#define M_LF_PICKUP_PLINTH_LOW   29
#define M_LF_PICKUP_PLINTH_HIGH  45
#define M_LF_PICKUP_HIDDEN       42
#define M_LF_PICKUP_CROWBAR      123
#define M_AID_DIST_MIN           (STEP_L * 5)      // 1280
#define M_AID_DIST_MAX           (WALL_L * 8)      // 8192
#define M_AID_WAIT_MIN           (LOGIC_FPS * 2.5) // 75
#define M_AID_WAIT_MAX           (LOGIC_FPS * 5)   // 150
#define M_AID_WAIT_BREAK_CHANCE  0x1200

typedef struct {
    int32_t aid_timer;
    uint32_t secret_mask;
    PICKUP_MODE pickup_mode;
    bool animate;
    bool show_pickup_aid;
    bool snap_to_sector;
    bool keep_simulated;
    int16_t rotation;
    RGB_888 glow_color;
} M_PRIV;

// clang-format on

static const OBJECT_BOUNDS m_PickUpBounds = {
    .shift = {
        .min = { .x = -WALL_L / 4, .y = -100, .z = -WALL_L / 4, },
        .max = { .x = +WALL_L / 4, .y = +100, .z = +WALL_L / 4, },
    },
    .rot = {
        .min = { .x = -10 * DEG_1, .y = 0, .z = 0, },
        .max = { .x = +10 * DEG_1, .y = 0, .z = 0, },
    },
};

static const OBJECT_BOUNDS m_PickUpBoundsControlled = {
    .shift = {
        .min = { .x = -WALL_L / 4, .y = -200, .z = -WALL_L / 4, },
        .max = { .x = +WALL_L / 4, .y = +200, .z = +WALL_L / 4, },
    },
    .rot = {
        .min = { .x = -10 * DEG_1, .y = 0, .z = 0, },
        .max = { .x = +10 * DEG_1, .y = 0, .z = 0, },
    },
};

static const OBJECT_BOUNDS m_PickUpBoundsUW = {
    .shift = {
        .min = { .x = -WALL_L / 2, .y = -WALL_L / 2, .z = -WALL_L / 2, },
        .max = { .x = +WALL_L / 2, .y = +WALL_L / 2, .z = +WALL_L / 2, },
    },
    .rot = {
        .min = { .x = -45 * DEG_1, .y = -45 * DEG_1, .z = -45 * DEG_1, },
        .max = { .x = +45 * DEG_1, .y = +45 * DEG_1, .z = +45 * DEG_1, },
    },
};

static const OBJECT_BOUNDS m_PlinthBounds = {
    .shift = {
        .min = { .x = -256, .y = -640, .z = -511, },
        .max = { .x = +256, .y = +640, .z = 320, },
    },
    .rot = {
        .min = { .x = -10 * DEG_1, .y = -30 * DEG_1, .z = 0, },
        .max = { .x = +10 * DEG_1, .y = +30 * DEG_1, .z = 0, },
    },
};

static const OBJECT_BOUNDS m_ScionBounds = {
    .shift = {
        .min = { .x = -256, .y = +640 - 100, .z = -350, },
        .max = { .x = +256, .y = +640 + 100, .z = -200, },
    },
    .rot = {
        .min = { .x = -10 * DEG_1, .y = 0, .z = 0, },
        .max = { .x = +10 * DEG_1, .y = 0, .z = 0, },
    },
};

static const OBJECT_BOUNDS m_HiddenPickupBounds = {
    .shift = {
        .min = { .x = -STEP_L, .y = -100, .z = -800, },
        .max = { .x = +STEP_L, .y = +100, .z = STEP_L, },
    },
    .rot = {
        .min = { .x = -10 * DEG_1, .y = -30 * DEG_1, .z = 0, },
        .max = { .x = +10 * DEG_1, .y = +30 * DEG_1, .z = 0, },
    },
};

static const OBJECT_BOUNDS m_CrowbarPickupBounds = {
    .shift = {
        .min = { .x = -STEP_L, .y = -100, .z = 200, },
        .max = { .x = +STEP_L, .y = +100, .z = STEP_L * 2, },
    },
    .rot = {
        .min = { .x = -10 * DEG_1, .y = -30 * DEG_1, .z = 0, },
        .max = { .x = +10 * DEG_1, .y = +30 * DEG_1, .z = 0, },
    },
};

static const XYZ_32 m_PickupPosition = { .x = 0, .y = 0, .z = -100 };
static const XYZ_32 m_PickupPositionUW = { .x = 0, .y = -200, .z = -350 };
static const XYZ_32 m_PickupPositionPlinth = { .x = 0, .y = 0, .z = -380 };
static const XYZ_32 m_PickupPositionScion = { .x = 0, .y = 0, .z = -310 };
static const XYZ_32 m_PickupPositionHidden = { .x = 0, .y = 0, .z = -690 };
static const XYZ_32 m_PickupPositionCrowbar = { .x = 0, .y = 0, .z = 225 };

static RESULT M_LoadPriv(ITEM *const item, JSON_READ_IO *const io)
{
    M_PRIV *const p = item->priv;
    MUST(JSON_READ_OPT(io, "animate", &p->animate));
    return OK;
}

static void M_SavePriv(const ITEM *const item, JSON_WRITE_IO *const io)
{
    const M_PRIV *const p = item->priv;
    JSONW_WRITE(io, "animate", p->animate);
}

static const char *M_CheckRotation(const TRX_VALUE *const in)
{
    return ABS(in->as_int) > DEG_90 ? "rotation is beyond a quarter turn"
                                    : nullptr;
}

static void M_Initialise(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    M_PRIV *const p = item->priv;
    p->aid_timer = -1;
    p->secret_mask = 0;

    if (ObjectFamily_Has(item->object_id, OBJ_FAMILY_SECRET)) {
        const GF_LEVEL *const level = Game_GetCurrentLevel();
        p->secret_mask = Stats_GetSecretMaskForItem(level, item_num);
    }

    if (item->is_visible) {
        Item_AddSimulated(item_num);
    }
}

static const char *M_CheckPickupMode(const TRX_VALUE *const in)
{
    return in->as_int < 0 || in->as_int >= PICKUP_MODE_NUMBER_OF
        ? "no such pickup mode"
        : nullptr;
}

static void M_HandleSave(ITEM *const item, const SAVEGAME_STAGE stage)
{
    if (stage == SAVEGAME_STAGE_AFTER_LOAD) {
        if (item->is_finished) {
            const int16_t item_num = Item_GetIndex(item);
            Item_DetachFromRoom(item_num);
        }
    }
}

static bool M_Trigger(ITEM *const item, const ITEM_TRIGGER *const trigger)
{
    const M_PRIV *const p = item->priv;
    if (trigger->kind == ITEM_TRIGGER_SWITCH) {
        item->trigger.mask ^= trigger->mask;
    } else if (trigger->kind == ITEM_TRIGGER_ANTI) {
        item->trigger.mask &= ~trigger->mask;
    } else {
        item->trigger.mask |= trigger->mask;
    }

    if (item->trigger.mask != TRIGGER_MASK_ALL) {
        Item_SetVisible(item, false);
        item->is_destroyed = true;
    } else if (!item->is_visible || p->keep_simulated) {
        item->touch_bits = 0;
        Item_SetVisible(item, true);
        Item_AddSimulated(Item_GetIndex(item));
    }

    return false;
}

static void M_SpawnPickupAid(const ITEM *const item)
{
    const OBJECT_ID obj_id =
        ObjectLink_Get(item->object_id, OBJ_LINK_ITEM_TO_OPTION);
    if (obj_id == NO_OBJECT) {
        return;
    }

    const OBJECT *const obj = Object_Get(obj_id);
    const ANIM_FRAME *const frame = obj->frame_base;
    if (!obj->loaded || frame == nullptr) {
        return;
    }

    bool item_visible = false;
    for (int32_t i = 0; i < Room_DrawGetCount(); i++) {
        const int16_t draw_room_num = Room_DrawGetRoom(i);
        if (draw_room_num == item->room_num) {
            item_visible = true;
            break;
        }
    }
    if (!item_visible) {
        return;
    }

    const XYZ_32 pos = {
        .x = item->pos.x + 20 * (Random_GetDraw() - 0x4000) / 0x4000,
        .y = item->pos.y - ABS(frame->bounds.max.y - frame->bounds.min.y)
            - 10 * (1 + (Random_GetDraw() - 0x4000) / 0x4000),
        .z = item->pos.z + 20 * (Random_GetDraw() - 0x4000) / 0x4000,
    };

    if (g_TRVersion >= 3) {
        for (int32_t i = 0; i < (Random_GetControl() & 3) + 4; i++) {
            Sparks_TriggerPickupAid(pos, (XZ_32) {});
        }
    } else {
        const int16_t effect_num = Effect_Create(item->room_num);
        if (effect_num != NO_EFFECT) {
            EFFECT *const effect = Effect_Get(effect_num);
            effect->pos = pos;
            effect->counter = 0;
            effect->object_id = O_PICKUP_AID;
            effect->frame_num = 0;
        }
    }
}

static void M_ControlPickupAids(ITEM *const item)
{
    const ITEM *const lara = Lara_GetItem();
    if (item->fall_speed != 0 || lara == nullptr
        || !Object_Get(O_PICKUP_AID)->loaded) {
        return;
    }

    const int32_t distance = Item_GetDistance(lara, item->pos);
    if (distance < M_AID_DIST_MIN || distance > M_AID_DIST_MAX) {
        return;
    }

    M_PRIV *const p = item->priv;
    int32_t timer = p->aid_timer;
    if (timer <= 0
        || (timer < M_AID_WAIT_MIN
            && Random_GetDraw() < M_AID_WAIT_BREAK_CHANCE)) {
        M_SpawnPickupAid(item);
        timer = M_AID_WAIT_MAX;
    } else {
        timer--;
    }

    p->aid_timer = timer;
}

static void M_ControlPickupLights(ITEM *const item)
{
    const M_PRIV *const p = item->priv;
    if (p->glow_color.r == 0 && p->glow_color.g == 0 && p->glow_color.b == 0) {
        return;
    }

    const int16_t timer = Output_GetTimeInGame();
    const int16_t angle = Math_Cos((timer & 0x3F) << 10);
    int32_t c = ABS(angle >> 9);
    CLAMPG(c, 31);
    c <<= 3;

    // Using 0xF8 rather than 0xFF allows for achieving the exact curve present
    // with OG TR3's quest items.
#define L_GLOW(channel, intensity) (((channel) * (intensity)) / 0xF8)
    const RGB_888 color = {
        .r = L_GLOW(p->glow_color.r, c),
        .g = L_GLOW(p->glow_color.g, c),
        .b = L_GLOW(p->glow_color.b, c),
    };
    Output_AddDynamicLightRGB(item->pos, 8, color);

#undef L_GLOW
}

static bool M_CanShowPickupAid(const M_PRIV *const p)
{
    switch (p->pickup_mode) {
    case PICKUP_MODE_HIDDEN:
    case PICKUP_MODE_CROWBAR:
    case PICKUP_MODE_SARCOPHAGUS:
        return false;
    default:
        return p->show_pickup_aid;
    }
}

static void M_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    if (!item->is_visible || item->is_finished) {
        Item_RemoveSimulated(item_num);
        return;
    }

    if (item->room_num == NO_ROOM) {
        return;
    }

    M_PRIV *const p = item->priv;
    if (p->animate) {
        if (Lara_GetLaraInfo()->interact_target.item_num == item_num) {
            Item_Animate(item);
        } else {
            Item_SwitchToAnim(item, 0, 0);
            p->animate = false;
        }
    }

    if (!Item_IsInactive(item)) {
        item->rot.y += p->rotation;
        M_ControlPickupLights(item);
    }

    if (g_Config.gameplay.enable_pickup_aids && M_CanShowPickupAid(p)) {
        M_ControlPickupAids(item);
    }
}

static bool M_IsPickupEraseFrame(const ITEM *const lara_item)
{
    const LARA_ANIMATION_ID anim = LA_U(Item_GetRelativeAnim(lara_item));
    const int16_t frame = Item_GetRelativeFrame(lara_item);
    switch (anim) {
    case LA_PICKUP:
        return frame == M_LF_PICKUP_ERASE;
    case LA_FAST_PICKUP:
        return frame == M_LF_FAST_PICKUP_ERASE;
    case LA_CROUCH_PICKUP:
        return frame == M_LF_PICKUP_CROUCH_1 || frame == M_LF_PICKUP_CROUCH_2;
    case LA_CRAWL_PICKUP:
        return frame == M_LF_PICKUP_CRAWL;
    case LA_UNDERWATER_PICKUP:
        return frame == M_LF_PICKUP_UW;
    case LA_FLARE_PICKUP:
        return frame == M_LF_PICKUP_FLARE;
    case LA_CROUCH_PICKUP_FLARE:
        return frame == M_LF_PICKUP_CROUCH_FLARE;
    case LA_UNDERWATER_FLARE_PICKUP:
        return frame == M_LF_PICKUP_FLARE_UW;
    case LA_PLINTH_LOW_PICKUP:
        return frame == M_LF_PICKUP_PLINTH_LOW;
    case LA_PLINTH_HIGH_PICKUP:
        return frame == M_LF_PICKUP_PLINTH_HIGH;
    case LA_HOLE_GRAB:
        return frame == M_LF_PICKUP_HIDDEN;
    case LA_CROWBAR_USE_ON_WALL:
        return frame == M_LF_PICKUP_CROWBAR;
    default:
        return false;
    }
}

static void M_DoPickup(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    if (item->object_id == O_FLARE_ITEM) {
        return;
    }

    Overlay_AddDisplayPickup(item->object_id);
    if (ObjectFamily_Has(item->object_id, OBJ_FAMILY_SECRET)) {
        Stats_MarkSecretCollected(item);
        if (Stats_CheckAllLevelSecretsCollected()) {
            GF_InventoryModifier_Apply(Game_GetCurrentLevel(), GF_INV_SECRET);
        }
    } else {
        Inv_AddItem(item->object_id);
    }
    Stats_AddPickup();
    // Notify Lua pickup listeners
    LUA_FireEventInt32(LUA_EVENT_PICKUP, item_num);

    Item_SetVisible(item, false);
    Item_Destroy(item_num);

    LARA_INFO *const lara = Lara_GetLaraInfo();
    lara->interact_target.is_moving = false;
}

static void M_DoFlarePickup(const int16_t item_num)
{
    const ITEM *const item = Item_Get(item_num);
    LARA_INFO *const lara = Lara_GetLaraInfo();
    lara->request_gun_type = Gun_GetFlareType();
    lara->gun_type = lara->request_gun_type;
    Gun_InitialiseNewWeapon();
    lara->gun_status = LGS_SPECIAL;
    lara->flare.age = FlareItem_GetAge(item);
    Item_Destroy(item_num);
    lara->interact_target.is_moving = false;
}

static void M_CollectAllAtPos(
    const XYZ_32 pos, const int16_t room_num, const PICKUP_MODE mode)
{
    int16_t pickup_num = Room_Get(room_num)->item_num;
    while (pickup_num != NO_ITEM) {
        const ITEM *const check_item = Item_Get(pickup_num);
        const int16_t next_item_num = check_item->next_item;
        if (Object_Get(check_item->object_id)->collision_func
                != Pickup_Collision
            || check_item->object_id == O_FLARE_ITEM
            || check_item->pos.x != pos.x || check_item->pos.z != pos.z) {
            goto loop_end;
        }

        const M_PRIV *const p = check_item->priv;
        if (p->pickup_mode == mode) {
            M_DoPickup(pickup_num);
        }

    loop_end:
        pickup_num = next_item_num;
    }
}

static bool M_UseMultiplePickups(const ITEM *const item)
{
    const M_PRIV *const p = item->priv;
    return g_Config.gameplay.enable_multiple_pickups
        || p->pickup_mode == PICKUP_MODE_HIDDEN;
}

static void M_Collect(const ITEM *const item, const bool controlled)
{
    const int16_t item_num = Item_GetIndex(item);
    if (item->object_id == O_FLARE_ITEM) {
        M_DoFlarePickup(item_num);
    } else if (M_UseMultiplePickups(item) && controlled) {
        const M_PRIV *const p = item->priv;
        M_CollectAllAtPos(item->pos, item->room_num, p->pickup_mode);
    } else {
        M_DoPickup(item_num);
    }

    const LARA_INFO *const lara = Lara_GetLaraInfo();
    if (item->object_id == O_FLARE_ITEM
        && (lara->water_status == LWS_UNDERWATER
            || lara->water_status == LWS_CHEAT)) {
        Gun_Flare_DrawMeshes();
    }
}

static bool M_CanCollect(
    const ITEM *const item, const ITEM *const lara_item, const bool controlled)
{
    const LARA_INFO *const lara = Lara_GetLaraInfo();
    if (!M_IsPickupEraseFrame(lara_item)) {
        return false;
    }

    if (lara->interact_target.item_num == Item_GetIndex(item)) {
        return true;
    }

    if (lara->interact_target.item_num == NO_ITEM) {
        return false;
    }

    if (item->object_id == O_FLARE_ITEM) {
        return lara->interact_target.item_num == NO_ITEM;
    }

    if (lara_item->current_anim_state == LS(LS_FLARE_PICKUP)) {
        return false;
    }

    if (controlled || !M_UseMultiplePickups(item)) {
        return false;
    }

    const ITEM *const current_item = Item_Get(lara->interact_target.item_num);
    const M_PRIV *const p1 = current_item->priv;
    const M_PRIV *const p2 = item->priv;
    return p1->pickup_mode == p2->pickup_mode;
}

static void M_BeginScionAnimation(const ITEM *const item, ITEM *const lara_item)
{
    // Animated interactions require a more lenient scion position while moving
    // Lara, but during the cinematic sequence itself she should be aligned as
    // per non-controlled movement.
    XYZ_32 pos = m_PickupPositionScion;
    pos.y = lara_item->pos.y - item->pos.y;
    Lara_AlignPosition(item, &pos);
    Lara_SwitchToExtraState(LS_EXTRA_SCION_PICKUP_1);
    Camera_InvokeCinematic(lara_item, 0, 0);
}

static void M_BeginPickupAnimation(const ITEM *const item, const bool is_ducked)
{
    LARA_STATE_ID goal_state;
    LARA_STATE_ID required_state = LS_PICKUP;
    ITEM *const lara_item = Lara_GetItem();

    if (item->object_id == O_FLARE_ITEM) {
        goal_state = LS_FLARE_PICKUP;
        required_state = LS_FLARE_PICKUP;
    } else if (is_ducked) {
        goal_state = LS_PICKUP;
    } else {
        M_PRIV *const p = item->priv;
        switch (p->pickup_mode) {
        case PICKUP_MODE_PLINTH_LOW:
            goal_state = LS_PLINTH_LOW_PICKUP;
            break;
        case PICKUP_MODE_PLINTH_HIGH:
            goal_state = LS_PLINTH_HIGH_PICKUP;
            break;
        case PICKUP_MODE_HIDDEN:
            goal_state = LS_HIDDEN_PICKUP;
            required_state = LS_HIDDEN_PICKUP;
            break;
        case PICKUP_MODE_CROWBAR:
            goal_state = LS_CROWBAR_PICKUP;
            p->animate = true;
            break;
        case PICKUP_MODE_PLINTH_SCION:
            M_BeginScionAnimation(item, lara_item);
            return;
        default:
            goal_state = g_Config.gameplay.enable_fast_pickups ? LS_FAST_PICKUP
                                                               : LS_PICKUP;
            break;
        }
    }

    if (!is_ducked) {
        Item_SwitchToAnim(lara_item, LA(LA_STAND_STILL), 0);
    }

    lara_item->goal_anim_state = LS(goal_state);
    do {
        Lara_Animate(lara_item);
    } while (lara_item->current_anim_state != LS(required_state));
}

static const BOUNDS_16 *M_FindPlinthBounds(const ITEM *const item)
{
    const ROOM *const room = Room_Get(item->room_num);
    const BOUNDS_16 *const item_bounds = Item_GetBoundsAccurate(item);
    for (int32_t i = 0; i < room->num_static_meshes; i++) {
        const STATIC_MESH *const mesh = &room->static_meshes[i];
        const STATIC_OBJECT_3D *const obj =
            Object_Get3DStatic(mesh->static_num);
        if (!obj->visible || mesh->pos.x != item->pos.x
            || mesh->pos.z != item->pos.z) {
            continue;
        }

        const BOUNDS_16 *const obj_bounds = &obj->collision_bounds;
        if (item_bounds->min.x <= obj_bounds->max.x
            && item_bounds->max.x >= obj_bounds->min.x
            && item_bounds->min.z <= obj_bounds->max.z
            && item_bounds->max.z >= obj_bounds->min.z
            && (obj_bounds->min.x != 0 || obj_bounds->max.x != 0)) {
            return obj_bounds;
        }
    }

    int16_t item_num = room->item_num;
    while (item_num != NO_ITEM) {
        const ITEM *const plinth_item = Item_Get(item_num);
        item_num = plinth_item->next_item;
        const OBJECT *const obj = Object_Get(plinth_item->object_id);
        if (obj->collision_func == Pickup_Collision) {
            continue;
        }

        if (item->pos.x == plinth_item->pos.x
            && item->pos.y <= plinth_item->pos.y
            && item->pos.z == plinth_item->pos.z) {
            return Item_GetBoundsAccurate(plinth_item);
        }
    }

    return nullptr;
}

static bool M_TestLaraPosition(const ITEM *const item, const bool controlled)
{
    OBJECT_BOUNDS test_bounds = *Object_Get(item->object_id)->bounds_func();
    if (item->object_id == O_FLARE_ITEM) {
        goto finish;
    }

    const M_PRIV *const p = item->priv;
    switch (p->pickup_mode) {
    case PICKUP_MODE_HIDDEN:
        test_bounds = m_HiddenPickupBounds;
        break;
    case PICKUP_MODE_CROWBAR:
        test_bounds = m_CrowbarPickupBounds;
        break;
    case PICKUP_MODE_PLINTH_LOW:
    case PICKUP_MODE_PLINTH_HIGH:
    case PICKUP_MODE_PLINTH_SCION: {
        if (p->pickup_mode == PICKUP_MODE_PLINTH_SCION && !controlled) {
            test_bounds = m_ScionBounds;
            break;
        }

        const ITEM *const lara_item = Lara_GetItem();
        const int32_t delta = lara_item->pos.y - item->pos.y;
        const int32_t offset =
            STEP_L * (p->pickup_mode == PICKUP_MODE_PLINTH_LOW ? 2 : 3);
        if (ABS(ABS(delta) - offset) > STEP_L / 2) {
            return false;
        }

        test_bounds = m_PlinthBounds;
        test_bounds.shift.max.y = delta + 100;
        const BOUNDS_16 *const plinth_bounds = M_FindPlinthBounds(item);
        if (plinth_bounds != nullptr) {
            test_bounds.shift.min.x = plinth_bounds->min.x;
            test_bounds.shift.max.x = plinth_bounds->max.x;
            test_bounds.shift.max.z = plinth_bounds->max.z;
        }
        break;
    }
    default:
        break;
    }

finish:
    return Lara_TestPosition(item, &test_bounds);
}

static XYZ_32 M_GetAlignmentPosition(
    const ITEM *const item, const bool controlled)
{
    XYZ_32 pos;
    if (item->object_id == O_FLARE_ITEM) {
        pos = m_PickupPosition;
        goto finish;
    }

    const M_PRIV *const p = item->priv;
    switch (p->pickup_mode) {
    case PICKUP_MODE_PLINTH_LOW:
    case PICKUP_MODE_PLINTH_HIGH:
    case PICKUP_MODE_PLINTH_SCION:
        if (p->pickup_mode == PICKUP_MODE_PLINTH_SCION && !controlled) {
            pos = m_PickupPositionScion;
            break;
        }

        pos = m_PickupPositionPlinth;
        const BOUNDS_16 *const plinth_bounds = M_FindPlinthBounds(item);
        if (plinth_bounds != nullptr) {
            pos.z = -200 - plinth_bounds->max.z;
        }
        break;
    case PICKUP_MODE_HIDDEN:
        pos = m_PickupPositionHidden;
        break;
    case PICKUP_MODE_CROWBAR:
        pos = m_PickupPositionCrowbar;
        break;
    default:
        pos = m_PickupPosition;
        break;
    }

finish:
    pos.y = Lara_GetItem()->pos.y - item->pos.y;
    return pos;
}

static bool M_LaraHasPickupState(const ITEM *const lara_item)
{
    return lara_item->current_anim_state == LS(LS_PICKUP)
        || lara_item->current_anim_state == LS(LS_HIDDEN_PICKUP)
        || lara_item->current_anim_state == LS(LS_FLARE_PICKUP);
}

static XYZ_16 M_PrepareAndCacheRot(
    ITEM *const item, const ITEM *const lara_item, const bool controlled)
{
    // Items are rotated to match Lara before performing alignment tests.
    // Non-controlled mode accounts for Lara being tilted e.g. in crawl state,
    // and particular pickup modes expect Y snapping.
    const XYZ_16 old_rot = item->rot;

    if (controlled) {
        item->rot.x = 0;
        item->rot.z = 0;
    } else {
        item->rot.x = lara_item->rot.x;
        item->rot.z = lara_item->rot.z;
    }

    if (item->object_id == O_FLARE_ITEM) {
        item->rot.y = lara_item->rot.y;
    } else {
        const M_PRIV *const p = item->priv;
        if (p->pickup_mode != PICKUP_MODE_HIDDEN
            && p->pickup_mode != PICKUP_MODE_CROWBAR) {
            item->rot.y = lara_item->rot.y;
        }
    }

    return old_rot;
}

static bool M_ShowCrowbarInventory(void)
{
    if (!Inv_HasItem(O_CROWBAR_ITEM)) {
        return false;
    }

    InvRing_SetRequestedObjectID(O_CROWBAR_OPTION);
    const GF_COMMAND gf_cmd = GF_ShowInventory(INV_KEYS_MODE);
    if (gf_cmd.action != GF_NOOP) {
        GF_OverrideCommand(gf_cmd);
    }
    return true;
}

static bool M_CanBeginPickup(const ITEM *const item)
{
    if (item->object_id == O_FLARE_ITEM) {
        return true;
    }
    const M_PRIV *const p = item->priv;
    const LARA_INFO *const lara = Lara_GetLaraInfo();
    if (p->pickup_mode != PICKUP_MODE_CROWBAR
        || lara->interact_target.is_moving) {
        return true;
    }

    return M_ShowCrowbarInventory()
        && Lara_Interact_HasActiveTarget(Item_GetIndex(item));
}

static void M_DoControlled(const int16_t item_num, ITEM *const lara_item)
{
    ITEM *const item = Item_Get(item_num);
    const XYZ_16 old_rot = M_PrepareAndCacheRot(item, lara_item, true);

    LARA_INFO *const lara = Lara_GetLaraInfo();
    if (Lara_Interact_CanControl(LARA_INTERACT_PICKUP, item_num)) {
        if (M_TestLaraPosition(item, true)) {
            if (!M_CanBeginPickup(item)) {
                goto cleanup;
            }

            if (lara_item->current_anim_state == LS(LS_STOP)) {
                lara->interact_target.is_moving = false;
            }

            const XYZ_32 pos = M_GetAlignmentPosition(item, true);
            if (Lara_MovePosition(item, &pos)) {
                M_BeginPickupAnimation(item, false);
                Lara_Interact_FinishControl(LARA_INTERACT_PICKUP);
            }
            lara->interact_target.item_num = item_num;
        } else if (Lara_Interact_HasActiveTarget(item_num)) {
            lara->interact_target.is_moving = false;
            lara->interact_target.item_num = NO_ITEM;
            lara->gun_status = LGS_ARMLESS;
        }

        goto cleanup;
    } else if (
        !M_LaraHasPickupState(lara_item) && !lara->interact_target.is_moving
        && lara->interact_target.item_num == item_num) {
        lara->interact_target.item_num = NO_ITEM;
    }

    if (M_LaraHasPickupState(lara_item)) {
        if (M_CanCollect(item, lara_item, true)) {
            M_Collect(item, true);
        }
        goto cleanup;
    }

cleanup:
    item->rot = old_rot;
}

static void M_DoAboveWater(const int16_t item_num, ITEM *const lara_item)
{
    ITEM *const item = Item_Get(item_num);
    const LARA_ANIMATION_ID anim = LA_U(Item_GetRelativeAnim(lara_item));

    // clang-format off
    const bool is_ducked = (
        anim == LA_CRAWL_IDLE ||
        anim == LA_CRAWL_PICKUP ||
        anim == LA_CROUCH_IDLE ||
        anim == LA_CROUCH_PICKUP ||
        anim == LA_CROUCH_PICKUP_FLARE);
    // clang-format on

    if (g_Config.gameplay.enable_walk_to_items && !is_ducked) {
        M_DoControlled(item_num, lara_item);
        return;
    }

    if (item->object_id != O_FLARE_ITEM) {
        const M_PRIV *const p = item->priv;
        if (is_ducked && p->pickup_mode != PICKUP_MODE_NORMAL) {
            return;
        }
    }

    const XYZ_16 old_rot = M_PrepareAndCacheRot(item, lara_item, false);

    if (!M_TestLaraPosition(item, false)) {
        goto cleanup;
    }

    if (M_LaraHasPickupState(lara_item)) {
        if (M_CanCollect(item, lara_item, false)) {
            M_Collect(item, false);
        }
        goto cleanup;
    }

    const bool is_flare_item = item->object_id == O_FLARE_ITEM;
    if (g_Input.action && lara_item->current_anim_state == LS(LS_CRAWL_IDLE)
        && (is_flare_item || !g_Config.gameplay.enable_responsive_crawl)) {
        lara_item->goal_anim_state = LS(LS_CROUCH_IDLE);
        goto cleanup;
    }

    LARA_INFO *const lara = Lara_GetLaraInfo();
    if ((g_Input.action || Lara_Interact_HasActiveTarget(item_num))
        && !lara_item->gravity
        && (lara->gun_status == LGS_ARMLESS || anim == LA_CRAWL_IDLE)
        && Lara_Interact_CanBegin(LARA_INTERACT_PICKUP)) {
        if (is_flare_item) {
            if (g_TRVersion >= 4) {
                const XYZ_32 pos = M_GetAlignmentPosition(item, false);
                Lara_AlignPosition(item, &pos);
            }
            Lara_AnimateUntil(lara_item, LS(LS_FLARE_PICKUP));
        } else if (!M_CanBeginPickup(item)) {
            goto cleanup;
        } else {
            const XYZ_32 pos = M_GetAlignmentPosition(item, false);
            Lara_AlignPosition(item, &pos);
            M_BeginPickupAnimation(item, is_ducked);
        }
        if (is_ducked) {
            lara_item->goal_anim_state =
                LS(anim == LA_CRAWL_IDLE ? LS_CRAWL_IDLE : LS_CROUCH_IDLE);
        } else {
            lara_item->goal_anim_state = LS(LS_STOP);
        }
        Lara_Interact_FinishControl(LARA_INTERACT_PICKUP);
        lara->interact_target.item_num = item_num;
        goto cleanup;
    }

cleanup:
    item->rot = old_rot;
}

static void M_DoUnderwater(const int16_t item_num, ITEM *const lara_item)
{
    ITEM *const item = Item_Get(item_num);
    const OBJECT *const obj = Object_Get(item->object_id);
    const XYZ_16 old_rot = item->rot;

    item->rot.x = -25 * DEG_1;
    item->rot.y = lara_item->rot.y;
    item->rot.z = 0;

    if (!Lara_TestPosition(item, obj->bounds_func())) {
        goto cleanup;
    }

    if (M_LaraHasPickupState(lara_item)) {
        if (M_CanCollect(item, lara_item, false)) {
            M_Collect(item, false);
        }
        goto cleanup;
    }

    LARA_INFO *const lara = Lara_GetLaraInfo();
    if (g_Input.action && lara_item->current_anim_state == LS(LS_TREAD)
        && lara->gun_status == LGS_ARMLESS) {
        if (!Lara_MovePosition(item, &m_PickupPositionUW)) {
            goto cleanup;
        }

        if (item->object_id == O_FLARE_ITEM) {
            lara_item->fall_speed = 0;
            Item_SwitchToAnim(lara_item, LA(LA_UNDERWATER_FLARE_PICKUP), 0);
            lara_item->current_anim_state = LS(LS_FLARE_PICKUP);
        } else {
            if (g_Config.gameplay.fix_lara_pickup_embed) {
                lara_item->fall_speed = 0;
            }
            Lara_AnimateUntil(lara_item, LS(LS_PICKUP));
        }
        lara_item->goal_anim_state = LS(LS_TREAD);
        lara->interact_target.item_num = item_num;
        goto cleanup;
    }

cleanup:
    item->rot = old_rot;
}

static bool M_CanCollide(const int16_t item_num)
{
    const ITEM *const item = Item_Get(item_num);
    if (item->trigger.spent) {
        return false;
    }

    if (item->object_id == O_FLARE_ITEM) {
        return !Gun_IsFlareType(Lara_GetLaraInfo()->gun_type);
    }

    const M_PRIV *const p = item->priv;
    return p->pickup_mode != PICKUP_MODE_SARCOPHAGUS;
}

static bool M_Draw(const ITEM *const item)
{
    const M_PRIV *const p = item->priv;
    if (p->pickup_mode == PICKUP_MODE_CROWBAR) {
        return Object_DrawAnimatingItem(item);
    }
    return Object_DrawPickupItem(item);
}

static void M_Setup(OBJECT *const obj)
{
    obj->trigger_func = M_Trigger;
    obj->control_func = M_Control;
    obj->collision_func = Pickup_Collision;
    obj->bounds_func = Pickup_Bounds;
    obj->draw_func = M_Draw;
    obj->initialise_func = M_Initialise;
    obj->handle_save_func = M_HandleSave;
    obj->priv_size = sizeof(M_PRIV);
    obj->priv_load_func = M_LoadPriv;
    obj->priv_save_func = M_SavePriv;
    obj->save_position = true;
    obj->save_flags = true;
    obj->save_anim = true;

    OBJECT_PROPERTIES(
        obj,
        OBJECT_PROPERTY_CHECKED(
            M_PRIV, pickup_mode, PICKUP_MODE_NORMAL, M_CheckPickupMode,
            "Pickup animation mode - 0: normal; 1: low pedestal; 2: high "
            "pedestal; 3: hidden reach-in; 4: crowbar; 5: hidden sarcophagus; "
            "6: scion pedestal."),
        OBJECT_PROPERTY(
            M_PRIV, show_pickup_aid, true,
            "Show a twinkle effect above the item; applies only to normal and "
            "pedestal pickup modes."),
        OBJECT_PROPERTY(
            M_PRIV, snap_to_sector, true,
            "Move the item to the middle of its sector where a carrier drops "
            "it."),
        OBJECT_PROPERTY(
            M_PRIV, keep_simulated, false,
            "Simulate the item again once it becomes visible, so that it goes "
            "on rotating, glowing and showing its twinkle after a carrier "
            "drops it or a trigger brings it back."),
        OBJECT_PROPERTY_CHECKED(
            M_PRIV, rotation, 0, M_CheckRotation,
            "How much to rotate the item by each frame while it's active, in "
            "engine angle units. Value range: "
            "minimum -16384; maximum 16384."),
        OBJECT_PROPERTY(
            M_PRIV, glow_color, ((RGB_888) { 0, 0, 0 }),
            "The color of the item's glow while it's active. Black infers no "
            "glow."));
}

const OBJECT_BOUNDS *Pickup_Bounds(void)
{
    const LARA_INFO *const lara = Lara_GetLaraInfo();
    if (lara->water_status == LWS_UNDERWATER
        || lara->water_status == LWS_CHEAT) {
        return &m_PickUpBoundsUW;
    } else if (g_Config.gameplay.enable_walk_to_items) {
        return &m_PickUpBoundsControlled;
    } else {
        return &m_PickUpBounds;
    }
}

uint32_t Pickup_GetSecretMask(const ITEM *const item)
{
    const M_PRIV *const p = item->priv;
    return p->secret_mask;
}

bool Pickup_Trigger(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    // is_visible prevents the trigger activating before the item has been
    // collected, while is_finished prevents it running more than once.
    if (item->is_visible || item->is_finished) {
        return false;
    }

    Item_SetFinished(item, true);
    return true;
}

void Pickup_Collision(
    const int16_t item_num, ITEM *const lara_item, COLL_INFO *const coll)
{
    if (!M_CanCollide(item_num)) {
        return;
    }

    const LARA_INFO *const lara = Lara_GetLaraInfo();
    if (lara->water_status == LWS_ABOVE_WATER
        || lara->water_status == LWS_WADE) {
        M_DoAboveWater(item_num, lara_item);
    } else if (
        lara->water_status == LWS_UNDERWATER
        || lara->water_status == LWS_CHEAT) {
        M_DoUnderwater(item_num, lara_item);
    }
}

int16_t Pickup_FindNearbyCrowbarPryPickup(void)
{
    for (int16_t item_num = 0; item_num < Item_GetLevelCount(); item_num++) {
        const ITEM *const item = Item_Get(item_num);
        if (Object_Get(item->object_id)->collision_func != Pickup_Collision
            || item->object_id == O_FLARE_ITEM
            || !Lara_IsNearItem(&item->pos, WALL_L) || !Item_IsInPlay(item)) {
            continue;
        }

        const M_PRIV *const p = item->priv;
        if (p->pickup_mode == PICKUP_MODE_CROWBAR
            && M_TestLaraPosition(item, true)) {
            return item_num;
        }
    }
    return NO_ITEM;
}

void Pickup_Collect(const GAME_VECTOR pos, const PICKUP_MODE mode)
{
    M_CollectAllAtPos(pos.pos, pos.room_num, mode);
}

// O_FLARE_ITEM registers its own specialized setup.
#define X_PICKUP(item, option) REGISTER_OBJECT(item, M_Setup)
#define X_PICKUP_SPECIAL(item, option) REGISTER_OBJECT(item, M_Setup)
#define X_PICKUP_SUPPLY_VARIANT(item, option)
#include <trx/game/objects/pickups.def>
#undef X_PICKUP_SUPPLY_VARIANT
#undef X_PICKUP_SPECIAL
#undef X_PICKUP
