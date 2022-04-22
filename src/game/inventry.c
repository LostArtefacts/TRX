#include "game/inv.h"

#include "3dsystem/3d_gen.h"
#include "3dsystem/matrix.h"
#include "config.h"
#include "game/clock.h"
#include "game/draw.h"
#include "game/gameflow.h"
#include "game/input.h"
#include "game/lara.h"
#include "game/music.h"
#include "game/option.h"
#include "game/output.h"
#include "game/overlay.h"
#include "game/savegame.h"
#include "game/screen.h"
#include "game/settings.h"
#include "game/sound.h"
#include "game/text.h"
#include "game/viewport.h"
#include "global/const.h"
#include "global/types.h"
#include "global/vars.h"
#include "specific/s_misc.h"

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

static TEXTSTRING *m_VersionText = NULL;
static int16_t m_InvNFrames = 2;
static int16_t m_CompassNeedle = 0;
static int16_t m_CompassSpeed = 0;
static CAMERA_INFO m_OldCamera;

static void Inventory_Draw(RING_INFO *ring, IMOTION_INFO *imo);

static void Inventory_Draw(RING_INFO *ring, IMOTION_INFO *imo)
{
    Output_InitialisePolyList();

    if (g_InvMode == INV_TITLE_MODE) {
        Output_CopyPictureToScreen();
        Output_DrawBackdropScreen();
    } else {
        phd_LookAt(
            m_OldCamera.pos.x, m_OldCamera.pos.y + m_OldCamera.shift,
            m_OldCamera.pos.z, m_OldCamera.target.x, m_OldCamera.target.y,
            m_OldCamera.target.z, 0);
        Draw_DrawScene(false);

        int32_t width = Screen_GetResWidth();
        int32_t height = Screen_GetResHeight();
        ViewPort_Init(width, height);
    }

    Output_SetupAboveWater(false);

    PHD_3DPOS viewer;
    Inv_RingGetView(ring, &viewer);
    phd_GenerateW2V(&viewer);
    Inv_RingLight(ring);

    phd_PushMatrix();
    phd_TranslateAbs(ring->ringpos.x, ring->ringpos.y, ring->ringpos.z);
    phd_RotYXZ(ring->ringpos.y_rot, ring->ringpos.x_rot, ring->ringpos.z_rot);

    PHD_ANGLE angle = 0;
    for (int i = 0; i < ring->number_of_objects; i++) {
        INVENTORY_ITEM *inv_item = ring->list[i];

        if (imo->status == RNG_DONE) {
            g_LsAdder = LOW_LIGHT;
        } else if (i == ring->current_object) {
            for (int j = 0; j < m_InvNFrames; j++) {
                if (ring->rotating) {
                    g_LsAdder = LOW_LIGHT;
                    if (inv_item->y_rot < 0) {
                        inv_item->y_rot += 512;
                    } else if (inv_item->y_rot > 0) {
                        inv_item->y_rot -= 512;
                    }
                } else if (
                    imo->status == RNG_SELECTED
                    || imo->status == RNG_DESELECTING
                    || imo->status == RNG_SELECTING
                    || imo->status == RNG_DESELECT
                    || imo->status == RNG_CLOSING_ITEM) {
                    g_LsAdder = HIGH_LIGHT;
                    if (inv_item->y_rot != inv_item->y_rot_sel) {
                        if (inv_item->y_rot_sel - inv_item->y_rot > 0
                            && inv_item->y_rot_sel - inv_item->y_rot < 0x8000) {
                            inv_item->y_rot += 1024;
                        } else {
                            inv_item->y_rot -= 1024;
                        }
                        inv_item->y_rot &= 0xFC00u;
                    }
                } else if (
                    ring->number_of_objects == 1
                    || (!g_Input.left && !g_Input.right) || !g_Input.left) {
                    g_LsAdder = HIGH_LIGHT;
                    inv_item->y_rot += 256;
                }
            }
        } else {
            g_LsAdder = LOW_LIGHT;
            for (int j = 0; j < m_InvNFrames; j++) {
                if (inv_item->y_rot < 0) {
                    inv_item->y_rot += 256;
                } else if (inv_item->y_rot > 0) {
                    inv_item->y_rot -= 256;
                }
            }
        }

        phd_PushMatrix();
        phd_RotYXZ(angle, 0, 0);
        phd_TranslateRel(ring->radius, 0, 0);
        phd_RotYXZ(PHD_90, inv_item->pt_xrot, 0);
        DrawInventoryItem(inv_item);
        angle += ring->angle_adder;
        phd_PopMatrix();
    }

    phd_PopMatrix();
}

