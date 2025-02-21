#include "game/lara/common.h"

#include "game/camera.h"
#include "game/collide.h"
#include "game/game.h"
#include "game/game_flow.h"
#include "game/gun.h"
#include "game/input.h"
#include "game/inventory.h"
#include "game/item_actions.h"
#include "game/items.h"
#include "game/lara/cheat.h"
#include "game/lara/control.h"
#include "game/lara/misc.h"
#include "game/lot.h"
#include "game/music.h"
#include "game/objects/common.h"
#include "game/objects/vars.h"
#include "game/room.h"
#include "game/savegame.h"
#include "game/sound.h"
#include "game/spawn.h"
#include "global/const.h"
#include "global/vars.h"

#include <libtrx/config.h>
#include <libtrx/game/math.h>
#include <libtrx/log.h>
#include <libtrx/utils.h>

#define LARA_MOVE_TIMEOUT 90
#define LARA_PUSH_TIMEOUT 15
#define LARA_MOVE_ANIM_VELOCITY 12
#define LARA_MOVE_SPEED 16
#define LARA_UW_DAMAGE 5

static int16_t m_DeathCameraTarget = NO_ITEM;

LARA_INFO *Lara_GetLaraInfo(void)
{
    return &g_Lara;
}

ITEM *Lara_GetItem(void)
{
    return g_LaraItem;
}

ITEM *Lara_GetDeathCameraTarget(void)
{
    return m_DeathCameraTarget == -1 ? nullptr : Item_Get(m_DeathCameraTarget);
}

void Lara_SetDeathCameraTarget(const int16_t item_num)
{
    m_DeathCameraTarget = item_num;
}

