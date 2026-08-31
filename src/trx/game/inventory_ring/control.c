#include <trx/game/inventory_ring/control.h>

#include <trx/config.h>
#include <trx/core/memory.h>
#include <trx/core/strings.h>
#include <trx/debug.h>
#include <trx/game/camera.h>
#include <trx/game/console.h>
#include <trx/game/cutseq/playback.h>
#include <trx/game/flyby_mode.h>
#include <trx/game/game.h>
#include <trx/game/game/control.h>
#include <trx/game/game_flow.h>
#include <trx/game/game_strings/entries.h>
#include <trx/game/gun.h>
#include <trx/game/gun/registry.h>
#include <trx/game/gym.h>
#include <trx/game/input.h>
#include <trx/game/interpolation.h>
#include <trx/game/inventory.h>
#include <trx/game/inventory_ring/priv.h>
#include <trx/game/inventory_ring/vars.h>
#include <trx/game/lara.h>
#include <trx/game/lua/events.h>
#include <trx/game/music.h>
#include <trx/game/objects.h>
#include <trx/game/objects/families.h>
#include <trx/game/objects/links.h>
#include <trx/game/option.h>
#include <trx/game/option/examine.h>
#include <trx/game/option/globe_select.h>
#include <trx/game/option/passport.h>
#include <trx/game/option/save_crystal.h>
#include <trx/game/option/stats.h>
#include <trx/game/output/overlay.h>
#include <trx/game/overlay.h>
#include <trx/game/savegame.h>
#include <trx/game/shell.h>
#include <trx/game/sound.h>
#include <trx/game/stats.h>
#include <trx/version.h>

#define M_INV_RING_FADE_TIME_FAST                                              \
    (INV_RING_CLOSE_FRAMES / INV_RING_FRAMES / (float)LOGIC_FPS)
#define M_INV_RING_FADE_TIME_TO_BLACK 0.25f
#define M_RING_SWITCH_FRAMES (96 / 2)
#define M_SELECTING_FRAMES (32 / 2)

static CLOCK_TIMER m_DemoTimer = { .type = CLOCK_TIMER_SIM };
static int32_t m_StartLevel;
static OBJECT_ID m_InvChosen = NO_OBJECT;

// The entry each ring was left on, so that it can open there again. An object
// id rather than a position: a ring is rebuilt as Lara's belongings change,
// and a position would come back pointing at something else.
static OBJECT_ID m_LastRingObject[RT_NUMBER_OF] = {
    [RT_MAIN] = NO_OBJECT,
    [RT_OPTION] = NO_OBJECT,
    [RT_KEYS] = NO_OBJECT,
    [RT_GLOBE_SELECT] = NO_OBJECT,
};
static INV_RING *m_ActiveRing = nullptr;

// Display-only filter for the rings: hidden items stay in the real
// inventory (g_InvRing_Source) so nothing else in the game (savegames, gun
// logic, Inv_GetItemCount callers) ever sees them as missing. Only what
// InvRing_Open/M_TransitionToRing show is affected.
static INVENTORY_ITEM *m_VisibleRingItems[RT_NUMBER_OF][INV_RING_MAX_ITEMS];

// Which ring an entry belongs to, which its position says: the main ring
// counts from zero, the keys from a hundred, the menu from two hundred and the
// globe from three.
static RING_TYPE M_GetRingType(const INVENTORY_ITEM *const inv_item)
{
    if (inv_item->inv_pos < 100) {
        return RT_MAIN;
    } else if (inv_item->inv_pos < 200) {
        return RT_KEYS;
    } else if (inv_item->inv_pos < 300) {
        return RT_OPTION;
    } else {
        return RT_GLOBE_SELECT;
    }
}

static void M_InsertIntoRing(INVENTORY_ITEM *const inv_item, const int32_t qty)
{
    INV_RING_SOURCE *const source = &g_InvRing_Source[M_GetRingType(inv_item)];
    if (source->count >= INV_RING_MAX_ITEMS) {
        LOG_WARNING("no room in the ring for object %d", inv_item->object_id);
        return;
    }

    int32_t n;
    for (n = 0; n < source->count; n++) {
        if (source->items[n]->inv_pos > inv_item->inv_pos) {
            break;
        }
    }
    for (int32_t i = source->count; i > n; i--) {
        source->items[i] = source->items[i - 1];
        source->qtys[i] = source->qtys[i - 1];
    }
    source->items[n] = inv_item;
    source->qtys[n] = MIN(qty, MAX_QTY);
    source->count++;
}

static bool M_IsRuntimeHidden(const OBJECT_ID object_id)
{
    return object_id == O_BINOCULARS_OPTION
        && !g_Config.gameplay.enable_binoculars;
}

static bool M_IsRingRemembered(const RING_TYPE type)
{
    switch (g_Config.gameplay.ring_memory_mode) {
    case RING_MEMORY_MAIN:
        return type == RT_MAIN;
    case RING_MEMORY_ALL:
        return type == RT_MAIN || type == RT_KEYS;
    default:
        return false;
    }
}

