#include <trx/game/lara/common.h>

#include <trx/config.h>
#include <trx/debug.h>
#include <trx/game/camera.h>
#include <trx/game/catalog/manager.h>
#include <trx/game/creature.h>
#include <trx/game/game.h>
#include <trx/game/gun.h>
#include <trx/game/inventory.h>
#include <trx/game/lara.h>
#include <trx/game/lara/draw.h>
#include <trx/game/lara/poison.h>
#include <trx/game/lara/pose.h>
#include <trx/game/matrix.h>
#include <trx/game/objects.h>
#include <trx/game/objects/general/door.h>
#include <trx/game/output.h>
#include <trx/game/pathing.h>
#include <trx/game/rooms.h>
#include <trx/game/rope.h>
#include <trx/game/rules.h>
#include <trx/game/savegame.h>
#include <trx/game/sound.h>
#include <trx/game/stats.h>
#include <trx/version.h>

#define M_MOVE_ANIM_VELOCITY 12
#define M_MOVE_SPEED 16
#define M_MOVE_ANGLE (2 * DEG_1) // = 364

static const LARA_TRX_ANIMATION m_InvalidInterpAnims[] = {
    // clang-format off
    LA_JUMP_NEUTRAL_ROLL,
    LA_JUMP_BACK_ROLL_START,
    LA_JUMP_BACK_ROLL_END,
    LA_CONTROLLED_DROP_CONTINUE,
    LA_HANG_TO_JUMP_BACK,
    LA_TRX_INVALID, // sentinel
    // clang-format on
};

static LARA_INFO m_Lara = {};
static ITEM *m_LaraItem = nullptr;
static bool m_Controllable = false;
static int16_t m_DeathCameraTarget = NO_ITEM;
static LARA_EXTRA_STATE m_StartAnimState = LS_EXTRA_BREATH;

static bool M_IsInvalidInterpAnim(const LARA_TRX_ANIMATION anim_idx)
{
    for (int32_t i = 0; m_InvalidInterpAnims[i] != LA_TRX_INVALID; i++) {
        if (m_InvalidInterpAnims[i] == anim_idx) {
            return true;
        }
    }
    return false;
}

static int32_t M_GetStartingHitPoints(void)
{
    if (g_Config.gameplay.disable_healing_between_levels) {
        const GF_LEVEL *const current_level = Game_GetCurrentLevel();
        RESUME_INFO *const resume = Savegame_GetCurrentInfo(current_level);
        if (resume != nullptr) {
            return resume->lara_hitpoints;
        }
    }
    return g_Config.gameplay.start_lara_hitpoints;
}

LARA_INFO *Lara_GetLaraInfo(void)
{
    return &m_Lara;
}

ITEM *Lara_GetItem(void)
{
    return m_LaraItem;
}

void Lara_InitialiseLoad(int16_t item_num)
{
    m_Lara.item_num = item_num;
    if (item_num == NO_ITEM) {
        m_LaraItem = nullptr;
    } else {
        m_LaraItem = Item_Get(item_num);
    }
}

void Lara_Initialise(const GF_LEVEL *const level)
{
    ITEM *const lara_item = Lara_GetItem();
    LARA_INFO *const lara_info = Lara_GetLaraInfo();
    lara_item->is_collidable = false;

    m_Controllable = true;
    m_DeathCameraTarget = NO_ITEM;
    Lara_Vehicle_SetIndex(NO_ITEM);

    lara_item->hit_points = M_GetStartingHitPoints();
    lara_info->gun_item_num = NO_ITEM;
    lara_info->flare.age = 0;
    lara_info->flare.control = false;
    lara_info->flare.frame_num = 0;
    lara_info->calc_fall_speed = 0;
    lara_info->pose_count = 0;
    lara_info->hit_direction = DIR_UNKNOWN;
    lara_info->hit_effect = nullptr;
    lara_info->hit_effect_count = 0;
    lara_info->hit_frame = 0;
    lara_info->air = LARA_MAX_AIR;
    lara_info->sprint_timer = LARA_MAX_SPRINT;
    lara_info->exposure_timer = g_Rules.exposure.max;
    lara_info->water_surface_dist = 100;
    lara_info->death_timer = 0;
    lara_info->dive_timer = 0;
    lara_info->idle_timer = 0;
    lara_info->current.active = 0;
    lara_info->extra_anim = false;
    lara_info->burn = false;
    lara_info->electric = 0;
    lara_info->climb_status = false;
    lara_info->sprinting = false;
    lara_info->mesh_effects = 0;
    lara_info->torso_rot.x = 0;
    lara_info->torso_rot.y = 0;
    lara_info->torso_rot.z = 0;
    lara_info->head_rot.x = 0;
    lara_info->head_rot.y = 0;
    lara_info->head_rot.z = 0;
    lara_info->move_angle = 0;
    lara_info->turn_rate = 0;
    lara_info->target = nullptr;
    lara_info->last_pos = lara_item->pos;
    lara_info->right_arm.flash_gun = 0;
    lara_info->left_arm.flash_gun = 0;
    lara_info->right_arm.lock = 0;
    lara_info->left_arm.lock = 0;
    lara_info->interact_target.is_moving = false;
    lara_info->interact_target.item_num = NO_ITEM;
    lara_info->interact_target.move_count = 0;
    lara_info->rope.index = NO_ROPE;
    lara_info->poison.value = 0;
    lara_info->poison.target = 0;
    lara_info->tr3_smoke_count_l = 0;
    lara_info->tr3_smoke_count_r = 0;
    lara_info->mesh_pos_matrices_valid = false;

    LOT_InitialiseLOT(&lara_info->lot);
    lara_info->lot.setup.step = WALL_L * 20;
    lara_info->lot.setup.drop = -WALL_L * 20;
    lara_info->lot.setup.fly = STEP_L;

    Lara_Skin_Initialise();
    if (level->type == GFL_CUTSCENE) {
        Lara_Mesh_Initialise(level);
        lara_info->gun_status = LGS_ARMLESS;
    } else {
        Lara_InitialiseInventory(level);
    }

    Lara_Control_Initialise(level->type, m_StartAnimState);
}

