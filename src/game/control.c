#include "game/control.h"

#include "3dsystem/phd_math.h"
#include "config.h"
#include "game/camera.h"
#include "game/demo.h"
#include "game/gameflow.h"
#include "game/hair.h"
#include "game/input.h"
#include "game/inv.h"
#include "game/items.h"
#include "game/lara.h"
#include "game/lot.h"
#include "game/music.h"
#include "game/objects/keyhole.h"
#include "game/objects/puzzle_hole.h"
#include "game/objects/switch.h"
#include "game/overlay.h"
#include "game/sound.h"
#include "game/traps/lava.h"
#include "game/traps/movable_block.h"
#include "global/vars.h"

#include <stddef.h>

static const int32_t m_AnimationRate = 0x8000;

static void Control_TriggerMusicTrack(
    int16_t track, int16_t flags, int16_t type);

static void Control_TriggerMusicTrack(
    int16_t track, int16_t flags, int16_t type)
{
    if (track <= 1 || track >= MAX_CD_TRACKS) {
        return;
    }

    // handle g_Lara gym routines
    switch (track) {
    case 28:
        if ((g_MusicTrackFlags[track] & IF_ONESHOT)
            && g_LaraItem->current_anim_state == AS_UPJUMP) {
            track = 29;
        }
        break;

    case 37:
        if (g_LaraItem->current_anim_state != AS_HANG) {
            return;
        }
        break;

    case 41:
        if (g_LaraItem->current_anim_state != AS_HANG) {
            return;
        }
        break;

    case 42:
        if ((g_MusicTrackFlags[track] & IF_ONESHOT)
            && g_LaraItem->current_anim_state == AS_HANG) {
            track = 43;
        }
        break;

    case 49:
        if (g_LaraItem->current_anim_state != AS_SURFTREAD) {
            return;
        }
        break;

    case 50:
        if (g_MusicTrackFlags[track] & IF_ONESHOT) {
            static int16_t gym_completion_counter = 0;
            gym_completion_counter++;
            if (gym_completion_counter == FRAMES_PER_SECOND * 4) {
                g_LevelComplete = true;
                gym_completion_counter = 0;
            }
        } else if (g_LaraItem->current_anim_state != AS_WATEROUT) {
            return;
        }
        break;
    }
    // end of g_Lara gym routines

    if (g_MusicTrackFlags[track] & IF_ONESHOT) {
        return;
    }

    if (type == TT_SWITCH) {
        g_MusicTrackFlags[track] ^= flags & IF_CODE_BITS;
    } else if (type == TT_ANTIPAD) {
        g_MusicTrackFlags[track] &= -1 - (flags & IF_CODE_BITS);
    } else if (flags & IF_CODE_BITS) {
        g_MusicTrackFlags[track] |= flags & IF_CODE_BITS;
    }

    if ((g_MusicTrackFlags[track] & IF_CODE_BITS) == IF_CODE_BITS) {
        if (flags & IF_ONESHOT) {
            g_MusicTrackFlags[track] |= IF_ONESHOT;
        }
        Music_Play(track);
    } else {
        Music_Stop();
    }
}

