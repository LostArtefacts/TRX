#include "game/ui/widgets/stats_dialog.h"

#include "game/game_flow.h"
#include "game/game_string.h"
#include "game/input.h"
#include "game/stats.h"
#include "global/vars.h"

#include <libtrx/config.h>
#include <libtrx/game/ui/common.h>
#include <libtrx/game/ui/widgets/label.h>
#include <libtrx/game/ui/widgets/stack.h>
#include <libtrx/game/ui/widgets/window.h>
#include <libtrx/memory.h>

#include <stdio.h>
#include <string.h>

#define ROW_HEIGHT_BARE 25
#define ROW_HEIGHT_BORDERED 18
#define ROW_WIDTH_BORDERED 200
#define ROW_WIDTH_BORDERED_FULL 315

typedef enum {
    M_ROW_KILLS,
    M_ROW_PICKUPS,
    M_ROW_SECRETS,
    M_ROW_DEATHS,
    M_ROW_TIMER,
    M_ROW_AMMO,
    M_ROW_MEDIPACKS_USED,
    M_ROW_DISTANCE_TRAVELLED,
} M_ROW_ROLE;

typedef struct {
    M_ROW_ROLE role;
    UI_WIDGET *stack;
    UI_WIDGET *key_label;
    UI_WIDGET *value_label;
} M_ROW;

typedef struct {
    UI_WIDGET_VTABLE vtable;
    UI_STATS_DIALOG_ARGS args;
    GF_LEVEL_TYPE level_type;
    int32_t listener;

    int32_t row_count;
    UI_WIDGET *title;
    UI_WIDGET *stack;
    UI_WIDGET *window;
    UI_WIDGET *root; // just a pointer to either stack or window
    M_ROW *rows;
} UI_STATS_DIALOG;

static void M_FormatTime(char *out, int32_t total_frames);
static const char *M_GetDialogTitle(UI_STATS_DIALOG *self);
static void M_AddRow(
    UI_STATS_DIALOG *self, M_ROW_ROLE role, const char *key, const char *value);
static void M_AddRowFromRole(
    UI_STATS_DIALOG *self, M_ROW_ROLE role, const STATS_COMMON *stats,
    const GAME_INFO *game_info);
static void M_AddCommonRows(
    UI_STATS_DIALOG *self, const STATS_COMMON *stats,
    const GAME_INFO *game_info);
static void M_AddLevelStatsRows(UI_STATS_DIALOG *self);
static void M_AddFinalStatsRows(UI_STATS_DIALOG *self);
static void M_UpdateTimerRow(UI_STATS_DIALOG *self);
static void M_DoLayout(UI_STATS_DIALOG *self);
static void M_HandleLayoutUpdate(const EVENT *event, void *data);

static int32_t M_GetWidth(const UI_STATS_DIALOG *self);
static int32_t M_GetHeight(const UI_STATS_DIALOG *self);
static void M_SetPosition(UI_STATS_DIALOG *self, int32_t x, int32_t y);
static void M_Control(UI_STATS_DIALOG *self);
static void M_Draw(UI_STATS_DIALOG *self);
static void M_Free(UI_STATS_DIALOG *self);

static void M_FormatTime(char *const out, const int32_t total_frames)
{
    const int32_t total_seconds = total_frames / LOGIC_FPS;
    const int32_t hours = total_seconds / 3600;
    const int32_t minutes = (total_seconds / 60) % 60;
    const int32_t seconds = total_seconds % 60;
    if (hours != 0) {
        sprintf(out, "%d:%02d:%02d", hours, minutes, seconds);
    } else {
        sprintf(out, "%d:%02d", minutes, seconds);
    }
}

static const char *M_GetDialogTitle(UI_STATS_DIALOG *const self)
{
    switch (self->args.mode) {
    case UI_STATS_DIALOG_MODE_LEVEL:
        return GF_GetLevel(GFLT_MAIN, self->args.level_num)->title;

    case UI_STATS_DIALOG_MODE_FINAL:
        return self->level_type == GFL_BONUS ? GS(STATS_BONUS_STATISTICS)
                                             : GS(STATS_FINAL_STATISTICS);
    }

    return nullptr;
}

static void M_AddRow(
    UI_STATS_DIALOG *const self, const M_ROW_ROLE role, const char *const key,
    const char *const value)
{
    self->row_count++;
    self->rows = Memory_Realloc(self->rows, sizeof(M_ROW) * self->row_count);
    M_ROW *const row = &self->rows[self->row_count - 1];
    row->role = role;

    // create a stack
    int32_t row_height;
    if (self->args.style == UI_STATS_DIALOG_STYLE_BARE) {
        row_height = ROW_HEIGHT_BARE;
        row->stack = UI_Stack_Create(
            UI_STACK_LAYOUT_HORIZONTAL, UI_STACK_AUTO_SIZE, row_height);
    } else {
        row_height = ROW_HEIGHT_BORDERED;
        const int32_t row_width = g_Config.gameplay.stat_detail_mode == SDM_FULL
            ? ROW_WIDTH_BORDERED_FULL
            : ROW_WIDTH_BORDERED;
        row->stack =
            UI_Stack_Create(UI_STACK_LAYOUT_HORIZONTAL, row_width, row_height);
        UI_Stack_SetHAlign(row->stack, UI_STACK_H_ALIGN_DISTRIBUTE);
    }

    // create a key label; append space for the bare style
    char key2[strlen(key) + 2];
    sprintf(
        key2, self->args.style == UI_STATS_DIALOG_STYLE_BARE ? "%s " : "%s",
        key);
    row->key_label = UI_Label_Create(key2, UI_LABEL_AUTO_SIZE, row_height);

    // create a value label
    row->value_label = UI_Label_Create(value, UI_LABEL_AUTO_SIZE, row_height);

    UI_Stack_AddChild(row->stack, row->key_label);
    UI_Stack_AddChild(row->stack, row->value_label);
    UI_Stack_AddChild(self->stack, row->stack);
}

