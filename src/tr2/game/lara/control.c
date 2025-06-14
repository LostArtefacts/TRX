#include "game/lara/control.h"

#include "decomp/skidoo.h"
#include "game/creature.h"
#include "game/game.h"
#include "game/gun/gun.h"
#include "game/input.h"
#include "game/inventory.h"
#include "game/item_actions.h"
#include "game/lara/look.h"
#include "game/lara/misc.h"
#include "game/lara/state.h"
#include "game/savegame.h"
#include "game/sound.h"
#include "game/spawn.h"
#include "game/stats.h"
#include "global/vars.h"

#include <libtrx/config.h>
#include <libtrx/game/camera.h>
#include <libtrx/game/gym.h>
#include <libtrx/game/lara.h>
#include <libtrx/game/math.h>
#include <libtrx/game/music.h>
#include <libtrx/utils.h>

static int32_t m_OpenDoorsCheatCooldown = 0;

static SECTOR *M_GetCurrentSector(const ITEM *lara_item);

static SECTOR *M_GetCurrentSector(const ITEM *const lara_item)
{
    int16_t room_num = lara_item->room_num;
    return Room_GetSector(
        lara_item->pos.x, MAX_HEIGHT, lara_item->pos.z, &room_num);
}

void Lara_HandleAboveWater(ITEM *const item, COLL_INFO *const coll)
{
    coll->old.x = item->pos.x;
    coll->old.y = item->pos.y;
    coll->old.z = item->pos.z;
    coll->old_anim_state = item->current_anim_state;
    coll->old_anim_num = item->anim_num;
    coll->old_frame_num = item->frame_num;
    coll->radius = LARA_RADIUS;

    coll->slopes_are_walls = 0;
    coll->slopes_are_pits = 0;
    coll->lava_is_pit = 0;
    coll->enable_baddie_push = 1;
    coll->enable_hit = 1;

    if (g_Input.look && !g_Lara.extra_anim && g_Lara.enable_look) {
        Lara_LookLeftRight();
    } else {
        Lara_ResetLook();
    }
    g_Lara.enable_look = true;

    if (g_Lara.vehicle_item_num != NO_ITEM) {
        if (Item_Get(g_Lara.vehicle_item_num)->object_id == O_SKIDOO_FAST) {
            // TODO: make this Object_Get(O_SKIDOO_FAST)->control
            if (Skidoo_Control()) {
                return;
            }
        } else {
            Gun_Control();
            return;
        }
    }

    Lara_State_Update(item, coll);

    if (item->rot.z < -LARA_LEAN_UNDO) {
        item->rot.z += LARA_LEAN_UNDO;
    } else if (item->rot.z > LARA_LEAN_UNDO) {
        item->rot.z -= LARA_LEAN_UNDO;
    } else {
        item->rot.z = 0;
    }

    if (g_Lara.turn_rate < -LARA_TURN_UNDO) {
        g_Lara.turn_rate += LARA_TURN_UNDO;
    } else if (g_Lara.turn_rate > LARA_TURN_UNDO) {
        g_Lara.turn_rate -= LARA_TURN_UNDO;
    } else {
        g_Lara.turn_rate = 0;
    }
    item->rot.y += g_Lara.turn_rate;

    Lara_Animate(item);

    const SECTOR *const sector = M_GetCurrentSector(item);
    if (!g_Lara.extra_anim && g_Lara.water_status != LWS_CHEAT) {
        Lara_BaddieCollision(item, coll);
        if (g_Lara.vehicle_item_num == NO_ITEM) {
            Lara_Col_Update(item, coll);
        }
    }

    Lara_UpdateRoomToHeight(-LARA_HEIGHT / 2);
    Gun_Control();
    Room_TestSectorTrigger(item, sector);
}

