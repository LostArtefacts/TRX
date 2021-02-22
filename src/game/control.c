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

void T1MInjectGameControl()
{
    INJECT(0x004133B0, ControlPhase);
}
