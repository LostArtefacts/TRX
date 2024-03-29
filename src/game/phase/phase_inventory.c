#include "game/phase/phase_inventory.h"

#include "config.h"
#include "game/clock.h"
#include "game/game.h"
#include "game/gameflow.h"
#include "game/input.h"
#include "game/inventory.h"
#include "game/inventory/inventory_ring.h"
#include "game/inventory/inventory_vars.h"
#include "game/lara.h"
#include "game/music.h"
#include "game/option.h"
#include "game/output.h"
#include "game/overlay.h"
#include "game/savegame.h"
#include "game/screen.h"
#include "game/shell.h"
#include "game/sound.h"
#include "game/text.h"
#include "game/viewport.h"
#include "global/const.h"
#include "global/types.h"
#include "global/vars.h"
#include "math/matrix.h"

#include <stdbool.h>
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

static bool m_PlayedSpinin;
static bool m_PassportModeReady;
static int32_t m_StartLevel;
static bool m_StartDemo;
static int32_t m_StartMS;
static TEXTSTRING *m_VersionText = NULL;
static int16_t m_CompassNeedle = 0;
static int16_t m_CompassSpeed = 0;
static CAMERA_INFO m_OldCamera;
RING_INFO m_Ring;
IMOTION_INFO m_Motion;

static void Inv_Draw(RING_INFO *ring, IMOTION_INFO *motion);
static void Inv_Construct(void);
static GAMEFLOW_OPTION Inv_Close(void);
static void Inv_SelectMeshes(INVENTORY_ITEM *inv_item);
static bool Inv_AnimateItem(INVENTORY_ITEM *inv_item);
static void Inv_DrawItem(INVENTORY_ITEM *inv_item);

static void Phase_Inventory_Start(void *arg);
static void Phase_Inventory_End(void);
static GAMEFLOW_OPTION Phase_Inventory_ControlFrame(void);
static GAMEFLOW_OPTION Phase_Inventory_Control(int32_t nframes);
static void Phase_Inventory_Draw(void);

static void Inv_Draw(RING_INFO *ring, IMOTION_INFO *motion)
{
    if (g_InvMode == INV_TITLE_MODE) {
        Output_DrawBackdropImage();
        Output_DrawBackdropScreen();
    } else {
        Matrix_LookAt(
            m_OldCamera.pos.x, m_OldCamera.pos.y + m_OldCamera.shift,
            m_OldCamera.pos.z, m_OldCamera.target.x, m_OldCamera.target.y,
            m_OldCamera.target.z, 0);
        Game_DrawScene(false);

        int32_t width = Screen_GetResWidth();
        int32_t height = Screen_GetResHeight();
        Viewport_Init(0, 0, width, height);
    }

    int16_t old_fov = Viewport_GetFOV();
    Viewport_SetFOV(PASSPORT_FOV * PHD_DEGREE);

    Output_SetupAboveWater(false);

    XYZ_32 view_pos;
    XYZ_16 view_rot;
    Inv_Ring_GetView(ring, &view_pos, &view_rot);
    Matrix_GenerateW2V(&view_pos, &view_rot);
    Inv_Ring_Light(ring);

    Matrix_Push();
    Matrix_TranslateAbs(
        ring->ringpos.pos.x, ring->ringpos.pos.y, ring->ringpos.pos.z);
    Matrix_RotYXZ(
        ring->ringpos.rot.y, ring->ringpos.rot.x, ring->ringpos.rot.z);

    if (!(g_InvMode == INV_TITLE_MODE && Output_FadeIsAnimating()
          && motion->status == RNG_OPENING)) {
        PHD_ANGLE angle = 0;
        for (int i = 0; i < ring->number_of_objects; i++) {
            INVENTORY_ITEM *inv_item = ring->list[i];
            Matrix_Push();
            Matrix_RotYXZ(angle, 0, 0);
            Matrix_TranslateRel(ring->radius, 0, 0);
            Matrix_RotYXZ(PHD_90, inv_item->pt_xrot, 0);
            Inv_DrawItem(inv_item);
            angle += ring->angle_adder;
            Matrix_Pop();
        }
    }

    INVENTORY_ITEM *inv_item = ring->list[ring->current_object];
    switch (inv_item->object_number) {
    case O_MEDI_OPTION:
    case O_BIGMEDI_OPTION:
        Overlay_BarDrawHealth();
        break;

    default:
        break;
    }

    Matrix_Pop();
    Viewport_SetFOV(old_fov);
}

