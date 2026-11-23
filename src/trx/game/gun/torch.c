#include <trx/game/gun/torch.h>

#include <trx/config.h>
#include <trx/game/collision.h>
#include <trx/game/gun/common.h>
#include <trx/game/gun/flare.h>
#include <trx/game/gun/routines.h>
#include <trx/game/input.h>
#include <trx/game/items.h>
#include <trx/game/lara.h>
#include <trx/game/objects/general/burning_torch.h>
#include <trx/game/output.h>
#include <trx/game/random.h>
#include <trx/game/rooms.h>
#include <trx/game/sparks.h>

// clang-format off
#define M_DROP_START        1
#define M_DROP_CANCEL_LAST  11
#define M_DROP_RELEASE      12
#define M_DROP_END          27
#define M_THROW_START       31
#define M_THROW_RELEASE     36
#define M_THROW_END         41
#define M_ITEM_HAND_OFFSET  80
#define M_LIGHT_FRAME       40
// clang-format on

typedef enum {
    M_ARM_HOLD = 0,
    M_ARM_DROP = 1,
    M_ARM_THROW = 2,
    M_ARM_LIGHT = 3,
    M_ARM_DOUSE = 4,
} M_ARM_STATE;

typedef enum {
    M_ANIM_HOLD = 0,
    M_ANIM_DROP = 1,
    M_ANIM_THROW = 2,
} M_ANIM;

static const LARA_STATE_ID m_JumpStates[] = {
    // clang-format off
    LS_COMPRESS,
    LS_JUMP_UP,
    LS_JUMP_FORWARD,
    LS_JUMP_BACK,
    LS_JUMP_RIGHT,
    LS_JUMP_LEFT,
    NO_CATALOG_ID,
    // clang-format on
};

static const LARA_ANIMATION_ID m_LightAnims[] = {
    // clang-format off
    LA_TORCH_LIGHT_1,
    LA_TORCH_LIGHT_2,
    LA_TORCH_LIGHT_3,
    LA_TORCH_LIGHT_4,
    LA_TORCH_LIGHT_5,
    NO_CATALOG_ID,
    // clang-format on
};

static const OBJECT_BOUNDS m_FireBounds = {
    .shift = {
        .min = { .x = -256, .y = 0, .z = -800, },
        .max = { .x = +256, .y = WALL_L, .z = +800, },
    },
    .rot = {
        .min = { .x = -10 * DEG_1, .y = -30 * DEG_1, .z = -10 * DEG_1, },
        .max = { .x = +10 * DEG_1, .y = +30 * DEG_1, .z = +10 * DEG_1, },
    },
};

static int32_t m_FlameCount = 0;

static void M_SetArm(const M_ANIM anim_idx, const int16_t frame_num)
{
    LARA_INFO *const lara = Lara_GetLaraInfo();
    const OBJECT *const obj = Object_Get(Gun_GetLaraAnim(LGT_TORCH));
    lara->left_arm.anim_num = obj->anim_idx + anim_idx;
    lara->left_arm.frame_num = frame_num;
}

static bool M_CanDrop(void)
{
    const ITEM *const lara_item = Lara_GetItem();
    return g_Input.draw && !lara_item->gravity && lara_item->fall_speed == 0
        && !Lara_HasState(m_JumpStates);
}

// Leaves the torch in the world where Lara's hand holds it, or at her feet
// where that place is inside a wall or another object.
static void M_CreateItem(const bool thrown)
{
    const ITEM *const lara_item = Lara_GetItem();
    const LARA_INFO *const lara = Lara_GetLaraInfo();

    const int16_t item_num = Item_Create();
    if (item_num == NO_ITEM) {
        return;
    }

    ITEM *const item = Item_Get(item_num);
    item->object_id = O_BURNING_TORCH_ITEM;
    item->room_num = lara_item->room_num;

    XYZ_32 pos = { .x = -16, .y = 32, .z = 42 };
    if (!Lara_GetMeshPos(LM_HAND_L, &pos)) {
        pos = lara_item->pos;
    }
    item->pos = pos;

    int16_t room_num = lara_item->room_num;
    const SECTOR *const sector = Room_GetSector(pos, &room_num);
    COLLIDED_OBJECTS found;
    Collide_GetCollidedObjects(item, 0, true, false, &found);
    const bool collided = found.item_count > 0 || found.static_count > 0
        || Room_GetHeight(sector, pos) < pos.y;
    if (collided) {
        item->rot.y = lara_item->rot.y + DEG_180;
        item->pos =
            XYZ_32_OffsetYaw(lara_item->pos, item->rot.y, M_ITEM_HAND_OFFSET);
        item->pos.y = pos.y;
        item->room_num = lara_item->room_num;
    } else {
        item->rot.y = thrown ? lara_item->rot.y : lara_item->rot.y - DEG_45;
        item->room_num = room_num;
    }

    Item_Initialise(item_num);
    item->rot.x = 0;
    item->rot.z = 0;
    item->shade.value_1 = -1;

    if (thrown) {
        item->speed = lara_item->speed + 50;
        item->fall_speed = lara_item->fall_speed - 50;
    } else {
        item->speed = lara_item->speed + 10;
        item->fall_speed = lara_item->fall_speed + 50;
    }
    if (collided) {
        item->speed >>= 1;
    }

    BurningTorch_SetLit(item, lara->lit_torch);
    BurningTorch_Drop(item);
}

