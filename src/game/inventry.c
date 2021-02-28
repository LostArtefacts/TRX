#include "3dsystem/3d_gen.h"
#include "3dsystem/phd_math.h"
#include "game/effects.h"
#include "game/inv.h"
#include "game/lara.h"
#include "game/option.h"
#include "game/text.h"
#include "game/vars.h"
#include "specific/display.h"
#include "specific/input.h"
#include "specific/output.h"
#include "specific/shed.h"
#include "specific/sndpc.h"
#include "util.h"
#include <string.h>

#define RING_RADIUS 688
#define CAMERA_STARTHEIGHT -0x600
#define CAMERA_HEIGHT -0x100
#define NOINPUT_TIME 480
#define CAMERA_2_RING 598
#define LOW_LIGHT 0x1400
#define HIGH_LIGHT 0x1000

static int OldInputDB = 0;

int32_t Display_Inventory(int inv_mode)
{
    RING_INFO ring;
    IMOTION_INFO imo;

    memset(&imo, 0, sizeof(IMOTION_INFO));
    memset(&ring, 0, sizeof(RING_INFO));

    if (inv_mode == RM_KEYS && !InvKeysObjects) {
        InventoryChosen = -1;
        return 0;
    }

    int32_t pass_mode_open = 0;
    if (AmmoText) {
        T_RemovePrint(AmmoText);
        AmmoText = 0;
    }

    AlterFOV(GAME_FOV * PHD_DEGREE);
    InventoryMode = inv_mode;

    InvNFrames = 2;
    Construct_Inventory();

    if (InventoryMode != INV_TITLE_MODE) {
        S_FadeInInventory(1);
    } else {
        S_FadeInInventory(0);
    }

    mn_stop_ambient_samples();
    S_SoundStopAllSamples();
    if (InventoryMode != INV_TITLE_MODE) {
        S_CDVolume(0);
    }

    switch (InventoryMode) {
    case INV_DEATH_MODE:
    case INV_SAVE_MODE:
    case INV_LOAD_MODE:
    case INV_TITLE_MODE:
        Inv_RingInit(
            &ring, RM_OPTION, InvOptionList, InvOptionObjects, InvOptionCurrent,
            &imo);
        break;

    case INV_KEYS_MODE:
        Inv_RingInit(
            &ring, RM_KEYS, InvKeysList, InvKeysObjects, InvMainCurrent, &imo);
        break;

    default:
        if (InvMainObjects) {
            Inv_RingInit(
                &ring, RM_MAIN, InvMainList, InvMainObjects, InvMainCurrent,
                &imo);
        } else {
            Inv_RingInit(
                &ring, RM_OPTION, InvOptionList, InvOptionObjects,
                InvOptionCurrent, &imo);
        }
        break;
    }

    SoundEffect(111, 0, RM_KEYS);

    InvNFrames = 2;

    do {
        Inv_RingCalcAdders(&ring, ROTATE_DURATION);
        S_UpdateInput();

        InputDB = GetDebouncedInput(Input);

        if (InventoryMode != INV_TITLE_MODE || Input || InputDB) {
            NoInputCount = 0;
            ResetFlag = 0;
        } else {
            NoInputCount++;
            if (NoInputCount > NOINPUT_TIME) {
                ResetFlag = INV_TITLE_MODE;
            }
        }

        for (int i = 0; i < InvNFrames; ++i) {
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
            INVENTORY_ITEM* inv_item = ring.list[i];

            if (i == ring.current_object) {
                for (int j = 0; j < InvNFrames; j++) {
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
            phd_RotYXZ(0x4000, inv_item->pt_xrot, 0);
            DrawInventoryItem(inv_item);
            angle += ring.angle_adder;
            phd_PopMatrix();
        }

        phd_PopMatrix();

        mn_update_sound_effects();
        T_DrawText();
        S_OutputPolyList();

        InvNFrames = S_DumpScreen();
        Camera.number_frames = InvNFrames;

        if (ring.rotating) {
            continue;
        }

        if ((InventoryMode == INV_SAVE_MODE || InventoryMode == INV_LOAD_MODE
             || InventoryMode == INV_DEATH_MODE)
            && !pass_mode_open) {
            InputDB = IN_SELECT;
        }

        switch (imo.status) {
        case RNG_OPEN:
            if (CHK_ANY(Input, IN_RIGHT) && ring.number_of_objects > 1) {
                Inv_RingRotateLeft(&ring);
                SoundEffect(108, 0, SFX_ALWAYS);
                break;
            }

            if (CHK_ANY(Input, IN_LEFT) && ring.number_of_objects > 1) {
                Inv_RingRotateRight(&ring);
                SoundEffect(108, 0, SFX_ALWAYS);
                break;
            }

            if ((ResetFlag || CHK_ANY(InputDB, IN_OPTION))
                && (ResetFlag || InventoryMode != INV_TITLE_MODE)) {
                SoundEffect(112, 0, SFX_ALWAYS);
                InventoryChosen = -1;

                if (ring.type == RM_MAIN) {
                    InvMainCurrent = ring.current_object;
                } else {
                    InvOptionCurrent = ring.current_object;
                }

                if (InventoryMode == INV_TITLE_MODE) {
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
                if ((InventoryMode == INV_SAVE_MODE
                     || InventoryMode == INV_LOAD_MODE
                     || InventoryMode == INV_DEATH_MODE)
                    && !pass_mode_open) {
                    pass_mode_open = 1;
                }

                Item_Data = 0;

                INVENTORY_ITEM* inv_item;
                if (ring.type == RM_MAIN) {
                    InvMainCurrent = ring.current_object;
                    inv_item = InvMainList[ring.current_object];
                } else if (ring.type == RM_OPTION) {
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
                    &ring, 0, -0x4000 - ring.angle_adder * ring.current_object);
                Inv_RingMotionItemSelect(&ring, inv_item);
                Input = 0;
                InputDB = 0;

                switch (inv_item->object_number) {
                case O_MAP_OPTION:
                    SoundEffect(113, 0, SFX_ALWAYS);
                    break;

                case O_PHOTO_OPTION:
                    SoundEffect(109, 0, SFX_ALWAYS);
                    break;

                case O_CONTROL_OPTION:
                    SoundEffect(110, 0, SFX_ALWAYS);
                    break;

                case O_GUN_OPTION:
                case O_SHOTGUN_OPTION:
                case O_MAGNUM_OPTION:
                case O_UZI_OPTION:
                    SoundEffect(114, 0, SFX_ALWAYS);
                    break;

                default:
                    SoundEffect(111, 0, SFX_ALWAYS);
                    break;
                }
            }

            if (CHK_ANY(InputDB, IN_FORWARD) && InventoryMode != INV_TITLE_MODE
                && InventoryMode != INV_KEYS_MODE) {
                if (ring.type == RM_MAIN) {
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
                } else if (ring.type == RM_OPTION) {
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
                CHK_ANY(InputDB, IN_BACK) && InventoryMode != INV_TITLE_MODE
                && InventoryMode != INV_KEYS_MODE) {
                if (ring.type == RM_KEYS) {
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
                } else if (ring.type == RM_MAIN) {
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
            ring.type = RM_OPTION;
            ring.number_of_objects = InvOptionObjects;
            ring.current_object = InvOptionCurrent;
            Inv_RingCalcAdders(&ring, ROTATE_DURATION);
            Inv_RingMotionRotation(
                &ring, OPEN_ROTATION,
                -0x4000 - ring.angle_adder * ring.current_object);
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
            ring.type = RM_KEYS;
            ring.number_of_objects = InvKeysObjects;
            ring.current_object = InvKeysCurrent;
            Inv_RingCalcAdders(&ring, ROTATE_DURATION);
            Inv_RingMotionRotation(
                &ring, OPEN_ROTATION,
                -0x4000 - ring.angle_adder * ring.current_object);
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
            ring.type = RM_MAIN;
            ring.number_of_objects = InvMainObjects;
            ring.current_object = InvMainCurrent;
            Inv_RingCalcAdders(&ring, ROTATE_DURATION);
            Inv_RingMotionRotation(
                &ring, OPEN_ROTATION,
                -0x4000 - ring.angle_adder * ring.current_object);
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
            ring.type = RM_MAIN;
            ring.number_of_objects = InvMainObjects;
            ring.current_object = InvMainCurrent;
            Inv_RingCalcAdders(&ring, ROTATE_DURATION);
            Inv_RingMotionRotation(
                &ring, OPEN_ROTATION,
                -0x4000 - ring.angle_adder * ring.current_object);
            ring.ringpos.y_rot = imo.rotate_target + OPEN_ROTATION;
            break;

        case RNG_SELECTED: {
            INVENTORY_ITEM* inv_item = ring.list[ring.current_object];
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
                do_inventory_options(inv_item);

                if (CHK_ANY(InputDB, IN_DESELECT)) {
                    inv_item->sprlist = NULL;
                    Inv_RingMotionSetup(
                        &ring, RNG_CLOSING_ITEM, RNG_DESELECT, 0);
                    Input = 0;
                    InputDB = 0;

                    if (InventoryMode == INV_LOAD_MODE
                        || InventoryMode == INV_SAVE_MODE) {
                        Inv_RingMotionSetup(
                            &ring, RNG_CLOSING_ITEM, RNG_EXITING_INVENTORY, 0);
                        Input = 0;
                        InputDB = 0;
                    }
                }

                if (CHK_ANY(InputDB, IN_SELECT)) {
                    inv_item->sprlist = NULL;
                    InventoryChosen = inv_item->object_number;
                    if (ring.type == RM_MAIN) {
                        InvMainCurrent = ring.current_object;
                    } else {
                        InvOptionCurrent = ring.current_object;
                    }

                    if (InventoryMode == INV_TITLE_MODE
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
            SoundEffect(112, 0, SFX_ALWAYS);
            Inv_RingMotionSetup(
                &ring, RNG_DESELECTING, RNG_OPEN, SELECTING_FRAMES);
            Inv_RingMotionRotation(
                &ring, 0, -0x4000 - ring.angle_adder * ring.current_object);
            Input = 0;
            InputDB = 0;
            break;

        case RNG_CLOSING_ITEM: {
            INVENTORY_ITEM* inv_item = ring.list[ring.current_object];
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
                if (InventoryMode != INV_TITLE_MODE) {
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

    Inventory_Displaying = 0;

    if (ResetFlag) {
        return GF_EXIT_TO_TITLE;
    }

    switch (InventoryChosen) {
    case O_PASSPORT_OPTION:
        if (InventoryExtraData[0] == 1 && OptionMusicVolume) {
            S_CDVolume(25 * OptionMusicVolume + 5);
        }
        return GF_STARTGAME | LV_FIRSTLEVEL;

    case O_PHOTO_OPTION:
        InventoryExtraData[1] = 0;
        return GF_STARTGAME | LV_FIRSTLEVEL;

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

    if (InventoryMode != INV_TITLE_MODE && OptionMusicVolume) {
        S_CDVolume(25 * OptionMusicVolume + 5);
    }

    return 0;
}

int32_t AnimateInventoryItem(INVENTORY_ITEM* inv_item)
{
    if (inv_item->current_frame == inv_item->goal_frame) {
        SelectMeshes(inv_item);
        return 0;
    }
    if (inv_item->anim_count) {
        inv_item->anim_count--;
    } else {
        inv_item->anim_count = inv_item->anim_speed;
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

int32_t GetDebouncedInput(int32_t input)
{
    if (input && !OldInputDB) {
        OldInputDB = input;
    } else if (!input) {
        OldInputDB = 0;
    } else {
        input = 0;
    }
    return input;
}

void T1MInjectGameInvEntry()
{
    INJECT(0x0041E760, Display_Inventory);
}
