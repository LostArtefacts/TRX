#include "game/inv.h"

#include "3dsystem/3d_gen.h"
#include "3dsystem/scalespr.h"
#include "config.h"
#include "game/game.h"
#include "game/health.h"
#include "game/lara.h"
#include "game/mnsound.h"
#include "game/option.h"
#include "game/savegame.h"
#include "game/settings.h"
#include "game/sound.h"
#include "game/text.h"
#include "global/const.h"
#include "global/types.h"
#include "global/vars.h"
#include "specific/display.h"
#include "specific/frontend.h"
#include "specific/input.h"
#include "specific/output.h"
#include "specific/sndpc.h"
#include "util.h"

#include <stdint.h>
#include <string.h>

typedef enum {
    PSPINE = 1,
    PFRONT = 2,
    PINFRONT = 4,
    PPAGE2 = 8,
    PBACK = 16,
    PINBACK = 32,
    PPAGE1 = 64
} PASS_PAGE;

TEXTSTRING* BETA_TEXT = NULL;

int32_t Display_Inventory(int inv_mode)
{
    RING_INFO ring;
    IMOTION_INFO imo;

    memset(&imo, 0, sizeof(IMOTION_INFO));
    memset(&ring, 0, sizeof(RING_INFO));

    if (inv_mode == RT_KEYS && !InvKeysObjects) {
        InvChosen = -1;
        return GF_NOP;
    }

    int32_t pass_mode_open = 0;
    if (AmmoText) {
        T_RemovePrint(AmmoText);
        AmmoText = 0;
    }

    AlterFOV(T1MConfig.fov_value * PHD_DEGREE);
    InvMode = inv_mode;

    InvNFrames = 2;
    Construct_Inventory();

    if (InvMode != INV_TITLE_MODE) {
        S_FadeInInventory(1);
    } else {
        S_FadeInInventory(0);
    }

    mn_stop_ambient_samples();
    S_SoundStopAllSamples();

    switch (InvMode) {
    case INV_DEATH_MODE:
    case INV_SAVE_MODE:
    case INV_SAVE_CRYSTAL_MODE:
    case INV_LOAD_MODE:
    case INV_TITLE_MODE:
        Inv_RingInit(
            &ring, RT_OPTION, InvOptionList, InvOptionObjects, InvOptionCurrent,
            &imo);
        break;

    case INV_KEYS_MODE:
        Inv_RingInit(
            &ring, RT_KEYS, InvKeysList, InvKeysObjects, InvMainCurrent, &imo);
        break;

    default:
        if (InvMainObjects) {
            Inv_RingInit(
                &ring, RT_MAIN, InvMainList, InvMainObjects, InvMainCurrent,
                &imo);
        } else {
            Inv_RingInit(
                &ring, RT_OPTION, InvOptionList, InvOptionObjects,
                InvOptionCurrent, &imo);
        }
        break;
    }

    SoundEffect(SFX_MENU_SPININ, NULL, SPM_ALWAYS);

    InvNFrames = 2;

    do {
        Inv_RingCalcAdders(&ring, ROTATE_DURATION);
        S_UpdateInput();

        InputDB = GetDebouncedInput(Input);

        if (InvMode != INV_TITLE_MODE || Input || InputDB) {
            NoInputCount = 0;
            ResetFlag = 0;
        } else {
            if (!T1MConfig.disable_demo) {
                NoInputCount++;
                if (GF.has_demo && NoInputCount > GF.demo_delay) {
                    ResetFlag = 1;
                }
            }
        }

        for (int i = 0; i < InvNFrames; i++) {
            if (IDelay) {
                if (IDCount) {
                    IDCount--;
                } else {
                    IDelay = 0;
                }
            }
            Inv_RingDoMotions(&ring);
        }

        ring.camera.z = ring.radius + CAMERA_2_RING;

        PHD_3DPOS viewer;
        Inv_RingGetView(&ring, &viewer);
        phd_GenerateW2V(&viewer);

        Inv_RingLight(&ring);

        S_InitialisePolyList();
        S_CopyBufferToScreen();

        phd_PushMatrix();
        phd_TranslateAbs(ring.ringpos.x, ring.ringpos.y, ring.ringpos.z);
        phd_RotYXZ(ring.ringpos.y_rot, ring.ringpos.x_rot, ring.ringpos.z_rot);

        PHD_ANGLE angle = 0;
        for (int i = 0; i < ring.number_of_objects; i++) {
            INVENTORY_ITEM *inv_item = ring.list[i];

            if (i == ring.current_object) {
                for (int j = 0; j < (InvNFrames/ANIM_SCALE); j++) {
                    if (ring.rotating) {
                        LsAdder = LOW_LIGHT;
                        if (inv_item->y_rot) {
                            if (inv_item->y_rot < 0) {
                                inv_item->y_rot += 512;
                            } else {
                                inv_item->y_rot -= 512;
                            }
                        }
                    } else if (
                        imo.status == RNG_SELECTED
                        || imo.status == RNG_DESELECTING
                        || imo.status == RNG_SELECTING
                        || imo.status == RNG_DESELECT
                        || imo.status == RNG_CLOSING_ITEM) {
                        LsAdder = HIGH_LIGHT;
                        if (inv_item->y_rot != inv_item->y_rot_sel) {
                            if (inv_item->y_rot_sel - inv_item->y_rot > 0
                                && inv_item->y_rot_sel - inv_item->y_rot
                                    < 0x8000) {
                                inv_item->y_rot += 1024;
                            } else {
                                inv_item->y_rot -= 1024;
                            }
                            inv_item->y_rot &= 0xFC00u;
                        }
                    } else if (
                        ring.number_of_objects == 1
                        || !CHK_ANY(Input, IN_RIGHT | IN_LEFT)
                        || !CHK_ANY(Input, IN_LEFT)) {
                        LsAdder = HIGH_LIGHT;
                        inv_item->y_rot += 256;
                    }
                }

                if ((imo.status == RNG_OPEN || imo.status == RNG_SELECTING
                     || imo.status == RNG_SELECTED
                     || imo.status == RNG_DESELECTING
                     || imo.status == RNG_DESELECT
                     || imo.status == RNG_CLOSING_ITEM)
                    && !ring.rotating && !CHK_ANY(Input, IN_RIGHT | IN_LEFT)) {
                    RingNotActive(inv_item);
                }
            } else {
                LsAdder = LOW_LIGHT;
                for (int j = 0; j < InvNFrames; j++) {
                    if (inv_item->y_rot) {
                        if (inv_item->y_rot < 0) {
                            inv_item->y_rot += 256;
                        } else {
                            inv_item->y_rot -= 256;
                        }
                    }
                }
            }

            if (imo.status == RNG_OPEN || imo.status == RNG_SELECTING
                || imo.status == RNG_SELECTED || imo.status == RNG_DESELECTING
                || imo.status == RNG_DESELECT
                || imo.status == RNG_CLOSING_ITEM) {
                RingIsOpen(&ring);
            } else {
                RingIsNotOpen(&ring);
            }

            if (!imo.status || imo.status == RNG_CLOSING
                || imo.status == RNG_MAIN2OPTION
                || imo.status == RNG_OPTION2MAIN
                || imo.status == RNG_EXITING_INVENTORY || imo.status == RNG_DONE
                || ring.rotating) {
                RingActive();
            }

            phd_PushMatrix();
            phd_RotYXZ(angle, 0, 0);
            phd_TranslateRel(ring.radius, 0, 0);
            phd_RotYXZ(PHD_90, inv_item->pt_xrot, 0);
            DrawInventoryItem(inv_item);
            angle += ring.angle_adder;
            phd_PopMatrix();
        }

        phd_PopMatrix();

        mn_update_sound_effects();
        DrawFPSInfo();
        T_DrawText();
        S_OutputPolyList();

        InvNFrames = S_DumpScreen();
        Camera.number_frames = InvNFrames;

        if (T1MConfig.enable_timer_in_inventory) {
            SaveGame.timer += InvNFrames / 2;
        }

        if (ring.rotating) {
            continue;
        }

        if ((InvMode == INV_SAVE_MODE || InvMode == INV_SAVE_CRYSTAL_MODE
             || InvMode == INV_LOAD_MODE || InvMode == INV_DEATH_MODE)
            && !pass_mode_open) {
            InputDB = IN_SELECT;
        }

        switch (imo.status) {
        case RNG_OPEN:
            if (CHK_ANY(Input, IN_RIGHT) && ring.number_of_objects > 1) {
                Inv_RingRotateLeft(&ring);
                SoundEffect(SFX_MENU_ROTATE, NULL, SPM_ALWAYS);
                break;
            }

            if (CHK_ANY(Input, IN_LEFT) && ring.number_of_objects > 1) {
                Inv_RingRotateRight(&ring);
                SoundEffect(SFX_MENU_ROTATE, NULL, SPM_ALWAYS);
                break;
            }

            if ((ResetFlag || CHK_ANY(InputDB, IN_OPTION))
                && (ResetFlag || InvMode != INV_TITLE_MODE)) {
                SoundEffect(SFX_MENU_SPINOUT, NULL, SPM_ALWAYS);
                InvChosen = -1;

                if (ring.type == RT_MAIN) {
                    InvMainCurrent = ring.current_object;
                } else {
                    InvOptionCurrent = ring.current_object;
                }

                if (InvMode == INV_TITLE_MODE) {
                    S_FadeOutInventory(0);
                } else {
                    S_FadeOutInventory(1);
                }

                Inv_RingMotionSetup(&ring, RNG_CLOSING, RNG_DONE, CLOSE_FRAMES);
                Inv_RingMotionRadius(&ring, 0);
                Inv_RingMotionCameraPos(&ring, CAMERA_STARTHEIGHT);
                Inv_RingMotionRotation(
                    &ring, CLOSE_ROTATION, ring.ringpos.y_rot - CLOSE_ROTATION);
                Input = 0;
                InputDB = 0;
            }

            if (CHK_ANY(InputDB, IN_SELECT)) {
                if ((InvMode == INV_SAVE_MODE
                     || InvMode == INV_SAVE_CRYSTAL_MODE
                     || InvMode == INV_LOAD_MODE || InvMode == INV_DEATH_MODE)
                    && !pass_mode_open) {
                    pass_mode_open = 1;
                }

                Item_Data = 0;

                INVENTORY_ITEM *inv_item;
                if (ring.type == RT_MAIN) {
                    InvMainCurrent = ring.current_object;
                    inv_item = InvMainList[ring.current_object];
                } else if (ring.type == RT_OPTION) {
                    InvOptionCurrent = ring.current_object;
                    inv_item = InvOptionList[ring.current_object];
                } else {
                    InvKeysCurrent = ring.current_object;
                    inv_item = InvKeysList[ring.current_object];
                }

                inv_item->goal_frame = inv_item->open_frame;
                inv_item->anim_direction = 1;

                Inv_RingMotionSetup(
                    &ring, RNG_SELECTING, RNG_SELECTED, SELECTING_FRAMES);
                Inv_RingMotionRotation(
                    &ring, 0, -PHD_90 - ring.angle_adder * ring.current_object);
                Inv_RingMotionItemSelect(&ring, inv_item);
                Input = 0;
                InputDB = 0;

                switch (inv_item->object_number) {
                case O_MAP_OPTION:
                    SoundEffect(SFX_MENU_COMPASS, NULL, SPM_ALWAYS);
                    break;

                case O_PHOTO_OPTION:
                    SoundEffect(SFX_MENU_CHOOSE, NULL, SPM_ALWAYS);
                    break;

                case O_CONTROL_OPTION:
                    SoundEffect(SFX_MENU_GAMEBOY, NULL, SPM_ALWAYS);
                    break;

                case O_GUN_OPTION:
                case O_SHOTGUN_OPTION:
                case O_MAGNUM_OPTION:
                case O_UZI_OPTION:
                    SoundEffect(SFX_MENU_GUNS, NULL, SPM_ALWAYS);
                    break;

                default:
                    SoundEffect(SFX_MENU_SPININ, NULL, SPM_ALWAYS);
                    break;
                }
            }

            if (CHK_ANY(InputDB, IN_FORWARD) && InvMode != INV_TITLE_MODE
                && InvMode != INV_KEYS_MODE) {
                if (ring.type == RT_MAIN) {
                    if (InvKeysObjects) {
                        Inv_RingMotionSetup(
                            &ring, RNG_CLOSING, RNG_MAIN2KEYS,
                            RINGSWITCH_FRAMES / 2);
                        Inv_RingMotionRadius(&ring, 0);
                        Inv_RingMotionRotation(
                            &ring, CLOSE_ROTATION,
                            ring.ringpos.y_rot - CLOSE_ROTATION);
                        Inv_RingMotionCameraPitch(&ring, 0x2000);
                        imo.misc = 0x2000;
                    }
                    Input = 0;
                    InputDB = 0;
                } else if (ring.type == RT_OPTION) {
                    if (InvMainObjects) {
                        Inv_RingMotionSetup(
                            &ring, RNG_CLOSING, RNG_OPTION2MAIN,
                            RINGSWITCH_FRAMES / 2);
                        Inv_RingMotionRadius(&ring, 0);
                        Inv_RingMotionRotation(
                            &ring, CLOSE_ROTATION,
                            ring.ringpos.y_rot - CLOSE_ROTATION);
                        Inv_RingMotionCameraPitch(&ring, 0x2000);
                        imo.misc = 0x2000;
                    }
                    InputDB = 0;
                }
            } else if (
                CHK_ANY(InputDB, IN_BACK) && InvMode != INV_TITLE_MODE
                && InvMode != INV_KEYS_MODE) {
                if (ring.type == RT_KEYS) {
                    if (InvMainObjects) {
                        Inv_RingMotionSetup(
                            &ring, RNG_CLOSING, RNG_KEYS2MAIN,
                            RINGSWITCH_FRAMES / 2);
                        Inv_RingMotionRadius(&ring, 0);
                        Inv_RingMotionRotation(
                            &ring, CLOSE_ROTATION,
                            ring.ringpos.y_rot - CLOSE_ROTATION);
                        Inv_RingMotionCameraPitch(&ring, -0x2000);
                        imo.misc = -0x2000;
                    }
                    Input = 0;
                    InputDB = 0;
                } else if (ring.type == RT_MAIN) {
                    if (InvOptionObjects) {
                        Inv_RingMotionSetup(
                            &ring, RNG_CLOSING, RNG_MAIN2OPTION,
                            RINGSWITCH_FRAMES / 2);
                        Inv_RingMotionRadius(&ring, 0);
                        Inv_RingMotionRotation(
                            &ring, CLOSE_ROTATION,
                            ring.ringpos.y_rot - CLOSE_ROTATION);
                        Inv_RingMotionCameraPitch(&ring, -0x2000);
                        imo.misc = -0x2000;
                    }
                    InputDB = 0;
                }
            }
            break;

        case RNG_MAIN2OPTION:
            Inv_RingMotionSetup(
                &ring, RNG_OPENING, RNG_OPEN, RINGSWITCH_FRAMES / 2);
            Inv_RingMotionRadius(&ring, RING_RADIUS);
            ring.camera_pitch = -imo.misc;
            imo.camera_pitch_rate = imo.misc / (RINGSWITCH_FRAMES / 2);
            imo.camera_pitch_target = 0;
            InvMainCurrent = ring.current_object;
            ring.list = InvOptionList;
            ring.type = RT_OPTION;
            ring.number_of_objects = InvOptionObjects;
            ring.current_object = InvOptionCurrent;
            Inv_RingCalcAdders(&ring, ROTATE_DURATION);
            Inv_RingMotionRotation(
                &ring, OPEN_ROTATION,
                -PHD_90 - ring.angle_adder * ring.current_object);
            ring.ringpos.y_rot = imo.rotate_target + OPEN_ROTATION;
            break;

        case RNG_MAIN2KEYS:
            Inv_RingMotionSetup(
                &ring, RNG_OPENING, RNG_OPEN, RINGSWITCH_FRAMES / 2);
            Inv_RingMotionRadius(&ring, RING_RADIUS);
            ring.camera_pitch = -imo.misc;
            imo.camera_pitch_rate = imo.misc / (RINGSWITCH_FRAMES / 2);
            imo.camera_pitch_target = 0;
            InvMainCurrent = ring.current_object;
            InvMainObjects = ring.number_of_objects;
            ring.list = InvKeysList;
            ring.type = RT_KEYS;
            ring.number_of_objects = InvKeysObjects;
            ring.current_object = InvKeysCurrent;
            Inv_RingCalcAdders(&ring, ROTATE_DURATION);
            Inv_RingMotionRotation(
                &ring, OPEN_ROTATION,
                -PHD_90 - ring.angle_adder * ring.current_object);
            ring.ringpos.y_rot = imo.rotate_target + OPEN_ROTATION;
            break;

        case RNG_KEYS2MAIN:
            Inv_RingMotionSetup(
                &ring, RNG_OPENING, RNG_OPEN, RINGSWITCH_FRAMES / 2);
            Inv_RingMotionRadius(&ring, RING_RADIUS);
            ring.camera_pitch = -imo.misc;
            imo.camera_pitch_rate = imo.misc / (RINGSWITCH_FRAMES / 2);
            imo.camera_pitch_target = 0;
            InvKeysCurrent = ring.current_object;
            ring.list = InvMainList;
            ring.type = RT_MAIN;
            ring.number_of_objects = InvMainObjects;
            ring.current_object = InvMainCurrent;
            Inv_RingCalcAdders(&ring, ROTATE_DURATION);
            Inv_RingMotionRotation(
                &ring, OPEN_ROTATION,
                -PHD_90 - ring.angle_adder * ring.current_object);
            ring.ringpos.y_rot = imo.rotate_target + OPEN_ROTATION;
            break;

        case RNG_OPTION2MAIN:
            Inv_RingMotionSetup(
                &ring, RNG_OPENING, RNG_OPEN, RINGSWITCH_FRAMES / 2);
            Inv_RingMotionRadius(&ring, RING_RADIUS);
            ring.camera_pitch = -imo.misc;
            imo.camera_pitch_rate = imo.misc / (RINGSWITCH_FRAMES / 2);
            imo.camera_pitch_target = 0;
            InvOptionObjects = ring.number_of_objects;
            InvOptionCurrent = ring.current_object;
            ring.list = InvMainList;
            ring.type = RT_MAIN;
            ring.number_of_objects = InvMainObjects;
            ring.current_object = InvMainCurrent;
            Inv_RingCalcAdders(&ring, ROTATE_DURATION);
            Inv_RingMotionRotation(
                &ring, OPEN_ROTATION,
                -PHD_90 - ring.angle_adder * ring.current_object);
            ring.ringpos.y_rot = imo.rotate_target + OPEN_ROTATION;
            break;

        case RNG_SELECTED: {
            INVENTORY_ITEM *inv_item = ring.list[ring.current_object];
            if (inv_item->object_number == O_PASSPORT_CLOSED) {
                inv_item->object_number = O_PASSPORT_OPTION;
            }

            int32_t busy = 0;
            for (int j = 0; j < InvNFrames; j++) {
                busy = 0;
                if (inv_item->y_rot == inv_item->y_rot_sel) {
                    busy = AnimateInventoryItem(inv_item);
                }
            }

            if (!busy && !IDelay) {
                DoInventoryOptions(inv_item);

                if (CHK_ANY(InputDB, IN_DESELECT)) {
                    inv_item->sprlist = NULL;
                    Inv_RingMotionSetup(
                        &ring, RNG_CLOSING_ITEM, RNG_DESELECT, 0);
                    Input = 0;
                    InputDB = 0;

                    if (InvMode == INV_LOAD_MODE || InvMode == INV_SAVE_MODE
                        || InvMode == INV_SAVE_CRYSTAL_MODE) {
                        Inv_RingMotionSetup(
                            &ring, RNG_CLOSING_ITEM, RNG_EXITING_INVENTORY, 0);
                        Input = 0;
                        InputDB = 0;
                    }
                }

                if (CHK_ANY(InputDB, IN_SELECT)) {
                    inv_item->sprlist = NULL;
                    InvChosen = inv_item->object_number;
                    if (ring.type == RT_MAIN) {
                        InvMainCurrent = ring.current_object;
                    } else {
                        InvOptionCurrent = ring.current_object;
                    }

                    if (InvMode == INV_TITLE_MODE
                        && ((inv_item->object_number == O_DETAIL_OPTION)
                            || inv_item->object_number == O_SOUND_OPTION
                            || inv_item->object_number == O_CONTROL_OPTION
                            || inv_item->object_number == O_GAMMA_OPTION)) {
                        Inv_RingMotionSetup(
                            &ring, RNG_CLOSING_ITEM, RNG_DESELECT, 0);
                    } else {
                        Inv_RingMotionSetup(
                            &ring, RNG_CLOSING_ITEM, RNG_EXITING_INVENTORY, 0);
                    }
                    Input = 0;
                    InputDB = 0;
                }
            }
            break;
        }

        case RNG_DESELECT:
            SoundEffect(SFX_MENU_SPINOUT, NULL, SPM_ALWAYS);
            Inv_RingMotionSetup(
                &ring, RNG_DESELECTING, RNG_OPEN, SELECTING_FRAMES);
            Inv_RingMotionRotation(
                &ring, 0, -PHD_90 - ring.angle_adder * ring.current_object);
            Input = 0;
            InputDB = 0;
            break;

        case RNG_CLOSING_ITEM: {
            INVENTORY_ITEM *inv_item = ring.list[ring.current_object];
            for (int j = 0; j < InvNFrames; j++) {
                if (!AnimateInventoryItem(inv_item)) {
                    if (inv_item->object_number == O_PASSPORT_OPTION) {
                        inv_item->object_number = O_PASSPORT_CLOSED;
                        inv_item->current_frame = 0;
                    }
                    imo.count = SELECTING_FRAMES;
                    imo.status = imo.status_target;
                    Inv_RingMotionItemDeselect(&ring, inv_item);
                    break;
                }
            }
            break;
        }

        case RNG_EXITING_INVENTORY:
            if (!imo.count) {
                if (InvMode != INV_TITLE_MODE) {
                    S_FadeOutInventory(1);
                } else {
                    S_FadeOutInventory(0);
                }
                Inv_RingMotionSetup(&ring, RNG_CLOSING, RNG_DONE, CLOSE_FRAMES);
                Inv_RingMotionRadius(&ring, 0);
                Inv_RingMotionCameraPos(&ring, CAMERA_STARTHEIGHT);
                Inv_RingMotionRotation(
                    &ring, CLOSE_ROTATION, ring.ringpos.y_rot - CLOSE_ROTATION);
            }
            break;
        }
    } while (imo.status != RNG_DONE);

    RemoveInventoryText();
    S_FinishInventory();
    if( BETA_TEXT != NULL ) {
		T_RemovePrint(BETA_TEXT);
		BETA_TEXT = NULL;
	}

    if (ResetFlag) {
        return GF_START_DEMO;
    }

    switch (InvChosen) {
    case O_PASSPORT_OPTION:
        if (InvMode == INV_TITLE_MODE) {
            if (InvExtraData[0] == 0) {
                // page 1: load game
                return GF_START_SAVED_GAME | InvExtraData[1];
            } else if (InvExtraData[0] == 1) {
                // page 2: new game
                switch (InvExtraData[1]) {
                case 0:
                    SaveGame.bonus_flag = 0;
                    break;
                case 1:
                    SaveGame.bonus_flag = GBF_NGPLUS;
                    break;
                case 2:
                    SaveGame.bonus_flag = GBF_JAPANESE;
                    break;
                case 3:
                    SaveGame.bonus_flag = GBF_JAPANESE | GBF_NGPLUS;
                    break;
                }
                InitialiseStartInfo();
                return GF_START_GAME | GF.first_level_num;
            } else {
                // page 3: exit game
                return GF_EXIT_GAME;
            }
        } else {
            if (InvExtraData[0] == 0) {
                // page 1: load game
                return GF_START_SAVED_GAME | InvExtraData[1];
            } else if (InvExtraData[0] == 1) {
                // page 1: save game, or new game in gym
                if (CurrentLevel == GF.gym_level_num) {
                    switch (InvExtraData[1]) {
                    case 0:
                        SaveGame.bonus_flag = 0;
                        break;
                    case 1:
                        SaveGame.bonus_flag = GBF_NGPLUS;
                        break;
                    case 2:
                        SaveGame.bonus_flag = GBF_JAPANESE;
                        break;
                    case 3:
                        SaveGame.bonus_flag = GBF_JAPANESE | GBF_NGPLUS;
                        break;
                    }
                    InitialiseStartInfo();
                    return GF_START_GAME | GF.first_level_num;
                } else {
                    CreateSaveGameInfo();
                    S_SaveGame(&SaveGame, InvExtraData[1]);
                    S_WriteUserSettings();
                    return GF_NOP;
                }
            } else {
                // page 3: exit to title
                return GF_EXIT_TO_TITLE;
            }
        }

    case O_PHOTO_OPTION:
        InvExtraData[1] = 0;
        return GF_START_GAME | GF.gym_level_num;

    case O_GUN_OPTION:
        UseItem(O_GUN_OPTION);
        break;

    case O_SHOTGUN_OPTION:
        UseItem(O_SHOTGUN_OPTION);
        break;

    case O_MAGNUM_OPTION:
        UseItem(O_MAGNUM_OPTION);
        break;

    case O_UZI_OPTION:
        UseItem(O_UZI_OPTION);
        break;

    case O_MEDI_OPTION:
        UseItem(O_MEDI_OPTION);
        break;

    case O_BIGMEDI_OPTION:
        UseItem(O_BIGMEDI_OPTION);
        break;
    }

    return GF_NOP;
}


