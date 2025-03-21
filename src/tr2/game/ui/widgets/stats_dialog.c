#include "game/ui/widgets/stats_dialog.h"

#include "game/game.h"
#include "game/game_flow.h"
#include "game/game_string.h"
#include "game/gym.h"
#include "game/input.h"
#include "game/stats.h"
#include "global/vars.h"

#include <libtrx/debug.h>
#include <libtrx/game/ui/common.h>
#include <libtrx/game/ui/widgets/label.h>
#include <libtrx/game/ui/widgets/requester.h>
#include <libtrx/game/ui/widgets/stack.h>
#include <libtrx/memory.h>

#include <stdio.h>
#include <string.h>

#define VISIBLE_ROWS 7
#define ROW_HEIGHT 18

typedef enum {
    M_ROW_GENERIC,
    M_ROW_TIMER,
    M_ROW_LEVEL_SECRETS,
    M_ROW_ALL_SECRETS,
    M_ROW_KILLS,
    M_ROW_AMMO_USED,
    M_ROW_AMMO_HITS,
    M_ROW_MEDIPACKS,
    M_ROW_DISTANCE_TRAVELED,
} M_ROW_ROLE;

typedef struct {
    UI_WIDGET_VTABLE vtable;
    UI_STATS_DIALOG_ARGS args;
    UI_WIDGET *requester;
    int32_t listener;
} UI_STATS_DIALOG;

static void M_AddRow(
    UI_STATS_DIALOG *self, M_ROW_ROLE role, const char *left_text,
    const char *right_text);
static void M_AddRowFromRole(
    UI_STATS_DIALOG *self, M_ROW_ROLE role, const STATS_COMMON *stats);
static void M_AddLevelStatsRows(UI_STATS_DIALOG *self);
static void M_AddFinalStatsRows(UI_STATS_DIALOG *self);
static void M_AddAssaultCourseStatsRows(UI_STATS_DIALOG *self);
static void M_UpdateTimerRow(UI_STATS_DIALOG *self);
static void M_DoLayout(UI_STATS_DIALOG *self);
static void M_HandleLayoutUpdate(const EVENT *event, void *data);

static int32_t M_GetWidth(const UI_STATS_DIALOG *self);
static int32_t M_GetHeight(const UI_STATS_DIALOG *self);
static void M_SetPosition(UI_STATS_DIALOG *self, int32_t x, int32_t y);
static void M_Control(UI_STATS_DIALOG *self);
static void M_Draw(UI_STATS_DIALOG *self);
static void M_Free(UI_STATS_DIALOG *self);

static void M_AddRow(
    UI_STATS_DIALOG *const self, const M_ROW_ROLE role,
    const char *const left_text, const char *const right_text)
{
    UI_Requester_AddRowLR(
        self->requester, left_text, right_text, (void *)(intptr_t)role);
}

