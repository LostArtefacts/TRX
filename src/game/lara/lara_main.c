#include "game/lara/lara_main.h"

#include "config.h"
#include "game/camera.h"
#include "game/collide.h"
#include "game/gameflow.h"
#include "game/gun.h"
#include "game/input.h"
#include "game/inventory/inventory_func.h"
#include "game/items.h"
#include "game/lara/lara_control.h"
#include "game/lot.h"
#include "game/music.h"
#include "game/objects/effects/splash.h"
#include "game/room.h"
#include "game/savegame.h"
#include "game/sound.h"
#include "global/vars.h"
#include "log.h"
#include "math/math.h"

#define LARA_MOVE_TIMEOUT 90
#define LARA_MOVE_ANIM_VELOCITY 12
#define LARA_MOVE_SPEED 16

void Lara_Control(void)
{
    COLL_INFO coll = { 0 };

    ITEM_INFO *item = g_LaraItem;
    ROOM_INFO *r = &g_RoomInfo[item->room_number];
    int32_t room_submerged = r->flags & RF_UNDERWATER;

    if (g_Lara.interact_target.is_moving
        && g_Lara.interact_target.move_count++ > LARA_MOVE_TIMEOUT) {
        g_Lara.interact_target.is_moving = false;
        g_Lara.gun_status = LGS_ARMLESS;
    }

    if (g_Input.level_skip_cheat) {
        g_LevelComplete = true;
    }

    if (g_Input.health_cheat) {
        item->hit_points +=
            (g_Input.slow ? -2 : 2) * LARA_HITPOINTS / 100; // change by 2%
        CLAMP(item->hit_points, 0, LARA_HITPOINTS);
    }

    if (g_InputDB.item_cheat) {
        Lara_CheatGetStuff();
    }

    if (g_Lara.water_status != LWS_CHEAT && g_Input.fly_cheat) {
        if (g_Lara.water_status != LWS_UNDERWATER || item->hit_points <= 0) {
            item->pos.y -= 0x80;
            item->current_anim_state = LS_SWIM;
            item->goal_anim_state = LS_SWIM;
            item->anim_number = LA_SWIM_GLIDE;
            item->frame_number = g_Anims[item->anim_number].frame_base;
            item->gravity_status = 0;
            item->pos.x_rot = 30 * PHD_DEGREE;
            item->fall_speed = 30;
            g_Lara.head_x_rot = 0;
            g_Lara.head_y_rot = 0;
            g_Lara.torso_x_rot = 0;
            g_Lara.torso_y_rot = 0;
        }
        g_Lara.water_status = LWS_CHEAT;
        g_Lara.spaz_effect_count = 0;
        g_Lara.spaz_effect = NULL;
        g_Lara.hit_frame = 0;
        g_Lara.hit_direction = -1;
        g_Lara.air = LARA_AIR;
        g_Lara.death_timer = 0;
        g_Lara.mesh_effects = 0;
        Lara_InitialiseMeshes(g_CurrentLevel);
    }

    if (g_Lara.water_status == LWS_ABOVE_WATER && room_submerged) {
        g_Lara.water_status = LWS_UNDERWATER;
        g_Lara.air = LARA_AIR;
        item->pos.y += 100;
        item->gravity_status = 0;
        Item_UpdateRoom(item, 0);
        Sound_StopEffect(SFX_LARA_FALL, NULL);
        if (item->current_anim_state == LS_SWAN_DIVE) {
            item->goal_anim_state = LS_DIVE;
            item->pos.x_rot = -45 * PHD_DEGREE;
            Lara_Animate(item);
            item->fall_speed *= 2;
        } else if (item->current_anim_state == LS_FAST_DIVE) {
            item->goal_anim_state = LS_DIVE;
            item->pos.x_rot = -85 * PHD_DEGREE;
            Lara_Animate(item);
            item->fall_speed *= 2;
        } else {
            item->current_anim_state = LS_DIVE;
            item->goal_anim_state = LS_SWIM;
            item->anim_number = LA_JUMP_IN;
            item->frame_number = AF_JUMPIN;
            item->pos.x_rot = -45 * PHD_DEGREE;
            item->fall_speed = (item->fall_speed * 3) / 2;
        }
        g_Lara.head_x_rot = 0;
        g_Lara.head_y_rot = 0;
        g_Lara.torso_x_rot = 0;
        g_Lara.torso_y_rot = 0;
        Splash_Spawn(item);
    } else if (g_Lara.water_status == LWS_UNDERWATER && !room_submerged) {
        int16_t wh = Room_GetWaterHeight(
            item->pos.x, item->pos.y, item->pos.z, item->room_number);
        if (wh != NO_HEIGHT && ABS(wh - item->pos.y) < STEP_L) {
            g_Lara.water_status = LWS_SURFACE;
            g_Lara.dive_timer = DIVE_WAIT + 1;
            item->current_anim_state = LS_SURF_TREAD;
            item->goal_anim_state = LS_SURF_TREAD;
            item->anim_number = LA_SURF_TREAD;
            item->frame_number = AF_SURFTREAD;
            item->fall_speed = 0;
            item->pos.y = wh + 1;
            item->pos.x_rot = 0;
            item->pos.z_rot = 0;
            g_Lara.head_x_rot = 0;
            g_Lara.head_y_rot = 0;
            g_Lara.torso_x_rot = 0;
            g_Lara.torso_y_rot = 0;
            Item_UpdateRoom(item, -LARA_HEIGHT / 2);
            Sound_Effect(SFX_LARA_BREATH, &item->pos, SPM_ALWAYS);
        } else {
            g_Lara.water_status = LWS_ABOVE_WATER;
            g_Lara.gun_status = LGS_ARMLESS;
            item->current_anim_state = LS_JUMP_FORWARD;
            item->goal_anim_state = LS_JUMP_FORWARD;
            item->anim_number = LA_FALL_DOWN;
            item->frame_number = AF_FALLDOWN;
            item->speed = item->fall_speed / 4;
            item->fall_speed = 0;
            item->gravity_status = 1;
            item->pos.x_rot = 0;
            item->pos.z_rot = 0;
            g_Lara.head_x_rot = 0;
            g_Lara.head_y_rot = 0;
            g_Lara.torso_x_rot = 0;
            g_Lara.torso_y_rot = 0;
        }
    } else if (g_Lara.water_status == LWS_SURFACE && !room_submerged) {
        g_Lara.water_status = LWS_ABOVE_WATER;
        g_Lara.gun_status = LGS_ARMLESS;
        item->current_anim_state = LS_JUMP_FORWARD;
        item->goal_anim_state = LS_JUMP_FORWARD;
        item->anim_number = LA_FALL_DOWN;
        item->frame_number = AF_FALLDOWN;
        item->speed = item->fall_speed / 4;
        item->fall_speed = 0;
        item->gravity_status = 1;
        item->pos.x_rot = 0;
        item->pos.z_rot = 0;
        g_Lara.head_x_rot = 0;
        g_Lara.head_y_rot = 0;
        g_Lara.torso_x_rot = 0;
        g_Lara.torso_y_rot = 0;
    }

    if (item->hit_points <= 0) {
        item->hit_points = -1;
        if (!g_Lara.death_timer) {
            Music_Stop();
            g_GameInfo.current[g_CurrentLevel].stats.death_count++;
            if (g_GameInfo.current_save_slot != -1) {
                Savegame_UpdateDeathCounters(
                    g_GameInfo.current_save_slot, &g_GameInfo);
            }
        }
        g_Lara.death_timer++;
        // make sure the enemy healthbar is no longer rendered. If g_Lara later
        // is resurrected with DOZY, she should no longer aim at the target.
        g_Lara.target = NULL;
    }

    int16_t camera_move_delta = PHD_45 / 30;

    if (g_Input.camera_left) {
        Camera_OffsetAdditionalAngle(camera_move_delta);
    } else if (g_Input.camera_right) {
        Camera_OffsetAdditionalAngle(-camera_move_delta);
    }
    if (g_Input.camera_up) {
        Camera_OffsetAdditionalElevation(-camera_move_delta);
    } else if (g_Input.camera_down) {
        Camera_OffsetAdditionalElevation(camera_move_delta);
    }
    if (g_Input.camera_reset) {
        Camera_OffsetReset();
    }

    switch (g_Lara.water_status) {
    case LWS_ABOVE_WATER:
        g_Lara.air = LARA_AIR;
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
            if (g_Lara.air > LARA_AIR) {
                g_Lara.air = LARA_AIR;
            }
        }
        Lara_HandleSurface(item, &coll);
        break;

    case LWS_CHEAT:
        item->hit_points = LARA_HITPOINTS;
        g_Lara.death_timer = 0;
        Lara_HandleUnderwater(item, &coll);
        if (g_Input.slow && !g_Input.look && !g_Input.fly_cheat) {
            int16_t wh = Room_GetWaterHeight(
                item->pos.x, item->pos.y, item->pos.z, item->room_number);
            if (room_submerged || (wh != NO_HEIGHT && wh > 0)) {
                g_Lara.water_status = LWS_UNDERWATER;
            } else {
                g_Lara.water_status = LWS_ABOVE_WATER;
                item->anim_number = LA_STOP;
                item->frame_number = g_Anims[item->anim_number].frame_base;
                item->pos.x_rot = item->pos.z_rot = 0;
                g_Lara.head_x_rot = 0;
                g_Lara.head_y_rot = 0;
                g_Lara.torso_x_rot = 0;
                g_Lara.torso_y_rot = 0;
            }
            g_Lara.gun_status = LGS_ARMLESS;
        }
        break;
    }
}

