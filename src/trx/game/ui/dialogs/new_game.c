#include <trx/game/ui/dialogs/new_game.h>

#include <trx/config.h>
#include <trx/core/memory.h>
#include <trx/core/vector.h>
#include <trx/game/game_flow.h>
#include <trx/game/game_strings/entries.h>
#include <trx/game/savegame.h>
#include <trx/game/ui.h>
#include <trx/version.h>

typedef struct {
    bool play_prev_levels;
    bool story_so_far;
} M_FEATURES;

typedef struct {
    GAME_STRING_ID label_id;
    UI_NEW_GAME_CHOICE choice;
} M_OPTION;

typedef struct UI_NEW_GAME_STATE {
    VECTOR *options;
    UI_REQUESTER_STATE req;
} UI_NEW_GAME_STATE;

static const M_OPTION m_Options[] = {
    {
        .label_id = GS_ID(PASSPORT_MODE_NEW_GAME),
        .choice = UI_NEW_GAME_CHOICE_NG,
    },
    {
        .label_id = GS_ID(PASSPORT_MODE_NEW_GAME_PLUS),
        .choice = UI_NEW_GAME_CHOICE_NGPLUS,
    },
    {
        .label_id = GS_ID(PASSPORT_MODE_NEW_GAME_JP),
        .choice = UI_NEW_GAME_CHOICE_JP_NG,
    },
    {
        .label_id = GS_ID(PASSPORT_MODE_NEW_GAME_JP_PLUS),
        .choice = UI_NEW_GAME_CHOICE_JP_NGPLUS,
    },
    {
        .label_id = GS_ID(PASSPORT_PLAY_PREVIOUS_LEVELS),
        .choice = UI_NEW_GAME_CHOICE_PLAY_PREV_LEVELS,
    },
    {
        .label_id = GS_ID(PASSPORT_STORY_SO_FAR),
        .choice = UI_NEW_GAME_CHOICE_STORY_SO_FAR,
    },
    { .label_id = nullptr, .choice = (UI_NEW_GAME_CHOICE)-1 },
};

static M_FEATURES M_CheckFeatures(void)
{
    M_FEATURES features = {};
    if (g_Config.flow.load_save_disabled) {
        return features;
    }
    for (int32_t slot_num = 0; slot_num < Savegame_GetSlotCount(); slot_num++) {
        if (Savegame_IsSlotFree(slot_num)) {
            continue;
        }
        if (!features.play_prev_levels) {
            const SAVEGAME_INFO *const info =
                Savegame_GetSavegameInfo(slot_num);
            if (info->features.select_level) {
                features.play_prev_levels = true;
            }
        }
        if (!features.story_so_far && GF_HasAvailableStory(slot_num)) {
            features.story_so_far = true;
        }
    }
    return features;
}

static bool M_OptionVisible(
    const M_FEATURES *const features, const M_OPTION *const option)
{
    if (option->choice == UI_NEW_GAME_CHOICE_STORY_SO_FAR) {
        return features->story_so_far;
    }
    if (option->choice == UI_NEW_GAME_CHOICE_PLAY_PREV_LEVELS) {
        return features->play_prev_levels;
    }
    return g_Config.gameplay.enable_game_modes
        || option->choice == UI_NEW_GAME_CHOICE_NG;
}

UI_NEW_GAME_STATE *UI_NewGame_Init(const bool show_play_prev_levels)
{
    UI_NEW_GAME_STATE *const s = Memory_Alloc(sizeof(UI_NEW_GAME_STATE));
    s->options = Vector_Create(sizeof(M_OPTION));

    const M_FEATURES features =
        (show_play_prev_levels && g_Config.gameplay.enable_play_previous_levels)
        ? M_CheckFeatures()
        : (M_FEATURES) {};
    for (int32_t i = 0; m_Options[i].label_id != nullptr; i++) {
        if (M_OptionVisible(&features, &m_Options[i])) {
            Vector_Add(s->options, &m_Options[i]);
        }
    }

    UI_Requester_Init(&s->req, s->options->count, s->options->count, true);
    return s;
}

void UI_NewGame_Free(UI_NEW_GAME_STATE *const s)
{
    Vector_Free(s->options);
    UI_Requester_Free(&s->req);
}

int32_t UI_NewGame_Control(UI_NEW_GAME_STATE *const s)
{
    const int32_t choice = UI_Requester_Control(&s->req);
    if (choice == UI_REQUESTER_CANCEL || choice == UI_REQUESTER_NO_CHOICE) {
        return choice;
    }
    const M_OPTION *const opt = Vector_Get(s->options, choice);
    return opt->choice;
}

void UI_NewGame(UI_NEW_GAME_STATE *const s)
{
    UI_BeginModal(0.5f, 2.0f / 3.0f);
    UI_BeginRequester(&s->req, GS(PASSPORT_SELECT_MODE));

    bool line_drawn = false;
    for (int32_t i = 0; i < s->options->count; i++) {
        const M_OPTION *const opt = Vector_Get(s->options, i);
        UI_BeginStackEx((UI_STACK_SETTINGS) {
            .orientation = UI_STACK_VERTICAL,
            { .h = UI_STACK_H_ALIGN_SPAN },
        });
        if (i > 0 && !line_drawn
            && (opt->choice == UI_NEW_GAME_CHOICE_PLAY_PREV_LEVELS
                || opt->choice == UI_NEW_GAME_CHOICE_STORY_SO_FAR)) {
            // TODO: do not hardcode the numbers (they come from
            // UI_BeginWindowBody)
            UI_BeginPad(g_TRVersion >= 2 ? -7.0f : -10.0f, 4.0f);
            UI_HorizontalLine();
            UI_EndPad();
            line_drawn = true;
        }
        UI_BeginRequesterRow(&s->req, i);
        UI_BeginAnchor(0.5f, 0.5f);
        UI_Label(GameString_Get(opt->label_id));
        UI_EndAnchor();
        UI_EndRequesterRow(&s->req, i);
        UI_EndStack();
    }

    UI_EndRequester(&s->req);
    UI_EndModal();
}