void Lara_Control(void)
{
    COLL_INFO coll = {};

    ITEM *item = g_LaraItem;
    const ROOM *const room = Room_Get(item->room_num);
    const bool room_submerged = (room->flags & RF_UNDERWATER) != 0;
    const int32_t water_depth = Lara_GetWaterDepth(
        item->pos.x, item->pos.y, item->pos.z, item->room_num);
    const int32_t water_height = Room_GetWaterHeight(
        item->pos.x, item->pos.y, item->pos.z, item->room_num);
    const int32_t water_height_diff =
        water_height == NO_HEIGHT ? NO_HEIGHT : item->pos.y - water_height;
    g_Lara.water_surface_dist = -water_height_diff;

    if (g_Lara.interact_target.is_moving
        && g_Lara.interact_target.move_count++ > LARA_MOVE_TIMEOUT) {
        g_Lara.interact_target.is_moving = false;
        g_Lara.gun_status = LGS_ARMLESS;
    }

    if (g_InputDB.level_skip_cheat) {
        Lara_Cheat_EndLevel();
    }

    if (g_InputDB.item_cheat) {
        Lara_Cheat_GiveAllItems();
    }

    if (g_Lara.water_status != LWS_CHEAT && g_InputDB.fly_cheat) {
        Lara_Cheat_EnterFlyMode();
    }

    switch (g_Lara.water_status) {
    case LWS_ABOVE_WATER: {
        if (g_Config.gameplay.enable_wading
            && (water_height_diff == NO_HEIGHT
                || water_height_diff < LARA_WADE_DEPTH)) {
            break;
        }

        if (g_Config.gameplay.enable_wading
            && water_depth <= LARA_SWIM_DEPTH - STEP_L) {
            if (water_height_diff > LARA_WADE_DEPTH) {
                g_Lara.water_status = LWS_WADE;
                if (!item->gravity) {
                    item->goal_anim_state = LS_STOP;
                }
            }
        } else if (room_submerged) {
            g_Lara.water_status = LWS_UNDERWATER;
            g_Lara.air = LARA_MAX_AIR;
            item->pos.y += 100;
            item->gravity = 0;
            Item_UpdateRoom(item, 0);
            Sound_StopEffect(SFX_LARA_FALL, nullptr);
            if (item->current_anim_state == LS_SWAN_DIVE) {
                item->goal_anim_state = LS_DIVE;
                item->rot.x = -45 * DEG_1;
                Lara_Animate(item);
                item->fall_speed *= 2;
            } else if (item->current_anim_state == LS_FAST_DIVE) {
                item->goal_anim_state = LS_DIVE;
                item->rot.x = -85 * DEG_1;
                Lara_Animate(item);
                item->fall_speed *= 2;
            } else {
                item->current_anim_state = LS_DIVE;
                item->goal_anim_state = LS_SWIM;
                Item_SwitchToAnim(item, LA_JUMP_IN, 0);
                item->rot.x = -45 * DEG_1;
                item->fall_speed = (item->fall_speed * 3) / 2;
            }
            g_Lara.head_rot.x = 0;
            g_Lara.head_rot.y = 0;
            g_Lara.torso_rot.x = 0;
            g_Lara.torso_rot.y = 0;
            Spawn_Splash(item);
        }

        break;
    }

    case LWS_UNDERWATER: {
        if (room_submerged) {
            break;
        }

        if (water_depth == NO_HEIGHT || ABS(water_height_diff) >= STEP_L) {
            g_Lara.water_status = LWS_ABOVE_WATER;
            g_Lara.gun_status = LGS_ARMLESS;
            item->current_anim_state = LS_JUMP_FORWARD;
            item->goal_anim_state = LS_JUMP_FORWARD;
            Item_SwitchToAnim(item, LA_FALL_DOWN, 0);
            item->speed = item->fall_speed / 4;
            item->fall_speed = 0;
            item->gravity = 1;
            item->rot.x = 0;
            item->rot.z = 0;
            g_Lara.head_rot.x = 0;
            g_Lara.head_rot.y = 0;
            g_Lara.torso_rot.x = 0;
            g_Lara.torso_rot.y = 0;
        } else {
            g_Lara.water_status = LWS_SURFACE;
            g_Lara.dive_timer = DIVE_WAIT + 1;
            item->current_anim_state = LS_SURF_TREAD;
            item->goal_anim_state = LS_SURF_TREAD;
            Item_SwitchToAnim(item, LA_SURF_TREAD, 0);
            item->fall_speed = 0;
            item->pos.y += 1 - water_height_diff;
            item->rot.x = 0;
            item->rot.z = 0;
            g_Lara.head_rot.x = 0;
            g_Lara.head_rot.y = 0;
            g_Lara.torso_rot.x = 0;
            g_Lara.torso_rot.y = 0;
            Item_UpdateRoom(item, -LARA_HEIGHT / 2);
            Sound_Effect(SFX_LARA_BREATH, &item->pos, SPM_ALWAYS);
        }
        break;
    }

    case LWS_SURFACE: {
        if (room_submerged) {
            break;
        }

        if (g_Config.gameplay.enable_wading
            && water_height_diff > LARA_WADE_DEPTH) {
            g_Lara.water_status = LWS_WADE;
            Item_SwitchToAnim(item, LA_STAND_IDLE, 0);
            item->current_anim_state = LS_STOP;
            item->goal_anim_state = LS_WADE;
            Item_Animate(item);
            item->fall_speed = 0;
        } else {
            g_Lara.water_status = LWS_ABOVE_WATER;
            g_Lara.gun_status = LGS_ARMLESS;
            item->current_anim_state = LS_JUMP_FORWARD;
            item->goal_anim_state = LS_JUMP_FORWARD;
            Item_SwitchToAnim(item, LA_FALL_DOWN, 0);
            item->speed = item->fall_speed / 4;
            item->fall_speed = 0;
            item->gravity = 1;
            item->rot.x = 0;
            item->rot.z = 0;
            g_Lara.head_rot.x = 0;
            g_Lara.head_rot.y = 0;
            g_Lara.torso_rot.x = 0;
            g_Lara.torso_rot.y = 0;
        }
        break;
    }

    case LWS_WADE: {
        g_Camera.target_elevation = CAM_WADE_ELEVATION;

        if (water_height_diff < LARA_WADE_DEPTH) {
            g_Lara.water_status = LWS_ABOVE_WATER;
            if (item->current_anim_state == LS_WADE) {
                item->goal_anim_state = LS_RUN;
            }
        } else if (water_height_diff > LARA_SWIM_DEPTH) {
            g_Lara.water_status = LWS_SURFACE;
            item->pos.y += 1 - water_height_diff;

            LARA_ANIMATION anim;
            switch (item->current_anim_state) {
            case LS_BACK:
                item->goal_anim_state = LS_SURF_BACK;
                anim = LA_SURF_SWIM_BACK;
                break;

            case LS_STEP_RIGHT:
                item->goal_anim_state = LS_SURF_RIGHT;
                anim = LA_SURF_SWIM_RIGHT;
                break;

            case LS_STEP_LEFT:
                item->goal_anim_state = LS_SURF_LEFT;
                anim = LA_SURF_SWIM_LEFT;
                break;

            default:
                item->goal_anim_state = LS_SURF_SWIM;
                anim = LA_SURF_SWIM_FORWARD;
                break;
            }

            item->current_anim_state = item->goal_anim_state;
            Item_SwitchToAnim(item, anim, 0);

            item->rot.z = 0;
            item->rot.x = 0;
            item->gravity = 0;
            item->fall_speed = 0;
            g_Lara.dive_timer = 0;
            g_Lara.torso_rot.y = 0;
            g_Lara.torso_rot.x = 0;
            g_Lara.head_rot.y = 0;
            g_Lara.head_rot.x = 0;
            Item_UpdateRoom(item, -LARA_HEIGHT / 2);
        }
        break;
    }

    default:
        break;
    }

    if (item->hit_points <= 0) {
        item->hit_points = -1;
        if (!g_Lara.death_timer) {
            Music_Stop();
            g_GameInfo.death_count++;
            if (Savegame_GetBoundSlot() != -1) {
                Savegame_UpdateDeathCounters(
                    Savegame_GetBoundSlot(), &g_GameInfo);
            }
        }
        g_Lara.death_timer++;
        // make sure the enemy healthbar is no longer rendered. If g_Lara later
        // is resurrected with DOZY, she should no longer aim at the target.
        g_Lara.target = nullptr;

        if (g_LaraItem->flags & IF_INVISIBLE) {
            return;
        }
    } else if (Room_IsNoFloorHeight(item->pos.y)) {
        item->hit_points = -1;
        g_Lara.death_timer = 9 * LOGIC_FPS;
    }

    Camera_MoveManual();

    switch (g_Lara.water_status) {
    case LWS_ABOVE_WATER:
    case LWS_WADE:
        g_Lara.air = LARA_MAX_AIR;
        Lara_HandleAboveWater(item, &coll);
        break;

    case LWS_UNDERWATER:
        if (item->hit_points >= 0) {
            g_Lara.air--;
            if (g_Lara.air < 0) {
                g_Lara.air = -1;
                Lara_TakeDamage(LARA_UW_DAMAGE, false);
            }
        }
        Lara_HandleUnderwater(item, &coll);
        break;

    case LWS_SURFACE:
        if (item->hit_points >= 0) {
            g_Lara.air += 10;
            if (g_Lara.air > LARA_MAX_AIR) {
                g_Lara.air = LARA_MAX_AIR;
            }
        }
        Lara_HandleSurface(item, &coll);
        break;

    case LWS_CHEAT:
        item->hit_points = LARA_MAX_HITPOINTS;
        g_Lara.death_timer = 0;
        Lara_HandleUnderwater(item, &coll);
        if (g_Input.slow && !g_Input.look && !g_Input.fly_cheat) {
            Lara_Cheat_ExitFlyMode();
        }
        break;

    default:
        break;
    }
}