void Lara_SwapMeshExtra(void)
{
    if (!g_Objects[O_LARA_EXTRA].loaded) {
        return;
    }
    for (int i = 0; i < LM_NUMBER_OF; i++) {
        g_Lara.mesh_ptrs[i] = g_Meshes[g_Objects[O_LARA_EXTRA].mesh_index + i];
    }
}

void Lara_Animate(ITEM_INFO *item)
{
    int16_t *command;
    ANIM_STRUCT *anim;

    item->frame_number++;
    anim = &g_Anims[item->anim_number];
    if (anim->number_changes > 0 && Item_GetAnimChange(item, anim)) {
        anim = &g_Anims[item->anim_number];
        item->current_anim_state = anim->current_anim_state;
    }

    if (item->frame_number > anim->frame_end) {
        if (anim->number_commands > 0) {
            command = &g_AnimCommands[anim->command_index];
            for (int i = 0; i < anim->number_commands; i++) {
                switch (*command++) {
                case AC_MOVE_ORIGIN:
                    Item_Translate(item, command[0], command[1], command[2]);
                    command += 3;
                    break;

                case AC_JUMP_VELOCITY:
                    item->fall_speed = command[0];
                    item->speed = command[1];
                    command += 2;
                    item->gravity_status = 1;
                    if (g_Lara.calc_fall_speed) {
                        item->fall_speed = g_Lara.calc_fall_speed;
                        g_Lara.calc_fall_speed = 0;
                    }
                    break;

                case AC_ATTACK_READY:
                    g_Lara.gun_status = LGS_ARMLESS;
                    break;

                case AC_SOUND_FX:
                case AC_EFFECT:
                    command += 2;
                    break;
                }
            }
        }

        item->anim_number = anim->jump_anim_num;
        item->frame_number = anim->jump_frame_num;

        anim = &g_Anims[anim->jump_anim_num];
        item->current_anim_state = anim->current_anim_state;
    }

    if (anim->number_commands > 0) {
        command = &g_AnimCommands[anim->command_index];
        for (int i = 0; i < anim->number_commands; i++) {
            switch (*command++) {
            case AC_MOVE_ORIGIN:
                command += 3;
                break;

            case AC_JUMP_VELOCITY:
                command += 2;
                break;

            case AC_SOUND_FX:
                if (item->frame_number == command[0]) {
                    Sound_Effect(command[1], &item->pos, SPM_ALWAYS);
                }
                command += 2;
                break;

            case AC_EFFECT:
                if (item->frame_number == command[0]) {
                    g_EffectRoutines[command[1]](item);
                }
                command += 2;
                break;
            }
        }
    }

    if (item->gravity_status) {
        int32_t speed = anim->velocity
            + anim->acceleration * (item->frame_number - anim->frame_base - 1);
        item->speed -= (int16_t)(speed >> 16);
        speed += anim->acceleration;
        item->speed += (int16_t)(speed >> 16);

        item->fall_speed += (item->fall_speed < FASTFALL_SPEED) ? GRAVITY : 1;
        item->pos.y += item->fall_speed;
    } else {
        int32_t speed = anim->velocity;
        if (anim->acceleration) {
            speed +=
                anim->acceleration * (item->frame_number - anim->frame_base);
        }
        item->speed = (int16_t)(speed >> 16);
    }

    item->pos.x += (Math_Sin(g_Lara.move_angle) * item->speed) >> W2V_SHIFT;
    item->pos.z += (Math_Cos(g_Lara.move_angle) * item->speed) >> W2V_SHIFT;
}