void Lara_HandleSurface(ITEM *const item, COLL_INFO *const coll)
{
    g_Camera.target_elevation = -22 * DEG_1;

    coll->old.x = item->pos.x;
    coll->old.y = item->pos.y;
    coll->old.z = item->pos.z;
    coll->radius = LARA_RADIUS_SURF;

    coll->bad_pos = NO_BAD_POS;
    coll->bad_neg = -STEP_L / 2;
    coll->bad_ceiling = 100;

    coll->slopes_are_walls = 0;
    coll->slopes_are_pits = 0;
    coll->lava_is_pit = 0;
    coll->enable_baddie_push = 0;
    coll->enable_hit = 0;

    if (g_Input.look && g_Lara.enable_look) {
        Lara_LookLeftRight();
    } else {
        Lara_ResetLook();
    }
    g_Lara.enable_look = true;

    Lara_State_Update(item, coll);

    if (item->rot.z > LARA_LEAN_UNDO_SURF) {
        item->rot.z -= LARA_LEAN_UNDO_SURF;
    } else if (item->rot.z < -LARA_LEAN_UNDO_SURF) {
        item->rot.z += LARA_LEAN_UNDO_SURF;
    } else {
        item->rot.z = 0;
    }

    if (g_Lara.current_active && g_Lara.water_status != LWS_CHEAT) {
        Lara_WaterCurrent(coll);
    } else {
        LOT_ClearLOT(&g_Lara.lot);
    }

    Lara_Animate(item);
    item->pos.x +=
        (item->fall_speed * Math_Sin(g_Lara.move_angle)) >> (W2V_SHIFT + 2);
    item->pos.z +=
        (item->fall_speed * Math_Cos(g_Lara.move_angle)) >> (W2V_SHIFT + 2);

    const SECTOR *const sector = M_GetCurrentSector(item);

    Lara_BaddieCollision(item, coll);

    if (g_Lara.vehicle_item_num == NO_ITEM) {
        Lara_Col_Update(item, coll);
    }

    Lara_UpdateRoomToHeight(100);
    Gun_Control();
    Room_TestSectorTrigger(item, sector);
}

void Lara_HandleUnderwater(ITEM *const item, COLL_INFO *const coll)
{
    coll->old.x = item->pos.x;
    coll->old.y = item->pos.y;
    coll->old.z = item->pos.z;
    coll->radius = LARA_RADIUS_UW;

    coll->bad_pos = NO_BAD_POS;
    coll->bad_neg = -LARA_HEIGHT_UW;
    coll->bad_ceiling = LARA_HEIGHT_UW;

    coll->slopes_are_walls = 0;
    coll->slopes_are_pits = 0;
    coll->lava_is_pit = 0;
    coll->enable_baddie_push = 1;
    coll->enable_hit = 0;

    if (g_Input.look && g_Lara.enable_look) {
        Lara_LookLeftRight();
    } else {
        Lara_ResetLook();
    }
    g_Lara.enable_look = true;

    Lara_State_Update(item, coll);

    if (item->rot.z > LARA_LEAN_UNDO_UW) {
        item->rot.z -= LARA_LEAN_UNDO_UW;
    } else if (item->rot.z < -LARA_LEAN_UNDO_UW) {
        item->rot.z += LARA_LEAN_UNDO_UW;
    } else {
        item->rot.z = 0;
    }

    CLAMP(item->rot.x, -85 * DEG_1, 85 * DEG_1);
    CLAMP(item->rot.z, -LARA_LEAN_MAX_UW, LARA_LEAN_MAX_UW);

    if (g_Lara.turn_rate < -LARA_TURN_UNDO) {
        g_Lara.turn_rate += LARA_TURN_UNDO;
    } else if (g_Lara.turn_rate > LARA_TURN_UNDO) {
        g_Lara.turn_rate -= LARA_TURN_UNDO;
    } else {
        g_Lara.turn_rate = 0;
    }

    item->rot.y += g_Lara.turn_rate;
    if (g_Lara.current_active && g_Lara.water_status != LWS_CHEAT) {
        Lara_WaterCurrent(coll);
    } else {
        LOT_ClearLOT(&g_Lara.lot);
    }

    Lara_Animate(item);
    item->pos.y -=
        (item->fall_speed * Math_Sin(item->rot.x)) >> (W2V_SHIFT + 2);
    item->pos.x +=
        (Math_Cos(item->rot.x)
         * ((item->fall_speed * Math_Sin(item->rot.y)) >> (W2V_SHIFT + 2)))
        >> W2V_SHIFT;
    item->pos.z +=
        (Math_Cos(item->rot.x)
         * ((item->fall_speed * Math_Cos(item->rot.y)) >> (W2V_SHIFT + 2)))
        >> W2V_SHIFT;

    const SECTOR *const sector = M_GetCurrentSector(item);

    if (g_Lara.water_status != LWS_CHEAT && !g_Lara.extra_anim) {
        Lara_BaddieCollision(item, coll);
    }

    if (g_Lara.water_status == LWS_CHEAT) {
        if (m_OpenDoorsCheatCooldown) {
            m_OpenDoorsCheatCooldown--;
        } else if (g_InputDB.draw) {
            m_OpenDoorsCheatCooldown = LOGIC_FPS;
            Lara_Cheat_OpenNearestDoor();
        }
    }

    if (!g_Lara.extra_anim) {
        Lara_Col_Update(item, coll);
    }

    Lara_UpdateRoomToHeight(0);
    Gun_Control();
    Room_TestSectorTrigger(item, sector);
}