void CheckCheatMode()
{
    static int32_t cheat_mode = 0;
    static int16_t cheat_angle = 0;
    static int32_t cheat_turn = 0;

    if (g_CurrentLevel == g_GameFlow.gym_level_num) {
        return;
    }

    LARA_STATE as = g_LaraItem->current_anim_state;
    switch (cheat_mode) {
    case 0:
        if (as == AS_WALK) {
            cheat_mode = 1;
        }
        break;

    case 1:
        if (as != AS_WALK) {
            cheat_mode = as == AS_STOP ? 2 : 0;
        }
        break;

    case 2:
        if (as != AS_STOP) {
            cheat_mode = as == AS_BACK ? 3 : 0;
        }
        break;

    case 3:
        if (as != AS_BACK) {
            cheat_mode = as == AS_STOP ? 4 : 0;
        }
        break;

    case 4:
        if (as != AS_STOP) {
            cheat_angle = g_LaraItem->pos.y_rot;
        }
        cheat_turn = 0;
        if (as == AS_TURN_L) {
            cheat_mode = 5;
        } else if (as == AS_TURN_R) {
            cheat_mode = 6;
        } else {
            cheat_mode = 0;
        }
        break;

    case 5:
        if (as == AS_TURN_L || as == AS_FASTTURN) {
            cheat_turn += (int16_t)(g_LaraItem->pos.y_rot - cheat_angle);
            cheat_angle = g_LaraItem->pos.y_rot;
        } else {
            cheat_mode = cheat_turn < -94208 ? 7 : 0;
        }
        break;

    case 6:
        if (as == AS_TURN_R || as == AS_FASTTURN) {
            cheat_turn += (int16_t)(g_LaraItem->pos.y_rot - cheat_angle);
            cheat_angle = g_LaraItem->pos.y_rot;
        } else {
            cheat_mode = cheat_turn > 94208 ? 7 : 0;
        }
        break;

    case 7:
        if (as != AS_STOP) {
            cheat_mode = as == AS_COMPRESS ? 8 : 0;
        }
        break;

    case 8:
        if (g_LaraItem->fall_speed > 0) {
            if (as == AS_FORWARDJUMP) {
                g_LevelComplete = true;
            } else if (as == AS_BACKJUMP) {
                Inv_AddItem(O_SHOTGUN_ITEM);
                Inv_AddItem(O_MAGNUM_ITEM);
                Inv_AddItem(O_UZI_ITEM);
                g_Lara.shotgun.ammo = 500;
                g_Lara.magnums.ammo = 500;
                g_Lara.uzis.ammo = 5000;
                Sound_Effect(SFX_LARA_HOLSTER, NULL, SPM_ALWAYS);
            }
            cheat_mode = 0;
        }
        break;

    default:
        cheat_mode = 0;
        break;
    }
}