void Lara_AnimateUntil(ITEM_INFO *lara_item, int32_t goal)
{
    lara_item->goal_anim_state = goal;
    do {
        Lara_Animate(lara_item);
    } while (lara_item->current_anim_state != goal);
}

void Lara_UseItem(int16_t object_num)
{
    LOG_INFO("%d", object_num);
    switch (object_num) {
    case O_GUN_ITEM:
    case O_GUN_OPTION:
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
            || g_LaraItem->hit_points >= LARA_HITPOINTS) {
            return;
        }
        g_LaraItem->hit_points += LARA_HITPOINTS / 2;
        CLAMPG(g_LaraItem->hit_points, LARA_HITPOINTS);
        Inv_RemoveItem(O_MEDI_ITEM);
        Sound_Effect(SFX_MENU_MEDI, NULL, SPM_ALWAYS);
        break;

    case O_BIGMEDI_ITEM:
    case O_BIGMEDI_OPTION:
        if (g_LaraItem->hit_points <= 0
            || g_LaraItem->hit_points >= LARA_HITPOINTS) {
            return;
        }
        g_LaraItem->hit_points = g_LaraItem->hit_points + LARA_HITPOINTS;
        CLAMPG(g_LaraItem->hit_points, LARA_HITPOINTS);
        Inv_RemoveItem(O_BIGMEDI_ITEM);
        Sound_Effect(SFX_MENU_MEDI, NULL, SPM_ALWAYS);
        break;
    }
}