void Lara_Control(const int16_t item_num)
{
    ITEM *const item = g_LaraItem;

    if (g_InputDB.level_skip_cheat) {
        Lara_Cheat_EndLevel();
    }

    if (g_InputDB.item_cheat) {
        Lara_Cheat_GiveAllItems();
    }

    if (g_Lara.water_status != LWS_CHEAT && g_InputDB.fly_cheat) {
        Lara_Cheat_EnterFlyMode();
    }

    const bool room_submerged = Room_Get(item->room_num)->flags & RF_UNDERWATER;
    const int32_t water_depth = Lara_GetWaterDepth(
        item->pos.x, item->pos.y, item->pos.z, item->room_num);
    const int32_t water_height = Room_GetWaterHeight(
        item->pos.x, item->pos.y, item->pos.z, item->room_num);
    const int32_t water_height_diff =
        water_height == NO_HEIGHT ? NO_HEIGHT : item->pos.y - water_height;

    g_Lara.water_surface_dist = -water_height_diff;

    if (g_Lara.vehicle_item_num == NO_ITEM && !g_Lara.extra_anim) {
        switch (g_Lara.water_status) {
        case LWS_ABOVE_WATER:
            if (water_height_diff == NO_HEIGHT
                || water_height_diff < LARA_WADE_DEPTH) {
                break;
            }

            if (water_depth <= LARA_SWIM_DEPTH - STEP_L) {
                if (water_height_diff > LARA_WADE_DEPTH) {
                    g_Lara.water_status = LWS_WADE;
                    if (!item->gravity) {
                        item->goal_anim_state = LS_STOP;
                    }
                }
            } else if (room_submerged) {
                g_Lara.air = LARA_MAX_AIR;
                g_Lara.water_status = LWS_UNDERWATER;
                item->gravity = false;
                item->pos.y += 100;
                Lara_UpdateRoomToHeight(0);
                Sound_StopEffect(SFX_LARA_FALL);
                if (item->current_anim_state == LS_SWAN_DIVE) {
                    item->rot.x = -45 * DEG_1;
                    item->goal_anim_state = LS_DIVE;
                    Lara_Animate(item);
                    item->fall_speed *= 2;
                } else if (item->current_anim_state == LS_FAST_DIVE) {
                    item->rot.x = -85 * DEG_1;
                    item->goal_anim_state = LS_DIVE;
                    Lara_Animate(item);
                    item->fall_speed *= 2;
                } else {
                    item->rot.x = -45 * DEG_1;
                    Item_SwitchToAnim(item, LA_FREEFALL_TO_UNDERWATER, 0);
                    item->current_anim_state = LS_DIVE;
                    item->goal_anim_state = LS_SWIM;
                    item->fall_speed = item->fall_speed * 3 / 2;
                }
                g_Lara.torso_rot.y = 0;
                g_Lara.torso_rot.x = 0;
                g_Lara.head_rot.y = 0;
                g_Lara.head_rot.x = 0;
                Spawn_Splash(item);
            }
            break;

        case LWS_UNDERWATER:
            if (room_submerged) {
                break;
            }

            if (water_depth == NO_HEIGHT || ABS(water_height_diff) >= STEP_L) {
                g_Lara.water_status = LWS_ABOVE_WATER;
                Item_SwitchToAnim(item, LA_FALL_START, 0);
                item->goal_anim_state = LS_JUMP_FORWARD;
                item->current_anim_state = LS_JUMP_FORWARD;
                item->gravity = true;
                item->speed = item->fall_speed / 4;
                item->fall_speed = 0;
                item->rot.x = 0;
                item->rot.z = 0;
                g_Lara.torso_rot.y = 0;
                g_Lara.torso_rot.x = 0;
                g_Lara.head_rot.y = 0;
                g_Lara.head_rot.x = 0;
            } else {
                g_Lara.water_status = LWS_SURFACE;
                Item_SwitchToAnim(item, LA_UNDERWATER_TO_ONWATER, 0);
                item->goal_anim_state = LS_SURF_TREAD;
                item->current_anim_state = LS_SURF_TREAD;
                item->fall_speed = 0;
                item->pos.y += 1 - water_height_diff;
                item->rot.z = 0;
                item->rot.x = 0;
                g_Lara.dive_timer = LARA_DIVE_WAIT + 1;
                g_Lara.torso_rot.y = 0;
                g_Lara.torso_rot.x = 0;
                g_Lara.head_rot.y = 0;
                g_Lara.head_rot.x = 0;
                Lara_UpdateRoomToHeight(-381);
                Sound_Effect(SFX_LARA_BREATH, &item->pos, SPM_ALWAYS);
            }
            break;

        case LWS_SURFACE:
            if (room_submerged) {
                break;
            }

            if (water_height_diff <= LARA_WADE_DEPTH) {
                g_Lara.water_status = LWS_ABOVE_WATER;
                Item_SwitchToAnim(item, LA_FALL_START, 0);
                item->goal_anim_state = LS_JUMP_FORWARD;
                item->current_anim_state = LS_JUMP_FORWARD;
                item->gravity = true;
                item->speed = item->fall_speed / 4;
            } else {
                g_Lara.water_status = LWS_WADE;
                Item_SwitchToAnim(item, LA_STAND_IDLE, 0);
                item->current_anim_state = LS_STOP;
                item->goal_anim_state = LS_WADE;
                Item_Animate(item);
                item->fall_speed = 0;
            }
            item->rot.x = 0;
            item->rot.z = 0;
            g_Lara.torso_rot.y = 0;
            g_Lara.torso_rot.x = 0;
            g_Lara.head_rot.y = 0;
            g_Lara.head_rot.x = 0;
            break;

        case LWS_WADE:
            g_Camera.target_elevation = -22 * DEG_1;

            if (water_height_diff < LARA_WADE_DEPTH) {
                g_Lara.water_status = LWS_ABOVE_WATER;
                if (item->current_anim_state == LS_WADE) {
                    item->goal_anim_state = LS_RUN;
                }
            } else if (water_height_diff > 730) {
                g_Lara.water_status = LWS_SURFACE;
                item->pos.y += 1 - water_height_diff;

                LARA_ANIMATION anim_idx;
                switch (item->current_anim_state) {
                case LS_WALK_BACK:
                    item->goal_anim_state = LS_SURF_BACK;
                    anim_idx = LA_ONWATER_IDLE_TO_SWIM_BACK;
                    break;

                case LS_STEP_RIGHT:
                    item->goal_anim_state = LS_SURF_RIGHT;
                    anim_idx = LA_ONWATER_SWIM_RIGHT;
                    break;

                case LS_STEP_LEFT:
                    item->goal_anim_state = LS_SURF_LEFT;
                    anim_idx = LA_ONWATER_SWIM_LEFT;
                    break;

                default:
                    item->goal_anim_state = LS_SURF_SWIM;
                    anim_idx = LA_ONWATER_SWIM_FORWARD;
                    break;
                }
                item->current_anim_state = item->goal_anim_state;
                Item_SwitchToAnim(item, anim_idx, 0);

                item->rot.z = 0;
                item->rot.x = 0;
                item->gravity = false;
                item->fall_speed = 0;
                g_Lara.dive_timer = 0;
                g_Lara.torso_rot.y = 0;
                g_Lara.torso_rot.x = 0;
                g_Lara.head_rot.y = 0;
                g_Lara.head_rot.x = 0;
                Lara_UpdateRoomToHeight(-LARA_HEIGHT / 2);
            }
            break;

        default:
            break;
        }
    }

    if (item->hit_points <= 0) {
        item->hit_points = -1;
        if (Game_IsInGym()) {
            Gym_SetInventoryOpenEnabled(true);
        }
        if (!g_Lara.death_timer) {
            Music_Stop();
        }
        g_Lara.death_timer++;
        if (item->flags & IF_ONE_SHOT) {
            g_Lara.death_timer++;
            return;
        }
    } else if (Room_IsAbyssHeight(item->pos.y)) {
        item->hit_points = -1;
        g_Lara.death_timer = 9 * LOGIC_FPS;
    }

    Camera_MoveManual();

    COLL_INFO coll;
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
                item->hit_points -= 5;
            }
        }
        Lara_HandleUnderwater(item, &coll);
        break;

    case LWS_SURFACE:
        if (item->hit_points >= 0) {
            g_Lara.air += 10;
            CLAMPG(g_Lara.air, LARA_MAX_AIR);
        }
        Lara_HandleSurface(item, &coll);
        break;

    case LWS_CHEAT:
        // TODO: make Lara immune to lava and flames
        item->hit_points = LARA_MAX_HITPOINTS;
        g_Lara.death_timer = 0;
        Lara_HandleUnderwater(item, &coll);
        if (g_Input.slow && !g_Input.look) {
            Lara_Cheat_ExitFlyMode();
        }
        break;

    default:
        break;
    }

    Stats_AddDistanceTravelled(item->pos, g_Lara.last_pos);
    g_Lara.last_pos = item->pos;
}