void Lara_InitialiseInventory(const GF_LEVEL *const level)
{
    Inv_RemoveAllItems();

    LARA_INFO *const lara_info = Lara_GetLaraInfo();
    RESUME_INFO *const resume = Savegame_GetCurrentInfo(level);

    if (resume != nullptr) {
        if (resume->flags.has_binoculars) {
            Inv_AddItem(O_BINOCULARS_ITEM);
        }

        if (resume->flags.has_pistols) {
            Inv_AddItem(O_PISTOL_ITEM);
        }
        lara_info->pistol_ammo.ammo = 1000;

        if (resume->flags.has_magnums) {
            Inv_AddItem(O_MAGNUM_ITEM);
            Item_GlobalReplace(O_MAGNUM_ITEM, O_MAGNUM_AMMO_ITEM);
        } else {
            Inv_AddItemNTimes(
                O_MAGNUM_AMMO_ITEM,
                resume->magnum_ammo / Gun_GetAmmoPickupQuantity(LGT_MAGNUMS));
        }
        lara_info->magnum_ammo.ammo = resume->magnum_ammo;

        if (resume->flags.has_autos) {
            Inv_AddItem(O_AUTOS_ITEM);
            Item_GlobalReplace(O_AUTOS_ITEM, O_AUTOS_AMMO_ITEM);
        } else {
            Inv_AddItemNTimes(
                O_AUTOS_AMMO_ITEM,
                resume->autos_ammo / Gun_GetAmmoPickupQuantity(LGT_AUTOS));
        }
        lara_info->autos_ammo.ammo = resume->autos_ammo;

        if (resume->flags.has_desert_eagle) {
            Inv_AddItem(O_DESERT_EAGLE_ITEM);
            Item_GlobalReplace(O_DESERT_EAGLE_ITEM, O_DESERT_EAGLE_AMMO_ITEM);
        } else {
            Inv_AddItemNTimes(
                O_DESERT_EAGLE_AMMO_ITEM,
                resume->desert_eagle_ammo
                    / Gun_GetAmmoPickupQuantity(LGT_DESERT_EAGLE));
        }
        lara_info->desert_eagle_ammo.ammo = resume->desert_eagle_ammo;

        if (resume->flags.has_uzis) {
            Inv_AddItem(O_UZI_ITEM);
            Item_GlobalReplace(O_UZI_ITEM, O_UZI_AMMO_ITEM);
        } else {
            Inv_AddItemNTimes(
                O_UZI_AMMO_ITEM,
                resume->uzi_ammo / Gun_GetAmmoPickupQuantity(LGT_UZIS));
        }
        lara_info->uzi_ammo.ammo = resume->uzi_ammo;

        if (resume->flags.has_shotgun) {
            Inv_AddItem(O_SHOTGUN_ITEM);
            Item_GlobalReplace(O_SHOTGUN_ITEM, O_SHOTGUN_AMMO_ITEM);
        } else {
            Inv_AddItemNTimes(
                O_SHOTGUN_AMMO_ITEM,
                resume->shotgun_ammo / Gun_GetAmmoPickupQuantity(LGT_SHOTGUN));
        }
        lara_info->shotgun_ammo.ammo = resume->shotgun_ammo;

        if (resume->flags.has_m16) {
            Inv_AddItem(O_M16_ITEM);
            Item_GlobalReplace(O_M16_ITEM, O_M16_AMMO_ITEM);
        } else {
            Inv_AddItemNTimes(
                O_M16_AMMO_ITEM,
                resume->m16_ammo / Gun_GetAmmoPickupQuantity(LGT_M16));
        }
        lara_info->m16_ammo.ammo = resume->m16_ammo;

        if (resume->flags.has_mp5) {
            Inv_AddItem(O_MP5_ITEM);
            Item_GlobalReplace(O_MP5_ITEM, O_MP5_AMMO_ITEM);
        } else {
            Inv_AddItemNTimes(
                O_MP5_AMMO_ITEM,
                resume->mp5_ammo / Gun_GetAmmoPickupQuantity(LGT_MP5));
        }
        lara_info->mp5_ammo.ammo = resume->mp5_ammo;

        if (resume->flags.has_grenade) {
            Inv_AddItem(O_GRENADE_GUN_ITEM);
            Item_GlobalReplace(O_GRENADE_GUN_ITEM, O_GRENADE_AMMO_ITEM);
        } else {
            Inv_AddItemNTimes(
                O_GRENADE_AMMO_ITEM,
                resume->grenade_ammo / Gun_GetAmmoPickupQuantity(LGT_GRENADE));
        }
        lara_info->grenade_ammo.ammo = resume->grenade_ammo;

        if (resume->flags.has_rocket) {
            Inv_AddItem(O_ROCKET_GUN_ITEM);
            Item_GlobalReplace(O_ROCKET_GUN_ITEM, O_ROCKET_AMMO_ITEM);
        } else {
            Inv_AddItemNTimes(
                O_ROCKET_AMMO_ITEM,
                resume->rocket_ammo / Gun_GetAmmoPickupQuantity(LGT_ROCKET));
        }
        lara_info->rocket_ammo.ammo = resume->rocket_ammo;

        if (resume->flags.has_harpoon) {
            Inv_AddItem(O_HARPOON_ITEM);
            Item_GlobalReplace(O_HARPOON_ITEM, O_HARPOON_AMMO_ITEM);
        } else {
            Inv_AddItemNTimes(
                O_HARPOON_AMMO_ITEM,
                resume->harpoon_ammo / Gun_GetAmmoPickupQuantity(LGT_HARPOON));
        }
        lara_info->harpoon_ammo.ammo = resume->harpoon_ammo;

        if (resume->flags.has_crossbow) {
            Inv_AddItem(O_CROSSBOW_ITEM);
            Item_GlobalReplace(O_CROSSBOW_ITEM, O_CROSSBOW_AMMO_1_ITEM);
        } else {
            Inv_AddItemNTimes(
                O_CROSSBOW_AMMO_1_ITEM,
                resume->crossbow_ammo
                    / Gun_GetAmmoPickupQuantity(LGT_CROSSBOW));
        }
        lara_info->crossbow_ammo.ammo = resume->crossbow_ammo;

        if (resume->flags.has_revolver) {
            Inv_AddItem(O_REVOLVER_ITEM);
            Item_GlobalReplace(O_REVOLVER_ITEM, O_REVOLVER_AMMO_ITEM);
        } else {
            Inv_AddItemNTimes(
                O_REVOLVER_AMMO_ITEM,
                resume->revolver_ammo
                    / Gun_GetAmmoPickupQuantity(LGT_REVOLVER));
        }
        lara_info->revolver_ammo.ammo = resume->revolver_ammo;

        Inv_AddItemNTimes(O_SMALL_MEDIPACK_ITEM, resume->small_medipacks);
        Inv_AddItemNTimes(O_LARGE_MEDIPACK_ITEM, resume->large_medipacks);
        Inv_AddItemNTimes(O_FLARE_ITEM, resume->flares);
        Inv_AddItemNTimes(O_SCION_ITEM_1, resume->num_scions);
        Inv_AddItemNTimes(O_QUEST_ITEM_1, resume->num_quest_item_1);
        Inv_AddItemNTimes(O_QUEST_ITEM_2, resume->num_quest_item_2);
        Inv_AddItemNTimes(O_QUEST_ITEM_3, resume->num_quest_item_3);
        Inv_AddItemNTimes(O_QUEST_ITEM_4, resume->num_quest_item_4);
        Inv_AddItemNTimes(O_QUEST_ITEM_5, resume->num_quest_item_5);
        Inv_AddItemNTimes(O_QUEST_ITEM_6, resume->num_quest_item_6);

        if (g_Config.gameplay.remember_gun_status) {
            lara_info->gun_status = resume->gun_status;
            lara_info->gun_type = resume->equipped_gun_type;
        }
        lara_info->last_gun_type = resume->equipped_gun_type;
        lara_info->holsters_gun_type = resume->holsters_gun_type;
        lara_info->back_gun_type = resume->back_gun_type;
    }

    if (!g_Config.gameplay.remember_gun_status
        || m_StartAnimState != LS_EXTRA_BREATH) {
        lara_info->gun_status = LGS_ARMLESS;
        lara_info->gun_type = lara_info->last_gun_type;
    }
    lara_info->request_gun_type = lara_info->last_gun_type;
    Lara_Mesh_Initialise(level);
    Gun_InitialiseNewWeapon();
    Gun_EnsureReady();
}