static void M_Release(const bool thrown)
{
    Lara_Skin_ClearEquipment(LM_HAND_L);
    M_CreateItem(thrown);
}

static void M_Finish(void)
{
    LARA_INFO *const lara = Lara_GetLaraInfo();
    lara->flare.control = false;
    lara->lit_torch = false;
    lara->left_arm.lock = M_ARM_HOLD;
    lara->gun_status = LGS_ARMLESS;
}

static void M_ControlArm(void)
{
    LARA_INFO *const lara = Lara_GetLaraInfo();
    const ITEM *const lara_item = Lara_GetItem();

    switch (lara->left_arm.lock) {
    case M_ARM_HOLD:
        if (lara->request_gun_type != lara->gun_type) {
            lara->left_arm.lock = M_ARM_THROW;
            M_SetArm(M_ANIM_THROW, M_THROW_START);
        } else if (M_CanDrop() || lara->water_status == LWS_UNDERWATER) {
            lara->left_arm.lock = M_ARM_DROP;
            M_SetArm(M_ANIM_DROP, M_DROP_START);
            if (lara->water_status == LWS_UNDERWATER) {
                lara->lit_torch = false;
            }
        }
        break;

    case M_ARM_DROP:
        if (lara->left_arm.frame_num <= M_DROP_CANCEL_LAST
            && lara_item->gravity) {
            lara->left_arm.lock = M_ARM_HOLD;
            M_SetArm(M_ANIM_HOLD, 0);
            break;
        }
        lara->left_arm.frame_num++;
        if (lara->left_arm.frame_num == M_DROP_END) {
            M_Finish();
            lara->gun_type = lara->last_gun_type;
            lara->request_gun_type = LGT_UNARMED;
        } else if (lara->left_arm.frame_num == M_DROP_RELEASE) {
            M_Release(true);
        }
        break;

    case M_ARM_THROW:
        lara->left_arm.frame_num++;
        if (lara->left_arm.frame_num == M_THROW_END) {
            M_Finish();
            lara->last_gun_type = LGT_UNARMED;
            lara->gun_type = LGT_UNARMED;
        } else if (lara->left_arm.frame_num == M_THROW_RELEASE) {
            M_Release(false);
        }
        break;

    case M_ARM_LIGHT:
    case M_ARM_DOUSE:
        if (lara_item->current_anim_state != LS(LS_CONTROLLED)) {
            if (lara->left_arm.lock == M_ARM_LIGHT) {
                lara->lit_torch = true;
            }
            lara->flare.control = true;
            lara->left_arm.lock = M_ARM_HOLD;
            M_SetArm(M_ANIM_HOLD, 0);
        }
        break;

    default:
        break;
    }
}

static void M_Burn(void)
{
    const LARA_INFO *const lara = Lara_GetLaraInfo();
    if (!lara->lit_torch) {
        return;
    }

    XYZ_32 pos = { .x = -32, .y = 64, .z = 256 };
    if (g_Config.visuals.enable_fire_lighting
        && Lara_GetMeshPos(LM_HAND_L, &pos)) {
        const RGB_888 color = {
            .r = (Random_GetControl() & 0x3F) + 192,
            .g = (Random_GetControl() & 0x1F) + 96,
            .b = 0,
        };
        Output_AddDynamicLightRGB(pos, 12 - (Random_GetControl() & 1), color);
    }

    m_FlameCount++;
    if ((m_FlameCount & 1) == 0) {
        Sparks_TriggerTorchFlame(
            Item_GetIndex(Lara_GetItem()), SPARK_NODE_LARA_TORCH);
    }
}

static void M_Control(
    const LARA_GUN_TYPE gun_type, const LARA_GUN_STATE gun_status)
{
    LARA_INFO *const lara = Lara_GetLaraInfo();
    M_ControlArm();
    if (lara->flare.control) {
        lara->gun_status = LGS_READY;
    }
    const ANIM *const anim = Anim_GetAnim(lara->left_arm.anim_num);
    lara->left_arm.frame_base = anim->frame_ptr;
    M_Burn();
}