void Lara_ControlExtra(const int16_t item_num)
{
    Item_Animate(Item_Get(item_num));
}

void Lara_UseItem(const GAME_OBJECT_ID obj_id)
{
    ITEM *const item = g_LaraItem;

    switch (obj_id) {
    case O_PISTOL_ITEM:
    case O_PISTOL_OPTION:
        g_Lara.request_gun_type = LGT_PISTOLS;
        break;

    case O_SHOTGUN_ITEM:
    case O_SHOTGUN_OPTION:
        g_Lara.request_gun_type = LGT_SHOTGUN;
        break;

    case O_MAGNUM_ITEM:
    case O_MAGNUM_OPTION:
        g_Lara.request_gun_type = LGT_MAGNUMS;
        break;

    case O_UZI_ITEM:
    case O_UZI_OPTION:
        g_Lara.request_gun_type = LGT_UZIS;
        break;

    case O_HARPOON_ITEM:
    case O_HARPOON_OPTION:
        g_Lara.request_gun_type = LGT_HARPOON;
        break;

    case O_M16_ITEM:
    case O_M16_OPTION:
        g_Lara.request_gun_type = LGT_M16;
        break;

    case O_GRENADE_ITEM:
    case O_GRENADE_OPTION:
        g_Lara.request_gun_type = LGT_GRENADE;
        break;

    case O_SMALL_MEDIPACK_ITEM:
    case O_SMALL_MEDIPACK_OPTION:
        if (item->hit_points > 0 && item->hit_points < LARA_MAX_HITPOINTS) {
            item->hit_points += LARA_MAX_HITPOINTS / 2;
            CLAMPG(item->hit_points, LARA_MAX_HITPOINTS);
            Inv_RemoveItem(O_SMALL_MEDIPACK_ITEM);
            Sound_Effect(SFX_MENU_MEDI, nullptr, SPM_ALWAYS);
            Stats_AddMedipacksUsed(0.5);
        }
        break;

    case O_LARGE_MEDIPACK_ITEM:
    case O_LARGE_MEDIPACK_OPTION:
        if (item->hit_points > 0 && item->hit_points < LARA_MAX_HITPOINTS) {
            item->hit_points = LARA_MAX_HITPOINTS;
            Inv_RemoveItem(O_LARGE_MEDIPACK_ITEM);
            Sound_Effect(SFX_MENU_MEDI, nullptr, SPM_ALWAYS);
            Stats_AddMedipacksUsed(1);
        }
        break;

    case O_FLARES_ITEM:
    case O_FLARES_OPTION:
        g_Lara.request_gun_type = LGT_FLARE;
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
    case O_PUZZLE_OPTION_4: {
        const int16_t receptacle_item_num = Object_FindReceptacle(obj_id);
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

void Lara_InitialiseLoad(const int16_t item_num)
{
    g_Lara.item_num = item_num;
    g_LaraItem = Item_Get(item_num);
}

void Lara_Initialise(const GF_LEVEL *const level)
{
    Lara_SetControllable(true);
    const RESUME_INFO *const resume = Savegame_GetCurrentInfo(level);
    ITEM *const item = g_LaraItem;

    item->data = &g_Lara;
    item->collidable = false;
    if (resume != nullptr) {
        item->hit_points = g_Config.gameplay.disable_healing_between_levels
            ? resume->lara_hitpoints
            : g_Config.gameplay.start_lara_hitpoints;
    }

    g_Lara.hit_direction = -1;
    g_Lara.vehicle_item_num = NO_ITEM;
    g_Lara.gun_item_num = NO_ITEM;
    g_Lara.calc_fall_speed = 0;
    g_Lara.climb_status = false;
    g_Lara.pose_count = 0;
    g_Lara.hit_frame = 0;
    g_Lara.air = LARA_MAX_AIR;
    g_Lara.dive_timer = 0;
    g_Lara.death_timer = 0;
    g_Lara.hit_effect_count = 0;
    g_Lara.flare.age = 0;
    g_Lara.back_gun_obj_id = O_LARA;
    g_Lara.flare.frame_num = 0;
    g_Lara.flare.control = false;
    g_Lara.extra_anim = false;
    g_Lara.enable_look = true;
    g_Lara.burn = false;
    g_Lara.water_surface_dist = 100;
    g_Lara.last_pos = item->pos;
    g_Lara.hit_effect = nullptr;
    g_Lara.mesh_effects = 0;
    g_Lara.target = nullptr;
    g_Lara.turn_rate = 0;
    g_Lara.move_angle = 0;
    g_Lara.head_rot.x = 0;
    g_Lara.head_rot.y = 0;
    g_Lara.head_rot.z = 0;
    g_Lara.torso_rot.x = 0;
    g_Lara.torso_rot.y = 0;
    g_Lara.torso_rot.z = 0;
    g_Lara.left_arm.flash_gun = 0;
    g_Lara.right_arm.flash_gun = 0;
    g_Lara.left_arm.lock = 0;
    g_Lara.right_arm.lock = 0;

    g_Lara.current_active = 0;

    LOT_InitialiseLOT(&g_Lara.lot);
    g_Lara.lot.setup.step = WALL_L * 20;
    g_Lara.lot.setup.drop = -WALL_L * 20;
    g_Lara.lot.setup.fly = STEP_L;

    if ((level->type == GFL_NORMAL || level->type == GFL_BONUS)
        && g_GF_LaraStartAnim) {
        g_Lara.water_status = LWS_ABOVE_WATER;
        g_Lara.gun_status = LGS_HANDS_BUSY;
        Item_SwitchToObjAnim(item, LS_EXTRA_BREATH, 0, O_LARA_EXTRA);
        item->current_anim_state = LS_EXTRA_BREATH;
        item->goal_anim_state = g_GF_LaraStartAnim;
        Lara_Animate(item);
        g_Lara.extra_anim = true;
        Camera_InvokeCinematic(item, 0, 0);
    } else if ((Room_Get(item->room_num)->flags & RF_UNDERWATER)) {
        g_Lara.water_status = LWS_UNDERWATER;
        item->fall_speed = 0;
        item->goal_anim_state = LS_TREAD;
        item->current_anim_state = LS_TREAD;
        Item_SwitchToAnim(item, LA_UNDERWATER_IDLE, 0);
    } else {
        g_Lara.water_status = LWS_ABOVE_WATER;
        item->goal_anim_state = LS_STOP;
        item->current_anim_state = LS_STOP;
        Item_SwitchToAnim(item, LA_STAND_STILL, 0);
    }

    if (level->type == GFL_CUTSCENE) {
        for (int32_t i = 0; i < LM_NUMBER_OF; i++) {
            Lara_SwapSingleMesh(i, O_LARA);
        }

        Lara_SwapSingleMesh(LM_THIGH_L, O_LARA_PISTOLS);
        Lara_SwapSingleMesh(LM_THIGH_R, O_LARA_PISTOLS);
        g_Lara.gun_status = LGS_ARMLESS;
    } else {
        Lara_InitialiseInventory(level);
    }
}

void Lara_InitialiseInventory(const GF_LEVEL *const level)
{
    Inv_RemoveAllItems();
    Inv_AddItem(O_COMPASS_OPTION);

    RESUME_INFO *const resume = Savegame_GetCurrentInfo(level);
    if (resume != nullptr) {
        g_Lara.pistol_ammo.ammo = 1000;
        if (resume->flags.has_pistols) {
            Inv_AddItem(O_PISTOL_ITEM);
        }

        if (resume->flags.has_magnums) {
            Inv_AddItem(O_MAGNUM_ITEM);
            g_Lara.magnum_ammo.ammo = resume->magnum_ammo;
            Item_GlobalReplace(O_MAGNUM_ITEM, O_MAGNUM_AMMO_ITEM);
        } else {
            Inv_AddItemNTimes(O_MAGNUM_AMMO_ITEM, resume->magnum_ammo / 40);
            g_Lara.magnum_ammo.ammo = 0;
        }

        if (resume->flags.has_uzis) {
            Inv_AddItem(O_UZI_ITEM);
            g_Lara.uzi_ammo.ammo = resume->uzi_ammo;
            Item_GlobalReplace(O_UZI_ITEM, O_UZI_AMMO_ITEM);
        } else {
            Inv_AddItemNTimes(O_UZI_AMMO_ITEM, resume->uzi_ammo / 80);
            g_Lara.uzi_ammo.ammo = 0;
        }

        if (resume->flags.has_shotgun) {
            Inv_AddItem(O_SHOTGUN_ITEM);
            g_Lara.shotgun_ammo.ammo = resume->shotgun_ammo;
            Item_GlobalReplace(O_SHOTGUN_ITEM, O_SHOTGUN_AMMO_ITEM);
        } else {
            Inv_AddItemNTimes(O_SHOTGUN_AMMO_ITEM, resume->shotgun_ammo / 12);
            g_Lara.shotgun_ammo.ammo = 0;
        }

        if (resume->flags.has_m16) {
            Inv_AddItem(O_M16_ITEM);
            g_Lara.m16_ammo.ammo = resume->m16_ammo;
            Item_GlobalReplace(O_M16_ITEM, O_M16_AMMO_ITEM);
        } else {
            Inv_AddItemNTimes(O_M16_AMMO_ITEM, resume->m16_ammo / 40);
            g_Lara.m16_ammo.ammo = 0;
        }

        if (resume->flags.has_grenade) {
            Inv_AddItem(O_GRENADE_ITEM);
            g_Lara.grenade_ammo.ammo = resume->grenade_ammo;
            Item_GlobalReplace(O_GRENADE_ITEM, O_GRENADE_AMMO_ITEM);
        } else {
            Inv_AddItemNTimes(O_GRENADE_AMMO_ITEM, resume->grenade_ammo / 2);
            g_Lara.grenade_ammo.ammo = 0;
        }

        if (resume->flags.has_harpoon) {
            Inv_AddItem(O_HARPOON_ITEM);
            g_Lara.harpoon_ammo.ammo = resume->harpoon_ammo;
            Item_GlobalReplace(O_HARPOON_ITEM, O_HARPOON_AMMO_ITEM);
        } else {
            Inv_AddItemNTimes(O_HARPOON_AMMO_ITEM, resume->harpoon_ammo / 3);
            g_Lara.harpoon_ammo.ammo = 0;
        }

        Inv_AddItemNTimes(O_FLARE_ITEM, resume->flares);
        Inv_AddItemNTimes(O_SMALL_MEDIPACK_ITEM, resume->small_medipacks);
        Inv_AddItemNTimes(O_LARGE_MEDIPACK_ITEM, resume->large_medipacks);

        g_Lara.last_gun_type = resume->equipped_gun_type;
    }

    g_Lara.gun_status = LGS_ARMLESS;
    g_Lara.gun_type = g_Lara.last_gun_type;
    g_Lara.request_gun_type = g_Lara.last_gun_type;

    Lara_InitialiseMeshes(level);
    Gun_InitialiseNewWeapon();
}

void Lara_InitialiseMeshes(const GF_LEVEL *const level)
{
    for (int32_t i = 0; i < LM_NUMBER_OF; i++) {
        Lara_SwapSingleMesh(i, O_LARA);
    }

    const RESUME_INFO *const resume = Savegame_GetCurrentInfo(level);
    if (resume == nullptr) {
        return;
    }

    GAME_OBJECT_ID holster_obj_id = NO_OBJECT;
    if (resume->equipped_gun_type != LGT_UNARMED) {
        if (resume->equipped_gun_type == LGT_MAGNUMS) {
            holster_obj_id = O_LARA_MAGNUMS;
        } else if (resume->equipped_gun_type == LGT_UZIS) {
            holster_obj_id = O_LARA_UZIS;
        } else {
            holster_obj_id = O_LARA_PISTOLS;
        }
    }

    if (holster_obj_id != NO_OBJECT) {
        Lara_SwapSingleMesh(LM_THIGH_L, holster_obj_id);
        Lara_SwapSingleMesh(LM_THIGH_R, holster_obj_id);
    }

    if (resume->equipped_gun_type == LGT_FLARE) {
        Lara_SwapSingleMesh(LM_HAND_L, O_LARA_FLARE);
    }

    switch (resume->equipped_gun_type) {
    case LGT_M16:
        g_Lara.back_gun_obj_id = O_LARA_M16;
        return;

    case LGT_GRENADE:
        g_Lara.back_gun_obj_id = O_LARA_GRENADE;
        return;

    case LGT_HARPOON:
        g_Lara.back_gun_obj_id = O_LARA_HARPOON;
        return;

    default:
        break;
    }

    if (resume->flags.has_shotgun) {
        g_Lara.back_gun_obj_id = O_LARA_SHOTGUN;
    } else if (resume->flags.has_m16) {
        g_Lara.back_gun_obj_id = O_LARA_M16;
    } else if (resume->flags.has_grenade) {
        g_Lara.back_gun_obj_id = O_LARA_GRENADE;
    } else if (resume->flags.has_harpoon) {
        g_Lara.back_gun_obj_id = O_LARA_HARPOON;
    }
}
