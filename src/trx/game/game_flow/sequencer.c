#include <trx/game/game_flow/sequencer.h>

#include <trx/core/enum_map.h>
#include <trx/debug.h>
#include <trx/game/game.h>
#include <trx/game/game_flow/sequencer_events.h>
#include <trx/game/inventory.h>
#include <trx/game/lara/common.h>
#include <trx/game/level.h>
#include <trx/game/lua.h>
#include <trx/game/output.h>
#include <trx/game/rooms.h>
#include <trx/game/savegame.h>

static RESULT M_RunEvent(
    const GF_LEVEL *const level, const GF_SEQUENCE *const sequence,
    const int32_t event_idx, const GF_SEQUENCE_CONTEXT seq_ctx,
    void *const seq_ctx_arg, GF_COMMAND *const out_cmd)
{
    GF_COMMAND gf_cmd = { .action = GF_NOOP };
    const GF_SEQUENCE_EVENT *const event = &sequence->events[event_idx];
    LOG_DEBUG(
        "event type=%s(%d) data=0x%x",
        ENUM_MAP_TO_STRING(GF_SEQUENCE_EVENT_TYPE, event->type), event->type,
        event->data);

    const GF_SEQUENCE_EVENT_HANDLER event_handler =
        GF_GetSequenceEventHandler(event->type);
    if (event_handler == nullptr) {
        *out_cmd = gf_cmd;
        return OK;
    }

    MUST(event_handler(
        level, sequence, event_idx, seq_ctx, seq_ctx_arg, &gf_cmd));
    LOG_DEBUG(
        "event type=%s(%d) data=0x%x finished, result: action=%s, "
        "param=%d",
        ENUM_MAP_TO_STRING(GF_SEQUENCE_EVENT_TYPE, event->type), event->type,
        event->data, ENUM_MAP_TO_STRING(GF_ACTION, gf_cmd.action),
        gf_cmd.param);
    *out_cmd = gf_cmd;
    return OK;
}

// Events whose effects the level loader bakes into static data (e.g. the
// skybox mesh bakes the horizon setup) are dispatched before the level file
// is loaded and skipped in the main sequence loop.
static bool M_IsPreLoadEvent(const GF_SEQUENCE_EVENT_TYPE type)
{
    return type == GFS_SETUP_HORIZON || type == GFS_SETUP_UV_ROTATE;
}

static GF_SEQUENCE_CONTEXT M_SwitchSequenceContext(
    const GF_SEQUENCE_EVENT *const event, const GF_SEQUENCE_CONTEXT seq_ctx)
{
    // Update sequence context if necessary
    if (event->type != GFS_LOOP_GAME) {
        return seq_ctx;
    }
    switch (seq_ctx) {
    case GFSC_SAVED:
    case GFSC_RESTART:
    case GFSC_SELECT:
        return GFSC_NORMAL;
    default:
        return seq_ctx;
    }
}

static const GF_LEVEL *M_GetCanonicalNextLevel(const GF_LEVEL *const level)
{
    // Canonical order is still used for console-driven linear simulation.
    return GF_GetLevelAfter(level);
}

static const GF_LEVEL *M_GetLinkedPrevLevel(const GF_LEVEL *const level)
{
    RESUME_INFO *const resume = SG_Resume_GetEntry(level);
    if (resume == nullptr) {
        return nullptr;
    }
    if (resume->prev_level == -1) {
        return nullptr;
    }
    return GF_GetLevel(GFLT_MAIN, resume->prev_level);
}

