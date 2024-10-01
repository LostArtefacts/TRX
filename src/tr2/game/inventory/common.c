#include "game/inventory/common.h"

#include "game/demo.h"
#include "game/input.h"
#include "game/inventory/backpack.h"
#include "game/inventory/ring.h"
#include "game/inventory/vars.h"
#include "game/lara/control.h"
#include "game/math_misc.h"
#include "game/matrix.h"
#include "game/music.h"
#include "game/objects/common.h"
#include "game/objects/vars.h"
#include "game/option/option.h"
#include "game/output.h"
#include "game/overlay.h"
#include "game/sound.h"
#include "game/text.h"
#include "global/const.h"
#include "global/funcs.h"
#include "global/types.h"
#include "global/vars.h"

#include <libtrx/game/objects/names.h>

#include <stdio.h>

#define TITLE_RING_OBJECTS 3
#define OPTION_RING_OBJECTS 3

static TEXTSTRING *m_VersionText = NULL;

static void M_RemoveItemsText(void);
static void M_RemoveAllText(void);
static void M_ShowItemQuantity(const char *fmt, int32_t qty);
static void M_ShowAmmoQuantity(const char *fmt, int32_t qty);

static void M_RemoveItemsText(void)
{
    for (int32_t i = 0; i < 2; i++) {
        Text_Remove(g_Inv_ItemText[i]);
        g_Inv_ItemText[i] = NULL;
    }
}

static void M_RemoveAllText(void)
{
    M_RemoveItemsText();

    Text_Remove(g_Inv_TagText);
    g_Inv_TagText = NULL;
    Text_Remove(g_Inv_RingText);
    g_Inv_RingText = NULL;
    Text_Remove(g_Inv_UpArrow1);
    g_Inv_UpArrow1 = NULL;
    Text_Remove(g_Inv_UpArrow2);
    g_Inv_UpArrow2 = NULL;
    Text_Remove(g_Inv_DownArrow1);
    g_Inv_DownArrow1 = NULL;
    Text_Remove(g_Inv_DownArrow2);
    g_Inv_DownArrow2 = NULL;

    Text_Remove(m_VersionText);
    m_VersionText = NULL;
}

static void M_ShowItemQuantity(const char *const fmt, const int32_t qty)
{
    if (g_Inv_ItemText[1] == NULL && !g_SaveGame.bonus_flag) {
        char string[64];
        sprintf(string, fmt, qty);
        Overlay_MakeAmmoString(string);
        g_Inv_ItemText[1] = Text_Create(64, -56, 0, string);
        Text_AlignBottom(g_Inv_ItemText[1], true);
        Text_CentreH(g_Inv_ItemText[1], true);
    }
}

static void M_ShowAmmoQuantity(const char *const fmt, const int32_t qty)
{
    if (!g_SaveGame.bonus_flag) {
        M_ShowItemQuantity(fmt, qty);
    }
}

void __cdecl Inv_InitColors(void)
{
    g_InvColors[INV_COLOR_BLACK] = S_COLOR(0x00, 0x00, 0x00);
    g_InvColors[INV_COLOR_GRAY] = S_COLOR(0x40, 0x40, 0x40);
    g_InvColors[INV_COLOR_WHITE] = S_COLOR(0xFF, 0xFF, 0xFF);
    g_InvColors[INV_COLOR_RED] = S_COLOR(0xFF, 0x00, 0x00);
    g_InvColors[INV_COLOR_ORANGE] = S_COLOR(0xFF, 0x80, 0x00);
    g_InvColors[INV_COLOR_YELLOW] = S_COLOR(0xFF, 0xFF, 0x00);
    g_InvColors[INV_COLOR_DARK_GREEN] = S_COLOR(0x00, 0x80, 0x00);
    g_InvColors[INV_COLOR_GREEN] = S_COLOR(0x00, 0xFF, 0x00);
    g_InvColors[INV_COLOR_CYAN] = S_COLOR(0x00, 0xFF, 0xFF);
    g_InvColors[INV_COLOR_BLUE] = S_COLOR(0x00, 0x00, 0xFF);
    g_InvColors[INV_COLOR_MAGENTA] = S_COLOR(0xFF, 0x00, 0xFF);
}

void __cdecl Inv_Construct(void)
{
    S_SetupAboveWater(0);

    if (g_Inv_Mode != INV_TITLE_MODE) {
        TempVideoAdjust(g_HiRes, 1.0);
    }

    g_PhdWinRight = g_PhdWinMaxX;
    g_PhdWinLeft = 0;
    g_PhdWinTop = 0;
    g_PhdWinBottom = g_PhdWinMaxY;

    g_Inv_IsActive = 1;
    g_Inv_Chosen = 0;

    if (g_Inv_Mode == INV_TITLE_MODE) {
        g_Inv_OptionObjectsCount = TITLE_RING_OBJECTS;
        if (g_GameFlow.gym_enabled) {
            g_Inv_OptionObjectsCount++;
        }
        m_VersionText = Text_Create(-20, -18, 0, g_TR2XVersion);
        Text_AlignRight(m_VersionText, 1);
        Text_AlignBottom(m_VersionText, 1);
        Text_SetScale(m_VersionText, PHD_ONE * 0.5, PHD_ONE * 0.5);
    } else {
        g_Inv_OptionObjectsCount = OPTION_RING_OBJECTS;
        Text_Remove(m_VersionText);
        m_VersionText = NULL;
    }

    for (int32_t i = 0; i < 8; i++) {
        g_Inv_ExtraData[i] = 0;
    }

    for (int32_t i = 0; i < g_Inv_MainObjectsCount; i++) {
        INVENTORY_ITEM *const inv_item = g_Inv_MainList[i];
        inv_item->meshes_drawn = inv_item->meshes_sel;
        inv_item->current_frame = 0;
        inv_item->goal_frame = 0;
        inv_item->anim_count = 0;
        inv_item->x_rot_pt = 0;
        inv_item->x_rot = 0;
        inv_item->y_rot = 0;
        inv_item->y_trans = 0;
        inv_item->z_trans = 0;
        if (inv_item->object_id == O_PASSPORT_OPTION) {
            inv_item->object_id = O_PASSPORT_CLOSED;
        }
    }

    for (int32_t i = 0; i < g_Inv_OptionObjectsCount; i++) {
        INVENTORY_ITEM *const inv_item = g_Inv_OptionList[i];
        inv_item->current_frame = 0;
        inv_item->goal_frame = 0;
        inv_item->anim_count = 0;
        inv_item->x_rot_pt = 0;
        inv_item->x_rot = 0;
        inv_item->y_rot = 0;
        inv_item->y_trans = 0;
        inv_item->z_trans = 0;
        if (inv_item->object_id == O_PASSPORT_OPTION) {
            inv_item->object_id = O_PASSPORT_CLOSED;
        }
    }

    g_Inv_MainCurrent = 0;

    if (g_GymInvOpenEnabled && g_Inv_Mode == INV_TITLE_MODE
        && !g_GameFlow.load_save_disabled && g_GameFlow.gym_enabled) {
        g_Inv_OptionCurrent = 3; // TODO: don't hardcode me
        g_SoundOptionLine = 0;
    } else {
        g_Inv_OptionCurrent = 0;
        g_GymInvOpenEnabled = 0;
        g_SoundOptionLine = 0;
    }
}