void Lara_ControlExtra(int16_t item_num)
{
    Item_Animate(&g_Items[item_num]);
}

void Lara_InitialiseLoad(int16_t item_num)
{
    g_Lara.item_number = item_num;
    if (item_num == NO_ITEM) {
        g_LaraItem = NULL;
    } else {
        g_LaraItem = &g_Items[item_num];
    }
}

void Lara_Initialise(int32_t level_num)
{
    RESUME_INFO *resume = &g_GameInfo.current[level_num];

    g_LaraItem->collidable = 0;
    g_LaraItem->data = &g_Lara;
    if (g_Config.disable_healing_between_levels) {
        g_LaraItem->hit_points = resume->lara_hitpoints;
    } else {
        g_LaraItem->hit_points = g_Config.start_lara_hitpoints;
    }

    g_Lara.air = LARA_AIR;
    g_Lara.torso_y_rot = 0;
    g_Lara.torso_x_rot = 0;
    g_Lara.torso_z_rot = 0;
    g_Lara.head_y_rot = 0;
    g_Lara.head_x_rot = 0;
    g_Lara.head_z_rot = 0;
    g_Lara.calc_fall_speed = 0;
    g_Lara.mesh_effects = 0;
    g_Lara.hit_frame = 0;
    g_Lara.hit_direction = 0;
    g_Lara.death_timer = 0;
    g_Lara.target = NULL;
    g_Lara.spaz_effect = NULL;
    g_Lara.spaz_effect_count = 0;
    g_Lara.turn_rate = 0;
    g_Lara.move_angle = 0;
    g_Lara.right_arm.flash_gun = 0;
    g_Lara.left_arm.flash_gun = 0;
    g_Lara.right_arm.lock = 0;
    g_Lara.left_arm.lock = 0;

    if (g_RoomInfo[g_LaraItem->room_number].flags & 1) {
        g_Lara.water_status = LWS_UNDERWATER;
        g_LaraItem->fall_speed = 0;
        g_LaraItem->goal_anim_state = LS_TREAD;
        g_LaraItem->current_anim_state = LS_TREAD;
        g_LaraItem->anim_number = LA_TREAD;
        g_LaraItem->frame_number = AF_TREAD;
    } else {
        g_Lara.water_status = LWS_ABOVE_WATER;
        g_LaraItem->goal_anim_state = LS_STOP;
        g_LaraItem->current_anim_state = LS_STOP;
        g_LaraItem->anim_number = LA_STOP;
        g_LaraItem->frame_number = AF_STOP;
    }

    g_Lara.current_active = 0;

    LOT_InitialiseLOT(&g_Lara.LOT);
    g_Lara.LOT.step = WALL_L * 20;
    g_Lara.LOT.drop = -WALL_L * 20;
    g_Lara.LOT.fly = STEP_L;

    Lara_InitialiseInventory(level_num);
}

