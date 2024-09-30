#include "game/phase/phase_inventory.h"

#include "config.h"
#include "game/clock.h"
#include "game/console/common.h"
#include "game/game.h"
#include "game/gameflow.h"
#include "game/input.h"
#include "game/interpolation.h"
#include "game/inventory.h"
#include "game/inventory/inventory_ring.h"
#include "game/inventory/inventory_vars.h"
#include "game/lara/common.h"
#include "game/music.h"
#include "game/objects/common.h"
#include "game/option.h"
#include "game/option/option_compass.h"
#include "game/output.h"
#include "game/overlay.h"
#include "game/savegame.h"
#include "game/screen.h"
#include "game/shell.h"
#include "game/sound.h"
#include "game/stats.h"
#include "game/text.h"
#include "game/viewport.h"
#include "global/const.h"
#include "global/types.h"
#include "global/vars.h"
#include "math/matrix.h"

#include <math.h>
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
static TEXTSTRING *m_VersionText = NULL;
static int32_t m_StatsCounter;
static CAMERA_INFO m_OldCamera;
static GAME_OBJECT_ID m_InvChosen;
static CLOCK_TIMER m_DemoTimer = { 0 };
static CLOCK_TIMER m_MotionTimer = { 0 };
static CLOCK_TIMER m_StatsTimer = { 0 };
RING_INFO m_Ring;
IMOTION_INFO m_Motion;

static void Inv_Draw(RING_INFO *ring, IMOTION_INFO *motion);
static void Inv_Construct(void);
static void Inv_Destroy(void);
static PHASE_CONTROL Inv_Close(GAME_OBJECT_ID inv_chosen);
static void Inv_SelectMeshes(INVENTORY_ITEM *inv_item);
static bool Inv_AnimateItem(INVENTORY_ITEM *inv_item);
static int32_t InvItem_GetFrames(
    const INVENTORY_ITEM *inv_item, FRAME_INFO **out_frame1,
    FRAME_INFO **out_frame2, int32_t *out_rate);
static void Inv_DrawItem(INVENTORY_ITEM *inv_item, int32_t frames);
static bool Inv_CheckDemoTimer(const IMOTION_INFO *motion);

static void M_Start(void *arg);
static void M_End(void);
static PHASE_CONTROL M_ControlFrame(void);
static PHASE_CONTROL M_Control(int32_t nframes);
static void M_Draw(void);

static void Inv_Draw(RING_INFO *ring, IMOTION_INFO *motion)
{
    const int32_t frames = Clock_GetFrameAdvance()
        * round(Clock_GetElapsedDrawFrames(&m_MotionTimer));
    ring->camera.pos.z = ring->radius + CAMERA_2_RING;

    if (g_InvMode == INV_TITLE_MODE) {
        Output_DrawBackdropScreen();
        Interpolation_Commit();
    } else {
        Matrix_LookAt(
            m_OldCamera.pos.x, m_OldCamera.pos.y + m_OldCamera.shift,
            m_OldCamera.pos.z, m_OldCamera.target.x, m_OldCamera.target.y,
            m_OldCamera.target.z, 0);
        Interpolation_Disable();
        Game_DrawScene(false);
        Interpolation_Enable();

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
            Inv_DrawItem(inv_item, frames);
            angle += ring->angle_adder;
            Matrix_Pop();
        }
    }

    INVENTORY_ITEM *inv_item = ring->list[ring->current_object];
    switch (inv_item->object_id) {
    case O_MEDI_OPTION:
    case O_BIGMEDI_OPTION:
        Overlay_BarDrawHealth();
        break;

    default:
        break;
    }

    Matrix_Pop();
    Viewport_SetFOV(old_fov);

    if ((motion->status != RNG_OPENING
         || (g_InvMode != INV_TITLE_MODE || !Output_FadeIsAnimating()))
        && motion->status != RNG_DONE) {
        for (int i = 0; i < frames; i++) {
            Inv_Ring_DoMotions(ring);
        }
    }
}