int32_t __cdecl Inv_Display(int32_t inventory_mode)
{
    RING_INFO ring = { 0 };
    IMOTION_INFO imo = { 0 };

    bool demo_needed = false;
    bool pass_open = false;
    if (inventory_mode == INV_KEYS_MODE && !g_Inv_KeyObjectsCount) {
        g_Inv_Chosen = NO_OBJECT;
        return 0;
    }

    Overlay_HideGameInfo();

    Output_AlterFOV(80 * PHD_DEGREE);
    g_Inv_Mode = inventory_mode;
    g_Inv_NFrames = 2;

    Inv_Construct();
    if (inventory_mode == INV_TITLE_MODE) {
        S_FadeInInventory(0);
    } else {
        S_FadeInInventory(1);
    }

    Sound_StopAllSamples();
    if (inventory_mode != INV_TITLE_MODE) {
        Music_SetVolume(0);
    }

    switch (inventory_mode) {
    case INV_TITLE_MODE:
    case INV_SAVE_MODE:
    case INV_LOAD_MODE:
    case INV_DEATH_MODE:
        Inv_Ring_Init(
            &ring, 1, g_Inv_OptionList, g_Inv_OptionObjectsCount,
            g_Inv_OptionCurrent, &imo);
        break;

    case INV_KEYS_MODE:
        Inv_Ring_Init(
            &ring, 2, g_Inv_KeysList, g_Inv_KeyObjectsCount, g_Inv_MainCurrent,
            &imo);
        break;

    default:
        if (g_Inv_MainObjectsCount) {
            Inv_Ring_Init(
                &ring, 0, g_Inv_MainList, g_Inv_MainObjectsCount,
                g_Inv_MainCurrent, &imo);
        } else {
            Inv_Ring_Init(
                &ring, 1, g_Inv_OptionList, g_Inv_OptionObjectsCount,
                g_Inv_OptionCurrent, &imo);
        }
        break;
    }

    Sound_Effect(SFX_MENU_SPININ, 0, SPM_ALWAYS);
    g_Inv_NFrames = 2;

    do {
        if (g_GF_OverrideDir != (GAME_FLOW_DIR)-1) {
            INVENTORY_ITEM *inv_item = ring.list[ring.current_object];
            M_RemoveAllText();
            Option_ShutdownInventory(inv_item);
            return GFD_OVERRIDE;
        }

        if (inventory_mode == INV_TITLE_MODE && g_CD_TrackID > 0) {
            Music_Init();
        }
        Inv_Ring_CalcAdders(&ring, 24);
        Input_Update();

        if (g_Inv_DemoMode) {
            if (g_InputDB != 0) {
                return g_GameFlow.on_demo_interrupt;
            }
            Demo_GetInput();
            if (g_Input == -1) {
                return g_GameFlow.on_demo_end;
            }
        } else if (g_InputDB != 0) {
            g_NoInputCounter = 0;
        }

        if (g_Inv_Mode != INV_TITLE_MODE || g_Input != 0 || g_InputDB != 0) {
            g_NoInputCounter = 0;
        } else if (g_GameFlow.num_demos || g_GameFlow.no_input_timeout) {
            g_NoInputCounter++;
            if (g_NoInputCounter > g_GameFlow.no_input_time) {
                demo_needed = true;
            }
        }

        if (g_StopInventory) {
            return GFD_EXIT_TO_TITLE;
        }

        if ((g_Inv_Mode == INV_SAVE_MODE || g_Inv_Mode == INV_LOAD_MODE
             || g_Inv_Mode == INV_DEATH_MODE)
            && !pass_open) {
            g_InputDB = IN_SELECT;
            g_Input = 0;
        }

        for (int32_t frame = 0; frame < g_Inv_NFrames; frame++) {
            if (g_Inv_IsOptionsDelay) {
                if (g_Inv_OptionsDelayCounter) {
                    g_Inv_OptionsDelayCounter--;
                } else {
                    g_Inv_IsOptionsDelay = 0;
                }
            }
            Inv_Ring_DoMotions(&ring);
        }
        ring.camera.pos.z = ring.radius + 598;

        S_InitialisePolyList(0);
        if (g_Inv_Mode == INV_TITLE_MODE) {
            Inv_DoInventoryPicture();
        } else {
            Inv_DoInventoryBackground();
        }
        S_AnimateTextures(g_Inv_NFrames);

        PHD_3DPOS view;
        Inv_Ring_GetView(&ring, &view);
        Matrix_GenerateW2V(&view);
        Inv_Ring_Light(&ring);

        Matrix_Push();
        Matrix_TranslateAbs(
            ring.ring_pos.pos.x, ring.ring_pos.pos.y, ring.ring_pos.pos.z);
        Matrix_RotYXZ(
            ring.ring_pos.rot.y, ring.ring_pos.rot.x, ring.ring_pos.rot.z);

        int32_t angle = 0;
        for (int32_t j = 0; j < ring.number_of_objects; j++) {
            INVENTORY_ITEM *inv_item = ring.list[j];
            if (j != ring.current_object) {
                g_LsAdder = LOW_LIGHT;
                for (int32_t k = 0; k < g_Inv_NFrames; k++) {
                    if (inv_item->y_rot < 0) {
                        inv_item->y_rot += 256;
                    } else if (inv_item->y_rot > 0) {
                        inv_item->y_rot -= 256;
                    }
                }
            } else {
                for (int32_t k = 0; k < g_Inv_NFrames; k++) {
                    if (ring.rotating) {
                        g_LsAdder = LOW_LIGHT;
                        if (inv_item->y_rot > 0) {
                            inv_item->y_rot -= 512;
                        } else if (inv_item->y_rot < 0) {
                            inv_item->y_rot += 512;
                        }
                    } else if (
                        imo.status == RNG_SELECTED
                        || imo.status == RNG_DESELECTING
                        || imo.status == RNG_SELECTING
                        || imo.status == RNG_DESELECT
                        || imo.status == RNG_CLOSING_ITEM) {
                        g_LsAdder = HIGH_LIGHT;
                        const int32_t delta =
                            inv_item->y_rot_sel - inv_item->y_rot;
                        if (delta != 0) {
                            if (delta > 0 && delta < PHD_180) {
                                inv_item->y_rot += 1024;
                            } else {
                                inv_item->y_rot -= 1024;
                            }
                            inv_item->y_rot &= ~(1024 - 1);
                        }
                    } else if (
                        ring.number_of_objects == 1
                        || (!(g_Input & IN_RIGHT) && !(g_Input & IN_LEFT))) {
                        g_LsAdder = HIGH_LIGHT;
                        inv_item->y_rot += 256;
                    }
                }

                if ((imo.status == RNG_OPEN || imo.status == RNG_SELECTING
                     || imo.status == RNG_SELECTED
                     || imo.status == RNG_DESELECTING
                     || imo.status == RNG_DESELECT
                     || imo.status == RNG_CLOSING_ITEM)
                    && !ring.rotating && !(g_Input & IN_LEFT)
                    && !(g_Input & IN_RIGHT)) {
                    Inv_RingNotActive(inv_item);
                }
            }

            if (imo.status == RNG_OPEN || imo.status == RNG_SELECTING
                || imo.status == RNG_SELECTED || imo.status == RNG_DESELECTING
                || imo.status == RNG_DESELECT
                || imo.status == RNG_CLOSING_ITEM) {
                Inv_RingIsOpen(&ring);
            } else {
                Inv_RingIsNotOpen(&ring);
            }

            if (imo.status == RNG_OPENING || imo.status == RNG_CLOSING
                || imo.status == RNG_MAIN2OPTION
                || imo.status == RNG_OPTION2MAIN
                || imo.status == RNG_EXITING_INVENTORY || imo.status == RNG_DONE
                || ring.rotating) {
                Inv_RingActive();
            }

            Matrix_Push();
            Matrix_RotYXZ(angle, 0, 0);
            Matrix_TranslateRel(ring.radius, 0, 0);
            Matrix_RotYXZ(PHD_90, inv_item->x_rot_pt, 0);
            Inv_DrawInventoryItem(inv_item);
            Matrix_Pop();
            angle += ring.angle_adder;
        }

        Matrix_Pop();
        Overlay_DrawModeInfo();
        Text_Draw();
        S_OutputPolyList();
        Sound_EndScene();

        const int32_t frames = S_DumpScreen();
        g_Inv_NFrames = frames;
        g_Camera.num_frames = frames;
        if (g_CurrentLevel) {
            g_SaveGame.statistics.timer += frames / 2;
        }

        if (!ring.rotating) {
            switch (imo.status) {
            case RNG_OPEN:
                if ((g_Input & IN_RIGHT) != 0 && ring.number_of_objects > 1) {
                    Inv_Ring_RotateLeft(&ring);
                    Sound_Effect(SFX_MENU_ROTATE, 0, SPM_ALWAYS);
                    break;
                }

                if ((g_Input & IN_LEFT) != 0 && ring.number_of_objects > 1) {
                    Inv_Ring_RotateRight(&ring);
                    Sound_Effect(SFX_MENU_ROTATE, 0, SPM_ALWAYS);
                    break;
                }

                if (demo_needed
                    || (((g_InputDB & IN_OPTION) || (g_InputDB & IN_DESELECT))
                        && g_Inv_Mode != INV_TITLE_MODE)) {
                    Sound_Effect(SFX_MENU_SPINOUT, 0, SPM_ALWAYS);
                    g_Inv_Chosen = NO_OBJECT;
                    if (ring.type != RT_MAIN) {
                        g_Inv_OptionCurrent = ring.current_object;
                    } else {
                        g_Inv_MainCurrent = ring.current_object;
                    }
                    if (inventory_mode == INV_TITLE_MODE) {
                        S_FadeOutInventory(0);
                    } else {
                        S_FadeOutInventory(1);
                    }
                    Inv_Ring_MotionSetup(&ring, RNG_CLOSING, RNG_DONE, 32);
                    Inv_Ring_MotionRadius(&ring, 0);
                    Inv_Ring_MotionCameraPos(&ring, -1536);
                    Inv_Ring_MotionRotation(
                        &ring, PHD_180, ring.ring_pos.rot.y + PHD_180);
                    g_Input = 0;
                    g_InputDB = 0;
                }

                if ((g_InputDB & IN_SELECT) != 0) {
                    if ((g_Inv_Mode == INV_SAVE_MODE
                         || g_Inv_Mode == INV_LOAD_MODE
                         || g_Inv_Mode == INV_DEATH_MODE)
                        && !pass_open) {
                        pass_open = true;
                    }

                    g_SoundOptionLine = 0;
                    INVENTORY_ITEM *inv_item;
                    if (ring.type == RT_MAIN) {
                        g_Inv_MainCurrent = ring.current_object;
                        inv_item = g_Inv_MainList[ring.current_object];
                    } else if (ring.type == RT_OPTION) {
                        g_Inv_OptionCurrent = ring.current_object;
                        inv_item = g_Inv_OptionList[ring.current_object];
                    } else {
                        g_Inv_KeysCurrent = ring.current_object;
                        inv_item = g_Inv_KeysList[ring.current_object];
                    }

                    inv_item->goal_frame = inv_item->open_frame;
                    inv_item->anim_direction = 1;
                    Inv_Ring_MotionSetup(
                        &ring, RNG_SELECTING, RNG_SELECTED, 16);
                    Inv_Ring_MotionRotation(
                        &ring, 0,
                        -16384 - ring.angle_adder * ring.current_object);
                    Inv_Ring_MotionItemSelect(&ring, inv_item);
                    g_Input = 0;
                    g_InputDB = 0;

                    switch (inv_item->object_id) {
                    case O_COMPASS_OPTION:
                        Sound_Effect(SFX_MENU_STOPWATCH, 0, SPM_ALWAYS);
                        break;

                    case O_PHOTO_OPTION:
                        Sound_Effect(SFX_MENU_LARA_HOME, 0, SPM_ALWAYS);
                        break;

                    case O_PISTOL_OPTION:
                    case O_SHOTGUN_OPTION:
                    case O_MAGNUM_OPTION:
                    case O_UZI_OPTION:
                    case O_HARPOON_OPTION:
                    case O_M16_OPTION:
                    case O_GRENADE_OPTION:
                        Sound_Effect(SFX_MENU_GUNS, 0, SPM_ALWAYS);
                        break;

                    default:
                        Sound_Effect(SFX_MENU_SPININ, 0, SPM_ALWAYS);
                        break;
                    }
                }

                if ((g_InputDB & IN_FORWARD) != 0
                    && inventory_mode != INV_TITLE_MODE
                    && inventory_mode != INV_KEYS_MODE) {
                    if (ring.type == RT_OPTION) {
                        if (g_Inv_MainObjectsCount > 0) {
                            Inv_Ring_MotionSetup(
                                &ring, RNG_CLOSING, RNG_OPTION2MAIN, 24);
                            Inv_Ring_MotionRadius(&ring, 0);
                            Inv_Ring_MotionRotation(
                                &ring, PHD_180, ring.ring_pos.rot.y + PHD_180);
                            Inv_Ring_MotionCameraPitch(&ring, 0x2000);
                            imo.misc = 0x2000;
                        }
                        g_InputDB = 0;
                    } else if (ring.type == RT_MAIN) {
                        if (g_Inv_KeyObjectsCount > 0) {
                            Inv_Ring_MotionSetup(
                                &ring, RNG_CLOSING, RNG_MAIN2KEYS, 24);
                            Inv_Ring_MotionRadius(&ring, 0);
                            Inv_Ring_MotionRotation(
                                &ring, PHD_180, ring.ring_pos.rot.y + PHD_180);
                            Inv_Ring_MotionCameraPitch(&ring, 0x2000);
                            imo.misc = 0x2000;
                        }
                        g_Input = 0;
                        g_InputDB = 0;
                    }
                } else if (
                    (g_InputDB & IN_BACK) != 0
                    && inventory_mode != INV_TITLE_MODE
                    && inventory_mode != INV_KEYS_MODE) {
                    if (ring.type == RT_KEYS) {
                        if (g_Inv_MainObjectsCount > 0) {
                            Inv_Ring_MotionSetup(
                                &ring, RNG_CLOSING, RNG_KEYS2MAIN, 24);
                            Inv_Ring_MotionRadius(&ring, 0);
                            Inv_Ring_MotionRotation(
                                &ring, PHD_180, ring.ring_pos.rot.y + PHD_180);
                            Inv_Ring_MotionCameraPitch(&ring, -0x2000);
                            imo.misc = -0x2000;
                        }
                        g_Input = 0;
                        g_InputDB = 0;
                    } else if (ring.type == RT_MAIN) {
                        if (g_Inv_OptionObjectsCount > 0
                            && !g_GameFlow.lockout_option_ring) {
                            Inv_Ring_MotionSetup(
                                &ring, RNG_CLOSING, RNG_MAIN2OPTION, 24);
                            Inv_Ring_MotionRadius(&ring, 0);
                            Inv_Ring_MotionRotation(
                                &ring, PHD_180, ring.ring_pos.rot.y + PHD_180);
                            Inv_Ring_MotionCameraPitch(&ring, -0x2000);
                            imo.misc = -0x2000;
                        }
                        g_InputDB = 0;
                    }
                }
                break;

            case RNG_MAIN2OPTION:
                Inv_Ring_MotionSetup(&ring, RNG_OPENING, RNG_OPEN, 24);
                Inv_Ring_MotionRadius(&ring, 688);
                ring.camera_pitch = -(int16_t)(imo.misc);
                imo.camera_pitch_rate = imo.misc / 24;
                imo.camera_pitch_target = 0;
                ring.list = g_Inv_OptionList;
                ring.type = RT_OPTION;
                g_Inv_MainCurrent = ring.current_object;
                g_Inv_MainObjectsCount = ring.number_of_objects;
                ring.number_of_objects = g_Inv_OptionObjectsCount;
                ring.current_object = g_Inv_OptionCurrent;
                Inv_Ring_CalcAdders(&ring, 24);
                Inv_Ring_MotionRotation(
                    &ring, PHD_180,
                    -16384 - ring.angle_adder * ring.current_object);
                ring.ring_pos.rot.y = imo.rotate_target + PHD_180;
                break;

            case RNG_MAIN2KEYS:
                Inv_Ring_MotionSetup(&ring, RNG_OPENING, RNG_OPEN, 24);
                Inv_Ring_MotionRadius(&ring, 688);
                imo.camera_pitch_target = 0;
                ring.camera_pitch = -(int16_t)(imo.misc);
                imo.camera_pitch_rate = imo.misc / 24;
                g_Inv_MainCurrent = ring.current_object;
                g_Inv_MainObjectsCount = ring.number_of_objects;
                ring.list = g_Inv_KeysList;
                ring.type = RT_KEYS;
                ring.number_of_objects = g_Inv_KeyObjectsCount;
                ring.current_object = g_Inv_KeysCurrent;
                Inv_Ring_CalcAdders(&ring, 24);
                Inv_Ring_MotionRotation(
                    &ring, PHD_180,
                    -16384 - ring.angle_adder * ring.current_object);
                ring.ring_pos.rot.y = imo.rotate_target + PHD_180;
                break;

            case RNG_KEYS2MAIN:
                Inv_Ring_MotionSetup(&ring, RNG_OPENING, RNG_OPEN, 24);
                Inv_Ring_MotionRadius(&ring, 688);
                ring.camera_pitch = -(int16_t)(imo.misc);
                imo.camera_pitch_rate = imo.misc / 24;
                imo.camera_pitch_target = 0;
                ring.list = g_Inv_MainList;
                ring.type = RT_MAIN;
                g_Inv_KeysCurrent = ring.current_object;
                ring.number_of_objects = g_Inv_MainObjectsCount;
                ring.current_object = g_Inv_MainCurrent;
                Inv_Ring_CalcAdders(&ring, 24);
                Inv_Ring_MotionRotation(
                    &ring, PHD_180,
                    -16384 - ring.angle_adder * ring.current_object);
                ring.ring_pos.rot.y = imo.rotate_target + PHD_180;
                break;

            case RNG_OPTION2MAIN:
                Inv_Ring_MotionSetup(&ring, RNG_OPENING, RNG_OPEN, 24);
                Inv_Ring_MotionRadius(&ring, 688);
                ring.camera_pitch = -(int16_t)(imo.misc);
                imo.camera_pitch_rate = imo.misc / 24;
                g_Inv_OptionCurrent = ring.current_object;
                g_Inv_OptionObjectsCount = ring.number_of_objects;
                imo.camera_pitch_target = 0;
                ring.list = g_Inv_MainList;
                ring.type = RT_MAIN;
                ring.number_of_objects = g_Inv_MainObjectsCount;
                ring.current_object = g_Inv_MainCurrent;
                Inv_Ring_CalcAdders(&ring, 24);
                Inv_Ring_MotionRotation(
                    &ring, PHD_180,
                    -16384 - ring.angle_adder * ring.current_object);
                ring.ring_pos.rot.y = imo.rotate_target + PHD_180;
                break;

            case RNG_SELECTED: {
                INVENTORY_ITEM *inv_item = ring.list[ring.current_object];
                if (inv_item->object_id == O_PASSPORT_CLOSED) {
                    inv_item->object_id = O_PASSPORT_OPTION;
                }

                bool busy = false;
                for (int32_t frame = 0; frame < g_Inv_NFrames; frame++) {
                    busy = false;
                    if (inv_item->y_rot == inv_item->y_rot_sel) {
                        busy = Inv_AnimateInventoryItem(inv_item);
                    }
                }

                if (!busy && !g_Inv_IsOptionsDelay) {
                    Option_DoInventory(inv_item);

                    if (g_InputDB & IN_DESELECT) {
                        inv_item->sprite_list = NULL;
                        Inv_Ring_MotionSetup(
                            &ring, RNG_CLOSING_ITEM, RNG_DESELECT, 0);
                        g_Input = 0;
                        g_InputDB = 0;
                        if (g_Inv_Mode == INV_LOAD_MODE
                            || g_Inv_Mode == INV_SAVE_MODE) {
                            Inv_Ring_MotionSetup(
                                &ring, RNG_CLOSING_ITEM, RNG_EXITING_INVENTORY,
                                0);
                            g_InputDB = 0;
                            g_Input = 0;
                        }
                    }

                    if ((g_InputDB & IN_SELECT) != 0) {
                        inv_item->sprite_list = NULL;
                        g_Inv_Chosen = inv_item->object_id;
                        if (ring.type != RT_MAIN) {
                            g_Inv_OptionCurrent = ring.current_object;
                        } else {
                            g_Inv_MainCurrent = ring.current_object;
                        }
                        if (g_Inv_Mode == INV_TITLE_MODE
                            && (inv_item->object_id == O_DETAIL_OPTION
                                || inv_item->object_id == O_SOUND_OPTION
                                || inv_item->object_id == O_CONTROL_OPTION
                                || inv_item->object_id == O_GAMMA_OPTION)) {
                            Inv_Ring_MotionSetup(
                                &ring, RNG_CLOSING_ITEM, RNG_DESELECT, 0);
                        } else {
                            Inv_Ring_MotionSetup(
                                &ring, RNG_CLOSING_ITEM, RNG_EXITING_INVENTORY,
                                0);
                        }
                        g_Input = 0;
                        g_InputDB = 0;
                    }
                }
                break;
            }

            case RNG_DESELECT:
                Sound_Effect(SFX_MENU_SPINOUT, 0, SPM_ALWAYS);
                Inv_Ring_MotionSetup(&ring, RNG_DESELECTING, RNG_OPEN, 16);
                Inv_Ring_MotionRotation(
                    &ring, 0, -16384 - ring.angle_adder * ring.current_object);
                g_Input = 0;
                g_InputDB = 0;
                break;

            case RNG_CLOSING_ITEM: {
                INVENTORY_ITEM *inv_item = ring.list[ring.current_object];
                for (int32_t frame = 0; frame < g_Inv_NFrames; frame++) {
                    if (!Inv_AnimateInventoryItem(inv_item)) {
                        if (inv_item->object_id == O_PASSPORT_OPTION) {
                            inv_item->object_id = O_PASSPORT_CLOSED;
                            inv_item->current_frame = 0;
                        }

                        imo.count = 16;
                        imo.status = imo.status_target;
                        Inv_Ring_MotionItemDeselect(&ring, inv_item);
                        break;
                    }
                }
                break;
            }

            case RNG_EXITING_INVENTORY:
                if (!imo.count) {
                    if (inventory_mode == INV_TITLE_MODE) {
                        S_FadeOutInventory(0);
                    } else {
                        S_FadeOutInventory(1);
                    }
                    Inv_Ring_MotionSetup(&ring, RNG_CLOSING, RNG_DONE, 32);
                    Inv_Ring_MotionRadius(&ring, 0);
                    Inv_Ring_MotionCameraPos(&ring, -1536);
                    Inv_Ring_MotionRotation(
                        &ring, PHD_180, ring.ring_pos.rot.y + PHD_180);
                }
                break;

            default:
                break;
            }
        }
    } while (imo.status != RNG_DONE);

    M_RemoveAllText();
    S_FinishInventory();
    g_Inv_IsActive = 0;

    // enable buffering
    g_OldInputDB = 0;

    if (demo_needed) {
        return GFD_START_DEMO;
    }

    if (g_Inv_Chosen == NO_OBJECT) {
        if (inventory_mode != INV_TITLE_MODE && g_OptionMusicVolume != 0) {
            Music_SetVolume(25 * g_OptionMusicVolume + 5);
        }
        return 0;
    }

    switch (g_Inv_Chosen) {
    case O_PASSPORT_OPTION:
        if (g_Inv_ExtraData[0] == 1 && g_OptionMusicVolume != 0) {
            Music_SetVolume(25 * g_OptionMusicVolume + 5);
        }
        return 1;

    case O_PHOTO_OPTION:
        if (g_GameFlow.gym_enabled) {
            g_Inv_ExtraData[1] = 0;
            return 1;
        }
        break;

    case O_PISTOL_OPTION:
    case O_SHOTGUN_OPTION:
    case O_MAGNUM_OPTION:
    case O_UZI_OPTION:
    case O_HARPOON_OPTION:
    case O_M16_OPTION:
    case O_GRENADE_OPTION:
    case O_SMALL_MEDIPACK_OPTION:
    case O_LARGE_MEDIPACK_OPTION:
    case O_FLARES_OPTION:
        Lara_UseItem(g_Inv_Chosen);
        break;

    default:
        break;
    }

    if (inventory_mode != INV_TITLE_MODE && g_OptionMusicVolume != 0) {
        Music_SetVolume(25 * g_OptionMusicVolume + 5);
    }
    return 0;
}