void Lara_InitialiseInventory(int32_t level_num)
{
    Inv_RemoveAllItems();

    RESUME_INFO *resume = &g_GameInfo.current[level_num];

    g_Lara.pistols.ammo = 1000;
    if (resume->flags.got_pistols) {
        Inv_AddItem(O_GUN_ITEM);
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
        Inv_AddItem(O_SCION_ITEM);
    }

    for (int i = 0; i < resume->num_medis; i++) {
        Inv_AddItem(O_MEDI_ITEM);
    }

    for (int i = 0; i < resume->num_big_medis; i++) {
        Inv_AddItem(O_BIGMEDI_ITEM);
    }

    g_Lara.gun_status = resume->gun_status;
    g_Lara.gun_type = resume->gun_type;
    g_Lara.request_gun_type = resume->gun_type;

    Lara_InitialiseMeshes(level_num);
    Gun_InitialiseNewWeapon();
}

void Lara_InitialiseMeshes(int32_t level_num)
{
    RESUME_INFO *resume = &g_GameInfo.current[level_num];

    if (resume->flags.costume) {
        for (int i = 0; i < LM_NUMBER_OF; i++) {
            int32_t use_orig_mesh = i == LM_HEAD;
            g_Lara.mesh_ptrs[i] = g_Meshes
                [g_Objects[use_orig_mesh ? O_LARA : O_LARA_EXTRA].mesh_index
                 + i];
        }
        return;
    }

    for (int i = 0; i < LM_NUMBER_OF; i++) {
        g_Lara.mesh_ptrs[i] = g_Meshes[g_Objects[O_LARA].mesh_index + i];
    }

    GAME_OBJECT_ID back_object_num = O_INVALID;
    GAME_OBJECT_ID hands_object_num = O_INVALID;
    GAME_OBJECT_ID holster_object_num = O_INVALID;
    switch (resume->gun_type) {
    case LGT_SHOTGUN:
        hands_object_num = O_SHOTGUN;
        break;
    case LGT_PISTOLS:
        holster_object_num = O_PISTOLS;
        break;
    case LGT_MAGNUMS:
        holster_object_num = O_MAGNUM;
        break;
    case LGT_UZIS:
        holster_object_num = O_UZI;
        break;
    }

    if (resume->flags.got_shotgun) {
        back_object_num = O_SHOTGUN;
    }

    if (g_Lara.gun_status != LGS_ARMLESS && hands_object_num != O_INVALID) {
        g_Lara.mesh_ptrs[LM_HAND_L] =
            g_Meshes[g_Objects[hands_object_num].mesh_index + LM_HAND_L];
        g_Lara.mesh_ptrs[LM_HAND_R] =
            g_Meshes[g_Objects[hands_object_num].mesh_index + LM_HAND_R];
    }

    if (holster_object_num != O_INVALID) {
        g_Lara.mesh_ptrs[LM_THIGH_L] =
            g_Meshes[g_Objects[holster_object_num].mesh_index + LM_THIGH_L];
        g_Lara.mesh_ptrs[LM_THIGH_R] =
            g_Meshes[g_Objects[holster_object_num].mesh_index + LM_THIGH_R];
    }

    if (back_object_num != O_INVALID) {
        g_Lara.mesh_ptrs[LM_TORSO] =
            g_Meshes[g_Objects[back_object_num].mesh_index + LM_TORSO];
    }
}

bool Lara_IsNearItem(PHD_3DPOS *pos, int32_t distance)
{
    return Item_IsNearItem(g_LaraItem, pos, distance);
}

bool Lara_TestBoundsCollide(ITEM_INFO *item, int32_t radius)
{
    return Item_TestBoundsCollide(g_LaraItem, item, radius);
}