void Lara_RevertToPistolsIfNeeded(void)
{
    if (g_Config.gameplay.remember_gun_status
        || !Inv_RequestItem(O_PISTOL_ITEM)) {
        return;
    }

    LARA_INFO *const lara_info = Lara_GetLaraInfo();
    lara_info->last_gun_type = LGT_PISTOLS;
    lara_info->holsters_gun_type = LGT_PISTOLS;

    if (lara_info->gun_status != LGS_ARMLESS) {
        lara_info->holsters_gun_type = LGT_UNARMED;
        lara_info->request_gun_type = LGT_PISTOLS;
        lara_info->gun_type = LGT_PISTOLS;
    }
    if (Inv_RequestItem(O_SHOTGUN_ITEM)) {
        lara_info->back_gun_type = LGT_SHOTGUN;
    } else {
        lara_info->back_gun_type = LGT_UNARMED;
    }
    Gun_InitialiseNewWeapon();
    Gun_SetLaraHolsterLMesh(lara_info->holsters_gun_type);
    Gun_SetLaraHolsterRMesh(lara_info->holsters_gun_type);
    Gun_SetLaraBackMesh(lara_info->back_gun_type);
}

void Lara_UseItem(const OBJECT_ID obj_id)
{
    LARA_INFO *const lara_info = Lara_GetLaraInfo();
    ITEM *const lara_item = Lara_GetItem();

    LARA_GUN_TYPE request_gun_type = LGT_UNARMED;
    switch (obj_id) {
    case O_PISTOL_ITEM:
    case O_PISTOL_OPTION:
        request_gun_type = LGT_PISTOLS;
        break;

    case O_SHOTGUN_ITEM:
    case O_SHOTGUN_OPTION:
        request_gun_type = LGT_SHOTGUN;
        break;

    case O_MAGNUM_ITEM:
    case O_MAGNUM_OPTION:
        request_gun_type = LGT_MAGNUMS;
        break;

    case O_AUTOS_ITEM:
    case O_AUTOS_OPTION:
        request_gun_type = LGT_AUTOS;
        break;

    case O_DESERT_EAGLE_ITEM:
    case O_DESERT_EAGLE_OPTION:
        request_gun_type = LGT_DESERT_EAGLE;
        break;

    case O_UZI_ITEM:
    case O_UZI_OPTION:
        request_gun_type = LGT_UZIS;
        break;

    case O_HARPOON_ITEM:
    case O_HARPOON_OPTION:
        request_gun_type = LGT_HARPOON;
        break;

    case O_M16_ITEM:
    case O_M16_OPTION:
        request_gun_type = LGT_M16;
        break;

    case O_MP5_ITEM:
    case O_MP5_OPTION:
        request_gun_type = LGT_MP5;
        break;

    case O_GRENADE_GUN_ITEM:
    case O_GRENADE_GUN_OPTION:
        request_gun_type = LGT_GRENADE;
        break;

    case O_ROCKET_GUN_ITEM:
    case O_ROCKET_GUN_OPTION:
        request_gun_type = LGT_ROCKET;
        break;

    case O_CROSSBOW_ITEM:
    case O_CROSSBOW_OPTION:
        request_gun_type = LGT_CROSSBOW;
        break;

    case O_REVOLVER_ITEM:
    case O_REVOLVER_OPTION:
        request_gun_type = LGT_REVOLVER;
        break;

    case O_FLAREBOX_ITEM:
    case O_FLAREBOX_OPTION:
        lara_info->request_gun_type = LGT_FLARE;
        break;

    case O_BINOCULARS_ITEM:
    case O_BINOCULARS_OPTION:
        Camera_Binoculars_Request();
        break;

    case O_SMALL_MEDIPACK_ITEM:
    case O_SMALL_MEDIPACK_OPTION:
        if ((lara_item->hit_points > 0
             && lara_item->hit_points < LARA_MAX_HITPOINTS)
            || lara_info->poison.value != 0 || lara_info->poison.target != 0) {
            Lara_Poison_Cure();
            lara_item->hit_points += LARA_MAX_HITPOINTS / 2;
            CLAMPG(lara_item->hit_points, LARA_MAX_HITPOINTS);
            Inv_RemoveItem(O_SMALL_MEDIPACK_ITEM);
            Sound_Effect(SFX_MENU_MEDI, nullptr, SPM_ALWAYS);
            Stats_AddMedipacksUsed(0.5);
        }
        break;

    case O_LARGE_MEDIPACK_ITEM:
    case O_LARGE_MEDIPACK_OPTION:
        if ((lara_item->hit_points > 0
             && lara_item->hit_points < LARA_MAX_HITPOINTS)
            || lara_info->poison.value != 0 || lara_info->poison.target != 0) {
            Lara_Poison_Cure();
            lara_item->hit_points = LARA_MAX_HITPOINTS;
            Inv_RemoveItem(O_LARGE_MEDIPACK_ITEM);
            Sound_Effect(SFX_MENU_MEDI, nullptr, SPM_ALWAYS);
            Stats_AddMedipacksUsed(1);
        }
        break;

    case O_CROWBAR_ITEM:
    case O_CROWBAR_OPTION: {
        const int16_t door_item_num = Door_FindNearbyCrowbarDoor();
        if (door_item_num == NO_ITEM || lara_info->interact_target.is_moving) {
            Sound_Effect(SFX_LARA_NO, nullptr, SPM_NORMAL);
            return;
        }

        lara_info->interact_target.item_num = door_item_num;
        lara_info->interact_target.is_moving = true;
        lara_info->interact_target.move_count = 0;
        break;
    }

    default: {
        const OBJECT_ID option_id = Inv_GetItemOption(obj_id);
        if (option_id != O_SCION_OPTION
            && Object_GetCognate(option_id, g_KeyItemToReceptacleMap)
                == NO_OBJECT) {
            break;
        }

        const int16_t receptacle_item_num = Object_FindReceptacle(option_id);
        if (receptacle_item_num == NO_ITEM
            || lara_info->interact_target.item_num != NO_ITEM) {
            Sound_Effect(SFX_LARA_NO, nullptr, SPM_NORMAL);
            return;
        }

        lara_info->interact_target.item_num = receptacle_item_num;
        lara_info->interact_target.is_moving = true;
        lara_info->interact_target.move_count = 0;
        break;
    }
    }

    if (request_gun_type != LGT_UNARMED) {
        lara_info->request_gun_type = request_gun_type;
        if (lara_info->gun_status == LGS_ARMLESS
            && lara_info->gun_type == request_gun_type) {
            lara_info->gun_type = LGT_UNARMED;
        }
    }
}