void __cdecl Inv_SelectMeshes(INVENTORY_ITEM *const inv_item)
{
    switch (inv_item->object_id) {
    case O_PASSPORT_OPTION:
        if (inv_item->current_frame < 4) {
            inv_item->meshes_drawn = PM_COMMON | PM_IN_FRONT;
        } else if (inv_item->current_frame <= 16) {
            inv_item->meshes_drawn = PM_COMMON | PM_IN_FRONT | PM_PAGE_1;
        } else if (inv_item->current_frame < 19) {
            inv_item->meshes_drawn =
                PM_COMMON | PM_IN_FRONT | PM_PAGE_1 | PM_PAGE_2;
        } else if (inv_item->current_frame == 19) {
            inv_item->meshes_drawn = PM_COMMON | PM_PAGE_1 | PM_PAGE_2;
        } else if (inv_item->current_frame < 24) {
            inv_item->meshes_drawn =
                PM_COMMON | PM_IN_BACK | PM_PAGE_1 | PM_PAGE_2;
        } else if (inv_item->current_frame < 29) {
            inv_item->meshes_drawn = PM_COMMON | PM_IN_BACK | PM_PAGE_2;
        } else if (inv_item->current_frame == 29) {
            inv_item->meshes_drawn = PM_COMMON;
        }
        break;

    case O_COMPASS_OPTION:
        if (inv_item->current_frame == 0 || inv_item->current_frame >= 18) {
            inv_item->meshes_drawn = inv_item->meshes_sel;
        } else {
            inv_item->meshes_drawn = -1;
        }
        break;

    default:
        inv_item->meshes_drawn = -1;
        break;
    }
}