int32_t ControlPhase(int32_t nframes, int32_t demo_mode)
{
    static int32_t frame_count = 0;
    int32_t return_val = 0;
    if (nframes > MAX_FRAMES) {
        nframes = MAX_FRAMES;
    }

    frame_count += m_AnimationRate * nframes;
    while (frame_count >= 0) {
        CheckCheatMode();
        if (g_LevelComplete) {
            if (g_Config.disable_healing_between_levels) {
                g_LevelStartLaraHealth = g_StoredLaraHealth;
            }
            return GF_NOP_BREAK;
        }

        Input_Update();

        if (g_ResetFlag) {
            return GF_NOP_BREAK;
        }

        if (demo_mode) {
            if (g_Input.any) {
                return GF_EXIT_TO_TITLE;
            }
            if (!ProcessDemoInput()) {
                return GF_EXIT_TO_TITLE;
            }
        }

        if (g_Lara.death_count > DEATH_WAIT
            || (g_Lara.death_count > DEATH_WAIT_MIN && g_Input.any
                && !g_Input.fly_cheat)
            || g_OverlayFlag == 2) {
            if (demo_mode) {
                return GF_EXIT_TO_TITLE;
            }
            if (g_OverlayFlag == 2) {
                g_OverlayFlag = 1;
                return_val = Display_Inventory(INV_DEATH_MODE);
                if (return_val != GF_NOP) {
                    return return_val;
                }
            } else {
                g_OverlayFlag = 2;
            }
        }

        if ((g_Input.option || g_Input.save || g_Input.load
             || g_OverlayFlag <= 0)
            && !g_Lara.death_count) {
            if (g_OverlayFlag > 0) {
                if (g_Input.load) {
                    g_OverlayFlag = -1;
                } else if (g_Input.save) {
                    g_OverlayFlag = -2;
                } else {
                    g_OverlayFlag = 0;
                }
            } else {
                if (g_OverlayFlag == -1) {
                    return_val = Display_Inventory(INV_LOAD_MODE);
                } else if (g_OverlayFlag == -2) {
                    return_val = Display_Inventory(INV_SAVE_MODE);
                } else {
                    return_val = Display_Inventory(INV_GAME_MODE);
                }

                g_OverlayFlag = 1;
                if (return_val != GF_NOP) {
                    return return_val;
                }
            }
        }

        if (!g_Lara.death_count && g_InputDB.pause) {
            if (Control_Pause()) {
                return GF_EXIT_TO_TITLE;
            }
        }

        int16_t item_num = g_NextItemActive;
        while (item_num != NO_ITEM) {
            ITEM_INFO *item = &g_Items[item_num];
            OBJECT_INFO *obj = &g_Objects[item->object_number];
            if (obj->control) {
                obj->control(item_num);
            }
            item_num = item->next_active;
        }

        item_num = g_NextFxActive;
        while (item_num != NO_ITEM) {
            FX_INFO *fx = &g_Effects[item_num];
            OBJECT_INFO *obj = &g_Objects[fx->object_number];
            if (obj->control) {
                obj->control(item_num);
            }
            item_num = fx->next_active;
        }

        LaraControl(0);
        HairControl(0);

        CalculateCamera();
        Sound_UpdateEffects();
        g_SaveGame.timer++;
        Overlay_BarHealthTimerTick();

        if (g_Config.disable_healing_between_levels) {
            int8_t lara_found = 0;
            for (int i = 0; i < g_LevelItemCount; i++) {
                if (g_Items[i].object_number == O_LARA) {
                    lara_found = 1;
                }
            }
            if (lara_found) {
                g_StoredLaraHealth =
                    g_LaraItem ? g_LaraItem->hit_points : LARA_HITPOINTS;
            }
        }

        frame_count -= 0x10000;
    }

    return GF_NOP;
}