void Lara_SwapMeshExtra(void)
{
    if (!Object_Get(O_LARA_EXTRA)->loaded) {
        return;
    }
    for (LARA_MESH mesh = LM_FIRST; mesh < LM_NUMBER_OF; mesh++) {
        Lara_SwapSingleMesh(mesh, O_LARA_EXTRA);
    }
}

void Lara_AnimateUntil(ITEM *lara_item, int32_t goal)
{
    lara_item->goal_anim_state = goal;
    do {
        Lara_Animate(lara_item);
    } while (lara_item->current_anim_state != goal);
}

void Lara_UseItem(const GAME_OBJECT_ID obj_id)
{
    LOG_INFO("%d", obj_id);
    switch (obj_id) {
    case O_PISTOL_ITEM:
    case O_PISTOL_OPTION:
        g_Lara.request_gun_type = LGT_PISTOLS;
        if (g_Lara.gun_status == LGS_ARMLESS
            && g_Lara.gun_type == LGT_PISTOLS) {
            g_Lara.gun_type = LGT_UNARMED;
        }
        break;

    case O_SHOTGUN_ITEM:
    case O_SHOTGUN_OPTION:
        g_Lara.request_gun_type = LGT_SHOTGUN;
        if (g_Lara.gun_status == LGS_ARMLESS
            && g_Lara.gun_type == LGT_SHOTGUN) {
            g_Lara.gun_type = LGT_UNARMED;
        }
        break;

    case O_MAGNUM_ITEM:
    case O_MAGNUM_OPTION:
        g_Lara.request_gun_type = LGT_MAGNUMS;
        if (g_Lara.gun_status == LGS_ARMLESS
            && g_Lara.gun_type == LGT_MAGNUMS) {
            g_Lara.gun_type = LGT_UNARMED;
        }
        break;

    case O_UZI_ITEM:
    case O_UZI_OPTION:
        g_Lara.request_gun_type = LGT_UZIS;
        if (g_Lara.gun_status == LGS_ARMLESS && g_Lara.gun_type == LGT_UZIS) {
            g_Lara.gun_type = LGT_UNARMED;
        }
        break;

    case O_MEDI_ITEM:
    case O_MEDI_OPTION:
        if (g_LaraItem->hit_points <= 0
            || g_LaraItem->hit_points >= LARA_MAX_HITPOINTS) {
            return;
        }
        g_LaraItem->hit_points += LARA_MAX_HITPOINTS / 2;
        CLAMPG(g_LaraItem->hit_points, LARA_MAX_HITPOINTS);
        Inv_RemoveItem(O_MEDI_ITEM);
        Sound_Effect(SFX_MENU_MEDI, nullptr, SPM_ALWAYS);
        break;

    case O_BIGMEDI_ITEM:
    case O_BIGMEDI_OPTION:
        if (g_LaraItem->hit_points <= 0
            || g_LaraItem->hit_points >= LARA_MAX_HITPOINTS) {
            return;
        }
        g_LaraItem->hit_points = g_LaraItem->hit_points + LARA_MAX_HITPOINTS;
        CLAMPG(g_LaraItem->hit_points, LARA_MAX_HITPOINTS);
        Inv_RemoveItem(O_BIGMEDI_ITEM);
        Sound_Effect(SFX_MENU_MEDI, nullptr, SPM_ALWAYS);
        break;

    case O_KEY_ITEM_1:
    case O_KEY_OPTION_1:
    case O_KEY_ITEM_2:
    case O_KEY_OPTION_2:
    case O_KEY_ITEM_3:
    case O_KEY_OPTION_3:
    case O_KEY_ITEM_4:
    case O_KEY_OPTION_4:
    case O_PUZZLE_ITEM_1:
    case O_PUZZLE_OPTION_1:
    case O_PUZZLE_ITEM_2:
    case O_PUZZLE_OPTION_2:
    case O_PUZZLE_ITEM_3:
    case O_PUZZLE_OPTION_3:
    case O_PUZZLE_ITEM_4:
    case O_PUZZLE_OPTION_4:
    case O_LEADBAR_ITEM:
    case O_LEADBAR_OPTION:
    case O_SCION_ITEM_1:
    case O_SCION_ITEM_2:
    case O_SCION_ITEM_3:
    case O_SCION_ITEM_4:
    case O_SCION_OPTION: {
        int16_t receptacle_item_num = Object_FindReceptacle(obj_id);
        if (receptacle_item_num == NO_OBJECT) {
            Sound_Effect(SFX_LARA_NO, nullptr, SPM_NORMAL);
            return;
        }
        g_Lara.interact_target.item_num = receptacle_item_num;
        g_Lara.interact_target.is_moving = true;
        g_Lara.interact_target.move_count = 0;
        Inv_RemoveItem(obj_id);
        break;
    }

    default:
        break;
    }
}