void Construct_Inventory()
{
    S_SetupAboveWater(0);
    if (InvMode != INV_TITLE_MODE) {
        TempVideoAdjust(HiRes, 1.0);
    }

    PhdLeft = 0;
    PhdTop = 0;
    PhdBottom = PhdWinMaxY;
    PhdRight = PhdWinMaxX;

    for (int i = 0; i < 8; i++) {
        InvExtraData[i] = 0;
    }

    InvChosen = 0;
    if (InvMode == INV_TITLE_MODE) {
        InvOptionObjects = TITLE_RING_OBJECTS;
        BETA_TEXT = T_Print(-32, 50,"RC 1");
        T_RightAlign(BETA_TEXT, 1);
    } else {
        InvOptionObjects = OPTION_RING_OBJECTS;
        BETA_TEXT = NULL;
    }

    for (int i = 0; i < InvMainObjects; i++) {
        INVENTORY_ITEM *inv_item = InvMainList[i];
        inv_item->drawn_meshes = inv_item->which_meshes;
        if ((inv_item->object_number == O_MAP_OPTION) && CompassStatus) {
            inv_item->current_frame = inv_item->open_frame;
            inv_item->drawn_meshes = -1;
        } else {
            inv_item->current_frame = 0;
        }
        inv_item->goal_frame = inv_item->current_frame;
        inv_item->anim_count = 0;
        inv_item->y_rot = 0;
    }

    for (int i = 0; i < InvOptionObjects; i++) {
        INVENTORY_ITEM *inv_item = InvOptionList[i];
        inv_item->current_frame = 0;
        inv_item->goal_frame = 0;
        inv_item->anim_count = 0;
        inv_item->y_rot = 0;
    }

    InvMainCurrent = 0;
    InvOptionCurrent = 0;
    Item_Data = 0;

    if (GF.gym_level_num == -1) {
        Inv_RemoveItem(O_PHOTO_OPTION);
    }
}