void AnimateItem(ITEM_INFO *item)
{
    item->touch_bits = 0;
    item->hit_status = 0;

    ANIM_STRUCT *anim = &g_Anims[item->anim_number];

    item->frame_number++;

    if (anim->number_changes > 0) {
        if (GetChange(item, anim)) {
            anim = &g_Anims[item->anim_number];
            item->current_anim_state = anim->current_anim_state;

            if (item->required_anim_state == item->current_anim_state) {
                item->required_anim_state = 0;
            }
        }
    }

    if (item->frame_number > anim->frame_end) {
        if (anim->number_commands > 0) {
            int16_t *command = &g_AnimCommands[anim->command_index];
            for (int i = 0; i < anim->number_commands; i++) {
                switch (*command++) {
                case AC_MOVE_ORIGIN:
                    TranslateItem(item, command[0], command[1], command[2]);
                    command += 3;
                    break;

                case AC_JUMP_VELOCITY:
                    item->fall_speed = command[0];
                    item->speed = command[1];
                    item->gravity_status = 1;
                    command += 2;
                    break;

                case AC_DEACTIVATE:
                    item->status = IS_DEACTIVATED;
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

        anim = &g_Anims[item->anim_number];
        item->current_anim_state = anim->current_anim_state;
        item->goal_anim_state = item->current_anim_state;
        if (item->required_anim_state == item->current_anim_state) {
            item->required_anim_state = 0;
        }
    }

    if (anim->number_commands > 0) {
        int16_t *command = &g_AnimCommands[anim->command_index];
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
                    Sound_Effect(
                        command[1], &item->pos,
                        g_RoomInfo[item->room_number].flags);
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

    if (!item->gravity_status) {
        int32_t speed = anim->velocity;
        if (anim->acceleration) {
            speed +=
                anim->acceleration * (item->frame_number - anim->frame_base);
        }
        item->speed = speed >> 16;
    } else {
        item->fall_speed += (item->fall_speed < FASTFALL_SPEED) ? GRAVITY : 1;
        item->pos.y += item->fall_speed;
    }

    item->pos.x += (phd_sin(item->pos.y_rot) * item->speed) >> W2V_SHIFT;
    item->pos.z += (phd_cos(item->pos.y_rot) * item->speed) >> W2V_SHIFT;
}

void TranslateItem(ITEM_INFO *item, int32_t x, int32_t y, int32_t z)
{
    int32_t c = phd_cos(item->pos.y_rot);
    int32_t s = phd_sin(item->pos.y_rot);

    item->pos.x += (c * x + s * z) >> W2V_SHIFT;
    item->pos.y += y;
    item->pos.z += (c * z - s * x) >> W2V_SHIFT;
}

void RefreshCamera(int16_t type, int16_t *data)
{
    int16_t trigger;
    int16_t target_ok = 2;
    do {
        trigger = *data++;
        int16_t value = trigger & VALUE_BITS;

        switch (TRIG_BITS(trigger)) {
        case TO_CAMERA:
            data++;

            if (value == g_Camera.last) {
                g_Camera.number = value;

                if (g_Camera.timer < 0 || g_Camera.type == CAM_LOOK
                    || g_Camera.type == CAM_COMBAT) {
                    g_Camera.timer = -1;
                    target_ok = 0;
                } else {
                    g_Camera.type = CAM_FIXED;
                    target_ok = 1;
                }
            } else {
                target_ok = 0;
            }
            break;

        case TO_TARGET:
            if (g_Camera.type != CAM_LOOK && g_Camera.type != CAM_COMBAT) {
                g_Camera.item = &g_Items[value];
            }
            break;
        }
    } while (!(trigger & END_BIT));

    if (g_Camera.item != NULL) {
        if (!target_ok
            || (target_ok == 2 && g_Camera.item->looked_at
                && g_Camera.item != g_Camera.last_item)) {
            g_Camera.item = NULL;
        }
    }
}

void TestTriggers(int16_t *data, int32_t heavy)
{
    if (!data) {
        return;
    }

    if ((*data & DATA_TYPE) == FT_LAVA) {
        if (!heavy && g_LaraItem->pos.y == g_LaraItem->floor) {
            LavaBurn(g_LaraItem);
        }

        if (*data & END_BIT) {
            return;
        }

        data++;
    }

    int16_t type = (*data++ >> 8) & 0x3F;
    int32_t switch_off = 0;
    int32_t flip = 0;
    int32_t new_effect = -1;
    int16_t flags = *data++;
    int16_t timer = flags & 0xFF;

    if (g_Camera.type != CAM_HEAVY) {
        RefreshCamera(type, data);
    }

    if (heavy) {
        if (type != TT_HEAVY) {
            return;
        }
    } else {
        switch (type) {
        case TT_SWITCH: {
            int16_t value = *data++ & VALUE_BITS;
            if (!SwitchTrigger(value, timer)) {
                return;
            }
            switch_off = g_Items[value].current_anim_state == AS_RUN;
            break;
        }

        case TT_PAD:
        case TT_ANTIPAD:
            if (g_LaraItem->pos.y != g_LaraItem->floor) {
                return;
            }
            break;

        case TT_KEY: {
            int16_t value = *data++ & VALUE_BITS;
            if (!KeyTrigger(value)) {
                return;
            }
            break;
        }

        case TT_PICKUP: {
            int16_t value = *data++ & VALUE_BITS;
            if (!PickupTrigger(value)) {
                return;
            }
            break;
        }

        case TT_HEAVY:
        case TT_DUMMY:
            return;

        case TT_COMBAT:
            if (g_Lara.gun_status != LGS_READY) {
                return;
            }
            break;
        }
    }

    ITEM_INFO *camera_item = NULL;
    int16_t trigger;
    do {
        trigger = *data++;
        int16_t value = trigger & VALUE_BITS;

        switch (TRIG_BITS(trigger)) {
        case TO_OBJECT: {
            ITEM_INFO *item = &g_Items[value];

            if (item->flags & IF_ONESHOT) {
                break;
            }

            item->timer = timer;
            if (timer != 1) {
                item->timer *= FRAMES_PER_SECOND;
            }

            if (type == TT_SWITCH) {
                item->flags ^= flags & IF_CODE_BITS;
            } else if (type == TT_ANTIPAD) {
                item->flags &= -1 - (flags & IF_CODE_BITS);
            } else if (flags & IF_CODE_BITS) {
                item->flags |= flags & IF_CODE_BITS;
            }

            if ((item->flags & IF_CODE_BITS) != IF_CODE_BITS) {
                break;
            }

            if (flags & IF_ONESHOT) {
                item->flags |= IF_ONESHOT;
            }

            if (!item->active) {
                if (g_Objects[item->object_number].intelligent) {
                    if (item->status == IS_NOT_ACTIVE) {
                        item->touch_bits = 0;
                        item->status = IS_ACTIVE;
                        AddActiveItem(value);
                        EnableBaddieAI(value, 1);
                    } else if (item->status == IS_INVISIBLE) {
                        item->touch_bits = 0;
                        if (EnableBaddieAI(value, 0)) {
                            item->status = IS_ACTIVE;
                        } else {
                            item->status = IS_INVISIBLE;
                        }
                        AddActiveItem(value);
                    }
                } else {
                    item->touch_bits = 0;
                    item->status = IS_ACTIVE;
                    AddActiveItem(value);
                }
            }
            break;
        }

        case TO_CAMERA: {
            trigger = *data++;
            int16_t camera_flags = trigger;
            int16_t camera_timer = trigger & 0xFF;

            if (g_Camera.fixed[value].flags & IF_ONESHOT) {
                break;
            }

            g_Camera.number = value;

            if (g_Camera.type == CAM_LOOK || g_Camera.type == CAM_COMBAT) {
                break;
            }

            if (type == TT_COMBAT) {
                break;
            }

            if (type == TT_SWITCH && timer && switch_off) {
                break;
            }

            if (g_Camera.number == g_Camera.last && type != TT_SWITCH) {
                break;
            }

            g_Camera.timer = camera_timer;
            if (g_Camera.timer != 1) {
                g_Camera.timer *= FRAMES_PER_SECOND;
            }

            if (camera_flags & IF_ONESHOT) {
                g_Camera.fixed[g_Camera.number].flags |= IF_ONESHOT;
            }

            g_Camera.speed = ((camera_flags & IF_CODE_BITS) >> 6) + 1;
            g_Camera.type = heavy ? CAM_HEAVY : CAM_FIXED;
            break;
        }

        case TO_TARGET:
            camera_item = &g_Items[value];
            break;

        case TO_SINK: {
            OBJECT_VECTOR *obvector = &g_Camera.fixed[value];

            if (g_Lara.LOT.required_box != obvector->flags) {
                g_Lara.LOT.target.x = obvector->x;
                g_Lara.LOT.target.y = obvector->y;
                g_Lara.LOT.target.z = obvector->z;
                g_Lara.LOT.required_box = obvector->flags;
            }

            g_Lara.current_active = obvector->data * 6;
            break;
        }

        case TO_FLIPMAP:
            if (g_FlipMapTable[value] & IF_ONESHOT) {
                break;
            }

            if (type == TT_SWITCH) {
                g_FlipMapTable[value] ^= flags & IF_CODE_BITS;
            } else if (flags & IF_CODE_BITS) {
                g_FlipMapTable[value] |= flags & IF_CODE_BITS;
            }

            if ((g_FlipMapTable[value] & IF_CODE_BITS) == IF_CODE_BITS) {
                if (flags & IF_ONESHOT) {
                    g_FlipMapTable[value] |= IF_ONESHOT;
                }

                if (!g_FlipStatus) {
                    flip = 1;
                }
            } else if (g_FlipStatus) {
                flip = 1;
            }
            break;

        case TO_FLIPON:
            if ((g_FlipMapTable[value] & IF_CODE_BITS) == IF_CODE_BITS
                && !g_FlipStatus) {
                flip = 1;
            }
            break;

        case TO_FLIPOFF:
            if ((g_FlipMapTable[value] & IF_CODE_BITS) == IF_CODE_BITS
                && g_FlipStatus) {
                flip = 1;
            }
            break;

        case TO_FLIPEFFECT:
            new_effect = value;
            break;

        case TO_FINISH:
            g_LevelComplete = true;
            break;

        case TO_CD:
            Control_TriggerMusicTrack(value, flags, type);
            break;

        case TO_SECRET:
            if ((g_SaveGame.secrets & (1 << value))) {
                break;
            }
            g_SaveGame.secrets |= 1 << value;
            Music_Play(13);
            break;
        }
    } while (!(trigger & END_BIT));

    if (camera_item
        && (g_Camera.type == CAM_FIXED || g_Camera.type == CAM_HEAVY)) {
        g_Camera.item = camera_item;
    }

    if (flip) {
        FlipMap();
        if (new_effect != -1) {
            g_FlipEffect = new_effect;
            g_FlipTimer = 0;
        }
    }
}

int32_t TriggerActive(ITEM_INFO *item)
{
    int32_t ok = (item->flags & IF_REVERSE) ? 0 : 1;

    if ((item->flags & IF_CODE_BITS) != IF_CODE_BITS) {
        return !ok;
    }

    if (!item->timer) {
        return ok;
    }

    if (item->timer == -1) {
        return !ok;
    }

    item->timer--;

    if (!item->timer) {
        item->timer = -1;
    }

    return ok;
}

void FlipMap()
{
    Sound_StopAmbientSounds();

    for (int i = 0; i < g_RoomCount; i++) {
        ROOM_INFO *r = &g_RoomInfo[i];
        if (r->flipped_room < 0) {
            continue;
        }

        RemoveRoomFlipItems(r);

        ROOM_INFO *flipped = &g_RoomInfo[r->flipped_room];
        ROOM_INFO temp = *r;
        *r = *flipped;
        *flipped = temp;

        r->flipped_room = flipped->flipped_room;
        flipped->flipped_room = -1;

        // XXX: is this really necessary given the assignments above?
        r->item_number = flipped->item_number;
        r->fx_number = flipped->fx_number;

        AddRoomFlipItems(r);
    }

    g_FlipStatus = !g_FlipStatus;
}

void RemoveRoomFlipItems(ROOM_INFO *r)
{
    for (int16_t item_num = r->item_number; item_num != NO_ITEM;
         item_num = g_Items[item_num].next_item) {
        ITEM_INFO *item = &g_Items[item_num];

        switch (item->object_number) {
        case O_MOVABLE_BLOCK:
        case O_MOVABLE_BLOCK2:
        case O_MOVABLE_BLOCK3:
        case O_MOVABLE_BLOCK4:
            AlterFloorHeight(item, WALL_L);
            break;

        case O_ROLLING_BLOCK:
            AlterFloorHeight(item, WALL_L * 2);
            break;
        }
    }
}

void AddRoomFlipItems(ROOM_INFO *r)
{
    for (int16_t item_num = r->item_number; item_num != NO_ITEM;
         item_num = g_Items[item_num].next_item) {
        ITEM_INFO *item = &g_Items[item_num];

        switch (item->object_number) {
        case O_MOVABLE_BLOCK:
        case O_MOVABLE_BLOCK2:
        case O_MOVABLE_BLOCK3:
        case O_MOVABLE_BLOCK4:
            AlterFloorHeight(item, -WALL_L);
            break;

        case O_ROLLING_BLOCK:
            AlterFloorHeight(item, -WALL_L * 2);
            break;
        }
    }
}