void Lara_ControlExtra(int16_t item_num)
{
    Item_Animate(Item_Get(item_num));
}

void Lara_InitialiseLoad(int16_t item_num)
{
    g_Lara.item_num = item_num;
    if (item_num == NO_ITEM) {
        g_LaraItem = nullptr;
    } else {
        g_LaraItem = Item_Get(item_num);
    }
}

void Lara_Initialise(const GF_LEVEL *const level)
{
    RESUME_INFO *const resume = Savegame_GetCurrentInfo(level);

    g_LaraItem->collidable = 0;
    g_LaraItem->data = &g_Lara;
    if (resume != nullptr && g_Config.gameplay.disable_healing_between_levels) {
        g_LaraItem->hit_points = resume->lara_hitpoints;
    } else {
        g_LaraItem->hit_points = g_Config.gameplay.start_lara_hitpoints;
    }

    m_DeathCameraTarget = NO_ITEM;
    g_Lara.air = LARA_MAX_AIR;
    g_Lara.torso_rot.y = 0;
    g_Lara.torso_rot.x = 0;
    g_Lara.torso_rot.z = 0;
    g_Lara.head_rot.y = 0;
    g_Lara.head_rot.x = 0;
    g_Lara.head_rot.z = 0;
    g_Lara.calc_fall_speed = 0;
    g_Lara.mesh_effects = 0;
    g_Lara.hit_frame = 0;
    g_Lara.hit_direction = 0;
    g_Lara.death_timer = 0;
    g_Lara.target = nullptr;
    g_Lara.hit_effect = nullptr;
    g_Lara.hit_effect_count = 0;
    g_Lara.turn_rate = 0;
    g_Lara.move_angle = 0;
    g_Lara.right_arm.flash_gun = 0;
    g_Lara.left_arm.flash_gun = 0;
    g_Lara.right_arm.lock = 0;
    g_Lara.left_arm.lock = 0;
    g_Lara.interact_target.is_moving = false;
    g_Lara.interact_target.item_num = NO_OBJECT;
    g_Lara.interact_target.move_count = 0;

    if (Room_Get(g_LaraItem->room_num)->flags & RF_UNDERWATER) {
        g_Lara.water_status = LWS_UNDERWATER;
        g_LaraItem->fall_speed = 0;
        g_LaraItem->goal_anim_state = LS_TREAD;
        g_LaraItem->current_anim_state = LS_TREAD;
        Item_SwitchToAnim(g_LaraItem, LA_TREAD, 0);
    } else {
        g_Lara.water_status = LWS_ABOVE_WATER;
        g_LaraItem->goal_anim_state = LS_STOP;
        g_LaraItem->current_anim_state = LS_STOP;
        Item_SwitchToAnim(g_LaraItem, LA_STOP, 0);
    }

    g_Lara.current_active = 0;

    LOT_InitialiseLOT(&g_Lara.lot);
    g_Lara.lot.step = WALL_L * 20;
    g_Lara.lot.drop = -WALL_L * 20;
    g_Lara.lot.fly = STEP_L;

    Lara_InitialiseInventory(level);
}