static void Inv_Construct(void)
{
    g_PhdLeft = Viewport_GetMinX();
    g_PhdTop = Viewport_GetMinY();
    g_PhdBottom = Viewport_GetMaxY();
    g_PhdRight = Viewport_GetMaxX();

    m_InvChosen = NO_OBJECT;
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
        Inv_Ring_ResetItem(g_InvMainList[i]);
    }

    for (int i = 0; i < g_InvOptionObjects; i++) {
        Inv_Ring_ResetItem(g_InvOptionList[i]);
    }

    g_InvMainCurrent = 0;
    g_InvOptionCurrent = 0;
    g_OptionSelected = 0;

    if (g_GameFlow.gym_level_num == -1) {
        Inv_RemoveItem(O_PHOTO_OPTION);
    }

    // reset the delta timer before starting the spinout animation
    Clock_ResetTimer(&m_MotionTimer);
}

static void Inv_Destroy(void)
{
    Inv_Ring_RemoveAllText();
    m_InvChosen = NO_OBJECT;

    if (m_VersionText) {
        Text_Remove(m_VersionText);
        m_VersionText = NULL;
    }
}

static PHASE_CONTROL Inv_Close(GAME_OBJECT_ID inv_chosen)
{
    Inv_Destroy();

    if (m_StartLevel != -1) {
        return (PHASE_CONTROL) {
            .end = true,
            .command = {
                .action = GF_SELECT_GAME,
                .param = m_StartLevel,
            },
        };
    }

    if (m_StartDemo) {
        return (PHASE_CONTROL) {
            .end = true,
            .command = { .action = GF_START_DEMO },
        };
    }

    switch (inv_chosen) {
    case O_PASSPORT_OPTION:
        switch (g_GameInfo.passport_selection) {
        case PASSPORT_MODE_LOAD_GAME:
            return (PHASE_CONTROL) {
                .end = true,
                .command = {
                    .action = GF_START_SAVED_GAME,
                    .param = g_GameInfo.current_save_slot,
                },
            };

        case PASSPORT_MODE_SELECT_LEVEL:
            return (PHASE_CONTROL) {
                .end = true,
                .command = {
                    .action = GF_SELECT_GAME,
                    .param = g_GameInfo.select_level_num,
                },
            };

        case PASSPORT_MODE_STORY_SO_FAR:
            return (PHASE_CONTROL) {
                .end = true,
                .command = {
                    .action = GF_STORY_SO_FAR,
                    .param = g_GameInfo.current_save_slot,
                },
            };

        case PASSPORT_MODE_NEW_GAME:
            Savegame_InitCurrentInfo();
            return (PHASE_CONTROL) {
                .end = true,
                .command = {
                    .action = GF_START_GAME,
                    .param = g_GameFlow.first_level_num,
                },
            };

        case PASSPORT_MODE_SAVE_GAME:
            Savegame_Save(g_GameInfo.current_save_slot);
            Music_Unpause();
            Sound_UnpauseAll();
            Phase_Set(PHASE_GAME, 0);
            return (PHASE_CONTROL) { .end = false };

        case PASSPORT_MODE_RESTART:
            return (PHASE_CONTROL) {
                .end = true,
                .command = {
                    .action = GF_RESTART_GAME,
                    .param = g_CurrentLevel,
                },
            };

        case PASSPORT_MODE_EXIT_TITLE:
            return (PHASE_CONTROL) {
                .end = true,
                .command = { .action = GF_EXIT_TO_TITLE },
            };

        case PASSPORT_MODE_EXIT_GAME:
            return (PHASE_CONTROL) {
                .end = true,
                .command = { .action = GF_EXIT_GAME },
            };

        case PASSPORT_MODE_BROWSE:
        case PASSPORT_MODE_UNAVAILABLE:
        default:
            return (PHASE_CONTROL) {
                .end = true,
                .command = { .action = GF_EXIT_TO_TITLE },
            };
        }

    case O_PHOTO_OPTION:
        g_GameInfo.current_save_slot = -1;
        return (PHASE_CONTROL) {
            .end = true,
            .command = {
                .action = GF_START_GYM,
                .param = g_GameFlow.gym_level_num,
            },
        };

    case O_PISTOL_OPTION:
    case O_SHOTGUN_OPTION:
    case O_MAGNUM_OPTION:
    case O_UZI_OPTION:
    case O_MEDI_OPTION:
    case O_BIGMEDI_OPTION:
    case O_KEY_OPTION_1:
    case O_KEY_OPTION_2:
    case O_KEY_OPTION_3:
    case O_KEY_OPTION_4:
    case O_PUZZLE_OPTION_1:
    case O_PUZZLE_OPTION_2:
    case O_PUZZLE_OPTION_3:
    case O_PUZZLE_OPTION_4:
    case O_LEADBAR_OPTION:
    case O_SCION_OPTION:
        Lara_UseItem(inv_chosen);
        break;

    default:
        break;
    }

    if (g_InvMode == INV_TITLE_MODE) {
        return (PHASE_CONTROL) {
            .end = true,
            .command = { .action = GF_CONTINUE_SEQUENCE },
        };
    } else {
        Music_Unpause();
        Sound_UnpauseAll();
        Phase_Set(PHASE_GAME, 0);
        return (PHASE_CONTROL) { .end = false };
    }
}

