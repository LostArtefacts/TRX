#include "3dsystem/phd_math.h"
#include "game/camera.h"
#include "game/control.h"
#include "game/demo.h"
#include "game/effects.h"
#include "game/game.h"
#include "game/inv.h"
#include "game/lara.h"
#include "game/savegame.h"
#include "game/vars.h"
#include "specific/input.h"
#include "specific/shed.h"
#include "specific/sndpc.h"
#include "config.h"
#include "util.h"

int32_t ControlPhase(int32_t nframes, int demo_mode)
{
    int32_t return_val = 0;
    if (nframes > MAX_FRAMES) {
        nframes = MAX_FRAMES;
    }
    FrameCount += AnimationRate * nframes;
    if (FrameCount < 0) {
        return 0;
    }

    for (; FrameCount >= 0; FrameCount -= 0x10000) {
        if (CDTrack > 0)
            S_CDLoop();

        CheckCheatMode();
        if (LevelComplete) {
            return 1;
        }

        S_UpdateInput();

        if (ResetFlag) {
            return GF_EXIT_TO_TITLE;
        }

        if (demo_mode) {
            if (KeyData->keys_held) {
                return 1;
            }
            GetDemoInput();
            if (Input == -1) {
                return 1;
            }
        }

        if (Lara.death_count > DEATH_WAIT
            || (Lara.death_count > DEATH_WAIT_MIN && Input)
            || OverlayFlag == 2) {
            if (demo_mode) {
                return 1;
            }
            if (OverlayFlag == 2) {
                OverlayFlag = 1;
                return_val = Display_Inventory(INV_DEATH_MODE);
                if (return_val) {
                    return return_val;
                }
            } else {
                OverlayFlag = 2;
            }
        }

        if ((Input & (IN_OPTION | IN_SAVE | IN_LOAD) || OverlayFlag <= 0)
            && !Lara.death_count) {
            if (OverlayFlag > 0) {
                if (Input & IN_LOAD) {
                    OverlayFlag = -1;
                } else if (Input & IN_SAVE) {
                    OverlayFlag = -2;
                } else {
                    OverlayFlag = 0;
                }
            } else {
                if (OverlayFlag == -1) {
                    return_val = Display_Inventory(INV_LOAD_MODE);
                } else if (OverlayFlag == -2) {
                    return_val = Display_Inventory(INV_SAVE_MODE);
                } else {
                    return_val = Display_Inventory(INV_GAME_MODE);
                }

                OverlayFlag = 1;
                if (return_val) {
                    if (InventoryExtraData[0] != 1) {
                        return return_val;
                    }
                    if (CurrentLevel == LV_GYM) {
                        return GF_STARTGAME | LV_FIRSTLEVEL;
                    }

                    CreateSaveGameInfo();
                    S_SaveGame(
                        &SaveGame[0], sizeof(SAVEGAME_INFO),
                        InventoryExtraData[1]);
                    WriteTombAtiSettings();
                }
            }
        }

        int16_t item_num = NextItemActive;
        while (item_num != NO_ITEM) {
            int nex = Items[item_num].next_active;
            if (Objects[Items[item_num].object_number].control)
                (*Objects[Items[item_num].object_number].control)(item_num);
            item_num = nex;
        }

        item_num = NextFxActive;
        while (item_num != NO_ITEM) {
            int nex = Effects[item_num].next_active;
            if (Objects[Effects[item_num].object_number].control)
                (*Objects[Effects[item_num].object_number].control)(item_num);
            item_num = nex;
        }

        LaraControl(0);
        CalculateCamera();
        SoundEffects();
        ++SaveGame[0].timer;
        --HealthBarTimer;

#ifdef T1M_FEAT_GAMEPLAY
        if (T1MConfig.disable_healing_between_levels) {
            int lara_found = 0;
            for (int i = 0; i < LevelItemCount; i++) {
                if (Items[i].object_number == O_LARA) {
                    lara_found = 1;
                }
            }
            if (lara_found) {
                StoredLaraHealth =
                    LaraItem ? LaraItem->hit_points : LARA_HITPOINTS;
            }
        }

        if (T1MConfig.healthbar_showing_mode == T1M_BSM_ALWAYS
            || (T1MConfig.healthbar_showing_mode == T1M_BSM_FLASHING && LaraItem
                && LaraItem->hit_points < (LARA_HITPOINTS * 20) / 100)) {
            HealthBarTimer = 1;
        }
#endif
    }
    return 0;
}

void AnimateItem(ITEM_INFO* item)
{
    item->touch_bits = 0;
    item->hit_status = 0;

    ANIM_STRUCT* anim = &Anims[item->anim_number];

    item->frame_number++;

    if (anim->number_changes > 0) {
        if (GetChange(item, anim)) {
            anim = &Anims[item->anim_number];
            item->current_anim_state = anim->current_anim_state;

            if (item->required_anim_state == item->current_anim_state) {
                item->required_anim_state = 0;
            }
        }
    }

    if (item->frame_number > anim->frame_end) {
        if (anim->number_commands > 0) {
            int16_t* command = &AnimCommands[anim->command_index];
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

        anim = &Anims[item->anim_number];
        item->current_anim_state = anim->current_anim_state;
        item->goal_anim_state = item->current_anim_state;
        if (item->required_anim_state == item->current_anim_state) {
            item->required_anim_state = 0;
        }
    }

    if (anim->number_commands > 0) {
        int16_t* command = &AnimCommands[anim->command_index];
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
                    SoundEffect(
                        command[1], &item->pos,
                        RoomInfo[item->room_number].flags);
                }
                command += 2;
                break;

            case AC_EFFECT:
                if (item->frame_number == command[0]) {
                    EffectRoutines[command[1]](item);
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

int32_t GetChange(ITEM_INFO* item, ANIM_STRUCT* anim)
{
    if (item->current_anim_state == item->goal_anim_state) {
        return 0;
    }

    ANIM_CHANGE_STRUCT* change = &AnimChanges[anim->change_index];
    for (int i = 0; i < anim->number_changes; i++, change++) {
        if (change->goal_anim_state == item->goal_anim_state) {
            ANIM_RANGE_STRUCT* range = &AnimRanges[change->range_index];
            for (int j = 0; j < change->number_ranges; j++, range++) {
                if (item->frame_number >= range->start_frame
                    && item->frame_number <= range->end_frame) {
                    item->anim_number = range->link_anim_num;
                    item->frame_number = range->link_frame_num;
                    return 1;
                }
            }
        }
    }

    return 0;
}

void TranslateItem(ITEM_INFO* item, int32_t x, int32_t y, int32_t z)
{
    int32_t c = phd_cos(item->pos.y_rot);
    int32_t s = phd_sin(item->pos.y_rot);

    item->pos.x += (c * x + s * z) >> W2V_SHIFT;
    item->pos.y += y;
    item->pos.z += (c * z - s * x) >> W2V_SHIFT;
}

void T1MInjectGameControl()
{
    INJECT(0x004133B0, ControlPhase);
    INJECT(0x00413660, AnimateItem);
    INJECT(0x00413960, GetChange);
    INJECT(0x00413A10, TranslateItem);
}