int32_t AnimateInventoryItem(INVENTORY_ITEM *inv_item)
{
    if (inv_item->current_frame == inv_item->goal_frame) {
        SelectMeshes(inv_item);
        return 0;
    }
    if (inv_item->anim_count) {
        inv_item->anim_count--;
    } else {
        inv_item->anim_count = inv_item->anim_speed * ANIM_SCALE;
        inv_item->current_frame += inv_item->anim_direction;
        if (inv_item->current_frame >= inv_item->frames_total) {
            inv_item->current_frame = 0;
        } else if (inv_item->current_frame < 0) {
            inv_item->current_frame = inv_item->frames_total - 1;
        }
    }
    SelectMeshes(inv_item);
    return 1;
}

void SelectMeshes(INVENTORY_ITEM *inv_item)
{
    if (inv_item->object_number == O_PASSPORT_OPTION) {
        if (inv_item->current_frame <= 14) {
            inv_item->drawn_meshes = PASS_MESH | PINFRONT | PPAGE1;
        } else if (inv_item->current_frame < 19) {
            inv_item->drawn_meshes = PASS_MESH | PINFRONT | PPAGE1 | PPAGE2;
        } else if (inv_item->current_frame == 19) {
            inv_item->drawn_meshes = PASS_MESH | PPAGE1 | PPAGE2;
        } else if (inv_item->current_frame < 24) {
            inv_item->drawn_meshes = PASS_MESH | PPAGE1 | PPAGE2 | PINBACK;
        } else if (inv_item->current_frame < 29) {
            inv_item->drawn_meshes = PASS_MESH | PPAGE2 | PINBACK;
        } else if (inv_item->current_frame == 29) {
            inv_item->drawn_meshes = PASS_MESH;
        }
    } else if (inv_item->object_number == O_MAP_OPTION) {
        if (inv_item->current_frame && inv_item->current_frame < 18) {
            inv_item->drawn_meshes = -1;
        } else {
            inv_item->drawn_meshes = inv_item->which_meshes;
        }
    } else {
        inv_item->drawn_meshes = -1;
    }
}