void Lara_SetStartAnimState(const LARA_EXTRA_STATE state)
{
    m_StartAnimState = state;
}

bool Lara_IsControllable(void)
{
    return m_Controllable;
}

void Lara_SetControllable(const bool controllable)
{
    m_Controllable = controllable;
}

bool Lara_CanInterpolate(
    const ITEM *const item, const int32_t frame_a, const int32_t frame_b)
{
    if (item->frame_num == item->prev_frame_num) {
        return false;
    }

    const LARA_ANIMATION anim_idx = Item_GetRelativeAnim(item);
    if (!M_IsInvalidInterpAnim(LA_U(anim_idx))) {
        return true;
    }

    // Avoid the flip 180 command having a bad effect on interpolated frames
    // on rate 1 animations, such as neutral jump twist. TODO: improve this.
    const ANIM *const anim = Item_GetAnim(item);
    return !Anim_HasFXCommandBetween(
        anim, ITEM_ACTION_TURN_180, frame_a, frame_b);
}

ITEM *Lara_GetDeathCameraTarget(void)
{
    return Item_Get(m_DeathCameraTarget);
}

void Lara_SetDeathCameraTarget(const int16_t item_num)
{
    m_DeathCameraTarget = item_num;
}

OBJECT_ID Lara_GetAnimationObject(void)
{
    const LARA_INFO *const lara_info = Lara_GetLaraInfo();
    if (lara_info->extra_anim) {
        return O_LARA_EXTRA;
    }

    if (Lara_Vehicle_IsMounted()) {
        return Lara_Vehicle_GetAnimationObject();
    }

    return O_LARA;
}