static void M_AddRowFromRole(
    UI_STATS_DIALOG *const self, const M_ROW_ROLE role,
    const STATS_COMMON *const stats, const GAME_INFO *const game_info)
{
    char buf[50];
    const char *const num_fmt =
        g_Config.gameplay.stat_detail_mode == SDM_MINIMAL
        ? GS(STATS_BASIC_FMT)
        : GS(STATS_DETAIL_FMT);

    switch (role) {
    case M_ROW_KILLS:
        sprintf(buf, num_fmt, stats->kill_count, stats->max_kill_count);
        M_AddRow(self, role, GS(STATS_KILLS), buf);
        break;

    case M_ROW_PICKUPS:
        sprintf(buf, num_fmt, stats->pickup_count, stats->max_pickup_count);
        M_AddRow(self, role, GS(STATS_PICKUPS), buf);
        break;

    case M_ROW_SECRETS:
        sprintf(
            buf, GS(STATS_DETAIL_FMT), stats->secret_count,
            stats->max_secret_count);
        M_AddRow(self, role, GS(STATS_SECRETS), buf);
        break;

    case M_ROW_DEATHS:
        sprintf(buf, GS(STATS_BASIC_FMT), game_info->death_count);
        M_AddRow(self, role, GS(STATS_DEATHS), buf);
        break;

    case M_ROW_TIMER:
        M_FormatTime(buf, stats->timer);
        M_AddRow(self, role, GS(STATS_TIME_TAKEN), buf);
        break;

    case M_ROW_AMMO:
        sprintf(buf, GS(PAGINATION_NAV), stats->ammo_hits, stats->ammo_used);
        M_AddRow(self, role, GS(STATS_AMMO), buf);
        break;

    case M_ROW_MEDIPACKS_USED:
        sprintf(buf, GS(DETAIL_FLOAT_FMT), stats->medipacks_used);
        M_AddRow(self, role, GS(STATS_MEDIPACKS_USED), buf);
        break;

    case M_ROW_DISTANCE_TRAVELLED:
        const int32_t distance_travelled = stats->distance_travelled / 445;
        if (distance_travelled < 1000) {
            sprintf(buf, "%dm", distance_travelled);
        } else {
            sprintf(
                buf, "%d.%02dkm", distance_travelled / 1000,
                distance_travelled % 100);
        }
        M_AddRow(self, role, GS(STATS_DISTANCE_TRAVELLED), buf);
        break;

    default:
        break;
    }
}

static void M_AddCommonRows(
    UI_STATS_DIALOG *const self, const STATS_COMMON *const stats,
    const GAME_INFO *const game_info)
{
    if (g_Config.gameplay.stat_detail_mode == SDM_MINIMAL) {
        M_AddRowFromRole(self, M_ROW_KILLS, stats, game_info);
        M_AddRowFromRole(self, M_ROW_PICKUPS, stats, game_info);
        M_AddRowFromRole(self, M_ROW_SECRETS, stats, game_info);
        M_AddRowFromRole(self, M_ROW_TIMER, stats, game_info);
    } else {
        M_AddRowFromRole(self, M_ROW_TIMER, stats, game_info);
        M_AddRowFromRole(self, M_ROW_SECRETS, stats, game_info);
        M_AddRowFromRole(self, M_ROW_PICKUPS, stats, game_info);
        M_AddRowFromRole(self, M_ROW_KILLS, stats, game_info);
        if (g_Config.gameplay.stat_detail_mode == SDM_FULL) {
            M_AddRowFromRole(self, M_ROW_AMMO, stats, game_info);
            M_AddRowFromRole(self, M_ROW_MEDIPACKS_USED, stats, game_info);
            M_AddRowFromRole(self, M_ROW_DISTANCE_TRAVELLED, stats, game_info);
        }
    }

    if (g_Config.gameplay.enable_deaths_counter
        && game_info->death_count >= 0) {
        // Always use sum of all levels for the deaths.
        // Deaths get stored in the resume info for the level they happen
        // on, so if the player dies in Vilcabamba and reloads Caves, they
        // should still see an incremented death counter.
        M_AddRowFromRole(self, M_ROW_DEATHS, stats, game_info);
    }
}