static bool M_IsLevelDescendantOf(
    const GF_LEVEL *const level, const int32_t ancestor_level_num)
{
    RESUME_INFO *const resume = SG_Resume_GetEntry(level);
    if (resume == nullptr) {
        return false;
    }

    const int32_t count = GF_GetLevelTable(GFLT_MAIN)->count;
    int32_t current_prev = resume->prev_level;
    for (int32_t i = 0; i < count && current_prev != -1; i++) {
        if (current_prev == ancestor_level_num) {
            return true;
        }
        const GF_LEVEL *const prev_level = GF_GetLevel(GFLT_MAIN, current_prev);
        if (prev_level == nullptr) {
            break;
        }
        RESUME_INFO *const prev_resume = SG_Resume_GetEntry(prev_level);
        if (prev_resume == nullptr) {
            break;
        }
        current_prev = prev_resume->prev_level;
    }
    return false;
}

void GF_ResetLevelSetup(const GF_SEQUENCE_CONTEXT seq_ctx)
{
    Room_SetAbyssHeight(0);
    Output_SetSunsetEnabled(false);
    Output_Sky_Reset();
    Output_LensFlares_Reset();
    Output_SetUVRotateSpeed(0);
    Lara_SetControllable(false);
    Lara_SetStartAnimState(LS_EXTRA_BREATH);
    if (seq_ctx == GFSC_SAVED) {
        Game_SetBonusFlag(GBF_NONE);
    }
}