static void M_AddRowFromRole(
    UI_STATS_DIALOG *const self, const M_ROW_ROLE role,
    const STATS_COMMON *const stats)
{
    char buf[32];

    switch (role) {
    case M_ROW_TIMER: {
        const int32_t sec = stats->timer / FRAMES_PER_SECOND;
        sprintf(
            buf, "%02d:%02d:%02d", (sec / 60) / 60, (sec / 60) % 60, sec % 60);
        M_AddRow(self, role, GS(STATS_TIME_TAKEN), buf);
        break;
    }

    case M_ROW_LEVEL_SECRETS: {
        char *ptr = buf;
        int32_t num_secrets = 0;
        for (int32_t i = 0; i < 3; i++) {
            if (((LEVEL_STATS *)stats)->secret_flags & (1 << i)) {
                sprintf(ptr, "\\{secret %d}", i + 1);
                num_secrets++;
            } else {
                strcpy(ptr, "   ");
            }
            ptr += strlen(ptr);
        }
        *ptr++ = '\0';
        if (num_secrets == 0) {
            strcpy(buf, GS(MISC_NONE));
        }
        M_AddRow(self, role, GS(STATS_SECRETS), buf);
        break;
    }

    case M_ROW_ALL_SECRETS:
        sprintf(
            buf, GS(STATS_DETAIL_FMT), ((FINAL_STATS *)stats)->found_secrets,
            ((FINAL_STATS *)stats)->total_secrets);
        M_AddRow(self, role, GS(STATS_SECRETS), buf);
        break;

    case M_ROW_KILLS:
        sprintf(buf, GS(STATS_BASIC_FMT), stats->kills);
        M_AddRow(self, role, GS(STATS_KILLS), buf);
        break;

    case M_ROW_AMMO_USED:
        sprintf(buf, "%d", stats->ammo_used);
        M_AddRow(self, role, GS(STATS_AMMO_USED), buf);
        break;

    case M_ROW_AMMO_HITS:
        sprintf(buf, "%d", stats->ammo_hits);
        M_AddRow(self, role, GS(STATS_AMMO_HITS), buf);
        break;

    case M_ROW_MEDIPACKS:
        if ((stats->medipacks & 1) != 0) {
            sprintf(buf, "%d.5", stats->medipacks >> 1);
        } else {
            sprintf(buf, "%d.0", stats->medipacks >> 1);
        }
        M_AddRow(self, role, GS(STATS_MEDIPACKS_USED), buf);
        break;

    case M_ROW_DISTANCE_TRAVELED:
        const int32_t distance = stats->distance / 445;
        if (distance < 1000) {
            sprintf(buf, "%dm", distance);
        } else {
            sprintf(buf, "%d.%02dkm", distance / 1000, (distance % 1000) / 10);
        }
        M_AddRow(self, role, GS(STATS_DISTANCE_TRAVELLED), buf);
        break;

    default:
        break;
    }
}

static void M_AddLevelStatsRows(UI_STATS_DIALOG *const self)
{
    const GF_LEVEL *const current_level = Game_GetCurrentLevel();
    const STATS_COMMON *stats =
        current_level != nullptr && self->args.level_num == current_level->num
        ? (STATS_COMMON *)&g_SaveGame.current_stats
        : (STATS_COMMON *)&g_SaveGame.start[self->args.level_num].stats;
    M_AddRowFromRole(self, M_ROW_TIMER, stats);
    if (g_GF_NumSecrets != 0) {
        M_AddRowFromRole(self, M_ROW_LEVEL_SECRETS, stats);
    }
    M_AddRowFromRole(self, M_ROW_KILLS, stats);
    M_AddRowFromRole(self, M_ROW_AMMO_USED, stats);
    M_AddRowFromRole(self, M_ROW_AMMO_HITS, stats);
    M_AddRowFromRole(self, M_ROW_MEDIPACKS, stats);
    M_AddRowFromRole(self, M_ROW_DISTANCE_TRAVELED, stats);
}

static void M_AddFinalStatsRows(UI_STATS_DIALOG *const self)
{
    const FINAL_STATS final_stats = Stats_ComputeFinalStats();
    const STATS_COMMON *stats = (STATS_COMMON *)&final_stats;
    M_AddRowFromRole(self, M_ROW_TIMER, stats);
    M_AddRowFromRole(self, M_ROW_ALL_SECRETS, stats);
    M_AddRowFromRole(self, M_ROW_KILLS, stats);
    M_AddRowFromRole(self, M_ROW_AMMO_USED, stats);
    M_AddRowFromRole(self, M_ROW_AMMO_HITS, stats);
    M_AddRowFromRole(self, M_ROW_MEDIPACKS, stats);
    M_AddRowFromRole(self, M_ROW_DISTANCE_TRAVELED, stats);
}

static void M_AddAssaultCourseStatsRows(UI_STATS_DIALOG *const self)
{
    const ASSAULT_STATS stats = Gym_GetAssaultStats();
    if (stats.best_time[0] == 0) {
        M_AddRow(self, M_ROW_GENERIC, GS(STATS_ASSAULT_NO_TIMES_SET), nullptr);
        return;
    }

    for (int32_t i = 0; i < 10; i++) {
        char left_buf[32] = "";
        char right_buf[32] = "";
        if (stats.best_time[i] != 0) {
            sprintf(
                left_buf, "%2d: %s %d", i + 1, GS(STATS_ASSAULT_FINISH),
                stats.best_finish[i]);

            const int32_t sec = stats.best_time[i] / FRAMES_PER_SECOND;
            sprintf(
                right_buf, "%02d:%02d.%-2d", sec / 60, sec % 60,
                stats.best_time[i] % FRAMES_PER_SECOND
                    / (FRAMES_PER_SECOND / 10));
        }

        M_AddRow(self, M_ROW_GENERIC, left_buf, right_buf);
    }
}