void Lara_InitialiseInventory(const GF_LEVEL *const level)
{
    Inv_RemoveAllItems();

    RESUME_INFO *const resume = Savegame_GetCurrentInfo(level);

    g_Lara.pistols.ammo = 1000;

    if (resume != nullptr) {
        if (g_GameInfo.remove_guns) {
            resume->flags.got_pistols = 0;
            resume->flags.got_shotgun = 0;
            resume->flags.got_magnums = 0;
            resume->flags.got_uzis = 0;
            resume->equipped_gun_type = LGT_UNARMED;
            resume->holsters_gun_type = LGT_UNARMED;
            resume->back_gun_type = LGT_UNARMED;
            resume->gun_status = LGS_ARMLESS;
        }

        if (g_GameInfo.remove_scions) {
            resume->num_scions = 0;
        }

        if (g_GameInfo.remove_ammo) {
            resume->shotgun_ammo = 0;
            resume->magnum_ammo = 0;
            resume->uzi_ammo = 0;
        }

        if (g_GameInfo.remove_medipacks) {
            resume->num_medis = 0;
            resume->num_big_medis = 0;
        }

        if (resume->flags.got_pistols) {
            Inv_AddItem(O_PISTOL_ITEM);
        }

        if (resume->flags.got_magnums) {
            Inv_AddItem(O_MAGNUM_ITEM);
            g_Lara.magnums.ammo = resume->magnum_ammo;
            Item_GlobalReplace(O_MAGNUM_ITEM, O_MAG_AMMO_ITEM);
        } else {
            int32_t ammo = resume->magnum_ammo / MAGNUM_AMMO_QTY;
            for (int i = 0; i < ammo; i++) {
                Inv_AddItem(O_MAG_AMMO_ITEM);
            }
            g_Lara.magnums.ammo = 0;
        }

        if (resume->flags.got_uzis) {
            Inv_AddItem(O_UZI_ITEM);
            g_Lara.uzis.ammo = resume->uzi_ammo;
            Item_GlobalReplace(O_UZI_ITEM, O_UZI_AMMO_ITEM);
        } else {
            int32_t ammo = resume->uzi_ammo / UZI_AMMO_QTY;
            for (int i = 0; i < ammo; i++) {
                Inv_AddItem(O_UZI_AMMO_ITEM);
            }
            g_Lara.uzis.ammo = 0;
        }

        if (resume->flags.got_shotgun) {
            Inv_AddItem(O_SHOTGUN_ITEM);
            g_Lara.shotgun.ammo = resume->shotgun_ammo;
            Item_GlobalReplace(O_SHOTGUN_ITEM, O_SG_AMMO_ITEM);
        } else {
            int32_t ammo = resume->shotgun_ammo / SHOTGUN_AMMO_QTY;
            for (int i = 0; i < ammo; i++) {
                Inv_AddItem(O_SG_AMMO_ITEM);
            }
            g_Lara.shotgun.ammo = 0;
        }

        for (int i = 0; i < resume->num_scions; i++) {
            Inv_AddItem(O_SCION_ITEM_1);
        }

        for (int i = 0; i < resume->num_medis; i++) {
            Inv_AddItem(O_MEDI_ITEM);
        }

        for (int i = 0; i < resume->num_big_medis; i++) {
            Inv_AddItem(O_BIGMEDI_ITEM);
        }

        g_Lara.gun_status = resume->gun_status;
        g_Lara.gun_type = resume->equipped_gun_type;
        g_Lara.request_gun_type = resume->equipped_gun_type;
        g_Lara.holsters_gun_type = resume->holsters_gun_type;
        g_Lara.back_gun_type = resume->back_gun_type;
    }

    Lara_InitialiseMeshes(level);
    Gun_InitialiseNewWeapon();
}