RESULT GF_InterpretSequence(
    const GF_LEVEL *const level, GF_SEQUENCE_CONTEXT seq_ctx,
    void *const seq_ctx_arg, GF_COMMAND *const out_cmd)
{
    ASSERT(level != nullptr);
    LOG_DEBUG(
        "running sequence for level=%d type=%d seq_ctx=%d", level->num,
        level->type, seq_ctx);

    if (level->type == GFL_DUMMY || level->type == GFL_CURRENT) {
        *out_cmd = (GF_COMMAND) { .action = GF_NOOP };
        return OK;
    }

    GF_ResetLevelSetup(seq_ctx);

    GF_COMMAND gf_cmd = { .action = GF_EXIT_TO_TITLE };

    const GF_LEVEL *const prev_level = M_GetLinkedPrevLevel(level);

    // before load
    switch (seq_ctx) {
    case GFSC_STORY:
        break;

    case GFSC_SAVED:
        GF_InventoryModifier_Scan(level);
        // reset current info to the defaults so that we do not do
        // Item_GlobalReplace in the inventory initialization routines too early
        SG_Resume_ResetAllEntries();
        break;

    case GFSC_RESTART:
        if (level == GF_GetGymLevel() || level == GF_GetFirstLevel()) {
            SG_Resume_ResetAllEntries();
        } else {
            const int32_t prev_level_num =
                SG_Resume_GetEntry(level)->prev_level;
            SG_Resume_ResetEntry(level);
            if (prev_level_num != -1) {
                const GF_LEVEL *const linked_prev_level =
                    GF_GetLevel(GFLT_MAIN, prev_level_num);
                if (linked_prev_level != nullptr) {
                    SG_Resume_CarryEntry(linked_prev_level, level);
                }
            }
            SG_Resume_ApplyRulesToEntry(level);
        }
        if (level->type == GFL_NORMAL || level->type == GFL_BONUS) {
            GF_InventoryModifier_Scan(level);
            GF_InventoryModifier_ApplyToResumeInfo(level);
        }
        break;

    case GFSC_SELECT: {
        const SAVEGAME_SLOT_REF slot = SG_Manager_GetBoundSlot();
        if (SG_Manager_IsValidSlotRef(slot)) {
            // select level feature
            SG_Resume_ResetAllEntries();
            if (level->num > GF_GetFirstLevel()->num) {
                SHOULD(Savegame_LoadOnlyResumeInfo(slot));

                const int32_t prev_level_num =
                    SG_Resume_GetEntry(level)->prev_level;

                const GF_LEVEL_TABLE *const level_table =
                    GF_GetLevelTable(GFLT_MAIN);
                for (int32_t i = 0; i < level_table->count; i++) {
                    const GF_LEVEL *const tmp_level = &level_table->levels[i];
                    if (tmp_level->type == GFL_GYM) {
                        continue;
                    }
                    if (tmp_level == level
                        || M_IsLevelDescendantOf(tmp_level, level->num)) {
                        SG_Resume_ResetEntry(tmp_level);
                    }
                }

                if (prev_level_num != -1) {
                    const GF_LEVEL *const linked_prev_level =
                        GF_GetLevel(GFLT_MAIN, prev_level_num);
                    if (linked_prev_level != nullptr) {
                        SG_Resume_CarryEntry(linked_prev_level, level);
                    }
                }

                SG_Resume_ApplyRulesToEntry(level);
                GF_InventoryModifier_Scan(level);
                GF_InventoryModifier_ApplyToResumeInfo(level);
            } else {
                SG_Resume_ApplyRulesToEntry(level);
                GF_InventoryModifier_Scan(level);
                GF_InventoryModifier_ApplyToResumeInfo(level);
            }
        } else {
            // console /play level feature
            Inv_RemoveAllItems();
            SG_Resume_ResetAllEntries();
            if (level == GF_GetGymLevel()) {
                GF_InventoryModifier_Scan(level);
                GF_InventoryModifier_ApplyToResumeInfo(level);
            } else {
                const GF_LEVEL *tmp_level = GF_GetFirstLevel();
                SG_Resume_ResetEntry(tmp_level);
                while (tmp_level != nullptr && tmp_level <= level) {
                    SG_Resume_ApplyRulesToEntry(tmp_level);
                    GF_InventoryModifier_Scan(tmp_level);
                    GF_InventoryModifier_ApplyToResumeInfo(tmp_level);
                    if (tmp_level == level) {
                        break;
                    }

                    const GF_LEVEL *const next_level =
                        M_GetCanonicalNextLevel(tmp_level);
                    if (next_level != nullptr) {
                        SG_Resume_CarryEntry(tmp_level, next_level);
                    }
                    tmp_level = next_level;
                }
            }
        }
        break;
    }

    default:
        if (level->type == GFL_GYM) {
            SG_Resume_ResetEntry(level);
            SG_Resume_ApplyRulesToEntry(level);
        } else if (level->type == GFL_DEMO) {
            SG_Resume_ApplyRulesToEntry(level);
        } else if (level->type == GFL_NORMAL || level->type == GFL_BONUS) {
            SG_Resume_ApplyRulesToEntry(level);
            GF_InventoryModifier_Scan(level);
            GF_InventoryModifier_ApplyToResumeInfo(level);
        }
    }

    // load the level
    const GF_SEQUENCE *const sequence = &level->sequence;
    for (int32_t i = 0; i < sequence->length; i++) {
        if (M_IsPreLoadEvent(sequence->events[i].type)) {
            GF_COMMAND pre_cmd;
            MUST(
                M_RunEvent(level, sequence, i, seq_ctx, seq_ctx_arg, &pre_cmd));
        }
    }
    if (seq_ctx != GFSC_STORY || level->type == GFL_CUTSCENE) {
        EXIT_ON_FAIL(
            Level_Initialise(level, seq_ctx), "TRX cannot load the level");
    }

    for (int32_t i = 0; i < sequence->length; i++) {
        const GF_SEQUENCE_EVENT *const event = &sequence->events[i];
        if (M_IsPreLoadEvent(event->type)) {
            continue;
        }
        MUST(M_RunEvent(level, sequence, i, seq_ctx, seq_ctx_arg, &gf_cmd));
        if (gf_cmd.action != GF_NOOP) {
            *out_cmd = gf_cmd;
            return OK;
        }

        // Update sequence context if necessary
        seq_ctx = M_SwitchSequenceContext(event, seq_ctx);
    }

    LOG_DEBUG(
        "sequence finished: action=%s param=%d",
        ENUM_MAP_TO_STRING(GF_ACTION, gf_cmd.action), gf_cmd.param);
    *out_cmd = gf_cmd;
    return OK;
}