int32_t Display_Inventory(int inv_mode)
{
    RING_INFO ring;
    IMOTION_INFO imo;

    memset(&imo, 0, sizeof(IMOTION_INFO));
    memset(&ring, 0, sizeof(RING_INFO));

    if (inv_mode == RT_KEYS && !g_InvKeysObjects) {
        g_InvChosen = -1;
        return GF_NOP;
    }

    bool pass_mode_open = false;
    g_InvMode = inv_mode;

    int no_input_count = 0;
    bool start_demo = false;
    m_InvNFrames = 2;
    if (g_InvMode != INV_TITLE_MODE) {
        Screen_ApplyResolution();
    }
    Construct_Inventory();

    if (g_Config.disable_music_in_inventory && g_InvMode != INV_TITLE_MODE) {
        Music_Pause();
        Sound_StopAmbientSounds();
        Sound_StopAllSamples();
    }

    switch (g_InvMode) {
    case INV_DEATH_MODE:
    case INV_SAVE_MODE:
    case INV_SAVE_CRYSTAL_MODE:
    case INV_LOAD_MODE:
    case INV_TITLE_MODE:
        Inv_RingInit(
            &ring, RT_OPTION, g_InvOptionList, g_InvOptionObjects,
            g_InvOptionCurrent, &imo);
        break;

    case INV_KEYS_MODE:
        Inv_RingInit(
            &ring, RT_KEYS, g_InvKeysList, g_InvKeysObjects, g_InvMainCurrent,
            &imo);
        break;

    default:
        if (g_InvMainObjects) {
            Inv_RingInit(
                &ring, RT_MAIN, g_InvMainList, g_InvMainObjects,
                g_InvMainCurrent, &imo);
        } else {
            Inv_RingInit(
                &ring, RT_OPTION, g_InvOptionList, g_InvOptionObjects,
                g_InvOptionCurrent, &imo);
        }
        break;
    }

    m_InvNFrames = 2;

    m_OldCamera = g_Camera;

    if (g_InvMode == INV_TITLE_MODE) {
        // reset the clock after delay from loading the title level to reset
        // the initial spike in the lost frames and have smooth fades
        Clock_SyncTicks(TICKS_PER_FRAME);
        Output_FadeResetToBlack();
        // make main menu fades faster
        Output_FadeSetSpeed(2.0);
        Output_FadeToTransparent(true);
        while (Output_FadeIsAnimating()) {
            Output_InitialisePolyList();
            Output_CopyPictureToScreen();
            Output_DrawBackdropScreen();
            Output_DumpScreen();
        }
    } else {
        Output_FadeToSemiBlack(true);
    }

    Sound_Effect(SFX_MENU_SPININ, NULL, SPM_ALWAYS);

    do {
        Inv_RingCalcAdders(&ring, ROTATE_DURATION);
        Input_Update();

        if (g_InvMode != INV_TITLE_MODE || g_Input.any || g_InputDB.any) {
            no_input_count = 0;
        } else if (!g_Config.disable_demo && imo.status == RNG_OPEN) {
            no_input_count++;
            if (g_GameFlow.has_demo && no_input_count > g_GameFlow.demo_delay) {
                start_demo = true;
            }
        }

        for (int i = 0; i < m_InvNFrames; i++) {
            if (g_IDelay) {
                if (g_IDCount) {
                    g_IDCount--;
                } else {
                    g_IDelay = false;
                }
            }
            Inv_RingDoMotions(&ring);
        }

        ring.camera.z = ring.radius + CAMERA_2_RING;

        Inventory_Draw(&ring, &imo);

        if (imo.status == RNG_OPEN || imo.status == RNG_SELECTING
            || imo.status == RNG_SELECTED || imo.status == RNG_DESELECTING
            || imo.status == RNG_DESELECT || imo.status == RNG_CLOSING_ITEM) {
            if (!ring.rotating && !g_Input.left && !g_Input.right) {
                INVENTORY_ITEM *inv_item = ring.list[ring.current_object];
                RingActive(inv_item);
            }
            RingIsOpen(&ring);
        } else {
            RingIsNotOpen(&ring);
        }

        if (!imo.status || imo.status == RNG_CLOSING
            || imo.status == RNG_MAIN2OPTION || imo.status == RNG_OPTION2MAIN
            || imo.status == RNG_EXITING_INVENTORY || imo.status == RNG_DONE
            || ring.rotating) {
            RingNotActive();
        }

        Sound_UpdateEffects();
        Overlay_DrawFPSInfo();
        Text_Draw();

        m_InvNFrames = Output_DumpScreen();
        g_Camera.number_frames = m_InvNFrames;

        if (g_Config.enable_timer_in_inventory) {
            g_GameInfo.current[g_CurrentLevel].stats.timer += m_InvNFrames / 2;
        }

        if (ring.rotating) {
            continue;
        }

        if ((g_InvMode == INV_SAVE_MODE || g_InvMode == INV_SAVE_CRYSTAL_MODE
             || g_InvMode == INV_LOAD_MODE || g_InvMode == INV_DEATH_MODE)
            && !pass_mode_open) {
            g_InputDB = (INPUT_STATE) { 0, .select = 1 };
        }

        switch (imo.status) {
        case RNG_OPEN:
            if (g_Input.right && ring.number_of_objects > 1) {
                Inv_RingRotateLeft(&ring);
                Sound_Effect(SFX_MENU_ROTATE, NULL, SPM_ALWAYS);
                break;
            }

            if (g_Input.left && ring.number_of_objects > 1) {
                Inv_RingRotateRight(&ring);
                Sound_Effect(SFX_MENU_ROTATE, NULL, SPM_ALWAYS);
                break;
            }

            if (start_demo
                || (g_InputDB.option && g_InvMode != INV_TITLE_MODE)) {
                Sound_Effect(SFX_MENU_SPINOUT, NULL, SPM_ALWAYS);
                g_InvChosen = -1;

                if (ring.type == RT_MAIN) {
                    g_InvMainCurrent = ring.current_object;
                } else {
                    g_InvOptionCurrent = ring.current_object;
                }

                if (g_InvMode != INV_TITLE_MODE) {
                    Output_FadeToTransparent(false);
                }

                Inv_RingMotionSetup(&ring, RNG_CLOSING, RNG_DONE, CLOSE_FRAMES);
                Inv_RingMotionRadius(&ring, 0);
                Inv_RingMotionCameraPos(&ring, CAMERA_STARTHEIGHT);
                Inv_RingMotionRotation(
                    &ring, CLOSE_ROTATION, ring.ringpos.y_rot - CLOSE_ROTATION);
                g_Input = (INPUT_STATE) { 0 };
                g_InputDB = (INPUT_STATE) { 0 };
            }

            if (g_InputDB.select) {
                if ((g_InvMode == INV_SAVE_MODE
                     || g_InvMode == INV_SAVE_CRYSTAL_MODE
                     || g_InvMode == INV_LOAD_MODE
                     || g_InvMode == INV_DEATH_MODE)
                    && !pass_mode_open) {
                    pass_mode_open = true;
                }

                g_OptionSelected = 0;

                INVENTORY_ITEM *inv_item;
                if (ring.type == RT_MAIN) {
                    g_InvMainCurrent = ring.current_object;
                    inv_item = g_InvMainList[ring.current_object];
                } else if (ring.type == RT_OPTION) {
                    g_InvOptionCurrent = ring.current_object;
                    inv_item = g_InvOptionList[ring.current_object];
                } else {
                    g_InvKeysCurrent = ring.current_object;
                    inv_item = g_InvKeysList[ring.current_object];
                }

                inv_item->goal_frame = inv_item->open_frame;
                inv_item->anim_direction = 1;

                Inv_RingMotionSetup(
                    &ring, RNG_SELECTING, RNG_SELECTED, SELECTING_FRAMES);
                Inv_RingMotionRotation(
                    &ring, 0, -PHD_90 - ring.angle_adder * ring.current_object);
                Inv_RingMotionItemSelect(&ring, inv_item);
                g_Input = (INPUT_STATE) { 0 };
                g_InputDB = (INPUT_STATE) { 0 };

                switch (inv_item->object_number) {
                case O_MAP_OPTION:
                    Sound_Effect(SFX_MENU_COMPASS, NULL, SPM_ALWAYS);
                    break;

                case O_PHOTO_OPTION:
                    Sound_Effect(SFX_MENU_CHOOSE, NULL, SPM_ALWAYS);
                    break;

                case O_CONTROL_OPTION:
                    Sound_Effect(SFX_MENU_GAMEBOY, NULL, SPM_ALWAYS);
                    break;

                case O_GUN_OPTION:
                case O_SHOTGUN_OPTION:
                case O_MAGNUM_OPTION:
                case O_UZI_OPTION:
                    Sound_Effect(SFX_MENU_GUNS, NULL, SPM_ALWAYS);
                    break;

                default:
                    Sound_Effect(SFX_MENU_SPININ, NULL, SPM_ALWAYS);
                    break;
                }
            }

            if (g_InputDB.forward && g_InvMode != INV_TITLE_MODE
                && g_InvMode != INV_KEYS_MODE) {
                if (ring.type == RT_MAIN) {
                    if (g_InvKeysObjects) {
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
                    g_Input = (INPUT_STATE) { 0 };
                    g_InputDB = (INPUT_STATE) { 0 };
                } else if (ring.type == RT_OPTION) {
                    if (g_InvMainObjects) {
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
                    g_InputDB = (INPUT_STATE) { 0 };
                }
            } else if (
                g_InputDB.back && g_InvMode != INV_TITLE_MODE
                && g_InvMode != INV_KEYS_MODE) {
                if (ring.type == RT_KEYS) {
                    if (g_InvMainObjects) {
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
                    g_Input = (INPUT_STATE) { 0 };
                    g_InputDB = (INPUT_STATE) { 0 };
                } else if (ring.type == RT_MAIN) {
                    if (g_InvOptionObjects) {
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
                    g_InputDB = (INPUT_STATE) { 0 };
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
            g_InvMainCurrent = ring.current_object;
            ring.list = g_InvOptionList;
            ring.type = RT_OPTION;
            ring.number_of_objects = g_InvOptionObjects;
            ring.current_object = g_InvOptionCurrent;
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
            g_InvMainCurrent = ring.current_object;
            g_InvMainObjects = ring.number_of_objects;
            ring.list = g_InvKeysList;
            ring.type = RT_KEYS;
            ring.number_of_objects = g_InvKeysObjects;
            ring.current_object = g_InvKeysCurrent;
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
            g_InvKeysCurrent = ring.current_object;
            ring.list = g_InvMainList;
            ring.type = RT_MAIN;
            ring.number_of_objects = g_InvMainObjects;
            ring.current_object = g_InvMainCurrent;
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
            g_InvOptionObjects = ring.number_of_objects;
            g_InvOptionCurrent = ring.current_object;
            ring.list = g_InvMainList;
            ring.type = RT_MAIN;
            ring.number_of_objects = g_InvMainObjects;
            ring.current_object = g_InvMainCurrent;
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

            bool busy = false;
            for (int j = 0; j < m_InvNFrames; j++) {
                busy = false;
                if (inv_item->y_rot == inv_item->y_rot_sel) {
                    busy = AnimateInventoryItem(inv_item);
                }
            }

            if (!busy && !g_IDelay) {
                Option_DoInventory(inv_item);

                if (g_InputDB.deselect) {
                    inv_item->sprlist = NULL;
                    Inv_RingMotionSetup(
                        &ring, RNG_CLOSING_ITEM, RNG_DESELECT, 0);
                    g_Input = (INPUT_STATE) { 0 };
                    g_InputDB = (INPUT_STATE) { 0 };

                    if (g_InvMode == INV_LOAD_MODE || g_InvMode == INV_SAVE_MODE
                        || g_InvMode == INV_SAVE_CRYSTAL_MODE) {
                        Inv_RingMotionSetup(
                            &ring, RNG_CLOSING_ITEM, RNG_EXITING_INVENTORY, 0);
                        g_Input = (INPUT_STATE) { 0 };
                        g_InputDB = (INPUT_STATE) { 0 };
                    }
                }

                if (g_InputDB.select) {
                    inv_item->sprlist = NULL;
                    g_InvChosen = inv_item->object_number;
                    if (ring.type == RT_MAIN) {
                        g_InvMainCurrent = ring.current_object;
                    } else {
                        g_InvOptionCurrent = ring.current_object;
                    }

                    if (g_InvMode == INV_TITLE_MODE
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
                    g_Input = (INPUT_STATE) { 0 };
                    g_InputDB = (INPUT_STATE) { 0 };
                }
            }
            break;
        }

        case RNG_DESELECT:
            Sound_Effect(SFX_MENU_SPINOUT, NULL, SPM_ALWAYS);
            Inv_RingMotionSetup(
                &ring, RNG_DESELECTING, RNG_OPEN, SELECTING_FRAMES);
            Inv_RingMotionRotation(
                &ring, 0, -PHD_90 - ring.angle_adder * ring.current_object);
            g_Input = (INPUT_STATE) { 0 };
            g_InputDB = (INPUT_STATE) { 0 };
            break;

        case RNG_CLOSING_ITEM: {
            INVENTORY_ITEM *inv_item = ring.list[ring.current_object];
            for (int j = 0; j < m_InvNFrames; j++) {
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
            if (g_InvMode == INV_TITLE_MODE) {
            } else if (
                g_InvChosen == O_PASSPORT_OPTION
                && (g_InvMode == INV_LOAD_MODE /* f6 menu */
                    || g_InvMode == INV_DEATH_MODE /* Lara died */
                    || (g_InvMode == INV_GAME_MODE /* esc menu */
                        && g_InvExtraData[IED_PAGE_NUM]
                            != PASSPORT_PAGE_2 /* but not the save page */
                        )
                    || g_CurrentLevel == g_GameFlow.gym_level_num /* Gym */
                    )) {
                Output_FadeToBlack(false);
            } else {
                Output_FadeToTransparent(false);
            }

            if (!imo.count) {
                Inv_RingMotionSetup(&ring, RNG_CLOSING, RNG_DONE, CLOSE_FRAMES);
                Inv_RingMotionRadius(&ring, 0);
                Inv_RingMotionCameraPos(&ring, CAMERA_STARTHEIGHT);
                Inv_RingMotionRotation(
                    &ring, CLOSE_ROTATION, ring.ringpos.y_rot - CLOSE_ROTATION);
            }
            break;
        }
    } while (imo.status != RNG_DONE);

    // finish fading
    if (g_InvMode == INV_TITLE_MODE) {
        Output_FadeToBlack(true);
    }
    bool fade_finished = !Output_FadeIsAnimating();
    while (!fade_finished) {
        fade_finished = !Output_FadeIsAnimating();
        Inventory_Draw(&ring, &imo);
        m_InvNFrames = Output_DumpScreen();
        g_Camera.number_frames = m_InvNFrames;
    }

    RemoveInventoryText();
    Output_FadeReset();
    Output_FadeSetSpeed(1.0);

    if (g_InvMode != INV_TITLE_MODE) {
        Screen_ApplyResolution();
    }
    g_ModeLock = false;

    if (m_VersionText) {
        Text_Remove(m_VersionText);
        m_VersionText = NULL;
    }

    if (g_Config.disable_music_in_inventory && g_InvMode != INV_TITLE_MODE) {
        Music_Unpause();
    }

    if (start_demo) {
        no_input_count = 0;
        start_demo = false;
        return GF_START_DEMO;
    }

    switch (g_InvChosen) {
    case O_PASSPORT_OPTION:
        if (g_InvMode == INV_TITLE_MODE) {
            if (g_InvExtraData[IED_PAGE_NUM] == PASSPORT_PAGE_1
                && g_InvExtraData[IED_PASSPORT_MODE]
                    == PASSPORT_MODE_SHOW_SAVES) {
                // page 1: load game
                return GF_START_SAVED_GAME | g_InvExtraData[IED_SAVEGAME_NUM];
            } else if (
                g_InvExtraData[IED_PAGE_NUM] == PASSPORT_PAGE_1
                && g_InvExtraData[IED_PASSPORT_MODE]
                    == PASSPORT_MODE_SELECT_LEVEL) {
                // page 1: select level
                return GF_SELECT_GAME | g_InvExtraData[IED_LEVEL_NUM];
            } else if (g_InvExtraData[IED_PAGE_NUM] == PASSPORT_PAGE_2) {
                // page 2: new game
                switch (g_InvExtraData[IED_SAVEGAME_NUM]) {
                case 0:
                    g_GameInfo.bonus_flag = 0;
                    break;
                case 1:
                    g_GameInfo.bonus_flag = GBF_NGPLUS;
                    break;
                case 2:
                    g_GameInfo.bonus_flag = GBF_JAPANESE;
                    break;
                case 3:
                    g_GameInfo.bonus_flag = GBF_JAPANESE | GBF_NGPLUS;
                    break;
                }
                Savegame_InitCurrentInfo();
                return GF_START_GAME | g_GameFlow.first_level_num;
            } else {
                // page 3: exit game
                return GF_EXIT_GAME;
            }
        } else if (g_InvMode == INV_DEATH_MODE) {
            if (g_InvExtraData[IED_PAGE_NUM] == PASSPORT_PAGE_1
                && g_InvExtraData[IED_PASSPORT_MODE]
                    == PASSPORT_MODE_SHOW_SAVES) {
                // page 1: load game
                return GF_START_SAVED_GAME | g_InvExtraData[IED_SAVEGAME_NUM];
            } else if (
                g_InvExtraData[IED_PAGE_NUM] == PASSPORT_PAGE_1
                && g_InvExtraData[IED_PASSPORT_MODE]
                    == PASSPORT_MODE_SELECT_LEVEL) {
                // page 1: select level
                return GF_SELECT_GAME | g_InvExtraData[IED_LEVEL_NUM];
            } else if (g_InvExtraData[IED_PAGE_NUM] == PASSPORT_PAGE_2) {
                // page 2: restart level
                return GF_RESTART_GAME | g_CurrentLevel;
            } else {
                // page 3: exit game
                return GF_EXIT_TO_TITLE;
            }
        } else {
            if (g_InvExtraData[IED_PAGE_NUM] == PASSPORT_PAGE_1
                && g_InvExtraData[IED_PASSPORT_MODE]
                    == PASSPORT_MODE_SHOW_SAVES) {
                // page 1: load game
                return GF_START_SAVED_GAME | g_InvExtraData[IED_SAVEGAME_NUM];
            } else if (
                g_InvExtraData[IED_PAGE_NUM] == PASSPORT_PAGE_1
                && g_InvExtraData[IED_PASSPORT_MODE]
                    == PASSPORT_MODE_SELECT_LEVEL) {
                // page 1: select level
                return GF_SELECT_GAME | g_InvExtraData[IED_LEVEL_NUM];
            } else if (g_InvExtraData[IED_PAGE_NUM] == PASSPORT_PAGE_2) {
                // page 1: save game, or new game in gym
                if (g_CurrentLevel == g_GameFlow.gym_level_num) {
                    switch (g_InvExtraData[IED_SAVEGAME_NUM]) {
                    case 0:
                        g_GameInfo.bonus_flag = 0;
                        break;
                    case 1:
                        g_GameInfo.bonus_flag = GBF_NGPLUS;
                        break;
                    case 2:
                        g_GameInfo.bonus_flag = GBF_JAPANESE;
                        break;
                    case 3:
                        g_GameInfo.bonus_flag = GBF_JAPANESE | GBF_NGPLUS;
                        break;
                    }
                    Savegame_InitCurrentInfo();
                    return GF_START_GAME | g_GameFlow.first_level_num;
                } else {
                    Savegame_Save(
                        g_InvExtraData[IED_SAVEGAME_NUM], &g_GameInfo);
                    Settings_Write();
                    return GF_NOP;
                }
            } else {
                // page 3: exit to title
                return GF_EXIT_TO_TITLE;
            }
        }

    case O_PHOTO_OPTION:
        g_InvExtraData[IED_SAVEGAME_NUM] = 0;
        return GF_START_GAME | g_GameFlow.gym_level_num;

    case O_GUN_OPTION:
        Lara_UseItem(O_GUN_OPTION);
        break;

    case O_SHOTGUN_OPTION:
        Lara_UseItem(O_SHOTGUN_OPTION);
        break;

    case O_MAGNUM_OPTION:
        Lara_UseItem(O_MAGNUM_OPTION);
        break;

    case O_UZI_OPTION:
        Lara_UseItem(O_UZI_OPTION);
        break;

    case O_MEDI_OPTION:
        Lara_UseItem(O_MEDI_OPTION);
        break;

    case O_BIGMEDI_OPTION:
        Lara_UseItem(O_BIGMEDI_OPTION);
        break;
    }

    return GF_NOP;
}

void Construct_Inventory(void)
{
    g_PhdLeft = ViewPort_GetMinX();
    g_PhdTop = ViewPort_GetMinY();
    g_PhdBottom = ViewPort_GetMaxY();
    g_PhdRight = ViewPort_GetMaxX();

    g_InvChosen = 0;
    if (g_InvMode == INV_TITLE_MODE) {
        g_InvOptionObjects = TITLE_RING_OBJECTS;
        m_VersionText = Text_Create(-20, -18, g_T1MVersion);
        Text_AlignRight(m_VersionText, 1);
        Text_AlignBottom(m_VersionText, 1);
        Text_SetScale(m_VersionText, PHD_ONE * 0.5, PHD_ONE * 0.5);
    } else {
        g_InvOptionObjects = OPTION_RING_OBJECTS;
        Text_Remove(m_VersionText);
        m_VersionText = NULL;
    }

    for (int i = 0; i < g_InvMainObjects; i++) {
        INVENTORY_ITEM *inv_item = g_InvMainList[i];
        inv_item->drawn_meshes = inv_item->which_meshes;
        inv_item->current_frame = 0;
        inv_item->goal_frame = inv_item->current_frame;
        inv_item->anim_count = 0;
        inv_item->y_rot = 0;
    }

    for (int i = 0; i < g_InvOptionObjects; i++) {
        INVENTORY_ITEM *inv_item = g_InvOptionList[i];
        inv_item->current_frame = 0;
        inv_item->goal_frame = 0;
        inv_item->anim_count = 0;
        inv_item->y_rot = 0;
    }

    g_InvMainCurrent = 0;
    g_InvOptionCurrent = 0;
    g_OptionSelected = 0;

    if (g_GameFlow.gym_level_num == -1) {
        Inv_RemoveItem(O_PHOTO_OPTION);
    }
}

bool AnimateInventoryItem(INVENTORY_ITEM *inv_item)
{
    if (inv_item->current_frame == inv_item->goal_frame) {
        SelectMeshes(inv_item);
        return false;
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
    return true;
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

    OBJECT_INFO *obj = &g_Objects[inv_item->object_number];
    if (obj->nmeshes < 0) {
        Output_DrawSpriteRel(0, 0, 0, obj->mesh_index, 4096);
        return;
    }

    if (inv_item->sprlist) {
        int32_t zv = g_PhdMatrixPtr->_23;
        int32_t zp = zv / g_PhdPersp;
        int32_t sx = ViewPort_GetCenterX() + g_PhdMatrixPtr->_03 / zp;
        int32_t sy = ViewPort_GetCenterY() + g_PhdMatrixPtr->_13 / zp;

        INVENTORY_SPRITE **sprlist = inv_item->sprlist;
        INVENTORY_SPRITE *spr;
        while ((spr = *sprlist++)) {
            if (zv < Output_GetNearZ() || zv > Output_GetFarZ()) {
                break;
            }

            while (spr->shape) {
                switch (spr->shape) {
                case SHAPE_SPRITE:
                    Output_DrawScreenSprite(
                        sx + spr->x, sy + spr->y, spr->z, spr->param1,
                        spr->param2,
                        g_Objects[O_ALPHABET].mesh_index + spr->sprnum, 4096,
                        0);
                    break;
                case SHAPE_LINE:
                    Output_DrawScreenLine(
                        sx + spr->x, sy + spr->y, spr->param1, spr->param2,
                        Output_RGB2RGBA(
                            Output_GetPaletteColor((uint8_t)spr->sprnum)));
                    break;
                case SHAPE_BOX:
                    Output_DrawScreenBox(
                        sx + spr->x, sy + spr->y, spr->param1, spr->param2);
                    break;
                case SHAPE_FBOX:
                    Output_DrawScreenFBox(
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

        int32_t *bone = &g_AnimBones[obj->bone_index];
        if (inv_item->drawn_meshes & mesh_num) {
            Output_DrawPolygons(g_Meshes[obj->mesh_index], clip);
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
                m_CompassSpeed = m_CompassSpeed * 19 / 20
                    + (int16_t)(-inv_item->y_rot - g_LaraItem->pos.y_rot - m_CompassNeedle)
                        / 50;
                m_CompassNeedle += m_CompassSpeed;
                phd_RotY(m_CompassNeedle);
            }

            if (inv_item->drawn_meshes & mesh_num) {
                Output_DrawPolygons(g_Meshes[obj->mesh_index + i], clip);
            }

            bone += 4;
        }
    }
    phd_PopMatrix();
}