static void Inv_Construct(void)
{
    g_PhdLeft = Viewport_GetMinX();
    g_PhdTop = Viewport_GetMinY();
    g_PhdBottom = Viewport_GetMaxY();
    g_PhdRight = Viewport_GetMaxX();

    g_InvChosen = 0;
    if (g_InvMode == INV_TITLE_MODE) {
        g_InvOptionObjects = TITLE_RING_OBJECTS;
        m_VersionText = Text_Create(-20, -18, g_TR1XVersion);
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

static GAMEFLOW_OPTION Inv_Close(void)
{
    // finish fading
    if (g_InvMode == INV_TITLE_MODE) {
        Output_FadeToBlack(true);
    }

    if (Output_FadeIsAnimating()) {
        return GF_PHASE_CONTINUE;
    }

    Inv_Ring_RemoveAlText();

    if (m_VersionText) {
        Text_Remove(m_VersionText);
        m_VersionText = NULL;
    }

    if (m_StartLevel != -1) {
        return GF_SELECT_GAME | m_StartLevel;
    }

    if (m_StartDemo) {
        return GF_START_DEMO;
    }

    switch (g_InvChosen) {
    case O_PASSPORT_OPTION:

        switch (g_GameInfo.passport_selection) {
        case PASSPORT_MODE_LOAD_GAME:
            return GF_START_SAVED_GAME | g_GameInfo.current_save_slot;
        case PASSPORT_MODE_SELECT_LEVEL:
            return GF_SELECT_GAME | g_GameInfo.select_level_num;
        case PASSPORT_MODE_STORY_SO_FAR:
            return GF_STORY_SO_FAR | g_GameInfo.current_save_slot;
        case PASSPORT_MODE_NEW_GAME:
            Savegame_InitCurrentInfo();
            return GF_START_GAME | g_GameFlow.first_level_num;
        case PASSPORT_MODE_SAVE_GAME:
            Savegame_Save(g_GameInfo.current_save_slot, &g_GameInfo);
            Config_Write();
            Music_Unpause();
            Sound_UnpauseAll();
            Phase_Set(PHASE_GAME, 0);
            return GF_PHASE_CONTINUE;
        case PASSPORT_MODE_RESTART:
            return GF_RESTART_GAME | g_CurrentLevel;
        case PASSPORT_MODE_EXIT_TITLE:
            return GF_EXIT_TO_TITLE;
        case PASSPORT_MODE_EXIT_GAME:
            return GF_EXIT_GAME;
        case PASSPORT_MODE_BROWSE:
        case PASSPORT_MODE_UNAVAILABLE:
        default:
            return GF_EXIT_TO_TITLE;
        }

    case O_PHOTO_OPTION:
        g_GameInfo.current_save_slot = -1;
        return GF_START_GYM | g_GameFlow.gym_level_num;

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

    if (g_InvMode == INV_TITLE_MODE) {
        return GF_PHASE_BREAK;
    } else {
        Music_Unpause();
        Sound_UnpauseAll();
        Phase_Set(PHASE_GAME, 0);
        return GF_PHASE_CONTINUE;
    }
}

static void Inv_SelectMeshes(INVENTORY_ITEM *inv_item)
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

static bool Inv_AnimateItem(INVENTORY_ITEM *inv_item)
{
    if (inv_item->current_frame == inv_item->goal_frame) {
        Inv_SelectMeshes(inv_item);
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
    Inv_SelectMeshes(inv_item);
    return true;
}

static void Inv_DrawItem(INVENTORY_ITEM *inv_item)
{
    const RING_INFO *ring = &m_Ring;
    const IMOTION_INFO *motion = &m_Motion;

    if (motion->status == RNG_DONE) {
        g_LsAdder = LOW_LIGHT;
    } else if (inv_item == ring->list[ring->current_object]) {
        for (int j = 0; j < Clock_GetFrameAdvance(); j++) {
            if (ring->rotating) {
                g_LsAdder = LOW_LIGHT;
                if (inv_item->y_rot < 0) {
                    inv_item->y_rot += 512;
                } else if (inv_item->y_rot > 0) {
                    inv_item->y_rot -= 512;
                }
            } else if (
                motion->status == RNG_SELECTED
                || motion->status == RNG_DESELECTING
                || motion->status == RNG_SELECTING
                || motion->status == RNG_DESELECT
                || motion->status == RNG_CLOSING_ITEM) {
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
                || (!g_Input.menu_left && !g_Input.menu_right)
                || !g_Input.menu_left) {
                g_LsAdder = HIGH_LIGHT;
                inv_item->y_rot += 256;
            }
        }
    } else {
        g_LsAdder = LOW_LIGHT;
        for (int j = 0; j < Clock_GetFrameAdvance(); j++) {
            if (inv_item->y_rot < 0) {
                inv_item->y_rot += 256;
            } else if (inv_item->y_rot > 0) {
                inv_item->y_rot -= 256;
            }
        }
    }

    Matrix_TranslateRel(0, inv_item->ytrans, inv_item->ztrans);
    Matrix_RotYXZ(inv_item->y_rot, inv_item->x_rot, 0);

    OBJECT_INFO *obj = &g_Objects[inv_item->object_number];
    if (obj->nmeshes < 0) {
        Output_DrawSpriteRel(0, 0, 0, obj->mesh_index, 4096);
        return;
    }

    if (inv_item->sprlist) {
        int32_t zv = g_MatrixPtr->_23;
        int32_t zp = zv / g_PhdPersp;
        int32_t sx = Viewport_GetCenterX() + g_MatrixPtr->_03 / zp;
        int32_t sy = Viewport_GetCenterY() + g_MatrixPtr->_13 / zp;

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
                case SHAPE_BOX: {
                    double scale = Viewport_GetHeight() / 480.0;
                    Output_DrawScreenBox(
                        sx + spr->x - scale, sy + spr->y - scale, spr->param1,
                        spr->param2, Text_GetMenuColor(MC_GOLD_DARK),
                        Text_GetMenuColor(MC_GOLD_LIGHT),
                        TEXT_OUTLINE_THICKNESS * scale);
                } break;
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

    Matrix_Push();

    int32_t clip = Output_GetObjectBounds(frame);
    if (clip) {
        Matrix_TranslateRel(
            frame[FRAME_POS_X], frame[FRAME_POS_Y], frame[FRAME_POS_Z]);
        int32_t *packed_rotation = (int32_t *)(frame + FRAME_ROT);
        Matrix_RotYXZpack(*packed_rotation++);

        int32_t mesh_num = 1;

        int32_t *bone = &g_AnimBones[obj->bone_index];
        if (inv_item->drawn_meshes & mesh_num) {
            Output_DrawPolygons(g_Meshes[obj->mesh_index], clip);
        }

        for (int i = 1; i < obj->nmeshes; i++) {
            mesh_num *= 2;

            int32_t bone_extra_flags = bone[0];
            if (bone_extra_flags & BEB_POP) {
                Matrix_Pop();
            }
            if (bone_extra_flags & BEB_PUSH) {
                Matrix_Push();
            }

            Matrix_TranslateRel(bone[1], bone[2], bone[3]);
            Matrix_RotYXZpack(*packed_rotation++);

            if (inv_item->object_number == O_MAP_OPTION && i == 1) {
                int16_t delta =
                    -inv_item->y_rot - g_LaraItem->rot.y - m_CompassNeedle;
                m_CompassSpeed = m_CompassSpeed * 19 / 20 + delta / 50;
                m_CompassNeedle += m_CompassSpeed;
                Matrix_RotY(m_CompassNeedle);
            }

            if (inv_item->drawn_meshes & mesh_num) {
                Output_DrawPolygons(g_Meshes[obj->mesh_index + i], clip);
            }

            bone += 4;
        }
    }
    Matrix_Pop();
}

static void Phase_Inventory_Start(void *arg)
{
    INV_MODE inv_mode = (INV_MODE)arg;

    RING_INFO *ring = &m_Ring;
    IMOTION_INFO *motion = &m_Motion;

    memset(motion, 0, sizeof(IMOTION_INFO));
    memset(ring, 0, sizeof(RING_INFO));

    g_InvMode = inv_mode;

    m_PassportModeReady = false;
    m_StartLevel = -1;
    m_StartDemo = false;
    Inv_Construct();

    if (!g_Config.enable_music_in_inventory && g_InvMode != INV_TITLE_MODE) {
        Music_Pause();
        Sound_PauseAll();
    } else {
        Sound_ResetAmbient();
        Sound_UpdateEffects();
    }

    switch (g_InvMode) {
    case INV_DEATH_MODE:
    case INV_SAVE_MODE:
    case INV_SAVE_CRYSTAL_MODE:
    case INV_LOAD_MODE:
    case INV_TITLE_MODE:
        Inv_Ring_Init(
            ring, RT_OPTION, g_InvOptionList, g_InvOptionObjects,
            g_InvOptionCurrent, motion);
        break;

    case INV_KEYS_MODE:
        Inv_Ring_Init(
            ring, RT_KEYS, g_InvKeysList, g_InvKeysObjects, g_InvMainCurrent,
            motion);
        break;

    default:
        if (g_InvMainObjects) {
            Inv_Ring_Init(
                ring, RT_MAIN, g_InvMainList, g_InvMainObjects,
                g_InvMainCurrent, motion);
        } else {
            Inv_Ring_Init(
                ring, RT_OPTION, g_InvOptionList, g_InvOptionObjects,
                g_InvOptionCurrent, motion);
        }
        break;
    }

    m_PlayedSpinin = false;
    m_OldCamera = g_Camera;

    if (g_InvMode == INV_TITLE_MODE) {
        Output_FadeResetToBlack();
        Output_FadeToTransparent(true);
    } else {
        Output_FadeToSemiBlack(true);
    }
}

static GAMEFLOW_OPTION Phase_Inventory_ControlFrame(void)
{
    RING_INFO *ring = &m_Ring;
    IMOTION_INFO *motion = &m_Motion;

    if (motion->status == RNG_OPENING) {
        if (g_InvMode == INV_TITLE_MODE && Output_FadeIsAnimating()) {
            return GF_PHASE_CONTINUE;
        }

        m_StartMS = Clock_GetMS();
        if (!m_PlayedSpinin) {
            Sound_Effect(SFX_MENU_SPININ, NULL, SPM_ALWAYS);
            m_PlayedSpinin = true;
        }
    }

    if (motion->status == RNG_DONE) {
        return Inv_Close();
    }

    Inv_Ring_CalcAdders(ring, ROTATE_DURATION);

    Input_Update();
    Shell_ProcessInput();
    Game_ProcessInput();

    if (g_InvMode != INV_TITLE_MODE || g_Input.any || g_InputDB.any) {
        m_StartMS = Clock_GetMS();
    } else if (g_Config.enable_demo && motion->status == RNG_OPEN) {
        if (g_GameFlow.has_demo
            && (Clock_GetMS() - m_StartMS) / 1000.0 > g_GameFlow.demo_delay) {
            m_StartDemo = true;
        }
    }

    m_StartLevel = g_LevelComplete ? g_GameInfo.select_level_num : -1;

    for (int i = 0; i < Clock_GetFrameAdvance(); i++) {
        if (g_IDelay) {
            if (g_IDCount) {
                g_IDCount--;
            } else {
                g_IDelay = false;
            }
        }
        Inv_Ring_DoMotions(ring);
    }

    ring->camera.pos.z = ring->radius + CAMERA_2_RING;

    g_GameInfo.inv_ring_above = g_InvMode == INV_GAME_MODE
        && ((ring->type == RT_MAIN && g_InvKeysObjects)
            || (ring->type == RT_OPTION && g_InvMainObjects));

    if (g_Config.enable_timer_in_inventory) {
        g_GameInfo.current[g_CurrentLevel].stats.timer +=
            Clock_IsAtLogicalFrame(1);
    }

    if (ring->rotating) {
        return GF_PHASE_CONTINUE;
    }

    if ((g_InvMode == INV_SAVE_MODE || g_InvMode == INV_SAVE_CRYSTAL_MODE
         || g_InvMode == INV_LOAD_MODE || g_InvMode == INV_DEATH_MODE)
        && !m_PassportModeReady) {
        g_Input = (INPUT_STATE) { 0 };
        g_InputDB = (INPUT_STATE) { 0, .menu_confirm = 1 };
    }

    switch (motion->status) {
    case RNG_OPEN:
        if (g_Input.menu_right && ring->number_of_objects > 1) {
            Inv_Ring_RotateLeft(ring);
            Sound_Effect(SFX_MENU_ROTATE, NULL, SPM_ALWAYS);
            break;
        }

        if (g_Input.menu_left && ring->number_of_objects > 1) {
            Inv_Ring_RotateRight(ring);
            Sound_Effect(SFX_MENU_ROTATE, NULL, SPM_ALWAYS);
            break;
        }

        if (m_StartLevel != -1 || m_StartDemo
            || (g_InputDB.menu_back && g_InvMode != INV_TITLE_MODE)) {
            Sound_Effect(SFX_MENU_SPINOUT, NULL, SPM_ALWAYS);
            g_InvChosen = -1;

            if (ring->type == RT_MAIN) {
                g_InvMainCurrent = ring->current_object;
            } else {
                g_InvOptionCurrent = ring->current_object;
            }

            if (g_InvMode != INV_TITLE_MODE) {
                Output_FadeToTransparent(false);
            }

            Inv_Ring_MotionSetup(ring, RNG_CLOSING, RNG_DONE, CLOSE_FRAMES);
            Inv_Ring_MotionRadius(ring, 0);
            Inv_Ring_MotionCameraPos(ring, CAMERA_STARTHEIGHT);
            Inv_Ring_MotionRotation(
                ring, CLOSE_ROTATION, ring->ringpos.rot.y - CLOSE_ROTATION);
            g_Input = (INPUT_STATE) { 0 };
            g_InputDB = (INPUT_STATE) { 0 };
        }

        if (g_InputDB.menu_confirm) {
            if ((g_InvMode == INV_SAVE_MODE
                 || g_InvMode == INV_SAVE_CRYSTAL_MODE
                 || g_InvMode == INV_LOAD_MODE || g_InvMode == INV_DEATH_MODE)
                && !m_PassportModeReady) {
                m_PassportModeReady = true;
            }

            g_OptionSelected = 0;

            INVENTORY_ITEM *inv_item;
            if (ring->type == RT_MAIN) {
                g_InvMainCurrent = ring->current_object;
                inv_item = g_InvMainList[ring->current_object];
            } else if (ring->type == RT_OPTION) {
                g_InvOptionCurrent = ring->current_object;
                inv_item = g_InvOptionList[ring->current_object];
            } else {
                g_InvKeysCurrent = ring->current_object;
                inv_item = g_InvKeysList[ring->current_object];
            }

            inv_item->goal_frame = inv_item->open_frame;
            inv_item->anim_direction = 1;

            Inv_Ring_MotionSetup(
                ring, RNG_SELECTING, RNG_SELECTED, SELECTING_FRAMES);
            Inv_Ring_MotionRotation(
                ring, 0, -PHD_90 - ring->angle_adder * ring->current_object);
            Inv_Ring_MotionItemSelect(ring, inv_item);
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

        if (g_InputDB.menu_up && g_InvMode != INV_TITLE_MODE
            && g_InvMode != INV_KEYS_MODE) {
            if (ring->type == RT_MAIN) {
                if (g_InvKeysObjects) {
                    Inv_Ring_MotionSetup(
                        ring, RNG_CLOSING, RNG_MAIN2KEYS,
                        RINGSWITCH_FRAMES / 2);
                    Inv_Ring_MotionRadius(ring, 0);
                    Inv_Ring_MotionRotation(
                        ring, CLOSE_ROTATION,
                        ring->ringpos.rot.y - CLOSE_ROTATION);
                    Inv_Ring_MotionCameraPitch(ring, 0x2000);
                    motion->misc = 0x2000;
                }
                g_Input = (INPUT_STATE) { 0 };
                g_InputDB = (INPUT_STATE) { 0 };
            } else if (ring->type == RT_OPTION) {
                if (g_InvMainObjects) {
                    Inv_Ring_MotionSetup(
                        ring, RNG_CLOSING, RNG_OPTION2MAIN,
                        RINGSWITCH_FRAMES / 2);
                    Inv_Ring_MotionRadius(ring, 0);
                    Inv_Ring_MotionRotation(
                        ring, CLOSE_ROTATION,
                        ring->ringpos.rot.y - CLOSE_ROTATION);
                    Inv_Ring_MotionCameraPitch(ring, 0x2000);
                    motion->misc = 0x2000;
                }
                g_InputDB = (INPUT_STATE) { 0 };
            }
        } else if (
            g_InputDB.menu_down && g_InvMode != INV_TITLE_MODE
            && g_InvMode != INV_KEYS_MODE) {
            if (ring->type == RT_KEYS) {
                if (g_InvMainObjects) {
                    Inv_Ring_MotionSetup(
                        ring, RNG_CLOSING, RNG_KEYS2MAIN,
                        RINGSWITCH_FRAMES / 2);
                    Inv_Ring_MotionRadius(ring, 0);
                    Inv_Ring_MotionRotation(
                        ring, CLOSE_ROTATION,
                        ring->ringpos.rot.y - CLOSE_ROTATION);
                    Inv_Ring_MotionCameraPitch(ring, -0x2000);
                    motion->misc = -0x2000;
                }
                g_Input = (INPUT_STATE) { 0 };
                g_InputDB = (INPUT_STATE) { 0 };
            } else if (ring->type == RT_MAIN) {
                if (g_InvOptionObjects) {
                    Inv_Ring_MotionSetup(
                        ring, RNG_CLOSING, RNG_MAIN2OPTION,
                        RINGSWITCH_FRAMES / 2);
                    Inv_Ring_MotionRadius(ring, 0);
                    Inv_Ring_MotionRotation(
                        ring, CLOSE_ROTATION,
                        ring->ringpos.rot.y - CLOSE_ROTATION);
                    Inv_Ring_MotionCameraPitch(ring, -0x2000);
                    motion->misc = -0x2000;
                }
                g_InputDB = (INPUT_STATE) { 0 };
            }
        }
        break;

    case RNG_MAIN2OPTION:
        Inv_Ring_MotionSetup(
            ring, RNG_OPENING, RNG_OPEN, RINGSWITCH_FRAMES / 2);
        Inv_Ring_MotionRadius(ring, RING_RADIUS);
        ring->camera_pitch = -motion->misc;
        motion->camera_pitch_rate = motion->misc / (RINGSWITCH_FRAMES / 2);
        motion->camera_pitch_target = 0;
        g_InvMainCurrent = ring->current_object;
        ring->list = g_InvOptionList;
        ring->type = RT_OPTION;
        ring->number_of_objects = g_InvOptionObjects;
        ring->current_object = g_InvOptionCurrent;
        Inv_Ring_CalcAdders(ring, ROTATE_DURATION);
        Inv_Ring_MotionRotation(
            ring, OPEN_ROTATION,
            -PHD_90 - ring->angle_adder * ring->current_object);
        ring->ringpos.rot.y = motion->rotate_target + OPEN_ROTATION;
        break;

    case RNG_MAIN2KEYS:
        Inv_Ring_MotionSetup(
            ring, RNG_OPENING, RNG_OPEN, RINGSWITCH_FRAMES / 2);
        Inv_Ring_MotionRadius(ring, RING_RADIUS);
        ring->camera_pitch = -motion->misc;
        motion->camera_pitch_rate = motion->misc / (RINGSWITCH_FRAMES / 2);
        motion->camera_pitch_target = 0;
        g_InvMainCurrent = ring->current_object;
        g_InvMainObjects = ring->number_of_objects;
        ring->list = g_InvKeysList;
        ring->type = RT_KEYS;
        ring->number_of_objects = g_InvKeysObjects;
        ring->current_object = g_InvKeysCurrent;
        Inv_Ring_CalcAdders(ring, ROTATE_DURATION);
        Inv_Ring_MotionRotation(
            ring, OPEN_ROTATION,
            -PHD_90 - ring->angle_adder * ring->current_object);
        ring->ringpos.rot.y = motion->rotate_target + OPEN_ROTATION;
        break;

    case RNG_KEYS2MAIN:
        Inv_Ring_MotionSetup(
            ring, RNG_OPENING, RNG_OPEN, RINGSWITCH_FRAMES / 2);
        Inv_Ring_MotionRadius(ring, RING_RADIUS);
        ring->camera_pitch = -motion->misc;
        motion->camera_pitch_rate = motion->misc / (RINGSWITCH_FRAMES / 2);
        motion->camera_pitch_target = 0;
        g_InvKeysCurrent = ring->current_object;
        ring->list = g_InvMainList;
        ring->type = RT_MAIN;
        ring->number_of_objects = g_InvMainObjects;
        ring->current_object = g_InvMainCurrent;
        Inv_Ring_CalcAdders(ring, ROTATE_DURATION);
        Inv_Ring_MotionRotation(
            ring, OPEN_ROTATION,
            -PHD_90 - ring->angle_adder * ring->current_object);
        ring->ringpos.rot.y = motion->rotate_target + OPEN_ROTATION;
        break;

    case RNG_OPTION2MAIN:
        Inv_Ring_MotionSetup(
            ring, RNG_OPENING, RNG_OPEN, RINGSWITCH_FRAMES / 2);
        Inv_Ring_MotionRadius(ring, RING_RADIUS);
        ring->camera_pitch = -motion->misc;
        motion->camera_pitch_rate = motion->misc / (RINGSWITCH_FRAMES / 2);
        motion->camera_pitch_target = 0;
        g_InvOptionObjects = ring->number_of_objects;
        g_InvOptionCurrent = ring->current_object;
        ring->list = g_InvMainList;
        ring->type = RT_MAIN;
        ring->number_of_objects = g_InvMainObjects;
        ring->current_object = g_InvMainCurrent;
        Inv_Ring_CalcAdders(ring, ROTATE_DURATION);
        Inv_Ring_MotionRotation(
            ring, OPEN_ROTATION,
            -PHD_90 - ring->angle_adder * ring->current_object);
        ring->ringpos.rot.y = motion->rotate_target + OPEN_ROTATION;
        break;

    case RNG_SELECTED: {
        INVENTORY_ITEM *inv_item = ring->list[ring->current_object];
        if (inv_item->object_number == O_PASSPORT_CLOSED) {
            inv_item->object_number = O_PASSPORT_OPTION;
        }

        bool busy = false;
        for (int j = 0; j < Clock_GetFrameAdvance(); j++) {
            busy = false;
            if (inv_item->y_rot == inv_item->y_rot_sel) {
                busy = Inv_AnimateItem(inv_item);
            }
        }

        if (!busy && !g_IDelay) {
            Option_DoInventory(inv_item);

            if (g_InputDB.menu_back) {
                inv_item->sprlist = NULL;
                Inv_Ring_MotionSetup(ring, RNG_CLOSING_ITEM, RNG_DESELECT, 0);
                g_Input = (INPUT_STATE) { 0 };
                g_InputDB = (INPUT_STATE) { 0 };

                if (g_InvMode == INV_LOAD_MODE || g_InvMode == INV_SAVE_MODE
                    || g_InvMode == INV_SAVE_CRYSTAL_MODE) {
                    Inv_Ring_MotionSetup(
                        ring, RNG_CLOSING_ITEM, RNG_EXITING_INVENTORY, 0);
                    g_Input = (INPUT_STATE) { 0 };
                    g_InputDB = (INPUT_STATE) { 0 };
                }
            }

            if (g_InputDB.menu_confirm) {
                inv_item->sprlist = NULL;
                g_InvChosen = inv_item->object_number;
                if (ring->type == RT_MAIN) {
                    g_InvMainCurrent = ring->current_object;
                } else {
                    g_InvOptionCurrent = ring->current_object;
                }

                if (g_InvMode == INV_TITLE_MODE
                    && ((inv_item->object_number == O_DETAIL_OPTION)
                        || inv_item->object_number == O_SOUND_OPTION
                        || inv_item->object_number == O_CONTROL_OPTION
                        || inv_item->object_number == O_GAMMA_OPTION)) {
                    Inv_Ring_MotionSetup(
                        ring, RNG_CLOSING_ITEM, RNG_DESELECT, 0);
                } else {
                    Inv_Ring_MotionSetup(
                        ring, RNG_CLOSING_ITEM, RNG_EXITING_INVENTORY, 0);
                }
                g_Input = (INPUT_STATE) { 0 };
                g_InputDB = (INPUT_STATE) { 0 };
            }
        }
        break;
    }

    case RNG_DESELECT:
        Sound_Effect(SFX_MENU_SPINOUT, NULL, SPM_ALWAYS);
        Inv_Ring_MotionSetup(ring, RNG_DESELECTING, RNG_OPEN, SELECTING_FRAMES);
        Inv_Ring_MotionRotation(
            ring, 0, -PHD_90 - ring->angle_adder * ring->current_object);
        g_Input = (INPUT_STATE) { 0 };
        g_InputDB = (INPUT_STATE) { 0 };
        break;

    case RNG_CLOSING_ITEM: {
        INVENTORY_ITEM *inv_item = ring->list[ring->current_object];
        for (int j = 0; j < Clock_GetFrameAdvance(); j++) {
            if (!Inv_AnimateItem(inv_item)) {
                if (inv_item->object_number == O_PASSPORT_OPTION) {
                    inv_item->object_number = O_PASSPORT_CLOSED;
                    inv_item->current_frame = 0;
                }
                motion->count = SELECTING_FRAMES;
                motion->status = motion->status_target;
                Inv_Ring_MotionItemDeselect(ring, inv_item);
                break;
            }
        }
        break;
    }

    case RNG_EXITING_INVENTORY:
        if (g_InvMode == INV_TITLE_MODE) {
        } else if (
            g_InvChosen == O_PASSPORT_OPTION
            && ((g_InvMode == INV_LOAD_MODE && g_SavedGamesCount) /* f6 menu */
                || g_InvMode == INV_DEATH_MODE /* Lara died */
                || (g_InvMode == INV_GAME_MODE /* esc menu */
                    && g_GameInfo.passport_selection
                        != PASSPORT_MODE_SAVE_GAME /* but not save page */
                    )
                || g_CurrentLevel == g_GameFlow.gym_level_num /* Gym */
                || g_GameInfo.passport_selection == PASSPORT_MODE_RESTART)) {
            Output_FadeToBlack(false);
        } else {
            Output_FadeToTransparent(false);
        }

        if (!motion->count) {
            Inv_Ring_MotionSetup(ring, RNG_CLOSING, RNG_DONE, CLOSE_FRAMES);
            Inv_Ring_MotionRadius(ring, 0);
            Inv_Ring_MotionCameraPos(ring, CAMERA_STARTHEIGHT);
            Inv_Ring_MotionRotation(
                ring, CLOSE_ROTATION, ring->ringpos.rot.y - CLOSE_ROTATION);
        }
        break;
    }

    if (motion->status == RNG_OPEN || motion->status == RNG_SELECTING
        || motion->status == RNG_SELECTED || motion->status == RNG_DESELECTING
        || motion->status == RNG_DESELECT
        || motion->status == RNG_CLOSING_ITEM) {
        if (!ring->rotating && !g_Input.menu_left && !g_Input.menu_right) {
            INVENTORY_ITEM *inv_item = ring->list[ring->current_object];
            Inv_Ring_Active(inv_item);
        }
        Inv_Ring_InitHeader(ring);
    } else {
        Inv_Ring_RemoveHeader(ring);
    }

    if (!motion->status || motion->status == RNG_CLOSING
        || motion->status == RNG_MAIN2OPTION
        || motion->status == RNG_OPTION2MAIN
        || motion->status == RNG_EXITING_INVENTORY || motion->status == RNG_DONE
        || ring->rotating) {
        Inv_Ring_RemoveAlText();
    }

    return GF_PHASE_CONTINUE;
}

static GAMEFLOW_OPTION Phase_Inventory_Control(int32_t nframes)
{
    for (int32_t i = 0; i < nframes; i++) {
        GAMEFLOW_OPTION result = Phase_Inventory_ControlFrame();
        if (result != GF_PHASE_CONTINUE) {
            return result;
        }
    }
    return GF_PHASE_CONTINUE;
}

static void Phase_Inventory_End(void)
{
    if (g_Config.enable_buffering) {
        g_OldInputDB.any = 0;
    }
}

static void Phase_Inventory_Draw(void)
{
    RING_INFO *ring = &m_Ring;
    IMOTION_INFO *motion = &m_Motion;
    Inv_Draw(ring, motion);
    Output_AnimateFades();
    Text_Draw();
}

PHASER g_InventoryPhaser = {
    .start = Phase_Inventory_Start,
    .end = Phase_Inventory_End,
    .control = Phase_Inventory_Control,
    .draw = Phase_Inventory_Draw,
    .wait = NULL,
};