void Lara_RevertToPistolsIfNeeded(void)
{
    if (!g_Config.gameplay.revert_to_pistols
        || !Inv_RequestItem(O_PISTOL_ITEM)) {
        return;
    }

    g_Lara.gun_type = LGT_PISTOLS;

    if (g_Lara.gun_status != LGS_ARMLESS) {
        g_Lara.holsters_gun_type = LGT_UNARMED;
    }
    if (Inv_RequestItem(O_SHOTGUN_ITEM)) {
        g_Lara.back_gun_type = LGT_SHOTGUN;
    } else {
        g_Lara.back_gun_type = LGT_UNARMED;
    }
    Gun_InitialiseNewWeapon();
    Gun_SetLaraHolsterLMesh(g_Lara.holsters_gun_type);
    Gun_SetLaraHolsterRMesh(g_Lara.holsters_gun_type);
    Gun_SetLaraBackMesh(g_Lara.back_gun_type);
}

void Lara_InitialiseMeshes(const GF_LEVEL *const level)
{
    const RESUME_INFO *const resume = Savegame_GetCurrentInfo(level);

    if (resume != nullptr && resume->flags.costume) {
        for (LARA_MESH mesh = LM_FIRST; mesh < LM_NUMBER_OF; mesh++) {
            Lara_SwapSingleMesh(mesh, mesh == LM_HEAD ? O_LARA : O_LARA_EXTRA);
        }
        return;
    }

    for (LARA_MESH mesh = LM_FIRST; mesh < LM_NUMBER_OF; mesh++) {
        Lara_SwapSingleMesh(mesh, O_LARA);
    }

    LARA_GUN_TYPE holsters_gun_type =
        resume != nullptr ? resume->holsters_gun_type : LGT_UNKNOWN;
    LARA_GUN_TYPE back_gun_type =
        resume != nullptr ? resume->back_gun_type : LGT_UNKNOWN;

    if (holsters_gun_type != LGT_UNKNOWN) {
        Gun_SetLaraHolsterLMesh(holsters_gun_type);
        Gun_SetLaraHolsterRMesh(holsters_gun_type);
    }

    if (back_gun_type != LGT_UNKNOWN) {
        Gun_SetLaraBackMesh(back_gun_type);
    }
}

bool Lara_IsNearItem(const XYZ_32 *pos, int32_t distance)
{
    return Item_IsNearItem(g_LaraItem, pos, distance);
}

bool Lara_TestBoundsCollide(ITEM *item, int32_t radius)
{
    return Item_TestBoundsCollide(g_LaraItem, item, radius);
}

bool Lara_TestPosition(const ITEM *item, const OBJECT_BOUNDS *const bounds)
{
    return Item_TestPosition(g_LaraItem, item, bounds);
}