int32_t __cdecl Inv_AnimateInventoryItem(INVENTORY_ITEM *const inv_item)
{
    if (inv_item->current_frame == inv_item->goal_frame) {
        Inv_SelectMeshes(inv_item);
        return false;
    }

    if (inv_item->anim_count > 0) {
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

    Inv_SelectMeshes(inv_item);
    return true;
}

void __cdecl Inv_DrawInventoryItem(INVENTORY_ITEM *const inv_item)
{
    int32_t minutes;
    int32_t hours;
    int32_t seconds;
    if (inv_item->object_id == O_COMPASS_OPTION) {
        const int32_t total_seconds =
            g_SaveGame.statistics.timer / FRAMES_PER_SECOND;
        hours = (total_seconds % 43200) * PHD_DEGREE * -360 / 43200;
        minutes = (total_seconds % 3600) * PHD_DEGREE * -360 / 3600;
        seconds = (total_seconds % 60) * PHD_DEGREE * -360 / 60;
    } else {
        seconds = 0;
        minutes = 0;
        hours = 0;
    }

    Matrix_TranslateRel(0, inv_item->y_trans, inv_item->z_trans);
    Matrix_RotYXZ(inv_item->y_rot, inv_item->x_rot, 0);
    const OBJECT *const obj = &g_Objects[inv_item->object_id];
    if ((obj->flags & 1) == 0) {
        return;
    }

    if (obj->mesh_count < 0) {
        Output_DrawSprite(0, 0, 0, 0, obj->mesh_idx, 0, 0);
        return;
    }

    if (inv_item->sprite_list != NULL) {
        const int32_t zv = g_MatrixPtr->_23;
        const int32_t zp = zv / g_PhdPersp;
        const int32_t sx = g_PhdWinCenterX + g_MatrixPtr->_03 / zp;
        const int32_t sy = g_PhdWinCenterY + g_MatrixPtr->_13 / zp;

        INVENTORY_SPRITE **sprite_list = inv_item->sprite_list;
        INVENTORY_SPRITE *sprite;
        while ((sprite = *sprite_list++)) {
            if (zv < g_PhdNearZ || zv > g_PhdFarZ) {
                break;
            }

            while (sprite->shape) {
                switch (sprite->shape) {
                case SHAPE_SPRITE:
                    Output_DrawScreenSprite(
                        sx + sprite->pos.x, sy + sprite->pos.y, sprite->pos.z,
                        sprite->param1, sprite->param2,
                        g_Objects[O_ALPHABET].mesh_idx + sprite->sprite_num,
                        4096, 0);
                    break;

                case SHAPE_LINE:
                    S_DrawScreenLine(
                        sx + sprite->pos.x, sy + sprite->pos.y, sprite->pos.z,
                        sprite->param1, sprite->param2, sprite->sprite_num,
                        sprite->grdptr, 0);
                    break;

                case SHAPE_BOX:
                    S_DrawScreenBox(
                        sx + sprite->pos.x, sy + sprite->pos.y, sprite->pos.z,
                        sprite->param1, sprite->param2, sprite->sprite_num,
                        sprite->grdptr, 0);
                    break;

                case SHAPE_FBOX:
                    S_DrawScreenFBox(
                        sx + sprite->pos.x, sy + sprite->pos.y, sprite->pos.z,
                        sprite->param1, sprite->param2, sprite->sprite_num,
                        sprite->grdptr, 0);
                    break;

                default:
                    break;
                }
                sprite++;
            }
        }
    }

    FRAME_INFO *frame_ptr = (FRAME_INFO *)&obj->frame_base
                                [inv_item->current_frame
                                 * (g_Anims[obj->anim_idx].interpolation >> 8)];

    Matrix_Push();
    const int32_t clip = S_GetObjectBounds(&frame_ptr->bounds);
    if (!clip) {
        Matrix_Pop();
        return;
    }

    const int32_t *bone = &g_AnimBones[obj->bone_idx];
    Matrix_TranslateRel(
        frame_ptr->offset.x, frame_ptr->offset.y, frame_ptr->offset.z);
    const int16_t *rot = frame_ptr->mesh_rots;
    Matrix_RotYXZsuperpack(&rot, 0);

    for (int32_t mesh_idx = 0; mesh_idx < obj->mesh_count; mesh_idx++) {
        if (mesh_idx > 0) {
            const int32_t bone_flags = bone[0];
            if (bone_flags & BF_MATRIX_POP) {
                Matrix_Pop();
            }
            if (bone_flags & BF_MATRIX_PUSH) {
                Matrix_Push();
            }

            Matrix_TranslateRel(bone[1], bone[2], bone[3]);
            Matrix_RotYXZsuperpack(&rot, 0);
            bone += 4;

            if (inv_item->object_id == O_COMPASS_OPTION) {
                if (mesh_idx == 6) {
                    Matrix_RotZ(seconds);
                    const int32_t tmp = inv_item->reserved[0];
                    inv_item->reserved[0] = seconds;
                    inv_item->reserved[1] = tmp;
                }
                if (mesh_idx == 5) {
                    Matrix_RotZ(minutes);
                }
                if (mesh_idx == 4) {
                    Matrix_RotZ(hours);
                }
            }
        }

        if (inv_item->meshes_drawn & (1 << mesh_idx)) {
            Output_InsertPolygons(g_Meshes[obj->mesh_idx + mesh_idx], clip);
        }
    }

    Matrix_Pop();
}

GAME_OBJECT_ID Inv_GetItemOption(const GAME_OBJECT_ID object_id)
{
    if (Object_IsObjectType(object_id, g_InvObjects)) {
        return object_id;
    }

    return Object_GetCognate(object_id, g_ItemToInvObjectMap);
}

void __cdecl Inv_DoInventoryPicture(void)
{
    S_CopyBufferToScreen();
}

void __cdecl Inv_DoInventoryBackground(void)
{
    S_CopyBufferToScreen();
    if (!g_Objects[O_INV_BACKGROUND].loaded) {
        return;
    }

    int16_t angles[2];
    Math_GetVectorAngles(0, 4096, 0, angles);
    PHD_3DPOS view;
    view.pos.x = 0;
    view.pos.y = -512;
    view.pos.z = 0;
    view.rot.x = angles[1];
    view.rot.y = angles[0];
    view.rot.z = 0;
    Matrix_GenerateW2V(&view);

    g_LsDivider = 0x6000;
    Math_GetVectorAngles(-1536, 256, 1024, angles);
    Output_RotateLight(angles[1], angles[0]);

    Matrix_Push();
    Matrix_TranslateAbs(0, 12288, 0);
    Matrix_RotYXZ(0, PHD_90, PHD_180);

    const int16_t *rot =
        g_Anims[g_Objects[O_INV_BACKGROUND].anim_idx].frame_ptr + FBBOX_ROT;
    Matrix_RotYXZsuperpack(&rot, 0);
    Matrix_RotYXZ(PHD_180, 0, 0);

    Output_InsertInventoryBackground(
        g_Meshes[g_Objects[O_INV_BACKGROUND].mesh_idx]);

    Matrix_Pop();
}

void __cdecl Inv_RingIsOpen(RING_INFO *const ring)
{
    if (g_Inv_Mode == INV_TITLE_MODE) {
        return;
    }

    if (g_Inv_RingText == NULL) {
        switch (ring->type) {
        case RT_MAIN:
            g_Inv_RingText = Text_Create(
                0, 26, 0, g_GF_GameStrings[GF_S_GAME_HEADING_INVENTORY]);
            break;

        case RT_OPTION:
            if (g_Inv_Mode == INV_DEATH_MODE) {
                g_Inv_RingText = Text_Create(
                    0, 26, 0, g_GF_GameStrings[GF_S_GAME_HEADING_GAME_OVER]);
            } else {
                g_Inv_RingText = Text_Create(
                    0, 26, 0, g_GF_GameStrings[GF_S_GAME_HEADING_OPTION]);
            }
            Text_CentreH(g_Inv_RingText, true);
            break;

        case RT_KEYS:
            g_Inv_RingText = Text_Create(
                0, 26, 0, g_GF_GameStrings[GF_S_GAME_HEADING_ITEMS]);
            break;
        }

        Text_CentreH(g_Inv_RingText, true);
    }

    if (g_Inv_Mode == INV_KEYS_MODE || g_Inv_Mode == INV_DEATH_MODE) {
        return;
    }

    if (g_Inv_UpArrow1 == NULL) {
        if (ring->type == RT_OPTION
            || (ring->type == RT_MAIN && g_Inv_KeyObjectsCount > 0)) {
            g_Inv_UpArrow1 = Text_Create(20, 28, 0, "[");
            g_Inv_UpArrow2 = Text_Create(-20, 28, 0, "[");
            Text_AlignRight(g_Inv_UpArrow2, true);
        }
    }

    if (g_Inv_DownArrow1 == NULL
        && ((
            (ring->type == RT_MAIN && !g_GameFlow.lockout_option_ring)
            || ring->type == RT_KEYS))) {
        g_Inv_DownArrow1 = Text_Create(20, -15, 0, "]");
        Text_AlignBottom(g_Inv_DownArrow1, true);
        g_Inv_DownArrow2 = Text_Create(-20, -15, 0, "]");
        Text_AlignBottom(g_Inv_DownArrow2, true);
        Text_AlignRight(g_Inv_DownArrow2, true);
    }
}

void __cdecl Inv_RingIsNotOpen(RING_INFO *const ring)
{
    Text_Remove(g_Inv_TagText);
    g_Inv_TagText = NULL;
    Text_Remove(g_Inv_RingText);
    g_Inv_RingText = NULL;
    Text_Remove(g_Inv_UpArrow1);
    g_Inv_UpArrow1 = NULL;
    Text_Remove(g_Inv_UpArrow2);
    g_Inv_UpArrow2 = NULL;
    Text_Remove(g_Inv_DownArrow1);
    g_Inv_DownArrow1 = NULL;
    Text_Remove(g_Inv_DownArrow2);
    g_Inv_DownArrow2 = NULL;
}

void __cdecl Inv_RingNotActive(const INVENTORY_ITEM *const inv_item)
{
    if (g_Inv_ItemText[0] == NULL) {
        if (inv_item->object_id != O_PASSPORT_OPTION) {
            g_Inv_ItemText[0] =
                Text_Create(0, -16, 0, Object_GetName(inv_item->object_id));
        }

        if (g_Inv_ItemText[0]) {
            Text_AlignBottom(g_Inv_ItemText[0], true);
            Text_CentreH(g_Inv_ItemText[0], true);
        }
    }

    const int32_t qty = Inv_RequestItem(inv_item->object_id);
    switch (inv_item->object_id) {
    case O_SHOTGUN_OPTION:
        M_ShowAmmoQuantity("%5d", g_Lara.shotgun_ammo.ammo / SHOTGUN_AMMO_CLIP);
        break;
    case O_MAGNUM_OPTION:
        M_ShowAmmoQuantity("%5d", g_Lara.magnum_ammo.ammo);
        break;
    case O_UZI_OPTION:
        M_ShowAmmoQuantity("%5d", g_Lara.uzi_ammo.ammo);
        break;
    case O_HARPOON_OPTION:
        M_ShowAmmoQuantity("%5d", g_Lara.harpoon_ammo.ammo);
        break;
    case O_M16_OPTION:
        M_ShowAmmoQuantity("%5d", g_Lara.m16_ammo.ammo);
        break;
    case O_GRENADE_OPTION:
        M_ShowAmmoQuantity("%5d", g_Lara.grenade_ammo.ammo);
        break;
    case O_SHOTGUN_AMMO_OPTION:
        M_ShowAmmoQuantity("%d", SHOTGUN_SHELL_COUNT * qty);
        break;

    case O_MAGNUM_AMMO_OPTION:
    case O_UZI_AMMO_OPTION:
    case O_HARPOON_AMMO_OPTION:
    case O_M16_AMMO_OPTION:
        M_ShowAmmoQuantity("%d", 2 * qty);
        break;

    case O_GRENADE_AMMO_OPTION:
    case O_FLARES_OPTION:
        M_ShowAmmoQuantity("%d", qty);
        break;

    case O_SMALL_MEDIPACK_OPTION:
    case O_LARGE_MEDIPACK_OPTION:
        g_HealthBarTimer = 40;
        Overlay_DrawHealthBar(Overlay_FlashCounter());
        M_ShowItemQuantity("%d", qty);
        break;

    case O_PUZZLE_OPTION_1:
    case O_PUZZLE_OPTION_2:
    case O_PUZZLE_OPTION_3:
    case O_PUZZLE_OPTION_4:
    case O_KEY_OPTION_1:
    case O_KEY_OPTION_2:
    case O_KEY_OPTION_3:
    case O_KEY_OPTION_4:
    case O_PICKUP_OPTION_1:
    case O_PICKUP_OPTION_2:
        if (qty > 1) {
            M_ShowItemQuantity("%d", qty);
        }
        break;

    default:
        break;
    }
}

void __cdecl Inv_RingActive(void)
{
    M_RemoveItemsText();
}