void Lara_Animate(ITEM *const item)
{
    const ROOM *const room = Room_Get(item->room_num);
    LARA_INFO *const lara = Lara_GetLaraInfo();
    item->prev_frame_num = item->frame_num;
    item->frame_num++;

    const ANIM *anim = Item_GetAnim(item);
    if (anim->num_changes > 0 && Item_GetAnimChange(item, anim)) {
        anim = Item_GetAnim(item);
        item->current_anim_state = anim->current_anim_state;
    }

    if (item->frame_num > anim->frame_end) {
        for (int32_t i = 0; i < anim->num_commands; i++) {
            const ANIM_COMMAND *const command = &anim->commands[i];
            switch (command->type) {
            case AC_MOVE_ORIGIN: {
                const XYZ_16 *const pos = (XYZ_16 *)command->data;
                const XYZ_32 old_pos = item->pos;
                Item_Translate(item, pos->x, pos->y, pos->z);
                // The re-base is invisible in OG as the pose compensates for
                // it, so move the interpolation baseline along to keep the
                // renderer from smearing it across the next tick.
                item->interp.prev.pos.x += item->pos.x - old_pos.x;
                item->interp.prev.pos.y += item->pos.y - old_pos.y;
                item->interp.prev.pos.z += item->pos.z - old_pos.z;
                break;
            }

            case AC_JUMP_VELOCITY: {
                const ANIM_COMMAND_VELOCITY_DATA *const data =
                    (ANIM_COMMAND_VELOCITY_DATA *)command->data;
                item->fall_speed = data->fall_speed;
                item->speed = data->speed;
                item->gravity = true;
                if (lara->calc_fall_speed != 0) {
                    item->fall_speed = lara->calc_fall_speed;
                    lara->calc_fall_speed = 0;
                }
                break;
            }

            case AC_ATTACK_READY:
                if (lara->gun_status != LGS_SPECIAL) {
                    lara->gun_status = LGS_ARMLESS;
                }
                break;
            default:
                break;
            }
        }

        item->anim_num = anim->jump_anim_num;
        item->frame_num = anim->jump_frame_num;
        anim = Item_GetAnim(item);
        item->current_anim_state = anim->current_anim_state;
    }

    for (int32_t i = 0; i < anim->num_commands; i++) {
        const ANIM_COMMAND *const command = &anim->commands[i];

        switch (command->type) {
        case AC_SOUND_FX: {
            const ANIM_COMMAND_EFFECT_DATA *const data =
                (ANIM_COMMAND_EFFECT_DATA *)command->data;
            Item_PlayAnimSFX(item, data);
            break;
        }

        case AC_EFFECT: {
            const ANIM_COMMAND_EFFECT_DATA *const data =
                (ANIM_COMMAND_EFFECT_DATA *)command->data;
            if (item->frame_num != data->frame_num) {
                break;
            }

            if (g_TRVersion >= 3) {
                ItemAction_RunDirectWithFX(
                    data->effect_num, item, data->fx_type);
                break;
            }

            const ANIM_COMMAND_ENVIRONMENT type = data->environment;
            const int32_t height = lara->water_surface_dist;
            if ((type == ACE_WATER && (height >= 0 || height == NO_HEIGHT))
                || (type == ACE_LAND && height < 0 && height != NO_HEIGHT
                    && !room->flags.swamp)) {
                break;
            }

            ItemAction_RunDirect(data->effect_num, item);
            break;
        }

        default:
            break;
        }
    }

    const int32_t rel_frame = item->frame_num - anim->frame_base;
    if (!item->gravity) {
        int32_t speed = anim->velocity;
        if (lara->water_status == LWS_WADE && room->flags.swamp) {
            speed /= 2;
            speed += (anim->acceleration * rel_frame) / 4;
        } else {
            speed += anim->acceleration * rel_frame;
        }
        item->speed = (int16_t)(speed >> 16);
    } else if (room->flags.swamp) {
        item->speed -= item->speed >> 3;
        if (ABS(item->speed) < 8) {
            item->speed = 0;
            item->gravity = false;
        }
        if (item->fall_speed > 128) {
            item->fall_speed /= 2;
        }
        item->fall_speed -= item->fall_speed / 4;
        CLAMPL(item->fall_speed, 4);
    } else {
        int32_t speed = anim->velocity + anim->acceleration * (rel_frame - 1);
        item->speed -= (int16_t)(speed >> 16);
        speed += anim->acceleration;
        item->speed += (int16_t)(speed >> 16);

        item->fall_speed += item->fall_speed < FAST_FALL_SPEED ? GRAVITY : 1;
        item->pos.y += item->fall_speed;
    }

    if (lara->rope.index != NO_ROPE) {
        Rope_AlignLara(item);
    }

    item->pos.x += (item->speed * Math_Sin(lara->move_angle)) >> W2V_SHIFT;
    item->pos.z += (item->speed * Math_Cos(lara->move_angle)) >> W2V_SHIFT;
}