static void M_UpdateTimerRow(UI_STATS_DIALOG *const self)
{
    if (self->args.mode != UI_STATS_DIALOG_MODE_LEVEL) {
        return;
    }

    for (int32_t i = 0; i < UI_Requester_GetRowCount(self->requester); i++) {
        if ((intptr_t)UI_Requester_GetRowUserData(self->requester, i)
            != M_ROW_TIMER) {
            continue;
        }
        char buf[32];
        const int32_t sec = g_SaveGame.current_stats.timer / FRAMES_PER_SECOND;
        sprintf(
            buf, "%02d:%02d:%02d", (sec / 60) / 60, (sec / 60) % 60, sec % 60);
        UI_Requester_ChangeRowLR(
            self->requester, i, nullptr, buf, (void *)(intptr_t)M_ROW_TIMER);
        return;
    }
}

static void M_DoLayout(UI_STATS_DIALOG *const self)
{
    M_SetPosition(
        self, (UI_GetCanvasWidth() - M_GetWidth(self)) / 2,
        (UI_GetCanvasHeight() - M_GetHeight(self)) - 50);
}

static void M_HandleLayoutUpdate(const EVENT *event, void *data)
{
    UI_STATS_DIALOG *const self = (UI_STATS_DIALOG *)data;
    M_DoLayout(self);
}

static int32_t M_GetWidth(const UI_STATS_DIALOG *const self)
{
    return self->requester->get_width(self->requester);
}

static int32_t M_GetHeight(const UI_STATS_DIALOG *const self)
{
    return self->requester->get_height(self->requester);
}

static void M_SetPosition(
    UI_STATS_DIALOG *const self, const int32_t x, const int32_t y)
{
    self->requester->set_position(self->requester, x, y);
}

static void M_Control(UI_STATS_DIALOG *const self)
{
    if (self->requester->control != nullptr) {
        self->requester->control(self->requester);
    }

    M_UpdateTimerRow(self);
}

static void M_Draw(UI_STATS_DIALOG *const self)
{
    if (self->requester->draw != nullptr) {
        self->requester->draw(self->requester);
    }
}

static void M_Free(UI_STATS_DIALOG *const self)
{
    self->requester->free(self->requester);
    UI_Events_Unsubscribe(self->listener);
    Memory_Free(self);
}

UI_WIDGET *UI_StatsDialog_Create(UI_STATS_DIALOG_ARGS args)
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

    // TODO: add support for the bare style by merging TR1 and TR2 stats dialog
    // implementations.
    ASSERT(args.style == UI_STATS_DIALOG_STYLE_BORDERED);

    self->args = args;
    self->requester = UI_Requester_Create((UI_REQUESTER_SETTINGS) {
        .is_selectable = false,
        .visible_rows = VISIBLE_ROWS,
        .width = 290,
        .row_height = ROW_HEIGHT,
    });

    self->listener = UI_Events_Subscribe(
        "layout_update", nullptr, M_HandleLayoutUpdate, self);

    switch (args.mode) {
    case UI_STATS_DIALOG_MODE_LEVEL:
        UI_Requester_SetTitle(
            self->requester, GF_GetLevel(GFLT_MAIN, args.level_num)->title);
        M_AddLevelStatsRows(self);
        break;

    case UI_STATS_DIALOG_MODE_FINAL:
        UI_Requester_SetTitle(self->requester, GS(STATS_FINAL_STATISTICS));
        M_AddFinalStatsRows(self);
        break;

    case UI_STATS_DIALOG_MODE_ASSAULT_COURSE:
        UI_Requester_SetTitle(self->requester, GS(STATS_ASSAULT_TITLE));
        M_AddAssaultCourseStatsRows(self);
        break;
    }

    M_DoLayout(self);

    return (UI_WIDGET *)self;
}