// Where a ring opens: the entry it was left on, where the player asked for
// that and it is still something Lara carries, and the entry the caller had in
// mind otherwise.
static int16_t M_GetStartingObject(
    const RING_TYPE type, const INV_RING_VISIBLE *const visible,
    const int16_t fallback)
{
    if (!M_IsRingRemembered(type) || m_LastRingObject[type] == NO_OBJECT) {
        return fallback;
    }
    for (int16_t i = 0; i < visible->count; i++) {
        if (visible->items[i]->object_id == m_LastRingObject[type]) {
            return i;
        }
    }
    return fallback;
}

static INV_RING_VISIBLE M_GetVisibleRing(const RING_TYPE type)
{
    const INV_RING_SOURCE *const source = &g_InvRing_Source[type];
    INVENTORY_ITEM **const dst = m_VisibleRingItems[type];
    int16_t count = 0;
    for (int16_t i = 0; i < source->count; i++) {
        if (!M_IsRuntimeHidden(source->items[i]->object_id)) {
            dst[count++] = source->items[i];
        }
    }
    return (INV_RING_VISIBLE) { .items = dst, .count = count };
}

// What Lara has for one weapon, and nothing at all where the number cannot run
// down: a weapon that never runs out has none to show.
static void M_ShowAmmoQuantity(
    const LARA_GUN_TYPE gun_type, const char *const fmt, const int32_t qty)
{
    if (!Gun_HasInfiniteAmmo(gun_type)) {
        InvRing_ShowItemQuantity(fmt, qty);
    }
}

// What a weapon has left, counted in shots rather than in the rounds behind
// them: the shotgun spends six of those on one shot. TR1 draws the icon of
// the ammunition beside the number, where the weapon has one.
static void M_ShowGunAmmoQuantity(const LARA_GUN_TYPE gun_type)
{
    const WEAPON_INFO *const info = Gun_Registry_Get(gun_type);
    const char *const icon = g_TRVersion == 1 ? info->ammo_icon : nullptr;
    char *fmt = icon != nullptr ? String_Format("%%5d %s", icon) : nullptr;
    M_ShowAmmoQuantity(
        gun_type, fmt != nullptr ? fmt : "%5d",
        Inv_GetAmmo(gun_type) / Gun_GetRoundsPerShot(gun_type));
    Memory_FreePointer(&fmt);
}

static void M_RingIsOpen(INV_RING *const ring)
{
    InvRing_ShowHeader(ring);
}

static void M_RingIsNotOpen(INV_RING *const ring)
{
    InvRing_RemoveHeader();
    InvRing_ShowExamine(NO_OBJECT, false);
}

static void M_RingNotActive(
    const INV_RING *const ring, const INVENTORY_ITEM *const inv_item)
{
    InvRing_ShowItemName(inv_item);

    const LARA_INFO *const lara = Lara_GetLaraInfo();
    const int32_t qty = Inv_GetItemCount(inv_item->object_id);

    const LARA_GUN_TYPE gun_type =
        Gun_GetType(Inv_GetItemPickup(inv_item->object_id));
    if (gun_type != LGT_UNARMED) {
        M_ShowGunAmmoQuantity(gun_type);
    }

    switch (inv_item->object_id) {
    case O_PISTOLS_AMMO_OPTION:
    case O_SHOTGUN_AMMO_OPTION:
    case O_MAGNUMS_AMMO_OPTION:
    case O_AUTOS_AMMO_OPTION:
    case O_DESERT_EAGLE_AMMO_OPTION:
    case O_UZIS_AMMO_OPTION:
    case O_HARPOON_GUN_AMMO_OPTION:
    case O_M16_AMMO_OPTION:
    case O_MP5_AMMO_OPTION:
    case O_GRENADE_AMMO_OPTION:
    case O_ROCKET_AMMO_OPTION: {
        const OBJECT_ID ammo_object_id = Inv_GetItemPickup(inv_item->object_id);
        const LARA_GUN_TYPE ammo_gun_type = Gun_GetType(
            ObjectLink_GetInverse(ammo_object_id, OBJ_LINK_GUN_TO_AMMO));
        M_ShowAmmoQuantity(
            ammo_gun_type, "%d",
            qty * Gun_GetAmmoInventoryQuantity(ammo_gun_type));
        break;
    }

    case O_FLARES_BOX_OPTION:
        M_ShowAmmoQuantity(Gun_GetFlareType(), "%d", qty);
        break;

    case O_SMALL_MEDIPACK_OPTION:
    case O_LARGE_MEDIPACK_OPTION:
        Overlay_ForceHealthBar(true);
        if (qty > 1) {
            InvRing_ShowItemQuantity("%d", qty);
        }
        break;

    case O_SAVE_CRYSTAL_OPTION:
        if (qty > 1) {
            InvRing_ShowItemQuantity("%d", qty);
        }
        break;

    default:
        if (inv_item->object_id == O_SCION_OPTION
            || ObjectFamily_Has(
                inv_item->object_id, OBJ_FAMILY_GENERIC_INV_OPTION)) {
            if (qty > 1) {
                InvRing_ShowItemQuantity("%d", qty);
            }
        }
        break;
    }

    InvRing_ShowExamine(
        inv_item->object_id,
        ring->status == RNG_OPEN
            && Option_Examine_CanExamine(inv_item->object_id));
}