void Lara_AnimateUntil(ITEM *lara_item, int32_t goal)
{
    lara_item->goal_anim_state = goal;
    do {
        Lara_Animate(lara_item);
    } while (lara_item->current_anim_state != goal);
}

const ANIM_FRAME *Lara_GetHitFrame(const ITEM *const item)
{
    const LARA_INFO *const lara = Lara_GetLaraInfo();
    if (lara->hit_direction < 0) {
        return nullptr;
    }

    // clang-format off
    LARA_ANIMATION anim_idx;
    if (lara->is_crouched) {
        switch (lara->hit_direction) {
        case DIR_EAST:  anim_idx = LA(LA_CROUCH_HIT_RIGHT); break;
        case DIR_SOUTH: anim_idx = LA(LA_CROUCH_HIT_FRONT); break;
        case DIR_WEST:  anim_idx = LA(LA_CROUCH_HIT_LEFT); break;
        default:        anim_idx = LA(LA_CROUCH_HIT_BACK); break;
        }
    } else {
        switch (lara->hit_direction) {
        case DIR_EAST:  anim_idx = LA(LA_HIT_LEFT); break;
        case DIR_SOUTH: anim_idx = LA(LA_HIT_BACK); break;
        case DIR_WEST:  anim_idx = LA(LA_HIT_RIGHT); break;
        default:        anim_idx = LA(LA_HIT_FRONT); break;
        }
    }
    // clang-format on

    const OBJECT *const obj = Object_Get(item->object_id);
    const ANIM *const anim = Object_GetAnim(obj, anim_idx);
    return &anim->frame_ptr[lara->hit_frame];
}

void Lara_TakeDamage(const int16_t damage, const bool hit_status)
{
    if (g_Config.debug.enable_invulnerability) {
        return;
    }
    Item_TakeDamage(
        Lara_GetItem(), damage, hit_status ? IDF_NONE : IDF_NO_HIT_STATUS,
        nullptr);
}

// TODO: This does the same thing in principle as Lara_GetJointAbsPosition().
// Consider merging these functions into a single function.
bool Lara_GetMeshPos(const LARA_MESH mesh, XYZ_32 *const out_pos)
{
    ASSERT(out_pos != nullptr);

    const ITEM *const lara_item = Lara_GetItem();
    const LARA_INFO *const lara = Lara_GetLaraInfo();
    if (!lara->mesh_pos_matrices_valid) {
        return false;
    }

    MATRIX *const m = g_MatrixPtr;
    *m = lara->mesh_pos_matrices[mesh];
    Matrix_TranslateRel32(*out_pos);
    *out_pos = (XYZ_32) {
        .x = (m->_03 >> W2V_SHIFT),
        .y = (m->_13 >> W2V_SHIFT),
        .z = (m->_23 >> W2V_SHIFT),
    };
    return true;
}