bool Lara_TestPosition(ITEM_INFO *item, int16_t *bounds)
{
    return Item_TestPosition(g_LaraItem, item, bounds);
}

void Lara_AlignPosition(ITEM_INFO *item, PHD_VECTOR *vec)
{
    Item_AlignPosition(g_LaraItem, item, vec);
}

bool Lara_MovePosition(ITEM_INFO *item, PHD_VECTOR *vec)
{
    int32_t velocity =
        g_Config.walk_to_items && g_Lara.water_status != LWS_UNDERWATER
        ? LARA_MOVE_ANIM_VELOCITY
        : LARA_MOVE_SPEED;

    bool ret = Item_MovePosition(g_LaraItem, item, vec, velocity);

    if (!g_Config.walk_to_items || g_Lara.interact_target.is_moving) {
        return ret;
    }

    if (g_Lara.water_status != LWS_UNDERWATER) {
        const int16_t step_to_anim_num[4] = {
            LA_SIDE_STEP_LEFT,
            LA_WALK_FORWARD,
            LA_SIDE_STEP_RIGHT,
            LA_WALK_BACK,
        };
        const int16_t step_to_anim_state[4] = {
            LS_STEP_LEFT,
            LS_WALK,
            LS_STEP_RIGHT,
            LS_BACK,
        };

        int32_t angle = (PHD_ONE
                         - Math_Atan(
                             g_LaraItem->pos.x - item->pos.x,
                             g_LaraItem->pos.z - item->pos.z))
            % PHD_ONE;
        uint32_t quadrant =
            ((((uint32_t)(angle + PHD_45) >> W2V_SHIFT)
              - ((uint16_t)(item->pos.y_rot + PHD_45) >> W2V_SHIFT))
             & 0x3);

        g_LaraItem->anim_number = step_to_anim_num[quadrant];
        g_LaraItem->frame_number = g_Anims[g_LaraItem->anim_number].frame_base;
        g_LaraItem->goal_anim_state = step_to_anim_state[quadrant];
        g_LaraItem->current_anim_state = step_to_anim_state[quadrant];

        g_Lara.gun_status = LGS_HANDS_BUSY;
    }

    g_Lara.interact_target.is_moving = true;
    g_Lara.interact_target.move_count = 0;

    return ret;
}

void Lara_Push(ITEM_INFO *item, COLL_INFO *coll, bool spaz_on, bool big_push)
{
    struct ITEM_INFO *lara_item = g_LaraItem;
    int32_t x = lara_item->pos.x - item->pos.x;
    int32_t z = lara_item->pos.z - item->pos.z;
    int32_t c = Math_Cos(item->pos.y_rot);
    int32_t s = Math_Sin(item->pos.y_rot);
    int32_t rx = (c * x - s * z) >> W2V_SHIFT;
    int32_t rz = (c * z + s * x) >> W2V_SHIFT;

    int16_t *bounds = Item_GetBestFrame(item);
    int32_t minx = bounds[FRAME_BOUND_MIN_X];
    int32_t maxx = bounds[FRAME_BOUND_MAX_X];
    int32_t minz = bounds[FRAME_BOUND_MIN_Z];
    int32_t maxz = bounds[FRAME_BOUND_MAX_Z];

    if (big_push) {
        minx -= coll->radius;
        maxx += coll->radius;
        minz -= coll->radius;
        maxz += coll->radius;
    }

    if (rx >= minx && rx <= maxx && rz >= minz && rz <= maxz) {
        int32_t l = rx - minx;
        int32_t r = maxx - rx;
        int32_t t = maxz - rz;
        int32_t b = rz - minz;

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

        rx = (bounds[FRAME_BOUND_MIN_X] + bounds[FRAME_BOUND_MAX_X]) / 2;
        rz = (bounds[FRAME_BOUND_MIN_Z] + bounds[FRAME_BOUND_MAX_Z]) / 2;
        x -= (c * rx + s * rz) >> W2V_SHIFT;
        z -= (c * rz - s * rx) >> W2V_SHIFT;

        if (spaz_on) {
            PHD_ANGLE hitang =
                lara_item->pos.y_rot - (PHD_180 + Math_Atan(z, x));
            g_Lara.hit_direction = (hitang + PHD_45) / PHD_90;
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
            lara_item->room_number, LARA_HEIGHT);
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
    }
}