void DrawInventoryItem(INVENTORY_ITEM *inv_item)
{
    phd_TranslateRel(0, inv_item->ytrans, inv_item->ztrans);
    phd_RotYXZ(inv_item->y_rot, inv_item->x_rot, 0);

    OBJECT_INFO *obj = &Objects[inv_item->object_number];
    if (obj->nmeshes < 0) {
        S_DrawSpriteRel(0, 0, 0, obj->mesh_index, 4096);
        return;
    }

    if (inv_item->sprlist) {
        int32_t zv = PhdMatrixPtr->_23;
        int32_t zp = zv / PhdPersp;
        int32_t sx = PhdCenterX + PhdMatrixPtr->_03 / zp;
        int32_t sy = PhdCenterY + PhdMatrixPtr->_13 / zp;

        INVENTORY_SPRITE **sprlist = inv_item->sprlist;
        INVENTORY_SPRITE *spr;
        while ((spr = *sprlist++)) {
            if (zv < PhdNearZ || zv > PhdFarZ) {
                break;
            }

            while (spr->shape) {
                switch (spr->shape) {
                case SHAPE_SPRITE:
                    S_DrawScreenSprite(
                        sx + spr->x, sy + spr->y, spr->z, spr->param1,
                        spr->param2,
                        StaticObjects[O_ALPHABET].mesh_number + spr->sprnum,
                        4096, 0);
                    break;
                case SHAPE_LINE:
                    S_DrawScreenLine(
                        sx + spr->x, sy + spr->y, spr->param1, spr->param2,
                        S_ColourFromPalette(spr->sprnum));
                    break;
                case SHAPE_BOX:
                    S_DrawScreenBox(
                        sx + spr->x, sy + spr->y, spr->param1, spr->param2);
                    break;
                case SHAPE_FBOX:
                    S_DrawScreenFBox(
                        sx + spr->x, sy + spr->y, spr->param1, spr->param2);
                    break;
                }
                spr++;
            }
        }
    }

    int16_t *frame =
        &obj->frame_base[inv_item->current_frame * (obj->nmeshes * 2 + 10)];

    phd_PushMatrix();

    int32_t clip = S_GetObjectBounds(frame);
    if (clip) {
        phd_TranslateRel(
            frame[FRAME_POS_X], frame[FRAME_POS_Y], frame[FRAME_POS_Z]);
        int32_t *packed_rotation = (int32_t *)(frame + FRAME_ROT);
        phd_RotYXZpack(*packed_rotation++);

        int32_t mesh_num = 1;

        int32_t *bone = &AnimBones[obj->bone_index];
        if (inv_item->drawn_meshes & mesh_num) {
            phd_PutPolygons(Meshes[obj->mesh_index], clip);
        }

        for (int i = 1; i < obj->nmeshes; i++) {
            mesh_num *= 2;

            int32_t bone_extra_flags = bone[0];
            if (bone_extra_flags & BEB_POP) {
                phd_PopMatrix();
            }
            if (bone_extra_flags & BEB_PUSH) {
                phd_PushMatrix();
            }

            phd_TranslateRel(bone[1], bone[2], bone[3]);
            phd_RotYXZpack(*packed_rotation++);

            if (inv_item->object_number == O_MAP_OPTION && i == 1) {
                CompassSpeed = CompassSpeed * 19 / 20
                    + (int16_t)(
                          -inv_item->y_rot - LaraItem->pos.y_rot
                          - CompassNeedle)
                        / 50;
                CompassNeedle += CompassSpeed;
                phd_RotY(CompassNeedle);
            }

            if (inv_item->drawn_meshes & mesh_num) {
                phd_PutPolygons(Meshes[obj->mesh_index + i], clip);
            }

            bone += 4;
        }
    }
    phd_PopMatrix();
}

void T1MInjectGameInvEntry()
{
    INJECT(0x0041E760, Display_Inventory);
    INJECT(0x0041F980, Construct_Inventory);
    INJECT(0x0041FAB0, SelectMeshes);
    INJECT(0x0041FB40, DrawInventoryItem);
}