static void M_RingActive(void)
{
    InvRing_RemoveItemTexts();
    InvRing_ShowExamine(NO_OBJECT, false);
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

    if (ring->mode == INV_GLOBE_SELECT_MODE) {
        if (ring->globe_select.confirmed && ring->globe_select.selection >= 0
            && ring->globe_select.selection < MAX_GLOBE_ZONES) {
            const int32_t start_level_num =
                ring->globe_select
                    .start_level_num[ring->globe_select.selection];
            if (start_level_num >= 0) {
                return (GF_COMMAND) {
                    .action = GF_START_GAME,
                    .param = start_level_num,
                };
            }
        }
        return (GF_COMMAND) { .action = GF_EXIT_TO_TITLE };
    }

    if (m_StartLevel != -1) {
        return (GF_COMMAND) {
            .action = GF_SELECT_GAME,
            .param = m_StartLevel,
        };
    }

    if (Shell_IsExiting()) {
        return (GF_COMMAND) { .action = GF_EXIT_GAME };
    } else if (GF_GetOverrideCommand().action != GF_NOOP) {
        return GF_GetOverrideCommand();
    } else if (ring->is_demo_needed) {
        return (GF_COMMAND) { .action = GF_START_DEMO, .param = -1 };
    }

    switch (m_InvChosen) {
    case O_PASSPORT_OPTION:
        switch (g_Passport.select_action) {
        case PASSPORT_ACTION_LOAD_GAME: {
            if (apply_changes) {
                Inv_RemoveAllItems();
            }
            return (GF_COMMAND) {
                .action = GF_START_SAVED_GAME,
                .param = SG_Manager_SlotToParam(g_Passport.select_save_slot),
            };
        }

        case PASSPORT_ACTION_NEW_GAME:
            if (apply_changes) {
                SG_Resume_ResetAllEntries();
            }
            SG_Manager_UnbindSlot();
            return (GF_COMMAND) {
                .action = GF_START_GAME,
                .param = g_Passport.select_level,
            };

        case PASSPORT_ACTION_SWITCH_MOD:
            return (GF_COMMAND) { .action = GF_SWITCH_MOD };

        case PASSPORT_ACTION_SAVE_GAME: {
            if (apply_changes) {
                Savegame_Save(g_Passport.select_save_slot);
            }
            return (GF_COMMAND) { .action = GF_NOOP };
        }

        case PASSPORT_ACTION_RESTART:
            return (GF_COMMAND) {
                .action = GF_RESTART_GAME,
                .param = Game_GetCurrentLevel()->num,
            };

        case PASSPORT_ACTION_EXIT_TO_TITLE:
            return (GF_COMMAND) { .action = GF_EXIT_TO_TITLE };

        case PASSPORT_ACTION_EXIT_GAME:
            return (GF_COMMAND) { .action = GF_EXIT_GAME };

        case PASSPORT_ACTION_SELECT_LEVEL:
            return (GF_COMMAND) {
                .action = GF_SELECT_GAME,
                .param = g_Passport.select_level,
            };

        case PASSPORT_ACTION_GLOBE_SELECT:
            return (GF_COMMAND) {
                .action = GF_GLOBE_SELECT,
                .param = g_Passport.select_level,
            };

        case PASSPORT_ACTION_STORY_SO_FAR:
            return (GF_COMMAND) {
                .action = GF_STORY_SO_FAR,
                .param = SG_Manager_SlotToParam(g_Passport.select_save_slot),
            };
        }
        break;

    case O_SAVE_CRYSTAL_OPTION:
        if (apply_changes) {
            Option_SaveCrystal_CommitSave();
        }
        break;

    case O_PHOTO_OPTION:
        if (apply_changes) {
            SG_Manager_UnbindSlot();
        }
        if (GF_GetGymLevel() != nullptr) {
            return (GF_COMMAND) {
                .action = GF_START_GAME,
                .param = GF_GetGymLevel()->num,
            };
        }
        break;

    default:
        // Anything else is handled (or ignored) by Lara_UseItem itself.
        if (apply_changes) {
            Lara_UseItem(m_InvChosen);
        }
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

    if (ring->mode != INV_TITLE_MODE || InputState_IsAnyPressed(g_Input)
        || InputState_IsAnyPressed(g_InputDB) || Console_IsOpened()) {
        ClockTimer_Sync(&m_DemoTimer);
        return false;
    }

    return ring->status == RNG_OPEN
        && ClockTimer_CheckElapsed(&m_DemoTimer, g_Config.flow.demo_delay);
}

static void M_SetupRingSwitchClose(
    INV_RING *const ring, const RING_STATUS status_target)
{
    InvRing_SetStatusTransition(
        ring, RNG_CLOSING, status_target, M_RING_SWITCH_FRAMES / 2);
}

static void M_TransitionToRing(
    INV_RING *const ring, const RING_TYPE source_type,
    const RING_TYPE target_type)
{
    // The real count never changes while the ring is being browsed, so
    // g_InvRing_Source[source_type].count doesn't need writing back; doing
    // so would clobber it with a filtered (possibly smaller) count.
    g_InvRing_Source[source_type].current = ring->current_object;
    ring->type = target_type;
    const INV_RING_VISIBLE visible = M_GetVisibleRing(target_type);
    ring->list = visible.items;
    ring->number_of_objects = visible.count;
    ring->current_object = g_InvRing_Source[target_type].current;
    InvRing_SetStatusTransition(
        ring, RNG_OPENING, RNG_OPEN, M_RING_SWITCH_FRAMES / 2);
}

static void M_SnapshotRingState(INV_RING *const ring)
{
    ring->prev_radius = ring->radius;
    ring->prev_camera_y = ring->camera.pos.y;
    ring->prev_camera_pitch = ring->camera_pitch;
    ring->prev_ring_rot_y = ring->ring_pos.rot.y;
}

static void M_SnapshotItemState(INVENTORY_ITEM *const inv_item)
{
    inv_item->prev_x_rot_pt = inv_item->x_rot_pt;
    inv_item->prev_x_rot = inv_item->x_rot;
    inv_item->prev_y_rot = inv_item->y_rot;
    inv_item->prev_y_trans = inv_item->y_trans;
    inv_item->prev_z_trans = inv_item->z_trans;
    inv_item->prev_manual_rot = inv_item->manual_rot;
}

static void M_SnapshotFrameState(INV_RING *const ring)
{
    M_SnapshotRingState(ring);
    for (int32_t i = 0; i < ring->number_of_objects; i++) {
        M_SnapshotItemState(ring->list[i]);
    }
}

// A minimal simulation tick keeping the title level alive behind the menu:
// the world, the camera it plays through and its actors - no player input,
// no HUD.
static void M_SimTick(void)
{
    Interpolation_Remember();
    Game_TickBeginFrame();
    Sound_ResetAmbient();
    Game_TickWorld();
    Game_TickPostControl();
    Game_TickEndFrame();
}

static GF_COMMAND M_Control(INV_RING *const ring)
{
    if (ring->status == RNG_OPENING) {
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

    if (ring->status == RNG_FADING_OUT) {
        if (ring->mode == INV_TITLE_MODE) {
            const GF_COMMAND gf_cmd = M_Finish(ring, true);
            ring->is_done = true;
            ring->status = RNG_DONE;
            return gf_cmd;
        }

        if (!Fader_IsActive(&ring->back_fader)
            && !Fader_IsActive(&ring->top_fader)) {
            Fader_InitFromCurrentHold(
                &ring->top_fader, 1.0f, M_INV_RING_FADE_TIME_TO_BLACK,
                1.0f / (float)LOGIC_FPS);
        }

        if (Fader_IsActive(&ring->top_fader)
            || Fader_IsActive(&ring->back_fader)) {
            return (GF_COMMAND) { .action = GF_NOOP };
        }
        ring->status = RNG_DONE;
    }

    if (ring->status == RNG_DONE && !ring->is_done) {
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

    if (ring->mode == INV_GLOBE_SELECT_MODE) {
        m_StartLevel = -1;
    } else {
        m_StartLevel = Game_IsLevelComplete() ? g_Passport.select_level : -1;
    }

    if (g_Config.gameplay.enable_timer_in_inventory
        && !(Game_IsInGym() && Gym_TrackManager_HasStats(GYM_TRACK_ASSAULT))) {
        Stats_UpdateTimer();
    }

    if (Shell_IsExiting()) {
        return (GF_COMMAND) { .action = GF_EXIT_GAME };
    }

    if ((ring->mode == INV_SAVE_MODE || ring->mode == INV_SAVE_CRYSTAL_MODE
         || ring->mode == INV_LOAD_MODE || ring->mode == INV_DEATH_MODE
         || ring->mode == INV_GLOBE_SELECT_MODE)
        && !ring->is_pass_open) {
        g_Input = (INPUT_STATE) {};
        g_InputDB = (INPUT_STATE) { .menu_confirm = 1 };
    }

    if (ring->mode != INV_TITLE_MODE && !Fader_IsActive(&ring->back_fader)
        && !Fader_IsActive(&ring->top_fader) && ring->status != RNG_OPENING) {
        for (int i = 0; i < ring->number_of_objects; i++) {
            INVENTORY_ITEM *const inv_item = ring->list[i];
            if (inv_item->object_id == O_COMPASS_OPTION) {
                Option_Stats_UpdateCompassNeedle(inv_item);
            }
        }
    }

    if (ring->rotating) {
        return (GF_COMMAND) { .action = GF_NOOP };
    }

    switch (ring->status) {
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
            || (g_InputDB.menu_back && ring->mode != INV_TITLE_MODE
                && ring->mode != INV_GLOBE_SELECT_MODE)) {
            Sound_Effect(SFX_MENU_SPINOUT, nullptr, SPM_ALWAYS);
            m_InvChosen = NO_OBJECT;

            if (ring->type == RT_MAIN) {
                g_InvRing_Source[RT_MAIN].current = ring->current_object;
            } else if (ring->type != RT_NUMBER_OF) {
                g_InvRing_Source[ring->type].current = ring->current_object;
            }

            if (M_Finish(ring, false).action != GF_NOOP) {
                InvRing_SetStatusTransition(
                    ring, RNG_CLOSING, RNG_FADING_OUT, INV_RING_CLOSE_FRAMES);
            } else {
                InvRing_SetStatusTransition(
                    ring, RNG_CLOSING, RNG_DONE, INV_RING_CLOSE_FRAMES);
                if (g_Config.visuals.enable_fade_effects
                    && g_Config.ui.inventory_fade_effects) {
                    Fader_InitFromCurrent(
                        &ring->back_fader, 0.0f, M_INV_RING_FADE_TIME_FAST);
                }
            }

            g_Input = (INPUT_STATE) {};
            g_InputDB = (INPUT_STATE) {};
        }

        const bool examine = g_InputDB.menu_show_info && InvRing_CanExamine();
        if (g_InputDB.menu_confirm || examine) {
            if ((ring->mode == INV_SAVE_MODE
                 || ring->mode == INV_SAVE_CRYSTAL_MODE
                 || ring->mode == INV_LOAD_MODE || ring->mode == INV_DEATH_MODE
                 || ring->mode == INV_GLOBE_SELECT_MODE)
                && !ring->is_pass_open) {
                ring->is_pass_open = true;
            }

            g_InvRing_Source[ring->type].current = ring->current_object;
            INVENTORY_ITEM *const inv_item = ring->list[ring->current_object];

            if (examine) {
                inv_item->action = ACTION_EXAMINE;
                inv_item->goal_frame = 0;
                inv_item->anim_direction = 1;
            } else {
                inv_item->action = ACTION_USE;
                inv_item->goal_frame = inv_item->open_frame;
                inv_item->anim_direction = 1;
            }
            InvRing_SetStatusTransition(
                ring, RNG_SELECTING, RNG_SELECTED, M_SELECTING_FRAMES);
            g_Input = (INPUT_STATE) {};
            g_InputDB = (INPUT_STATE) {};

            switch (inv_item->object_id) {
            case O_COMPASS_OPTION:
                Sound_Effect(SFX_MENU_COMPASS, nullptr, SPM_ALWAYS);
                break;

            case O_STOPWATCH_OPTION:
                break;

            case O_PHOTO_OPTION:
                Sound_Effect(SFX_MENU_LARA_HOME, nullptr, SPM_ALWAYS);
                break;

            case O_CONTROLS_OPTION:
                Sound_Effect(
                    g_TRVersion == 1 ? SFX_MENU_GAMEBOY : SFX_MENU_CHOOSE,
                    nullptr, SPM_ALWAYS);
                break;

            case O_PISTOLS_OPTION:
            case O_SHOTGUN_OPTION:
            case O_MAGNUMS_OPTION:
            case O_AUTOS_OPTION:
            case O_DESERT_EAGLE_OPTION:
            case O_UZIS_OPTION:
            case O_HARPOON_GUN_OPTION:
            case O_M16_OPTION:
            case O_MP5_OPTION:
            case O_GRENADE_GUN_OPTION:
            case O_ROCKET_GUN_OPTION:
                Sound_Effect(SFX_MENU_GUNS, nullptr, SPM_ALWAYS);
                break;

            default:
                Sound_Effect(SFX_MENU_CHOOSE, nullptr, SPM_ALWAYS);
                break;
            }
        }

        if (g_InputDB.menu_up && ring->mode != INV_TITLE_MODE
            && ring->mode != INV_KEYS_MODE
            && ring->mode != INV_GLOBE_SELECT_MODE) {
            if (ring->type == RT_MAIN) {
                if (InvRing_IsRingAvailable(RT_KEYS)) {
                    M_SetupRingSwitchClose(ring, RNG_MAIN2KEYS);
                }
                g_Input = (INPUT_STATE) {};
                g_InputDB = (INPUT_STATE) {};
            } else if (ring->type == RT_OPTION) {
                if (InvRing_IsRingAvailable(RT_MAIN)) {
                    M_SetupRingSwitchClose(ring, RNG_OPTION2MAIN);
                }
                g_InputDB = (INPUT_STATE) {};
            }
        } else if (
            g_InputDB.menu_down && ring->mode != INV_TITLE_MODE
            && ring->mode != INV_KEYS_MODE
            && ring->mode != INV_GLOBE_SELECT_MODE) {
            if (ring->type == RT_MAIN) {
                if (InvRing_IsRingAvailable(RT_OPTION)) {
                    M_SetupRingSwitchClose(ring, RNG_MAIN2OPTION);
                }
                g_InputDB = (INPUT_STATE) {};
            } else if (ring->type == RT_KEYS) {
                if (InvRing_IsRingAvailable(RT_MAIN)) {
                    M_SetupRingSwitchClose(ring, RNG_KEYS2MAIN);
                }
                g_Input = (INPUT_STATE) {};
                g_InputDB = (INPUT_STATE) {};
            }
        }
        break;

    case RNG_MAIN2OPTION:
        M_TransitionToRing(ring, RT_MAIN, RT_OPTION);
        break;

    case RNG_MAIN2KEYS:
        M_TransitionToRing(ring, RT_MAIN, RT_KEYS);
        break;

    case RNG_KEYS2MAIN:
        M_TransitionToRing(ring, RT_KEYS, RT_MAIN);
        break;

    case RNG_OPTION2MAIN:
        M_TransitionToRing(ring, RT_OPTION, RT_MAIN);
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
            if (g_InputDB.menu_back && ring->mode != INV_GLOBE_SELECT_MODE) {
                InvRing_SetStatusTransition(
                    ring, RNG_CLOSING_ITEM, RNG_DESELECT, 0);
                g_Input = (INPUT_STATE) {};
                g_InputDB = (INPUT_STATE) {};
                if (ring->mode == INV_LOAD_MODE || ring->mode == INV_SAVE_MODE
                    || ring->mode == INV_SAVE_CRYSTAL_MODE) {
                    InvRing_SetStatusTransition(
                        ring, RNG_CLOSING_ITEM, RNG_EXITING_INVENTORY, 0);
                    g_Input = (INPUT_STATE) {};
                    g_InputDB = (INPUT_STATE) {};
                }
            }

            if (g_InputDB.menu_confirm) {
                m_InvChosen = inv_item->object_id;
                if (ring->type == RT_MAIN) {
                    g_InvRing_Source[RT_MAIN].current = ring->current_object;
                } else if (ring->type != RT_NUMBER_OF) {
                    g_InvRing_Source[ring->type].current = ring->current_object;
                }

                if (ring->mode == INV_TITLE_MODE
                    && (inv_item->object_id == O_GRAPHICS_OPTION
                        || inv_item->object_id == O_SOUND_OPTION
                        || inv_item->object_id == O_PDA_OPTION
                        || inv_item->object_id == O_CONTROLS_OPTION
                        || inv_item->object_id == O_GLOBE_OPTION)) {
                    InvRing_SetStatusTransition(
                        ring, RNG_CLOSING_ITEM, RNG_DESELECT, 0);
                } else {
                    InvRing_SetStatusTransition(
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
        InvRing_SetStatusTransition(
            ring, RNG_DESELECTING, RNG_OPEN, M_SELECTING_FRAMES);
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

                InvRing_SetStatusTransition(
                    ring, ring->status_target, ring->status_target,
                    M_SELECTING_FRAMES);
                break;
            }
        }
        break;
    }

    case RNG_EXITING_INVENTORY:
        if (ring->status_frames == 0) {
            if (M_Finish(ring, false).action != GF_NOOP) {
                // Fade to black. Do it later once reaching RNG_FADING_OUT.
                InvRing_SetStatusTransition(
                    ring, RNG_CLOSING, RNG_FADING_OUT, INV_RING_CLOSE_FRAMES);
            } else {
                // Fade to game. Do it as soon as the ring starts to close.
                InvRing_SetStatusTransition(
                    ring, RNG_CLOSING, RNG_DONE, INV_RING_CLOSE_FRAMES);
                if (g_Config.visuals.enable_fade_effects
                    && g_Config.ui.inventory_fade_effects) {
                    Fader_InitFromCurrent(
                        &ring->back_fader, 0.0f, M_INV_RING_FADE_TIME_FAST);
                }
            }
        }
        break;

    default:
        break;
    }

    if (ring->status == RNG_OPEN || ring->status == RNG_SELECTING
        || ring->status == RNG_SELECTED || ring->status == RNG_DESELECTING
        || ring->status == RNG_DESELECT || ring->status == RNG_CLOSING_ITEM) {
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

    if (ring->status == RNG_OPENING || ring->status == RNG_CLOSING
        || ring->status == RNG_MAIN2OPTION || ring->status == RNG_OPTION2MAIN
        || ring->status == RNG_EXITING_INVENTORY
        || ring->status == RNG_FADING_OUT || ring->status == RNG_DONE
        || ring->rotating) {
        M_RingActive();
    }

    Interpolation_Remember();
    return (GF_COMMAND) { .action = GF_NOOP };
}

INV_RING *InvRing_GetActiveRing(void)
{
    return m_ActiveRing;
}

void InvRing_RemoveAllText(void)
{
    InvRing_RemoveHeader();
    InvRing_RemoveItemTexts();
    InvRing_ClearButtonHint();
}

INV_RING *InvRing_Open(const INVENTORY_MODE mode)
{
    if (mode == INV_KEYS_MODE && !InvRing_IsRingAvailable(RT_KEYS)) {
        m_InvChosen = NO_OBJECT;
        return nullptr;
    }

    m_InvChosen = NO_OBJECT;

    g_InvRing_OldCamera = g_Camera;
    m_StartLevel = -1;

    if (mode == INV_TITLE_MODE) {
        InvRing_ShowVersionText();
        SG_Manager_ScanSavedGames();
    } else {
        InvRing_RemoveVersionText();
    }

    if (mode != INV_GLOBE_SELECT_MODE) {
        // Reset option ring
        g_InvRing_Source[RT_OPTION].count = 0;
        InvRing_InsertItem(
            InvRing_GetByObjectID(O_PASSPORT_CLOSED) != nullptr
                ? InvRing_GetByObjectID(O_PASSPORT_CLOSED)
                : InvRing_GetByObjectID(O_PASSPORT_OPTION));
        if (g_TRVersion == 1) {
            InvRing_InsertItem(InvRing_GetByObjectID(O_CONTROLS_OPTION));
            InvRing_InsertItem(InvRing_GetByObjectID(O_SOUND_OPTION));
            InvRing_InsertItem(InvRing_GetByObjectID(O_GRAPHICS_OPTION));
        } else {
            InvRing_InsertItem(InvRing_GetByObjectID(O_GRAPHICS_OPTION));
            InvRing_InsertItem(InvRing_GetByObjectID(O_CONTROLS_OPTION));
            InvRing_InsertItem(InvRing_GetByObjectID(O_SOUND_OPTION));
        }
        InvRing_InsertItem(InvRing_GetByObjectID(O_PDA_OPTION));
        if (mode == INV_TITLE_MODE && GF_GetGymLevel() != nullptr) {
            InvRing_InsertItem(InvRing_GetByObjectID(O_PHOTO_OPTION));
        }
    } else if (g_InvRing_Source[RT_GLOBE_SELECT].count == 0) {
        INVENTORY_ITEM *const globe = InvRing_GetByObjectID(O_GLOBE_OPTION);
        if (globe != nullptr) {
            InvRing_InsertItem(globe);
        }
    }

    // Sending the keys ring back to its first entry and opening it where it
    // was left are at odds, and the memory is the one the player asked for.
    if (g_Config.gameplay.fix_item_duplication_glitch
        && !M_IsRingRemembered(RT_KEYS)) {
        g_InvRing_Source[RT_KEYS].current = 0;
    }
    for (int32_t i = 0; i < g_InvRing_Source[RT_KEYS].count; i++) {
        InvRing_InitInvItem(g_InvRing_Source[RT_KEYS].items[i]);
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

    g_InvRing_Source[RT_GLOBE_SELECT].current = 0;
    for (int32_t i = 0; i < g_InvRing_Source[RT_GLOBE_SELECT].count; i++) {
        g_InvRing_Source[RT_GLOBE_SELECT].qtys[i] = 1;
        InvRing_InitInvItem(g_InvRing_Source[RT_GLOBE_SELECT].items[i]);
    }

    if (mode == INV_TITLE_MODE && GF_GetGymLevel() != nullptr
        && Gym_IsInventoryOpenEnabled()) {
        for (int32_t i = 0; i < g_InvRing_Source[RT_OPTION].count; i++) {
            if (g_InvRing_Source[RT_OPTION].items[i]->object_id
                == O_PHOTO_OPTION) {
                g_InvRing_Source[RT_OPTION].current = i;
            }
        }
    }

    if (!g_Config.audio.enable_music_in_inventory && mode != INV_TITLE_MODE) {
        Music_Pause();
        Sound_PauseAll();
    }

    INV_RING *const ring = Memory_Alloc(sizeof(INV_RING));
    ring->mode = mode;
    // A title with no picture to show runs its level live behind the menu
    // instead. What plays there is the title script's business.
    ring->live_scene =
        mode == INV_TITLE_MODE && g_GameFlow.main_menu_use_live_scene;
    ring->background_style = mode != INV_TITLE_MODE
        ? g_Config.ui.inventory_background_style
        : (ring->live_scene ? BK_NONE : BK_IMAGE);
    // main_menu_background_path is the title screen's background image;
    // there is no separate configurable image for in-game inventory modes,
    // so BK_IMAGE outside of INV_TITLE_MODE falls back to no image rather
    // than incorrectly showing the title screen background mid-game.
    ring->background_path =
        mode == INV_TITLE_MODE && ring->background_style == BK_IMAGE
        ? g_GameFlow.main_menu_background_path
        : nullptr;

    switch (mode) {
    case INV_GLOBE_SELECT_MODE: {
        ring->background_style = BK_NONE;
        ring->background_path = nullptr;
        const INV_RING_VISIBLE visible = M_GetVisibleRing(RT_GLOBE_SELECT);
        InvRing_InitRing(
            ring, RT_GLOBE_SELECT, &visible,
            g_InvRing_Source[RT_GLOBE_SELECT].current);
        Option_GlobeSelect_UpdateSelectable(ring);
        break;
    }

    case INV_TITLE_MODE:
    case INV_SAVE_MODE:
    case INV_SAVE_CRYSTAL_MODE:
    case INV_LOAD_MODE:
    case INV_DEATH_MODE: {
        const INV_RING_VISIBLE visible = M_GetVisibleRing(RT_OPTION);
        InvRing_InitRing(
            ring, RT_OPTION, &visible, g_InvRing_Source[RT_OPTION].current);
        break;
    }

    case INV_KEYS_MODE: {
        const INV_RING_VISIBLE visible = M_GetVisibleRing(RT_KEYS);
        InvRing_InitRing(
            ring, RT_KEYS, &visible,
            M_GetStartingObject(
                RT_KEYS, &visible, g_InvRing_Source[RT_MAIN].current));
        break;
    }

    default: {
        const INV_RING_VISIBLE main_visible = M_GetVisibleRing(RT_MAIN);
        if (main_visible.count > 0) {
            InvRing_InitRing(
                ring, RT_MAIN, &main_visible,
                M_GetStartingObject(
                    RT_MAIN, &main_visible, g_InvRing_Source[RT_MAIN].current));
        } else {
            const INV_RING_VISIBLE option_visible = M_GetVisibleRing(RT_OPTION);
            InvRing_InitRing(
                ring, RT_OPTION, &option_visible,
                g_InvRing_Source[RT_OPTION].current);
        }
        break;
    }
    }

    g_InvRing_Mode = mode;

    if (mode == INV_TITLE_MODE) {
        Camera_Initialise();
        LUA_FireEvent(LUA_EVENT_TITLE_START);
    }

    Interpolation_Remember();

    if (mode == INV_TITLE_MODE) {
        if (ring->background_path != nullptr) {
            Output_Overlay_LoadImage(ring->background_path);
        }
        Fader_InitTo(&ring->top_fader, 1.0f, 0.0f, M_INV_RING_FADE_TIME_FAST);
    } else {
        Fader_InitTo(&ring->back_fader, 0.0f, 1.0f, M_INV_RING_FADE_TIME_FAST);
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
            if (ring->type < RT_NUMBER_OF) {
                m_LastRingObject[ring->type] = inv_item->object_id;
            }
        }
    }
    if (ring->mode == INV_TITLE_MODE) {
        Music_Stop();
        Sound_StopAll();
    }
    // Neither a cutscene nor a flyby may outlive the menu they played behind.
    // In this order: dropping the cutscene fires its end event, and a script
    // that answers one by starting a flyby - as the TR4 title does - would
    // otherwise leave that sequence holding the camera.
    if (ring->live_scene) {
        CutSeq_Reset();
        if (FlybyMode_IsActive()) {
            FlybyMode_Deactivate();
        }
    }

    if (g_Config.input.enable_buffering_inventory) {
        g_OldInputDB = (INPUT_STATE) {};
    }

    m_InvChosen = NO_OBJECT;
    Memory_Free(ring);
}

GF_COMMAND InvRing_Control(INV_RING *const ring)
{
    if (ring->live_scene) {
        M_SimTick();
    }
    InvRing_AdjustMusicVolume(ring);
    m_ActiveRing = ring;
    INVENTORY_ITEM **const prev_list = ring->list;
    M_SnapshotFrameState(ring);
    GF_COMMAND gf_cmd = M_Control(ring);

    if (ring->status != RNG_OPENING && ring->status != RNG_DONE
        && ring->status != RNG_FADING_OUT) {
        for (int32_t frame = 0; frame < INV_RING_FRAMES; frame++) {
            for (int32_t i = 0; i < ring->number_of_objects; i++) {
                InvRing_UpdateInventoryItem(ring, ring->list[i]);
            }
        }
    }

    if (ring->status != RNG_DONE
        && (ring->status != RNG_OPENING
            || (ring->mode != INV_TITLE_MODE
                || (!Fader_IsActive(&ring->top_fader)
                    && !Fader_IsActive(&ring->back_fader))))) {
        for (int32_t frame = 0; frame < INV_RING_FRAMES; frame++) {
            InvRing_DoMotions(ring);
        }
    }

    if (ring->list != prev_list) {
        M_SnapshotFrameState(ring);
    }

    // Running motions in control can reach RNG_DONE in this same tick.
    // Finalize immediately so phase code receives the non-NOOP GF command.
    if (gf_cmd.action == GF_NOOP && ring->status == RNG_DONE
        && !ring->is_done) {
        gf_cmd = M_Control(ring);
    }

    m_ActiveRing = nullptr;
    Overlay_Animate(1);
    return gf_cmd;
}

bool InvRing_IsRingAvailable(const RING_TYPE ring_type)
{
    if (ring_type == RT_OPTION && InvRing_IsOptionLockedOut()) {
        return false;
    }
    return M_GetVisibleRing(ring_type).count > 0;
}

bool InvRing_IsOptionLockedOut(void)
{
    return g_Config.flow.lockout_option_ring;
}

INVENTORY_ITEM *InvRing_GetByObjectID(const OBJECT_ID object_id)
{
    for (int32_t i = 0; i < g_InvRing_Items->count; i++) {
        INVENTORY_ITEM *const item =
            *(INVENTORY_ITEM **)Vector_Get(g_InvRing_Items, i);
        if (item->object_id == object_id) {
            return item;
        }
    }
    return nullptr;
}

void InvRing_Rebuild(void)
{
    INVENTORY_ENTRY entries[INV_MAX_ENTRIES];
    const int32_t count = Inv_GetDrawnEntries(entries, INV_MAX_ENTRIES);

    for (RING_TYPE ring_type = 0; ring_type < RT_NUMBER_OF; ring_type++) {
        if (ring_type != RT_OPTION) {
            g_InvRing_Source[ring_type].count = 0;
        }
    }

    for (int32_t i = 0; i < count; i++) {
        INVENTORY_ITEM *const inv_item =
            InvRing_GetByObjectID(entries[i].object_id);
        if (inv_item != nullptr && M_GetRingType(inv_item) != RT_OPTION) {
            M_InsertIntoRing(inv_item, entries[i].qty);
        }
    }
}

void InvRing_InsertItem(INVENTORY_ITEM *const inv_item)
{
    ASSERT(inv_item != nullptr);
    M_InsertIntoRing(inv_item, 1);
}

void InvRing_NotifyRemoved(const OBJECT_ID object_id)
{
    if (!g_Config.gameplay.fix_item_duplication_glitch) {
        return;
    }
    for (RING_TYPE ring_type = 0; ring_type < RT_NUMBER_OF; ring_type++) {
        INV_RING_SOURCE *const source = &g_InvRing_Source[ring_type];
        for (int32_t i = 0; i < source->count; i++) {
            if (source->items[i]->object_id != object_id) {
                continue;
            }
            if (source->current >= i) {
                source->current = 0;
            }
            return;
        }
    }
}

void InvRing_ClearSelection(void)
{
    g_InvRing_Source[RT_MAIN].current = 0;
    g_InvRing_Source[RT_KEYS].current = 0;
}

void InvRing_ForgetLastEntries(void)
{
    for (RING_TYPE type = 0; type < RT_NUMBER_OF; type++) {
        m_LastRingObject[type] = NO_OBJECT;
    }
}