static void Inv_SelectMeshes(INVENTORY_ITEM *inv_item)
{
    if (inv_item->object_id == O_PASSPORT_OPTION) {
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
    } else if (inv_item->object_id == O_MAP_OPTION) {
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

    inv_item->current_frame += inv_item->anim_direction;
    if (inv_item->current_frame >= inv_item->frames_total) {
        inv_item->current_frame = 0;
    } else if (inv_item->current_frame < 0) {
        inv_item->current_frame = inv_item->frames_total - 1;
    }
    Inv_SelectMeshes(inv_item);
    return true;
}

static int32_t InvItem_GetFrames(
    const INVENTORY_ITEM *inv_item, FRAME_INFO **out_frame1,
    FRAME_INFO **out_frame2, int32_t *out_rate)
{
    const RING_INFO *const ring = &m_Ring;
    const IMOTION_INFO *const motion = &m_Motion;
    const OBJECT *const obj = &g_Objects[inv_item->object_id];
    const INVENTORY_ITEM *const cur_inv_item = ring->list[ring->current_object];
    if (inv_item != cur_inv_item
        || (motion->status != RNG_SELECTED
            && motion->status != RNG_CLOSING_ITEM)) {
        // only apply to animations, eg. the states where Inv_AnimateItem is
        // being actively called
        goto fallback;
    }

    if (inv_item->current_frame == inv_item->goal_frame
        || inv_item->frames_total == 1 || g_Config.rendering.fps == 30) {
        goto fallback;
    }

    const int32_t cur_frame_num = inv_item->current_frame;
    int32_t next_frame_num = inv_item->current_frame + inv_item->anim_direction;
    if (next_frame_num < 0) {
        next_frame_num = 0;
    }
    if (next_frame_num >= inv_item->frames_total) {
        next_frame_num = 0;
    }

    *out_frame1 = &obj->frame_base[cur_frame_num];
    *out_frame2 = &obj->frame_base[next_frame_num];
    *out_rate = 10;
    return (Interpolation_GetRate() - 0.5) * 10.0;

    // OG
fallback:
    *out_frame1 = &obj->frame_base[inv_item->current_frame];
    *out_frame2 = *out_frame1;
    *out_rate = 1;
    return 0;
}

static void Inv_DrawItem(INVENTORY_ITEM *const inv_item, const int32_t frames)
{
    const RING_INFO *ring = &m_Ring;
    const IMOTION_INFO *motion = &m_Motion;

    if (motion->status == RNG_DONE) {
        g_LsAdder = LOW_LIGHT;
    } else if (inv_item == ring->list[ring->current_object]) {
        if (ring->rotating) {
            g_LsAdder = LOW_LIGHT;
            for (int j = 0; j < frames; j++) {
                if (inv_item->y_rot < 0) {
                    inv_item->y_rot += 512;
                } else if (inv_item->y_rot > 0) {
                    inv_item->y_rot -= 512;
                }
            }
        } else if (
            motion->status == RNG_SELECTED || motion->status == RNG_DESELECTING
            || motion->status == RNG_SELECTING || motion->status == RNG_DESELECT
            || motion->status == RNG_CLOSING_ITEM) {
            g_LsAdder = HIGH_LIGHT;
            for (int j = 0; j < frames; j++) {
                if (inv_item->y_rot != inv_item->y_rot_sel) {
                    if (inv_item->y_rot_sel - inv_item->y_rot > 0
                        && inv_item->y_rot_sel - inv_item->y_rot < 0x8000) {
                        inv_item->y_rot += 1024;
                    } else {
                        inv_item->y_rot -= 1024;
                    }
                    inv_item->y_rot &= 0xFC00u;
                }
            }
        } else if (
            ring->number_of_objects == 1
            || (!g_Input.menu_left && !g_Input.menu_right)
            || !g_Input.menu_left) {
            g_LsAdder = HIGH_LIGHT;
            for (int j = 0; j < frames; j++) {
                inv_item->y_rot += 256;
            }
        }
    } else {
        g_LsAdder = LOW_LIGHT;
        for (int j = 0; j < frames; j++) {
            if (inv_item->y_rot < 0) {
                inv_item->y_rot += 256;
            } else if (inv_item->y_rot > 0) {
                inv_item->y_rot -= 256;
            }
        }
    }

    Matrix_TranslateRel(0, inv_item->ytrans, inv_item->ztrans);
    Matrix_RotYXZ(inv_item->y_rot, inv_item->x_rot, 0);

    OBJECT *obj = &g_Objects[inv_item->object_id];
    if (obj->nmeshes < 0) {
        Output_DrawSpriteRel(0, 0, 0, obj->mesh_idx, 4096);
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
                        g_Objects[O_ALPHABET].mesh_idx + spr->sprnum, 4096, 0);
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

    int32_t rate;
    FRAME_INFO *frame1;
    FRAME_INFO *frame2;
    const int32_t frac = InvItem_GetFrames(inv_item, &frame1, &frame2, &rate);
    if (inv_item->object_id == O_MAP_OPTION) {
        const int16_t extra_rotation[1] = { Option_Compass_GetNeedleAngle() };
        int32_t *const bone = &g_AnimBones[obj->bone_idx];
        bone[0] |= BEB_ROT_Y;
        Object_DrawInterpolatedObject(
            obj, inv_item->drawn_meshes, extra_rotation, frame1, frame2, frac,
            rate);
    } else {
        Object_DrawInterpolatedObject(
            obj, inv_item->drawn_meshes, NULL, frame1, frame2, frac, rate);
    }
}

static bool Inv_CheckDemoTimer(const IMOTION_INFO *const motion)
{
    if (!g_Config.enable_demo || !g_GameFlow.has_demo) {
        return false;
    }

    if (g_InvMode != INV_TITLE_MODE || g_Input.any || g_InputDB.any
        || Console_IsOpened()) {
        Clock_ResetTimer(&m_DemoTimer);
        return false;
    }

    return motion->status == RNG_OPEN
        && Clock_CheckElapsedMilliseconds(
               &m_DemoTimer, g_GameFlow.demo_delay * 1000.0);
}

static void M_Start(void *arg)
{
    Interpolation_Remember();
    if (g_Config.enable_timer_in_inventory) {
        Stats_StartTimer();
    }

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

static PHASE_CONTROL M_ControlFrame(void)
{
    RING_INFO *ring = &m_Ring;
    IMOTION_INFO *motion = &m_Motion;

    if (motion->status == RNG_OPENING) {
        if (g_InvMode == INV_TITLE_MODE && Output_FadeIsAnimating()) {
            return (PHASE_CONTROL) { .end = false };
        }

        Clock_ResetTimer(&m_DemoTimer);
        if (!m_PlayedSpinin) {
            Sound_Effect(SFX_MENU_SPININ, NULL, SPM_ALWAYS);
            m_PlayedSpinin = true;
        }
    }

    if (motion->status == RNG_DONE) {
        // finish fading
        if (g_InvMode == INV_TITLE_MODE) {
            Output_FadeToBlack(true);
        }

        if (Output_FadeIsAnimating()) {
            return (PHASE_CONTROL) { .end = false };
        }

        return Inv_Close(m_InvChosen);
    }

    Inv_Ring_CalcAdders(ring, ROTATE_DURATION);

    Input_Update();
    // Do the demo inactivity check prior to postprocessing of the inputs.
    if (Inv_CheckDemoTimer(motion)) {
        m_StartDemo = true;
    }
    Shell_ProcessInput();
    Game_ProcessInput();

    m_StartLevel = g_LevelComplete ? g_GameInfo.select_level_num : -1;

    if (g_IDelay) {
        if (g_IDCount) {
            g_IDCount--;
        } else {
            g_IDelay = false;
        }
    }

    g_GameInfo.inv_ring_above = g_InvMode == INV_GAME_MODE
        && ((ring->type == RT_MAIN && g_InvKeysObjects)
            || (ring->type == RT_OPTION && g_InvMainObjects));

    if (ring->rotating) {
        return (PHASE_CONTROL) { .end = false };
    }

    if ((g_InvMode == INV_SAVE_MODE || g_InvMode == INV_SAVE_CRYSTAL_MODE
         || g_InvMode == INV_LOAD_MODE || g_InvMode == INV_DEATH_MODE)
        && !m_PassportModeReady) {
        g_Input = (INPUT_STATE) { 0 };
        g_InputDB = (INPUT_STATE) { 0, .menu_confirm = 1 };
    }

    if (!(g_InvMode == INV_TITLE_MODE || Output_FadeIsAnimating()
          || motion->status == RNG_OPENING)) {
        for (int i = 0; i < ring->number_of_objects; i++) {
            INVENTORY_ITEM *inv_item = ring->list[i];
            if (inv_item->object_id == O_MAP_OPTION) {
                Option_Compass_UpdateNeedle(inv_item);
            }
        }
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
            m_InvChosen = NO_OBJECT;

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

            switch (inv_item->object_id) {
            case O_MAP_OPTION:
                Sound_Effect(SFX_MENU_COMPASS, NULL, SPM_ALWAYS);
                break;

            case O_PHOTO_OPTION:
                Sound_Effect(SFX_MENU_CHOOSE, NULL, SPM_ALWAYS);
                break;

            case O_CONTROL_OPTION:
                Sound_Effect(SFX_MENU_GAMEBOY, NULL, SPM_ALWAYS);
                break;

            case O_PISTOL_OPTION:
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
        if (inv_item->object_id == O_PASSPORT_CLOSED) {
            inv_item->object_id = O_PASSPORT_OPTION;
        }

        bool busy = false;
        if (inv_item->y_rot == inv_item->y_rot_sel) {
            busy = Inv_AnimateItem(inv_item);
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
                m_InvChosen = inv_item->object_id;
                if (ring->type == RT_MAIN) {
                    g_InvMainCurrent = ring->current_object;
                } else {
                    g_InvOptionCurrent = ring->current_object;
                }

                if (g_InvMode == INV_TITLE_MODE
                    && ((inv_item->object_id == O_DETAIL_OPTION)
                        || inv_item->object_id == O_SOUND_OPTION
                        || inv_item->object_id == O_CONTROL_OPTION
                        || inv_item->object_id == O_GAMMA_OPTION)) {
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
        if (!Inv_AnimateItem(inv_item)) {
            if (inv_item->object_id == O_PASSPORT_OPTION) {
                inv_item->object_id = O_PASSPORT_CLOSED;
                inv_item->current_frame = 0;
            }
            motion->count = SELECTING_FRAMES;
            motion->status = motion->status_target;
            Inv_Ring_MotionItemDeselect(ring, inv_item);
            break;
        }
        break;
    }

    case RNG_EXITING_INVENTORY:
        if (g_InvMode == INV_TITLE_MODE) {
        } else if (
            m_InvChosen == O_PASSPORT_OPTION
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
        Inv_Ring_RemoveHeader();
    }

    if (!motion->status || motion->status == RNG_CLOSING
        || motion->status == RNG_MAIN2OPTION
        || motion->status == RNG_OPTION2MAIN
        || motion->status == RNG_EXITING_INVENTORY || motion->status == RNG_DONE
        || ring->rotating) {
        Inv_Ring_RemoveAllText();
    }

    return (PHASE_CONTROL) { .end = false };
}

static PHASE_CONTROL M_Control(int32_t nframes)
{
    Interpolation_Remember();
    if (g_Config.enable_timer_in_inventory) {
        Stats_UpdateTimer();
    }
    for (int32_t i = 0; i < nframes; i++) {
        const PHASE_CONTROL result = M_ControlFrame();
        if (result.end) {
            return result;
        }
    }

    return (PHASE_CONTROL) { .end = false };
}

static void M_End(void)
{
    INVENTORY_ITEM *const inv_item = m_Ring.list[m_Ring.current_object];
    if (inv_item != NULL) {
        Option_Shutdown(inv_item);
    }

    Inv_Destroy();
    if (g_Config.enable_buffering) {
        g_OldInputDB.any = 0;
    }
    if (g_InvMode == INV_TITLE_MODE) {
        Music_Stop();
        Sound_StopAllSamples();
    }
}

static void M_Draw(void)
{
    RING_INFO *ring = &m_Ring;
    IMOTION_INFO *motion = &m_Motion;
    Inv_Draw(ring, motion);
    Output_AnimateFades();
    Text_Draw();
}

PHASER g_InventoryPhaser = {
    .start = M_Start,
    .end = M_End,
    .control = M_Control,
    .draw = M_Draw,
    .wait = NULL,
};