bool Lara_TestBoundsCollide(const ITEM *const item, const int32_t radius)
{
    return Item_TestBoundsCollide(item, Lara_GetItem(), radius);
}

bool Lara_TestPosition(
    const ITEM *const item, const OBJECT_BOUNDS *const bounds)
{
    const ITEM *const lara = Lara_GetItem();
    const XYZ_16 ref_rot = bounds->ignore_rot
        ? (XYZ_16) { .x = 0, .y = lara->rot.y, .z = 0 }
        : item->rot;
    const XYZ_16 rot = {
        .x = lara->rot.x - ref_rot.x,
        .y = lara->rot.y - ref_rot.y,
        .z = lara->rot.z - ref_rot.z,
    };
    const XYZ_32 dist = {
        .x = lara->pos.x - item->pos.x,
        .y = lara->pos.y - item->pos.y,
        .z = lara->pos.z - item->pos.z,
    };

    // clang-format off
    if (rot.x < bounds->rot.min.x ||
        rot.x > bounds->rot.max.x ||
        rot.y < bounds->rot.min.y ||
        rot.y > bounds->rot.max.y ||
        rot.z < bounds->rot.min.z ||
        rot.z > bounds->rot.max.z
    ) {
        return false;
    }
    // clang-format on

    Matrix_PushUnit();
    Matrix_Rot16(ref_rot);
    const MATRIX *const m = g_MatrixPtr;
    const XYZ_32 shift = {
        .x = (dist.x * m->_00 + dist.y * m->_10 + dist.z * m->_20) >> W2V_SHIFT,
        .y = (dist.x * m->_01 + dist.y * m->_11 + dist.z * m->_21) >> W2V_SHIFT,
        .z = (dist.x * m->_02 + dist.y * m->_12 + dist.z * m->_22) >> W2V_SHIFT,
    };
    Matrix_Pop();

    // clang-format off
    return (
        shift.x >= bounds->shift.min.x &&
        shift.x <= bounds->shift.max.x &&
        shift.y >= bounds->shift.min.y &&
        shift.y <= bounds->shift.max.y &&
        shift.z >= bounds->shift.min.z &&
        shift.z <= bounds->shift.max.z
    );
    // clang-format on
}

void Lara_AlignPosition(const ITEM *const item, const XYZ_32 *const vec)
{
    ITEM *const lara = Lara_GetItem();
    lara->rot = item->rot;
    Matrix_PushUnit();
    Matrix_Rot16(item->rot);
    const MATRIX *const m = g_MatrixPtr;
    const XYZ_32 shift = {
        .x = (vec->x * m->_00 + vec->y * m->_01 + vec->z * m->_02) >> W2V_SHIFT,
        .y = (vec->x * m->_10 + vec->y * m->_11 + vec->z * m->_12) >> W2V_SHIFT,
        .z = (vec->x * m->_20 + vec->y * m->_21 + vec->z * m->_22) >> W2V_SHIFT,
    };
    Matrix_Pop();

    const XYZ_32 new_pos = {
        .x = item->pos.x + shift.x,
        .y = item->pos.y + shift.y,
        .z = item->pos.z + shift.z,
    };

    if (g_Config.gameplay.fix_lara_pickup_embed) {
        int16_t room_num = lara->room_num;
        const SECTOR *const sector = Room_GetSector(new_pos, &room_num);
        const int32_t height = Room_GetHeight(sector, new_pos);
        const int32_t ceiling = Room_GetCeiling(sector, new_pos);

        if (ABS(height - lara->pos.y) > STEP_L
            || ABS(ceiling - lara->pos.y) < LARA_HEIGHT) {
            return;
        }
    }

    lara->pos = new_pos;
}

bool Lara_IsNearItem(const XYZ_32 *const pos, const int32_t distance)
{
    const ITEM *const item = Lara_GetItem();
    const XYZ_32 d = {
        .x = pos->x - item->pos.x,
        .y = pos->y - item->pos.y,
        .z = pos->z - item->pos.z,
    };
    if (ABS(d.x) > distance || ABS(d.z) > distance || ABS(d.y) > WALL_L * 3) {
        return false;
    }

    if (SQUARE(d.x) + SQUARE(d.z) > SQUARE(distance)) {
        return false;
    }

    const BOUNDS_16 *const bounds = Item_GetBoundsAccurate(item);
    return d.y >= bounds->min.y && d.y <= bounds->max.y + 100;
}

