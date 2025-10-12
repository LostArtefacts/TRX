#include "game/inventory_ring/control.h"

#include "game/game.h"
#include "game/game_flow.h"
#include "game/game_string.h"
#include "game/inventory.h"
#include "game/inventory_ring/vars.h"
#include "game/lara.h"
#include "game/option/option_compass.h"
#include "game/savegame.h"
#include "game/shell.h"
#include "game/stats.h"
#include "global/vars.h"

#include <libtrx/config.h>
#include <libtrx/game/camera.h>
#include <libtrx/game/console.h>
#include <libtrx/game/gun/const.h>
#include <libtrx/game/input.h>
#include <libtrx/game/interpolation.h>
#include <libtrx/game/inventory_ring/priv.h>
#include <libtrx/game/music.h>
#include <libtrx/game/option.h>
#include <libtrx/game/option/examine.h>
#include <libtrx/game/output.h>
#include <libtrx/game/overlay.h>
#include <libtrx/game/sound.h>
#include <libtrx/game/viewport.h>
#include <libtrx/memory.h>

#define M_INV_RING_FADE_TIME_FAST                                              \
    (INV_RING_CLOSE_FRAMES / INV_RING_FRAMES / (double)LOGIC_FPS)
#define M_INV_RING_FADE_TIME_TITLE_FINISH 0.25
#define M_RING_SWITCH_FRAMES (96 / 2)
#define M_SELECTING_FRAMES (32 / 2)
#define M_OPTION_RING_OBJECTS 5
#define M_TITLE_RING_OBJECTS 6

static CLOCK_TIMER m_DemoTimer = { .type = CLOCK_TIMER_SIM };
static int32_t m_StartLevel;
static OBJECT_ID m_InvChosen = NO_OBJECT;

static void M_ShowAmmoQuantity(const char *const fmt, const int32_t qty)
{
    if (!Game_IsBonusFlagSet(GBF_NGPLUS)) {
        InvRing_ShowItemQuantity(fmt, qty);
    }
}

static void M_RingIsOpen(INV_RING *const ring)
{
    InvRing_ShowHeader(ring);
}

static void M_RingIsNotOpen(INV_RING *const ring)
{
    InvRing_RemoveHeader();
    InvRing_ShowExamine(false);
}

static void M_RingNotActive(
    const INV_RING *const ring, const INVENTORY_ITEM *const inv_item)
{
    InvRing_ShowItemName(inv_item);

    const int32_t qty = Inv_RequestItem(inv_item->object_id);

    const LARA_INFO *const lara = Lara_GetLaraInfo();
    switch (inv_item->object_id) {
    case O_SHOTGUN_OPTION:
        M_ShowAmmoQuantity(
            "%5d \\{ammo shotgun}",
            lara->shotgun_ammo.ammo / SHOTGUN_AMMO_CLIP);
        break;

    case O_MAGNUM_OPTION:
        M_ShowAmmoQuantity("%5d \\{ammo magnums}", lara->magnum_ammo.ammo);
        break;

    case O_UZI_OPTION:
        M_ShowAmmoQuantity("%5d \\{ammo uzis}", lara->uzi_ammo.ammo);
        break;

    case O_SHOTGUN_AMMO_OPTION:
        InvRing_ShowItemQuantity("%d", qty * SHOTGUN_SHELL_COUNT);
        break;

    case O_MAGNUM_AMMO_OPTION:
    case O_UZI_AMMO_OPTION:
        InvRing_ShowItemQuantity("%d", qty * 2);
        break;

    case O_SMALL_MEDIPACK_OPTION:
    case O_LARGE_MEDIPACK_OPTION:
        Overlay_SetHealthBarTimer(40);
        if (qty > 1) {
            InvRing_ShowItemQuantity("%d", qty);
        }
        break;

    case O_KEY_OPTION_1:
    case O_KEY_OPTION_2:
    case O_KEY_OPTION_3:
    case O_KEY_OPTION_4:
    case O_LEADBAR_OPTION:
    case O_PICKUP_OPTION_1:
    case O_PICKUP_OPTION_2:
    case O_PUZZLE_OPTION_1:
    case O_PUZZLE_OPTION_2:
    case O_PUZZLE_OPTION_3:
    case O_PUZZLE_OPTION_4:
    case O_SCION_OPTION:
        if (qty > 1) {
            InvRing_ShowItemQuantity("%d", qty);
        }

        break;

    default:
        break;
    }

    InvRing_ShowExamine(
        ring->motion.status == RNG_OPEN
        && Option_Examine_CanExamine(inv_item->object_id));
}