void Lara_AlignPosition(ITEM *item, XYZ_32 *vec)
{
    Item_AlignPosition(g_LaraItem, item, vec);
}

bool Lara_MovePosition(ITEM *item, XYZ_32 *vec)
{
    int32_t velocity = g_Config.gameplay.enable_walk_to_items
            && g_Lara.water_status != LWS_UNDERWATER
        ? LARA_MOVE_ANIM_VELOCITY
        : LARA_MOVE_SPEED;

    return Item_MovePosition(g_LaraItem, item, vec, velocity);
}

void Lara_Push(ITEM *item, COLL_INFO *coll, bool hit_on, bool big_push)
{
    ITEM *const lara_item = g_LaraItem;
    int32_t x = lara_item->pos.x - item->pos.x;
    int32_t z = lara_item->pos.z - item->pos.z;
    const int32_t c = Math_Cos(item->rot.y);
    const int32_t s = Math_Sin(item->rot.y);
    int32_t rx = (c * x - s * z) >> W2V_SHIFT;
    int32_t rz = (c * z + s * x) >> W2V_SHIFT;

    const BOUNDS_16 *const bounds = &Item_GetBestFrame(item)->bounds;
    int32_t min_x = bounds->min.x;
    int32_t max_x = bounds->max.x;
    int32_t min_z = bounds->min.z;
    int32_t max_z = bounds->max.z;

    if (big_push) {
        min_x -= coll->radius;
        max_x += coll->radius;
        min_z -= coll->radius;
        max_z += coll->radius;
    }

    if (rx >= min_x && rx <= max_x && rz >= min_z && rz <= max_z) {
        int32_t l = rx - min_x;
        int32_t r = max_x - rx;
        int32_t t = max_z - rz;
        int32_t b = rz - min_z;

        if (l <= r && l <= t && l <= b) {
            rx -= l;
        } else if (r <= l && r <= t && r <= b) {
            rx += r;
        } else if (t <= l && t <= r && t <= b) {
            rz += t;
        } else {
            rz -= b;
        }

        int32_t ax = (c * rx + s * rz) >> W2V_SHIFT;
        int32_t az = (c * rz - s * rx) >> W2V_SHIFT;

        lara_item->pos.x = item->pos.x + ax;
        lara_item->pos.z = item->pos.z + az;

        rx = (bounds->min.x + bounds->max.x) / 2;
        rz = (bounds->min.z + bounds->max.z) / 2;
        x -= (c * rx + s * rz) >> W2V_SHIFT;
        z -= (c * rz - s * rx) >> W2V_SHIFT;

        if (hit_on) {
            PHD_ANGLE hitang = lara_item->rot.y - (DEG_180 + Math_Atan(z, x));
            g_Lara.hit_direction = (hitang + DEG_45) / DEG_90;
            if (!g_Lara.hit_frame) {
                Sound_Effect(SFX_LARA_BODYSL, &lara_item->pos, SPM_NORMAL);
            }

            g_Lara.hit_frame++;
            if (g_Lara.hit_frame > 34) {
                g_Lara.hit_frame = 34;
            }
        }

        coll->bad_pos = NO_BAD_POS;
        coll->bad_neg = -STEPUP_HEIGHT;
        coll->bad_ceiling = 0;

        int16_t old_facing = coll->facing;
        coll->facing = Math_Atan(
            lara_item->pos.z - coll->old.z, lara_item->pos.x - coll->old.x);
        Collide_GetCollisionInfo(
            coll, lara_item->pos.x, lara_item->pos.y, lara_item->pos.z,
            lara_item->room_num, LARA_HEIGHT);
        coll->facing = old_facing;

        if (coll->coll_type != COLL_NONE) {
            lara_item->pos.x = coll->old.x;
            lara_item->pos.z = coll->old.z;
        } else {
            coll->old.x = lara_item->pos.x;
            coll->old.y = lara_item->pos.y;
            coll->old.z = lara_item->pos.z;
            Item_UpdateRoom(item, -10);
        }

        if (g_Lara.interact_target.is_moving
            && g_Lara.interact_target.move_count > LARA_PUSH_TIMEOUT) {
            g_Lara.interact_target.is_moving = false;
            g_Lara.gun_status = LGS_ARMLESS;
        }
    }
}

void Lara_TakeDamage(int16_t damage, bool hit_status)
{
    Item_TakeDamage(g_LaraItem, damage, hit_status);
}