bool Lara_MovePosition(const ITEM *const ref_item, const XYZ_32 *const vec)
{
    LARA_INFO *const lara_info = Lara_GetLaraInfo();
    const bool walk_to_items = g_Config.gameplay.enable_walk_to_items
        && ref_item->object_id != O_FLARE_ITEM;
    const bool lara_on_land = lara_info->water_status != LWS_UNDERWATER
        && lara_info->water_status != LWS_CHEAT;
    const int32_t velocity =
        walk_to_items && lara_on_land ? M_MOVE_ANIM_VELOCITY : M_MOVE_SPEED;

    ITEM *const lara_item = Lara_GetItem();
    const XYZ_16 new_rot = ref_item->rot;

    Matrix_PushUnit();
    Matrix_Rot16(new_rot);
    const MATRIX *const m = g_MatrixPtr;
    const XYZ_32 shift = {
        .x = (vec->y * m->_01 + vec->z * m->_02 + vec->x * m->_00) >> W2V_SHIFT,
        .y = (vec->x * m->_10 + vec->z * m->_12 + vec->y * m->_11) >> W2V_SHIFT,
        .z = (vec->y * m->_21 + vec->x * m->_20 + vec->z * m->_22) >> W2V_SHIFT,
    };
    Matrix_Pop();

    const XYZ_32 new_pos = {
        .x = ref_item->pos.x + shift.x,
        .y = ref_item->pos.y + shift.y,
        .z = ref_item->pos.z + shift.z,
    };

    if (ref_item->object_id == O_FLARE_ITEM) {
        int16_t room_num = lara_item->room_num;
        const SECTOR *const sector = Room_GetSector(new_pos, &room_num);
        const int32_t height = Room_GetHeight(sector, new_pos);
        if (ABS(height - lara_item->pos.y) > STEP_L * 2) {
            return false;
        }
        if (XYZ_32_GetDistance(new_pos, lara_item->pos) < STEP_L) {
            return true;
        }
    }

    const XYZ_32 dpos = {
        .x = new_pos.x - lara_item->pos.x,
        .y = new_pos.y - lara_item->pos.y,
        .z = new_pos.z - lara_item->pos.z,
    };
    const int32_t length = XYZ_32_GetLength(dpos);
    if (velocity >= length) {
        lara_item->pos = new_pos;
    } else {
        lara_item->pos.x += velocity * dpos.x / length;
        lara_item->pos.y += velocity * dpos.y / length;
        lara_item->pos.z += velocity * dpos.z / length;
    }

    if (walk_to_items && !lara_info->interact_target.is_moving) {
        if (lara_on_land) {
            const int16_t step_to_anim_num[4] = {
                LA(LA_SIDE_STEP_LEFT),
                LA(LA_WALK_FORWARD),
                LA(LA_SIDE_STEP_RIGHT),
                LA(LA_WALK_BACK),
            };
            const int16_t step_to_anim_state[4] = {
                LS(LS_STEP_LEFT),
                LS(LS_WALK),
                LS(LS_STEP_RIGHT),
                LS(LS_WALK_BACK),
            };

            const int32_t dx = lara_item->pos.x - new_pos.x;
            const int32_t dz = lara_item->pos.z - new_pos.z;
            const int32_t angle = (DEG_360 - Math_Atan(dx, dz)) % DEG_360;
            const uint32_t src_quadrant = (uint32_t)(angle + DEG_45) / DEG_90;
            const uint32_t dst_quadrant =
                (uint32_t)(new_rot.y + DEG_45) / DEG_90;
            const DIRECTION quadrant = (src_quadrant - dst_quadrant) % 4;

            Item_SwitchToAnim(lara_item, step_to_anim_num[quadrant], 0);
            lara_item->goal_anim_state = step_to_anim_state[quadrant];
            lara_item->current_anim_state = step_to_anim_state[quadrant];

            lara_info->gun_status = LGS_HANDS_BUSY;
        }

        lara_info->interact_target.is_moving = lara_on_land;
        lara_info->interact_target.move_count = 0;
    }

    const int16_t rotation = M_MOVE_ANGLE;
    ITEM_ADJUST_ROT(lara_item->rot.x, new_rot.x, rotation);
    ITEM_ADJUST_ROT(lara_item->rot.y, new_rot.y, rotation);
    ITEM_ADJUST_ROT(lara_item->rot.z, new_rot.z, rotation);

    return XYZ_32_AreEquivalent(lara_item->pos, new_pos)
        && XYZ_16_AreEquivalent(lara_item->rot, new_rot);
}

LARA_ANIMATION Lara_AnimToGameID(const LARA_TRX_ANIMATION anim)
{
    int32_t out;
    if (!Catalog_EnumToGameID(CATALOG_LARA_ANIMS, anim, &out)) {
        out = -1;
    }
    return out;
}

LARA_STATE Lara_StateToGameID(const LARA_TRX_STATE state)
{
    int32_t out;
    if (!Catalog_EnumToGameID(CATALOG_LARA_STATES, state, &out)) {
        out = -1;
    }
    return out;
}

LARA_TRX_ANIMATION Lara_AnimFromGameID(const LARA_ANIMATION anim)
{
    int32_t out;
    if (!Catalog_GameIDToEnum(CATALOG_LARA_ANIMS, anim, &out)) {
        out = -1;
    }
    return out;
}

LARA_TRX_STATE Lara_StateFromGameID(const LARA_STATE state)
{
    int32_t out;
    if (!Catalog_GameIDToEnum(CATALOG_LARA_STATES, state, &out)) {
        out = -1;
    }
    return out;
}