static void M_DrawMeshes(const LARA_GUN_TYPE gun_type)
{
    Lara_Skin_SetExtraEquipment(LM_HAND_L, EXTRA_MESH_WOODEN_TORCH);
}

// Whether the fire burns, which the torch either takes its flame from or
// gives its flame to, or whether the sprinkler runs.
static bool M_IsFireLit(const ITEM *const item)
{
    return item->is_simulated && Item_IsTriggerActiveRO(item);
}

static bool M_CanStartLighting(const ITEM *const fire_item)
{
    const LARA_INFO *const lara = Lara_GetLaraInfo();
    const ITEM *const lara_item = Lara_GetItem();
    return lara->gun_type == LGT_TORCH && lara->gun_status == LGS_READY
        && lara->left_arm.lock == M_ARM_HOLD
        && M_IsFireLit(fire_item) != lara->lit_torch && g_Input.action
        && lara_item->current_anim_state == LS(LS_STOP)
        && Item_TestAnimEqual(lara_item, LA(LA_STAND_IDLE))
        && !lara_item->gravity;
}

static void M_StartLighting(const int16_t item_num, ITEM *const lara_item)
{
    ITEM *const item = Item_Get(item_num);
    const XYZ_16 old_rot = item->rot;
    item->rot.y = lara_item->rot.y;
    OBJECT_BOUNDS bounds = m_FireBounds;
    if (item->object_id == O_FLAME_EMITTER_TR4_WALL) {
        bounds.shift.min.z = -600;
        bounds.shift.max.z = +600;
    } else if (item->object_id == O_SPRINKLER) {
        bounds.shift.min.z = -384;
        bounds.shift.max.z = 0;
    }
    if (Lara_TestPosition(item, &bounds)) {
        if (item->object_id == O_SPRINKLER) {
            Item_SwitchToAnim(lara_item, LA(LA_TORCH_LIGHT_4), 0);
        } else {
            const int32_t height = ABS(lara_item->pos.y - item->pos.y) >> 8;
            Item_SwitchToAnim(lara_item, LA(m_LightAnims[MIN(height, 4)]), 0);
        }
        lara_item->current_anim_state = LS(LS_CONTROLLED);
        LARA_INFO *const lara = Lara_GetLaraInfo();
        lara->flare.control = false;
        lara->left_arm.lock =
            item->object_id == O_SPRINKLER ? M_ARM_DOUSE : M_ARM_LIGHT;
        lara->interact_target.item_num = item_num;
    }
    item->rot = old_rot;
}

// Lights the fire from the torch at the moment Lara's animation holds the
// torch into it, and sets off the triggers beneath the fire. A sprinkler
// starts instead and puts the torch out.
static void M_LightFire(const int16_t item_num, const ITEM *const lara_item)
{
    ITEM *const item = Item_Get(item_num);
    LARA_INFO *const lara = Lara_GetLaraInfo();
    if (lara->interact_target.item_num != item_num || M_IsFireLit(item)
        || lara_item->current_anim_state != LS(LS_CONTROLLED)
        || !Lara_HasAnimation(m_LightAnims)
        || Item_GetRelativeFrame(lara_item) != M_LIGHT_FRAME) {
        return;
    }

    if (item->object_id == O_SPRINKLER) {
        lara->lit_torch = false;
    }

    Room_TestTriggersEx(item, true);
    item->trigger.mask = TRIGGER_MASK_ALL;
    item->trigger.reversed = false;
    item->timer = 0;
    Item_AddSimulated(item_num);
}

void Gun_Torch_FireCollision(
    const int16_t item_num, ITEM *const lara_item, COLL_INFO *const coll)
{
    if (M_CanStartLighting(Item_Get(item_num))) {
        M_StartLighting(item_num, lara_item);
    }
    M_LightFire(item_num, lara_item);
}

void Gun_Torch_Take(const bool is_lit)
{
    LARA_INFO *const lara = Lara_GetLaraInfo();
    if (Gun_IsFlareType(lara->gun_type)) {
        Gun_Flare_Dispose(false);
    }
    lara->request_gun_type = LGT_TORCH;
    lara->gun_type = LGT_TORCH;
    lara->gun_status = LGS_READY;
    lara->flare.control = true;
    lara->lit_torch = is_lit;
    lara->left_arm.lock = M_ARM_HOLD;
    M_SetArm(M_ANIM_HOLD, 0);
    const ANIM *const anim = Anim_GetAnim(lara->left_arm.anim_num);
    lara->left_arm.frame_base = anim->frame_ptr;
    M_DrawMeshes(LGT_TORCH);
}

// clang-format off
REGISTER_GUN_KIND(
    WEAPON_TYPE_TORCH,
    .draw_meshes_func = M_DrawMeshes,
    .control_func = M_Control,
    .owns_frame = true,
    .is_held_item = true)
// clang-format on