static void M_RingActive(INV_RING *const ring)
{
    InvRing_RemoveItemTexts();
    InvRing_ShowExamine(false);
}

static bool M_AnimateInventoryItem(INVENTORY_ITEM *const inv_item)
{
    if (inv_item->current_frame == inv_item->goal_frame) {
        InvRing_SelectMeshes(inv_item);
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

    InvRing_SelectMeshes(inv_item);
    return true;
}

static GF_COMMAND M_Finish(INV_RING *const ring, const bool apply_changes)
{
    // TODO: Make this function not have any side effects.
    // Consider adding new GF_ constants, but research other solutions first.

    if (m_StartLevel != -1) {
        return (GF_COMMAND) {
            .action = GF_SELECT_GAME,
            .param = m_StartLevel,
        };
    }

    if (ring->is_demo_needed) {
        return (GF_COMMAND) { .action = GF_START_DEMO, .param = -1 };
    }

    switch (m_InvChosen) {
    case O_PASSPORT_OPTION:
        switch (g_GameInfo.passport_selection) {
        case PASSPORT_MODE_LOAD_GAME:
            return (GF_COMMAND) {
                .action = GF_START_SAVED_GAME,
                .param = g_GameInfo.select_save_slot,
            };

        case PASSPORT_MODE_SELECT_LEVEL:
            return (GF_COMMAND) {
                .action = GF_SELECT_GAME,
                .param = g_GameInfo.select_level_num,
            };

        case PASSPORT_MODE_STORY_SO_FAR:
            return (GF_COMMAND) {
                .action = GF_STORY_SO_FAR,
                .param = g_GameInfo.select_save_slot,
            };

        case PASSPORT_MODE_NEW_GAME:
            if (apply_changes) {
                Savegame_InitCurrentInfo();
                Savegame_UnbindSlot();
            }
            return (GF_COMMAND) {
                .action = GF_START_GAME,
                .param = GF_GetFirstLevel()->num,
            };

        case PASSPORT_MODE_SAVE_GAME:
            if (apply_changes) {
                Savegame_Save(g_GameInfo.select_save_slot);
            }
            return (GF_COMMAND) { .action = GF_NOOP };

        case PASSPORT_MODE_RESTART:
            return (GF_COMMAND) {
                .action = GF_RESTART_GAME,
                .param = Game_GetCurrentLevel()->num,
            };

        case PASSPORT_MODE_EXIT_TITLE:
            return (GF_COMMAND) { .action = GF_EXIT_TO_TITLE };

        case PASSPORT_MODE_EXIT_GAME:
            return (GF_COMMAND) { .action = GF_EXIT_GAME };

        case PASSPORT_MODE_BROWSE:
        case PASSPORT_MODE_UNAVAILABLE:
        default:
            return (GF_COMMAND) { .action = GF_EXIT_TO_TITLE };
        }

    case O_PHOTO_OPTION:
        if (apply_changes) {
            Savegame_UnbindSlot();
        }
        if (GF_GetGymLevel() != nullptr) {
            return (GF_COMMAND) {
                .action = GF_START_GAME,
                .param = GF_GetGymLevel()->num,
            };
        }
        break;

    case O_PISTOL_OPTION:
    case O_SHOTGUN_OPTION:
    case O_MAGNUM_OPTION:
    case O_UZI_OPTION:
    case O_SMALL_MEDIPACK_OPTION:
    case O_LARGE_MEDIPACK_OPTION:
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
        if (apply_changes) {
            Lara_UseItem(m_InvChosen);
        }
        break;

    default:
        break;
    }

    return (GF_COMMAND) { .action = GF_NOOP };
}

static bool M_CheckDemoTimer(const INV_RING *const ring)
{
    if (!g_Config.gameplay.enable_demo
        || GF_GetLevelTable(GFLT_DEMOS)->count == 0) {
        return false;
    }

    if (ring->mode != INV_TITLE_MODE || g_Input.any || g_InputDB.any
        || Console_IsOpened()) {
        ClockTimer_Sync(&m_DemoTimer);
        return false;
    }

    return ring->motion.status == RNG_OPEN
        && ClockTimer_CheckElapsed(&m_DemoTimer, g_GameFlow.demo_delay);
}

static GF_COMMAND M_Control(INV_RING *const ring)
{
    if (ring->motion.status == RNG_OPENING) {
        if (ring->mode == INV_TITLE_MODE
            && (Fader_IsActive(&ring->top_fader)
                || Fader_IsActive(&ring->back_fader))) {
            return (GF_COMMAND) { .action = GF_NOOP };
        }

        ClockTimer_Sync(&m_DemoTimer);
        if (!ring->has_spun_out) {
            Sound_Effect(SFX_MENU_SPININ, nullptr, SPM_ALWAYS);
            ring->has_spun_out = true;
        }
    }

    if (ring->motion.status == RNG_FADING_OUT) {
        if (!Fader_IsActive(&ring->back_fader)
            && !Fader_IsActive(&ring->top_fader)) {
            Fader_InitEx(
                &ring->top_fader,
                (FADER_ARGS) {
                    .initial = FADER_ANY,
                    .target = FADER_BLACK,
                    .duration = M_INV_RING_FADE_TIME_TITLE_FINISH,
                    .debuff = 1. / (double)LOGIC_FPS,
                });
        }

        if (Fader_IsActive(&ring->top_fader)
            || Fader_IsActive(&ring->back_fader)) {
            return (GF_COMMAND) { .action = GF_NOOP };
        }
        ring->motion.status = RNG_DONE;
    }

    if (ring->motion.status == RNG_DONE && !ring->is_done) {
        const GF_COMMAND gf_cmd = M_Finish(ring, true);
        ring->is_done = true;
        // Returning to game – resume music
        if (gf_cmd.action == GF_NOOP) {
            Music_Unpause();
            Sound_UnpauseAll();
        }
        return gf_cmd;
    }

    InvRing_CalcAdders(ring, INV_RING_ROTATE_DURATION);

    Input_Update();
    // Do the demo inactivity check prior to postprocessing of the inputs.
    if (M_CheckDemoTimer(ring)) {
        ring->is_demo_needed = true;
    }
    Shell_ProcessInput();
    Game_ProcessInput();

    m_StartLevel = Game_IsLevelComplete() ? g_GameInfo.select_level_num : -1;

    if (g_Config.gameplay.enable_timer_in_inventory
        && !(TR_VERSION >= 2 && Game_IsInGym())) {
        Stats_UpdateTimer();
    }

    if (ring->rotating) {
        return (GF_COMMAND) { .action = GF_NOOP };
    }

    if ((ring->mode == INV_SAVE_MODE || ring->mode == INV_SAVE_CRYSTAL_MODE
         || ring->mode == INV_LOAD_MODE || ring->mode == INV_DEATH_MODE)
        && !ring->is_pass_open) {
        g_Input = (INPUT_STATE) {};
        g_InputDB = (INPUT_STATE) { .menu_confirm = 1 };
    }

    if (ring->mode != INV_TITLE_MODE && !Fader_IsActive(&ring->back_fader)
        && !Fader_IsActive(&ring->top_fader)
        && ring->motion.status != RNG_OPENING) {
        for (int i = 0; i < ring->number_of_objects; i++) {
            INVENTORY_ITEM *const inv_item = ring->list[i];
            if (inv_item->object_id == O_COMPASS_OPTION) {
                Option_Compass_UpdateNeedle(inv_item);
            }
        }
    }

    switch (ring->motion.status) {
    case RNG_OPEN:
        if (g_Input.menu_right && ring->number_of_objects > 1) {
            InvRing_RotateLeft(ring);
            Sound_Effect(SFX_MENU_ROTATE, nullptr, SPM_ALWAYS);
            break;
        }

        if (g_Input.menu_left && ring->number_of_objects > 1) {
            InvRing_RotateRight(ring);
            Sound_Effect(SFX_MENU_ROTATE, nullptr, SPM_ALWAYS);
            break;
        }

        if (m_StartLevel != -1 || ring->is_demo_needed
            || (g_InputDB.menu_back && ring->mode != INV_TITLE_MODE)) {
            Sound_Effect(SFX_MENU_SPINOUT, nullptr, SPM_ALWAYS);
            m_InvChosen = NO_OBJECT;

            if (ring->type == RT_MAIN) {
                g_InvRing_Source[RT_MAIN].current = ring->current_object;
            } else {
                g_InvRing_Source[RT_OPTION].current = ring->current_object;
            }

            if (M_Finish(ring, false).action != GF_NOOP) {
                InvRing_MotionSetup(
                    ring, RNG_CLOSING, RNG_FADING_OUT, INV_RING_CLOSE_FRAMES);
            } else {
                InvRing_MotionSetup(
                    ring, RNG_CLOSING, RNG_DONE, INV_RING_CLOSE_FRAMES);
                Fader_Init(
                    &ring->back_fader, FADER_ANY, FADER_TRANSPARENT,
                    M_INV_RING_FADE_TIME_FAST);
            }
            InvRing_MotionRadius(ring, 0);
            InvRing_MotionCameraPos(ring, INV_RING_CAMERA_START_HEIGHT);
            InvRing_MotionRotation(
                ring, INV_RING_CLOSE_ROTATION,
                ring->ring_pos.rot.y - INV_RING_CLOSE_ROTATION);

            g_Input = (INPUT_STATE) {};
            g_InputDB = (INPUT_STATE) {};
        }

        const bool examine = g_InputDB.look && InvRing_CanExamine();
        if (g_InputDB.menu_confirm || examine) {
            if ((ring->mode == INV_SAVE_MODE
                 || ring->mode == INV_SAVE_CRYSTAL_MODE
                 || ring->mode == INV_LOAD_MODE || ring->mode == INV_DEATH_MODE)
                && !ring->is_pass_open) {
                ring->is_pass_open = true;
            }

            g_InvRing_Source[ring->type].current = ring->current_object;
            INVENTORY_ITEM *const inv_item =
                g_InvRing_Source[ring->type].items[ring->current_object];

            if (examine) {
                inv_item->action = ACTION_EXAMINE;
                inv_item->goal_frame = 0;
                inv_item->anim_direction = 1;
            } else {
                inv_item->action = ACTION_USE;
                inv_item->goal_frame = inv_item->open_frame;
                inv_item->anim_direction = 1;
            }
            InvRing_MotionSetup(
                ring, RNG_SELECTING, RNG_SELECTED, M_SELECTING_FRAMES);
            InvRing_MotionRotation(
                ring, 0, -DEG_90 - ring->angle_adder * ring->current_object);
            InvRing_MotionItemSelect(ring, inv_item);
            g_Input = (INPUT_STATE) {};
            g_InputDB = (INPUT_STATE) {};

            switch (inv_item->object_id) {
            case O_COMPASS_OPTION:
                Sound_Effect(SFX_MENU_COMPASS, nullptr, SPM_ALWAYS);
                break;

            case O_PHOTO_OPTION:
                Sound_Effect(SFX_MENU_LARA_HOME, nullptr, SPM_ALWAYS);
                break;

            case O_CONTROL_OPTION:
                Sound_Effect(SFX_MENU_GAMEBOY, nullptr, SPM_ALWAYS);
                break;

            case O_PISTOL_OPTION:
            case O_SHOTGUN_OPTION:
            case O_MAGNUM_OPTION:
            case O_UZI_OPTION:
                Sound_Effect(SFX_MENU_GUNS, nullptr, SPM_ALWAYS);
                break;

            default:
                Sound_Effect(SFX_MENU_SPININ, nullptr, SPM_ALWAYS);
                break;
            }
        }

        if (g_InputDB.menu_up && ring->mode != INV_TITLE_MODE
            && ring->mode != INV_KEYS_MODE) {
            if (ring->type == RT_MAIN) {
                if (g_InvRing_Source[RT_KEYS].count != 0) {
                    InvRing_MotionSetup(
                        ring, RNG_CLOSING, RNG_MAIN2KEYS,
                        M_RING_SWITCH_FRAMES / 2);
                    InvRing_MotionRadius(ring, 0);
                    InvRing_MotionRotation(
                        ring, INV_RING_CLOSE_ROTATION,
                        ring->ring_pos.rot.y - INV_RING_CLOSE_ROTATION);
                    InvRing_MotionCameraPitch(ring, 0x2000);
                    ring->motion.misc = 0x2000;
                }
                g_Input = (INPUT_STATE) {};
                g_InputDB = (INPUT_STATE) {};
            } else if (ring->type == RT_OPTION) {
                if (g_InvRing_Source[RT_MAIN].count) {
                    InvRing_MotionSetup(
                        ring, RNG_CLOSING, RNG_OPTION2MAIN,
                        M_RING_SWITCH_FRAMES / 2);
                    InvRing_MotionRadius(ring, 0);
                    InvRing_MotionRotation(
                        ring, INV_RING_CLOSE_ROTATION,
                        ring->ring_pos.rot.y - INV_RING_CLOSE_ROTATION);
                    InvRing_MotionCameraPitch(ring, 0x2000);
                    ring->motion.misc = 0x2000;
                }
                g_InputDB = (INPUT_STATE) {};
            }
        } else if (
            g_InputDB.menu_down && ring->mode != INV_TITLE_MODE
            && ring->mode != INV_KEYS_MODE) {
            if (ring->type == RT_KEYS) {
                if (g_InvRing_Source[RT_MAIN].count) {
                    InvRing_MotionSetup(
                        ring, RNG_CLOSING, RNG_KEYS2MAIN,
                        M_RING_SWITCH_FRAMES / 2);
                    InvRing_MotionRadius(ring, 0);
                    InvRing_MotionRotation(
                        ring, INV_RING_CLOSE_ROTATION,
                        ring->ring_pos.rot.y - INV_RING_CLOSE_ROTATION);
                    InvRing_MotionCameraPitch(ring, -0x2000);
                    ring->motion.misc = -0x2000;
                }
                g_Input = (INPUT_STATE) {};
                g_InputDB = (INPUT_STATE) {};
            } else if (ring->type == RT_MAIN) {
                if (g_InvRing_Source[RT_OPTION].count != 0) {
                    InvRing_MotionSetup(
                        ring, RNG_CLOSING, RNG_MAIN2OPTION,
                        M_RING_SWITCH_FRAMES / 2);
                    InvRing_MotionRadius(ring, 0);
                    InvRing_MotionRotation(
                        ring, INV_RING_CLOSE_ROTATION,
                        ring->ring_pos.rot.y - INV_RING_CLOSE_ROTATION);
                    InvRing_MotionCameraPitch(ring, -0x2000);
                    ring->motion.misc = -0x2000;
                }
                g_InputDB = (INPUT_STATE) {};
            }
        }
        break;

    case RNG_MAIN2OPTION:
        InvRing_MotionSetup(
            ring, RNG_OPENING, RNG_OPEN, M_RING_SWITCH_FRAMES / 2);
        InvRing_MotionRadius(ring, INV_RING_RADIUS);
        ring->camera_pitch = -ring->motion.misc;
        ring->motion.camera_pitch_rate =
            ring->motion.misc / (M_RING_SWITCH_FRAMES / 2);
        ring->motion.camera_pitch_target = 0;
        g_InvRing_Source[RT_MAIN].current = ring->current_object;
        ring->type = RT_OPTION;
        ring->list = g_InvRing_Source[RT_OPTION].items;
        ring->number_of_objects = g_InvRing_Source[RT_OPTION].count;
        ring->current_object = g_InvRing_Source[RT_OPTION].current;
        InvRing_CalcAdders(ring, INV_RING_ROTATE_DURATION);
        InvRing_MotionRotation(
            ring, INV_RING_OPEN_ROTATION,
            -DEG_90 - ring->angle_adder * ring->current_object);
        ring->ring_pos.rot.y =
            ring->motion.rotate_target + INV_RING_OPEN_ROTATION;
        break;

    case RNG_MAIN2KEYS:
        InvRing_MotionSetup(
            ring, RNG_OPENING, RNG_OPEN, M_RING_SWITCH_FRAMES / 2);
        InvRing_MotionRadius(ring, INV_RING_RADIUS);
        ring->camera_pitch = -ring->motion.misc;
        ring->motion.camera_pitch_rate =
            ring->motion.misc / (M_RING_SWITCH_FRAMES / 2);
        ring->motion.camera_pitch_target = 0;
        g_InvRing_Source[RT_MAIN].current = ring->current_object;
        g_InvRing_Source[RT_MAIN].count = ring->number_of_objects;
        ring->type = RT_KEYS;
        ring->list = g_InvRing_Source[RT_KEYS].items;
        ring->number_of_objects = g_InvRing_Source[RT_KEYS].count;
        ring->current_object = g_InvRing_Source[RT_KEYS].current;
        InvRing_CalcAdders(ring, INV_RING_ROTATE_DURATION);
        InvRing_MotionRotation(
            ring, INV_RING_OPEN_ROTATION,
            -DEG_90 - ring->angle_adder * ring->current_object);
        ring->ring_pos.rot.y =
            ring->motion.rotate_target + INV_RING_OPEN_ROTATION;
        break;

    case RNG_KEYS2MAIN:
        InvRing_MotionSetup(
            ring, RNG_OPENING, RNG_OPEN, M_RING_SWITCH_FRAMES / 2);
        InvRing_MotionRadius(ring, INV_RING_RADIUS);
        ring->camera_pitch = -ring->motion.misc;
        ring->motion.camera_pitch_rate =
            ring->motion.misc / (M_RING_SWITCH_FRAMES / 2);
        ring->motion.camera_pitch_target = 0;
        g_InvRing_Source[RT_KEYS].current = ring->current_object;
        ring->type = RT_MAIN;
        ring->list = g_InvRing_Source[RT_MAIN].items;
        ring->number_of_objects = g_InvRing_Source[RT_MAIN].count;
        ring->current_object = g_InvRing_Source[RT_MAIN].current;
        InvRing_CalcAdders(ring, INV_RING_ROTATE_DURATION);
        InvRing_MotionRotation(
            ring, INV_RING_OPEN_ROTATION,
            -DEG_90 - ring->angle_adder * ring->current_object);
        ring->ring_pos.rot.y =
            ring->motion.rotate_target + INV_RING_OPEN_ROTATION;
        break;

    case RNG_OPTION2MAIN:
        InvRing_MotionSetup(
            ring, RNG_OPENING, RNG_OPEN, M_RING_SWITCH_FRAMES / 2);
        InvRing_MotionRadius(ring, INV_RING_RADIUS);
        ring->camera_pitch = -ring->motion.misc;
        ring->motion.camera_pitch_rate =
            ring->motion.misc / (M_RING_SWITCH_FRAMES / 2);
        ring->motion.camera_pitch_target = 0;
        g_InvRing_Source[RT_OPTION].count = ring->number_of_objects;
        g_InvRing_Source[RT_OPTION].current = ring->current_object;
        ring->type = RT_MAIN;
        ring->list = g_InvRing_Source[RT_MAIN].items;
        ring->number_of_objects = g_InvRing_Source[RT_MAIN].count;
        ring->current_object = g_InvRing_Source[RT_MAIN].current;
        InvRing_CalcAdders(ring, INV_RING_ROTATE_DURATION);
        InvRing_MotionRotation(
            ring, INV_RING_OPEN_ROTATION,
            -DEG_90 - ring->angle_adder * ring->current_object);
        ring->ring_pos.rot.y =
            ring->motion.rotate_target + INV_RING_OPEN_ROTATION;
        break;

    case RNG_SELECTED: {
        INVENTORY_ITEM *inv_item = ring->list[ring->current_object];
        if (inv_item->object_id == O_PASSPORT_CLOSED) {
            inv_item->object_id = O_PASSPORT_OPTION;
        }

        bool busy = false;
        for (int32_t frame = 0; frame < INV_RING_FRAMES; frame++) {
            busy = false;
            if (inv_item->y_rot == inv_item->y_rot_sel) {
                busy = M_AnimateInventoryItem(inv_item);
            }
        }

        Option_Control(inv_item, busy);

        if (!busy) {
            if (g_InputDB.menu_back) {
                InvRing_MotionSetup(ring, RNG_CLOSING_ITEM, RNG_DESELECT, 0);
                g_Input = (INPUT_STATE) {};
                g_InputDB = (INPUT_STATE) {};
                if (ring->mode == INV_LOAD_MODE || ring->mode == INV_SAVE_MODE
                    || ring->mode == INV_SAVE_CRYSTAL_MODE) {
                    InvRing_MotionSetup(
                        ring, RNG_CLOSING_ITEM, RNG_EXITING_INVENTORY, 0);
                    g_Input = (INPUT_STATE) {};
                    g_InputDB = (INPUT_STATE) {};
                }
            }

            if (g_InputDB.menu_confirm) {
                m_InvChosen = inv_item->object_id;
                if (ring->type == RT_MAIN) {
                    g_InvRing_Source[RT_MAIN].current = ring->current_object;
                } else {
                    g_InvRing_Source[RT_OPTION].current = ring->current_object;
                }

                if (ring->mode == INV_TITLE_MODE
                    && (inv_item->object_id == O_DETAIL_OPTION
                        || inv_item->object_id == O_SOUND_OPTION
                        || inv_item->object_id == O_MAP_OPTION
                        || inv_item->object_id == O_CONTROL_OPTION
                        || inv_item->object_id == O_GAMMA_OPTION)) {
                    InvRing_MotionSetup(
                        ring, RNG_CLOSING_ITEM, RNG_DESELECT, 0);
                } else {
                    InvRing_MotionSetup(
                        ring, RNG_CLOSING_ITEM, RNG_EXITING_INVENTORY, 0);
                }
                g_Input = (INPUT_STATE) {};
                g_InputDB = (INPUT_STATE) {};
            }
        }
        break;
    }

    case RNG_DESELECT: {
        INVENTORY_ITEM *const inv_item = ring->list[ring->current_object];
        Option_Close(inv_item);
        Sound_Effect(SFX_MENU_SPINOUT, nullptr, SPM_ALWAYS);
        InvRing_MotionSetup(
            ring, RNG_DESELECTING, RNG_OPEN, M_SELECTING_FRAMES);
        InvRing_MotionRotation(
            ring, 0, -DEG_90 - ring->angle_adder * ring->current_object);
        g_Input = (INPUT_STATE) {};
        g_InputDB = (INPUT_STATE) {};
        break;
    }

    case RNG_CLOSING_ITEM: {
        INVENTORY_ITEM *inv_item = ring->list[ring->current_object];
        for (int32_t frame = 0; frame < INV_RING_FRAMES; frame++) {
            if (!M_AnimateInventoryItem(inv_item)) {
                if (inv_item->object_id == O_PASSPORT_OPTION) {
                    inv_item->object_id = O_PASSPORT_CLOSED;
                    inv_item->current_frame = 0;
                }
                ring->motion.count = M_SELECTING_FRAMES;
                ring->motion.status = ring->motion.status_target;
                InvRing_MotionItemDeselect(ring, inv_item);
                break;
            }
        }
        break;
    }

    case RNG_EXITING_INVENTORY:
        if (!ring->motion.count) {
            if (M_Finish(ring, false).action != GF_NOOP) {
                // Fade to black. Do it later once reaching RNG_FADING_OUT.
                InvRing_MotionSetup(
                    ring, RNG_CLOSING, RNG_FADING_OUT, INV_RING_CLOSE_FRAMES);
            } else {
                // Fade to game. Do it as soon as the ring starts to close.
                InvRing_MotionSetup(
                    ring, RNG_CLOSING, RNG_DONE, INV_RING_CLOSE_FRAMES);
                Fader_Init(
                    &ring->back_fader, FADER_ANY, FADER_TRANSPARENT,
                    M_INV_RING_FADE_TIME_FAST);
            }
            InvRing_MotionRadius(ring, 0);
            InvRing_MotionCameraPos(ring, INV_RING_CAMERA_START_HEIGHT);
            InvRing_MotionRotation(
                ring, INV_RING_CLOSE_ROTATION,
                ring->ring_pos.rot.y - INV_RING_CLOSE_ROTATION);
        }
        break;

    default:
        break;
    }

    if (ring->motion.status == RNG_OPEN || ring->motion.status == RNG_SELECTING
        || ring->motion.status == RNG_SELECTED
        || ring->motion.status == RNG_DESELECTING
        || ring->motion.status == RNG_DESELECT
        || ring->motion.status == RNG_CLOSING_ITEM) {
        if (!ring->rotating
            && ((!g_Input.menu_left && !g_Input.menu_right)
                || ring->number_of_objects <= 1)) {
            INVENTORY_ITEM *const inv_item = ring->list[ring->current_object];
            M_RingNotActive(ring, inv_item);
        }
        M_RingIsOpen(ring);
    } else {
        M_RingIsNotOpen(ring);
    }

    if (ring->motion.status == RNG_OPENING || ring->motion.status == RNG_CLOSING
        || ring->motion.status == RNG_MAIN2OPTION
        || ring->motion.status == RNG_OPTION2MAIN
        || ring->motion.status == RNG_EXITING_INVENTORY
        || ring->motion.status == RNG_FADING_OUT
        || ring->motion.status == RNG_DONE || ring->rotating) {
        M_RingActive(ring);
    }

    Interpolation_Remember();
    return (GF_COMMAND) { .action = GF_NOOP };
}

void InvRing_RemoveAllText(void)
{
    InvRing_RemoveHeader();
    InvRing_RemoveItemTexts();
}

INV_RING *InvRing_Open(const INVENTORY_MODE mode)
{
    if (mode == INV_KEYS_MODE && g_InvRing_Source[RT_KEYS].count == 0) {
        m_InvChosen = NO_OBJECT;
        return nullptr;
    }

    g_PhdLeft = Viewport_GetMinX(VIEWPORT_GAME);
    g_PhdTop = Viewport_GetMinY(VIEWPORT_GAME);
    g_PhdBottom = Viewport_GetMaxY(VIEWPORT_GAME);
    g_PhdRight = Viewport_GetMaxX(VIEWPORT_GAME);
    m_InvChosen = NO_OBJECT;

    g_InvRing_OldCamera = g_Camera;
    m_StartLevel = -1;

    if (mode == INV_TITLE_MODE) {
        g_InvRing_Source[RT_OPTION].count = M_TITLE_RING_OBJECTS;
        InvRing_ShowVersionText();
        Savegame_ScanSavedGames();
    } else {
        g_InvRing_Source[RT_OPTION].count = M_OPTION_RING_OBJECTS;
        InvRing_RemoveVersionText();
    }

    g_InvRing_Source[RT_MAIN].current = 0;
    for (int32_t i = 0; i < g_InvRing_Source[RT_MAIN].count; i++) {
        InvRing_InitInvItem(g_InvRing_Source[RT_MAIN].items[i]);
    }
    g_InvRing_Source[RT_OPTION].current = 0;
    for (int32_t i = 0; i < g_InvRing_Source[RT_OPTION].count; i++) {
        g_InvRing_Source[RT_OPTION].qtys[i] = 1;
        InvRing_InitInvItem(g_InvRing_Source[RT_OPTION].items[i]);
    }
    if (GF_GetGymLevel() == nullptr) {
        Inv_RemoveItem(O_PHOTO_OPTION);
    }

    if (!g_Config.audio.enable_music_in_inventory && mode != INV_TITLE_MODE) {
        Music_Pause();
        Sound_PauseAll();
    }

    INV_RING *const ring = Memory_Alloc(sizeof(INV_RING));
    ring->mode = mode;

    switch (mode) {
    case INV_DEATH_MODE:
    case INV_SAVE_MODE:
    case INV_SAVE_CRYSTAL_MODE:
    case INV_LOAD_MODE:
    case INV_TITLE_MODE:
        InvRing_InitRing(
            ring, RT_OPTION, g_InvRing_Source[RT_OPTION].items,
            g_InvRing_Source[RT_OPTION].count,
            g_InvRing_Source[RT_OPTION].current);
        break;

    case INV_KEYS_MODE:
        InvRing_InitRing(
            ring, RT_KEYS, g_InvRing_Source[RT_KEYS].items,
            g_InvRing_Source[RT_KEYS].count, g_InvRing_Source[RT_MAIN].current);
        break;

    default:
        if (g_InvRing_Source[RT_MAIN].count != 0) {
            InvRing_InitRing(
                ring, RT_MAIN, g_InvRing_Source[RT_MAIN].items,
                g_InvRing_Source[RT_MAIN].count,
                g_InvRing_Source[RT_MAIN].current);
        } else {
            InvRing_InitRing(
                ring, RT_OPTION, g_InvRing_Source[RT_OPTION].items,
                g_InvRing_Source[RT_OPTION].count,
                g_InvRing_Source[RT_OPTION].current);
        }
        break;
    }

    g_Inv_Mode = mode;
    Interpolation_Remember();

    if (mode == INV_TITLE_MODE) {
        Output_LoadBackgroundFromFile(g_GameFlow.main_menu_background_path);
        Fader_Init(
            &ring->top_fader, FADER_BLACK, FADER_TRANSPARENT,
            M_INV_RING_FADE_TIME_FAST);
    } else {
        Output_UnloadBackground();
        Fader_Init(
            &ring->back_fader, FADER_TRANSPARENT, FADER_SEMI_BLACK,
            M_INV_RING_FADE_TIME_FAST);
    }

    return ring;
}

void InvRing_Close(INV_RING *const ring)
{
    InvRing_RemoveAllText();
    InvRing_RemoveVersionText();

    if (ring->list != nullptr) {
        INVENTORY_ITEM *const inv_item = ring->list[ring->current_object];
        if (inv_item != nullptr) {
            Option_Close(inv_item);
        }
    }
    if (ring->mode == INV_TITLE_MODE) {
        Music_Stop();
        Sound_StopAll();
    }
    Output_UnloadBackground();

    if (g_Config.input.enable_buffering) {
        g_OldInputDB = (INPUT_STATE) {};
    }

    m_InvChosen = NO_OBJECT;
    Memory_Free(ring);
}

GF_COMMAND InvRing_Control(INV_RING *const ring)
{
    InvRing_AdjustMusicVolume(ring);
    const GF_COMMAND gf_cmd = M_Control(ring);
    Overlay_Animate(1);
    return gf_cmd;
}

bool InvRing_IsRingAvailable(const RING_TYPE ring_type)
{
    if (ring_type == RT_OPTION && InvRing_IsOptionLockedOut()) {
        return false;
    }
    return g_InvRing_Source[ring_type].count > 0;
}

bool InvRing_IsOptionLockedOut(void)
{
    return false;
}