static void M_AddLevelStatsRows(UI_STATS_DIALOG *const self)
{
    const STATS_COMMON *stats =
        (STATS_COMMON *)&g_GameInfo.current[self->args.level_num].stats;
    M_AddCommonRows(self, stats, &g_GameInfo);
}

static void M_AddFinalStatsRows(UI_STATS_DIALOG *const self)
{
    FINAL_STATS final_stats;
    Stats_ComputeFinal(self->level_type, &final_stats);
    M_AddCommonRows(self, (STATS_COMMON *)&final_stats, &g_GameInfo);
}

static void M_UpdateTimerRow(UI_STATS_DIALOG *const self)
{
    if (self->args.mode != UI_STATS_DIALOG_MODE_LEVEL) {
        return;
    }

    for (int32_t i = 0; i < self->row_count; i++) {
        if (self->rows[i].role != M_ROW_TIMER) {
            continue;
        }
        char buf[50];
        M_FormatTime(buf, g_GameInfo.current[self->args.level_num].stats.timer);
        UI_Label_ChangeText(self->rows[i].value_label, buf);
        return;
    }
}

static void M_DoLayout(UI_STATS_DIALOG *const self)
{
    M_SetPosition(
        self, (UI_GetCanvasWidth() - M_GetWidth(self)) / 2,
        (UI_GetCanvasHeight() - M_GetHeight(self)) / 2);
}

static void M_HandleLayoutUpdate(const EVENT *event, void *data)
{
    UI_STATS_DIALOG *const self = (UI_STATS_DIALOG *)data;
    M_DoLayout(self);
}

static int32_t M_GetWidth(const UI_STATS_DIALOG *const self)
{
    return self->root->get_width(self->root);
}

static int32_t M_GetHeight(const UI_STATS_DIALOG *const self)
{
    return self->root->get_height(self->root);
}

static void M_SetPosition(
    UI_STATS_DIALOG *const self, const int32_t x, const int32_t y)
{
    self->root->set_position(self->root, x, y);
}

static void M_Control(UI_STATS_DIALOG *const self)
{
    if (self->root->control != nullptr) {
        self->root->control(self->root);
    }
    M_UpdateTimerRow(self);
}

static void M_Draw(UI_STATS_DIALOG *const self)
{
    if (self->root->draw != nullptr) {
        self->root->draw(self->root);
    }
}

static void M_Free(UI_STATS_DIALOG *const self)
{
    if (self->title != nullptr) {
        self->title->free(self->title);
    }
    for (int32_t i = 0; i < self->row_count; i++) {
        self->rows[i].key_label->free(self->rows[i].key_label);
        self->rows[i].value_label->free(self->rows[i].value_label);
        self->rows[i].stack->free(self->rows[i].stack);
    }
    if (self->window != nullptr) {
        self->window->free(self->window);
    }
    self->stack->free(self->stack);
    UI_Events_Unsubscribe(self->listener);
    Memory_Free(self);
}

UI_WIDGET *UI_StatsDialog_Create(const UI_STATS_DIALOG_ARGS args)
{
    UI_STATS_DIALOG *const self = Memory_Alloc(sizeof(UI_STATS_DIALOG));
    self->vtable = (UI_WIDGET_VTABLE) {
        .get_width = (UI_WIDGET_GET_WIDTH)M_GetWidth,
        .get_height = (UI_WIDGET_GET_HEIGHT)M_GetHeight,
        .set_position = (UI_WIDGET_SET_POSITION)M_SetPosition,
        .control = (UI_WIDGET_CONTROL)M_Control,
        .draw = (UI_WIDGET_DRAW)M_Draw,
        .free = (UI_WIDGET_FREE)M_Free,
    };

    self->args = args;
    self->level_type = GF_GetLevel(GFLT_MAIN, self->args.level_num)->type;

    self->row_count = 0;
    self->rows = nullptr;
    self->stack = UI_Stack_Create(
        UI_STACK_LAYOUT_VERTICAL, UI_STACK_AUTO_SIZE, UI_STACK_AUTO_SIZE);
    UI_Stack_SetHAlign(self->stack, UI_STACK_H_ALIGN_CENTER);

    self->listener = UI_Events_Subscribe(
        "layout_update", nullptr, M_HandleLayoutUpdate, self);

    const char *title = M_GetDialogTitle(self);
    switch (self->args.style) {
    case UI_STATS_DIALOG_STYLE_BARE:
        if (title != nullptr) {
            self->title =
                UI_Label_Create(title, UI_LABEL_AUTO_SIZE, ROW_HEIGHT_BARE);
            UI_Stack_AddChild(self->stack, self->title);
        }
        self->root = self->stack;
        break;

    case UI_STATS_DIALOG_STYLE_BORDERED:
        self->window = UI_Window_Create(self->stack, 8, 8, 8, 8);
        UI_Window_SetTitle(self->window, title);
        self->root = self->window;
        break;
    }

    if (self->args.mode == UI_STATS_DIALOG_MODE_LEVEL) {
        M_AddLevelStatsRows(self);
    } else if (self->args.mode == UI_STATS_DIALOG_MODE_FINAL) {
        M_AddFinalStatsRows(self);
    }

    M_DoLayout(self);
    return (UI_WIDGET *)self;
}
