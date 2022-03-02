#include "game/lara.h"

#include "3dsystem/phd_math.h"
#include "config.h"
#include "game/camera.h"
#include "game/collide.h"
#include "game/control.h"
#include "game/effects/splash.h"
#include "game/gameflow.h"
#include "game/input.h"
#include "game/inv.h"
#include "game/items.h"
#include "game/lot.h"
#include "game/music.h"
#include "game/savegame.h"
#include "game/sound.h"
#include "global/const.h"
#include "global/types.h"
#include "global/vars.h"
#include "log.h"
#include "util.h"

#include <stddef.h>
#include <stdint.h>

#define MOVE_TIMEOUT 90

void LaraControl(int16_t item_num)
{
    COLL_INFO coll = { 0 };

    ITEM_INFO *item = g_LaraItem;
    ROOM_INFO *r = &g_RoomInfo[item->room_number];
    int32_t room_submerged = r->flags & RF_UNDERWATER;

    if (g_Lara.interact_target.is_moving
        && g_Lara.interact_target.move_count++ > MOVE_TIMEOUT) {
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
        LaraCheatGetStuff();
    }

    if (g_Lara.water_status != LWS_CHEAT && g_Input.fly_cheat) {
        if (g_Lara.water_status != LWS_UNDERWATER || item->hit_points <= 0) {
            item->pos.y -= 0x80;
            item->current_anim_state = AS_SWIM;
            item->goal_anim_state = AS_SWIM;
            item->anim_number = AA_SWIMGLIDE;
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
        LaraInitialiseMeshes(g_CurrentLevel);
    }

    if (g_Lara.water_status == LWS_ABOVEWATER && room_submerged) {
        g_Lara.water_status = LWS_UNDERWATER;
        g_Lara.air = LARA_AIR;
        item->pos.y += 100;
        item->gravity_status = 0;
        UpdateLaraRoom(item, 0);
        Sound_StopEffect(SFX_LARA_FALL, NULL);
        if (item->current_anim_state == AS_SWANDIVE) {
            item->goal_anim_state = AS_DIVE;
            item->pos.x_rot = -45 * PHD_DEGREE;
            AnimateLara(item);
            item->fall_speed *= 2;
        } else if (item->current_anim_state == AS_FASTDIVE) {
            item->goal_anim_state = AS_DIVE;
            item->pos.x_rot = -85 * PHD_DEGREE;
            AnimateLara(item);
            item->fall_speed *= 2;
        } else {
            item->current_anim_state = AS_DIVE;
            item->goal_anim_state = AS_SWIM;
            item->anim_number = AA_JUMPIN;
            item->frame_number = AF_JUMPIN;
            item->pos.x_rot = -45 * PHD_DEGREE;
            item->fall_speed = (item->fall_speed * 3) / 2;
        }
        g_Lara.head_x_rot = 0;
        g_Lara.head_y_rot = 0;
        g_Lara.torso_x_rot = 0;
        g_Lara.torso_y_rot = 0;
        Splash(item);
    } else if (g_Lara.water_status == LWS_UNDERWATER && !room_submerged) {
        int16_t wh = GetWaterHeight(
            item->pos.x, item->pos.y, item->pos.z, item->room_number);
        if (wh != NO_HEIGHT && ABS(wh - item->pos.y) < STEP_L) {
            g_Lara.water_status = LWS_SURFACE;
            g_Lara.dive_timer = DIVE_WAIT + 1;
            item->current_anim_state = AS_SURFTREAD;
            item->goal_anim_state = AS_SURFTREAD;
            item->anim_number = AA_SURFTREAD;
            item->frame_number = AF_SURFTREAD;
            item->fall_speed = 0;
            item->pos.y = wh + 1;
            item->pos.x_rot = 0;
            item->pos.z_rot = 0;
            g_Lara.head_x_rot = 0;
            g_Lara.head_y_rot = 0;
            g_Lara.torso_x_rot = 0;
            g_Lara.torso_y_rot = 0;
            UpdateLaraRoom(item, -LARA_HITE / 2);
            Sound_Effect(SFX_LARA_BREATH, &item->pos, SPM_ALWAYS);
        } else {
            g_Lara.water_status = LWS_ABOVEWATER;
            g_Lara.gun_status = LGS_ARMLESS;
            item->current_anim_state = AS_FORWARDJUMP;
            item->goal_anim_state = AS_FORWARDJUMP;
            item->anim_number = AA_FALLDOWN;
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
        g_Lara.water_status = LWS_ABOVEWATER;
        g_Lara.gun_status = LGS_ARMLESS;
        item->current_anim_state = AS_FORWARDJUMP;
        item->goal_anim_state = AS_FORWARDJUMP;
        item->anim_number = AA_FALLDOWN;
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
            g_GameInfo.end[g_CurrentLevel].stats.death_count++;
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
        CameraOffsetAdditionalAngle(camera_move_delta);
    } else if (g_Input.camera_right) {
        CameraOffsetAdditionalAngle(-camera_move_delta);
    }
    if (g_Input.camera_up) {
        CameraOffsetAdditionalElevation(-camera_move_delta);
    } else if (g_Input.camera_down) {
        CameraOffsetAdditionalElevation(camera_move_delta);
    }
    if (g_Input.camera_reset) {
        CameraOffsetReset();
    }

    switch (g_Lara.water_status) {
    case LWS_ABOVEWATER:
        g_Lara.air = LARA_AIR;
        LaraAboveWater(item, &coll);
        break;

    case LWS_UNDERWATER:
        if (item->hit_points >= 0) {
            g_Lara.air--;
            if (g_Lara.air < 0) {
                g_Lara.air = -1;
                item->hit_points -= 5;
            }
        }
        LaraUnderWater(item, &coll);
        break;

    case LWS_SURFACE:
        if (item->hit_points >= 0) {
            g_Lara.air += 10;
            if (g_Lara.air > LARA_AIR) {
                g_Lara.air = LARA_AIR;
            }
        }
        LaraSurface(item, &coll);
        break;

    case LWS_CHEAT:
        item->hit_points = LARA_HITPOINTS;
        g_Lara.death_timer = 0;
        LaraUnderWater(item, &coll);
        if (g_Input.slow && !g_Input.look && !g_Input.fly_cheat) {
            int16_t wh = GetWaterHeight(
                item->pos.x, item->pos.y, item->pos.z, item->room_number);
            if (room_submerged || (wh != NO_HEIGHT && wh > 0)) {
                g_Lara.water_status = LWS_UNDERWATER;
            } else {
                g_Lara.water_status = LWS_ABOVEWATER;
                item->anim_number = AA_STOP;
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

void LaraSwapMeshExtra()
{
    if (!g_Objects[O_LARA_EXTRA].loaded) {
        return;
    }
    for (int i = 0; i < LM_NUMBER_OF; i++) {
        g_Lara.mesh_ptrs[i] = g_Meshes[g_Objects[O_LARA_EXTRA].mesh_index + i];
    }
}

void AnimateLara(ITEM_INFO *item)
{
    int16_t *command;
    ANIM_STRUCT *anim;

    item->frame_number++;
    anim = &g_Anims[item->anim_number];
    if (anim->number_changes > 0 && GetChange(item, anim)) {
        anim = &g_Anims[item->anim_number];
        item->current_anim_state = anim->current_anim_state;
    }

    if (item->frame_number > anim->frame_end) {
        if (anim->number_commands > 0) {
            command = &g_AnimCommands[anim->command_index];
            for (int i = 0; i < anim->number_commands; i++) {
                switch (*command++) {
                case AC_MOVE_ORIGIN:
                    TranslateItem(item, command[0], command[1], command[2]);
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

    item->pos.x += (phd_sin(g_Lara.move_angle) * item->speed) >> W2V_SHIFT;
    item->pos.z += (phd_cos(g_Lara.move_angle) * item->speed) >> W2V_SHIFT;
}

void AnimateLaraUntil(ITEM_INFO *lara_item, int32_t goal)
{
    lara_item->goal_anim_state = goal;
    do {
        AnimateLara(lara_item);
    } while (lara_item->current_anim_state != goal);
}

void UseItem(int16_t object_num)
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

void ControlLaraExtra(int16_t item_num)
{
    AnimateItem(&g_Items[item_num]);
}

void InitialiseLaraLoad(int16_t item_num)
{
    g_Lara.item_number = item_num;
    if (item_num == NO_ITEM) {
        g_LaraItem = NULL;
    } else {
        g_LaraItem = &g_Items[item_num];
    }
}

void InitialiseLara()
{
    START_INFO *start = &g_GameInfo.start[g_CurrentLevel];

    g_LaraItem->collidable = 0;
    g_LaraItem->data = &g_Lara;
    g_LaraItem->hit_points = start->lara_hitpoints;

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
        g_LaraItem->goal_anim_state = AS_TREAD;
        g_LaraItem->current_anim_state = AS_TREAD;
        g_LaraItem->anim_number = AA_TREAD;
        g_LaraItem->frame_number = AF_TREAD;
    } else {
        g_Lara.water_status = LWS_ABOVEWATER;
        g_LaraItem->goal_anim_state = AS_STOP;
        g_LaraItem->current_anim_state = AS_STOP;
        g_LaraItem->anim_number = AA_STOP;
        g_LaraItem->frame_number = AF_STOP;
    }

    g_Lara.current_active = 0;

    InitialiseLOT(&g_Lara.LOT);
    g_Lara.LOT.step = WALL_L * 20;
    g_Lara.LOT.drop = -WALL_L * 20;
    g_Lara.LOT.fly = STEP_L;

    InitialiseLaraInventory(g_CurrentLevel);
}

void InitialiseLaraInventory(int32_t level_num)
{
    Inv_RemoveAllItems();

    START_INFO *start = &g_GameInfo.start[level_num];

    g_Lara.pistols.ammo = 1000;
    if (start->flags.got_pistols) {
        Inv_AddItem(O_GUN_ITEM);
    }

    if (start->flags.got_magnums) {
        Inv_AddItem(O_MAGNUM_ITEM);
        g_Lara.magnums.ammo = start->magnum_ammo;
        GlobalItemReplace(O_MAGNUM_ITEM, O_MAG_AMMO_ITEM);
    } else {
        int32_t ammo = start->magnum_ammo / MAGNUM_AMMO_QTY;
        for (int i = 0; i < ammo; i++) {
            Inv_AddItem(O_MAG_AMMO_ITEM);
        }
        g_Lara.magnums.ammo = 0;
    }

    if (start->flags.got_uzis) {
        Inv_AddItem(O_UZI_ITEM);
        g_Lara.uzis.ammo = start->uzi_ammo;
        GlobalItemReplace(O_UZI_ITEM, O_UZI_AMMO_ITEM);
    } else {
        int32_t ammo = start->uzi_ammo / UZI_AMMO_QTY;
        for (int i = 0; i < ammo; i++) {
            Inv_AddItem(O_UZI_AMMO_ITEM);
        }
        g_Lara.uzis.ammo = 0;
    }

    if (start->flags.got_shotgun) {
        Inv_AddItem(O_SHOTGUN_ITEM);
        g_Lara.shotgun.ammo = start->shotgun_ammo;
        GlobalItemReplace(O_SHOTGUN_ITEM, O_SG_AMMO_ITEM);
    } else {
        int32_t ammo = start->shotgun_ammo / SHOTGUN_AMMO_QTY;
        for (int i = 0; i < ammo; i++) {
            Inv_AddItem(O_SG_AMMO_ITEM);
        }
        g_Lara.shotgun.ammo = 0;
    }

    for (int i = 0; i < start->num_scions; i++) {
        Inv_AddItem(O_SCION_ITEM);
    }

    for (int i = 0; i < start->num_medis; i++) {
        Inv_AddItem(O_MEDI_ITEM);
    }

    for (int i = 0; i < start->num_big_medis; i++) {
        Inv_AddItem(O_BIGMEDI_ITEM);
    }

    g_Lara.gun_status = start->gun_status;
    g_Lara.gun_type = start->gun_type;
    g_Lara.request_gun_type = start->gun_type;

    LaraInitialiseMeshes(level_num);
    InitialiseNewWeapon();
}

void LaraInitialiseMeshes(int32_t level_num)
{
    START_INFO *start = &g_GameInfo.start[level_num];

    if (start->flags.costume) {
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
    switch (start->gun_type) {
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

    if (start->flags.got_shotgun) {
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

void LaraCheatGetStuff()
{
    if (g_CurrentLevel == g_GameFlow.gym_level_num) {
        return;
    }

    // play pistols drawing sound
    Sound_Effect(SFX_LARA_DRAW, &g_LaraItem->pos, SPM_NORMAL);

    if (g_Objects[O_GUN_OPTION].loaded && !Inv_RequestItem(O_GUN_ITEM)) {
        Inv_AddItem(O_GUN_ITEM);
    }

    if (g_Objects[O_SHOTGUN_OPTION].loaded) {
        if (!Inv_RequestItem(O_SHOTGUN_ITEM)) {
            Inv_AddItem(O_SHOTGUN_ITEM);
        }
        g_Lara.shotgun.ammo = g_GameInfo.bonus_flag & GBF_NGPLUS ? 10001 : 300;
    }

    if (g_Objects[O_MAGNUM_OPTION].loaded) {
        if (!Inv_RequestItem(O_MAGNUM_ITEM)) {
            Inv_AddItem(O_MAGNUM_ITEM);
        }
        g_Lara.magnums.ammo = g_GameInfo.bonus_flag & GBF_NGPLUS ? 10001 : 1000;
    }

    if (g_Objects[O_UZI_OPTION].loaded) {
        if (!Inv_RequestItem(O_UZI_ITEM)) {
            Inv_AddItem(O_UZI_ITEM);
        }
        g_Lara.uzis.ammo = g_GameInfo.bonus_flag & GBF_NGPLUS ? 10001 : 2000;
    }

    for (int i = 0; i < 10; i++) {
        if (g_Objects[O_MEDI_OPTION].loaded
            && Inv_RequestItem(O_MEDI_ITEM) < 240) {
            Inv_AddItem(O_MEDI_ITEM);
        }
        if (g_Objects[O_BIGMEDI_OPTION].loaded
            && Inv_RequestItem(O_BIGMEDI_ITEM) < 240) {
            Inv_AddItem(O_BIGMEDI_ITEM);
        }
    }

    if (g_Objects[O_KEY_OPTION1].loaded && !Inv_RequestItem(O_KEY_ITEM1)) {
        Inv_AddItem(O_KEY_ITEM1);
    }
    if (g_Objects[O_KEY_OPTION2].loaded && !Inv_RequestItem(O_KEY_ITEM2)) {
        Inv_AddItem(O_KEY_ITEM2);
    }
    if (g_Objects[O_KEY_OPTION3].loaded && !Inv_RequestItem(O_KEY_ITEM3)) {
        Inv_AddItem(O_KEY_ITEM3);
    }
    if (g_Objects[O_KEY_OPTION4].loaded && !Inv_RequestItem(O_KEY_ITEM4)) {
        Inv_AddItem(O_KEY_ITEM4);
    }
    if (g_Objects[O_PUZZLE_OPTION1].loaded
        && !Inv_RequestItem(O_PUZZLE_ITEM1)) {
        Inv_AddItem(O_PUZZLE_ITEM1);
    }
    if (g_Objects[O_PUZZLE_OPTION2].loaded
        && !Inv_RequestItem(O_PUZZLE_ITEM2)) {
        Inv_AddItem(O_PUZZLE_ITEM2);
    }
    if (g_Objects[O_PUZZLE_OPTION3].loaded
        && !Inv_RequestItem(O_PUZZLE_ITEM3)) {
        Inv_AddItem(O_PUZZLE_ITEM3);
    }
    if (g_Objects[O_PUZZLE_OPTION4].loaded
        && !Inv_RequestItem(O_PUZZLE_ITEM4)) {
        Inv_AddItem(O_PUZZLE_ITEM4);
    }
    if (g_Objects[O_PICKUP_OPTION1].loaded
        && !Inv_RequestItem(O_PICKUP_ITEM1)) {
        Inv_AddItem(O_PICKUP_ITEM1);
    }
    if (g_Objects[O_PICKUP_OPTION2].loaded
        && !Inv_RequestItem(O_PICKUP_ITEM2)) {
        Inv_AddItem(O_PICKUP_ITEM2);
    }
}

void (*g_LaraControlRoutines[])(ITEM_INFO *item, COLL_INFO *coll) = {
    LaraAsWalk,       LaraAsRun,       LaraAsStop,      LaraAsForwardJump,
    LaraAsPose,       LaraAsFastBack,  LaraAsTurnR,     LaraAsTurnL,
    LaraAsDeath,      LaraAsFastFall,  LaraAsHang,      LaraAsReach,
    LaraAsSplat,      LaraAsTread,     LaraAsLand,      LaraAsCompress,
    LaraAsBack,       LaraAsSwim,      LaraAsGlide,     LaraAsNull,
    LaraAsFastTurn,   LaraAsStepRight, LaraAsStepLeft,  LaraAsRoll2,
    LaraAsSlide,      LaraAsBackJump,  LaraAsRightJump, LaraAsLeftJump,
    LaraAsUpJump,     LaraAsFallBack,  LaraAsHangLeft,  LaraAsHangRight,
    LaraAsSlideBack,  LaraAsSurfTread, LaraAsSurfSwim,  LaraAsDive,
    LaraAsPushBlock,  LaraAsPullBlock, LaraAsPPReady,   LaraAsPickup,
    LaraAsSwitchOn,   LaraAsSwitchOff, LaraAsUseKey,    LaraAsUsePuzzle,
    LaraAsUWDeath,    LaraAsRoll,      LaraAsSpecial,   LaraAsSurfBack,
    LaraAsSurfLeft,   LaraAsSurfRight, LaraAsUseMidas,  LaraAsDieMidas,
    LaraAsSwanDive,   LaraAsFastDive,  LaraAsGymnast,   LaraAsWaterOut,
    LaraAsControlled,
};

void (*g_LaraCollisionRoutines[])(ITEM_INFO *item, COLL_INFO *coll) = {
    LaraColWalk,       LaraColRun,       LaraColStop,      LaraColForwardJump,
    LaraColPose,       LaraColFastBack,  LaraColTurnR,     LaraColTurnL,
    LaraColDeath,      LaraColFastFall,  LaraColHang,      LaraColReach,
    LaraColSplat,      LaraColTread,     LaraColLand,      LaraColCompress,
    LaraColBack,       LaraColSwim,      LaraColGlide,     LaraColNull,
    LaraColFastTurn,   LaraColStepRight, LaraColStepLeft,  LaraColRoll2,
    LaraColSlide,      LaraColBackJump,  LaraColRightJump, LaraColLeftJump,
    LaraColUpJump,     LaraColFallBack,  LaraColHangLeft,  LaraColHangRight,
    LaraColSlideBack,  LaraColSurfTread, LaraColSurfSwim,  LaraColDive,
    LaraColPushBlock,  LaraColPullBlock, LaraColPPReady,   LaraColPickup,
    LaraColSwitchOn,   LaraColSwitchOff, LaraColUseKey,    LaraColUsePuzzle,
    LaraColUWDeath,    LaraColRoll,      LaraColSpecial,   LaraColSurfBack,
    LaraColSurfLeft,   LaraColSurfRight, LaraColUseMidas,  LaraColDieMidas,
    LaraColSwanDive,   LaraColFastDive,  LaraColGymnast,   LaraColWaterOut,
    LaraColControlled,
};
