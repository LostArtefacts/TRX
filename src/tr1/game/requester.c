#include "game/requester.h"

#include "game/input.h"
#include "game/screen.h"
#include "game/text.h"
#include "global/const.h"
#include "global/types.h"

#include <libtrx/memory.h>

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define BOX_BORDER 2
#define BOX_PADDING 10

static void M_SetItem(
    REQUEST_INFO *req, const int32_t idx, const bool is_blocked,
    const char *const fmt, va_list va);

static void M_SetItem(
    REQUEST_INFO *req, const int32_t idx, const bool is_blocked,
    const char *const fmt, va_list va)
{
    if (req->items[idx].content_text) {
        Memory_FreePointer(&req->items[idx].content_text);
    }

    va_list va_dup;
    va_copy(va_dup, va);
    const size_t out_size = vsnprintf(NULL, 0, fmt, va) + 1;
    req->items[idx].content_text = Memory_Alloc(sizeof(char) * out_size);
    vsnprintf(req->items[idx].content_text, out_size, fmt, va_dup);
    req->items[idx].is_blocked = is_blocked;
    va_end(va_dup);
}

void Requester_Init(REQUEST_INFO *req, const uint16_t max_items)
{
    req->max_items = max_items;
    req->items = Memory_Alloc(sizeof(REQUESTER_ITEM) * max_items);
    Requester_ClearTextstrings(req);
}

void Requester_Shutdown(REQUEST_INFO *req)
{
    Requester_ClearTextstrings(req);

    Memory_FreePointer(&req->heading_text);
    if (req->items != NULL) {
        for (int i = 0; i < req->max_items; i++) {
            Memory_FreePointer(&req->items[i].content_text);
        }
    }
    Memory_FreePointer(&req->items);
}

void Requester_ClearTextstrings(REQUEST_INFO *req)
{
    Text_Remove(req->heading);
    req->heading = NULL;
    Text_Remove(req->background);
    req->background = NULL;
    Text_Remove(req->moreup);
    req->moreup = NULL;
    Text_Remove(req->moredown);
    req->moredown = NULL;

    if (req->items != NULL) {
        for (int i = 0; i < req->max_items; i++) {
            Text_Remove(req->items[i].content);
            req->items[i].content = NULL;
        }
    }

    req->items_used = 0;
}

int32_t Requester_Display(REQUEST_INFO *req)
{
    int32_t edge_y = req->y;
    int32_t lines_height = req->vis_lines * req->line_height;
    int32_t box_width = req->pix_width;
    int32_t box_height =
        req->line_height + lines_height + BOX_PADDING * 2 + BOX_BORDER * 2;

    int32_t line_one_off = edge_y - lines_height - BOX_PADDING;
    int32_t box_y = line_one_off - req->line_height - BOX_PADDING - BOX_BORDER;
    int32_t line_qty = req->vis_lines;
    if (req->items_used < req->vis_lines) {
        line_qty = req->items_used;
    }

    if (!req->background) {
        req->background = Text_Create(req->x, box_y, " ");
        Text_CentreH(req->background, 1);
        Text_AlignBottom(req->background, 1);
        Text_AddBackground(
            req->background, box_width, box_height, 0, 0, TS_BACKGROUND);
        Text_AddOutline(req->background, true, TS_BACKGROUND);
    }

    if (!req->heading) {
        req->heading = Text_Create(
            req->x, line_one_off - req->line_height - BOX_PADDING,
            req->heading_text);
        Text_CentreH(req->heading, 1);
        Text_AlignBottom(req->heading, 1);
        Text_AddBackground(
            req->heading, req->pix_width - 2 * BOX_BORDER, 0, 0, 0, TS_HEADING);
        Text_AddOutline(req->heading, true, TS_HEADING);
    }

    if (g_InputDB.menu_down) {
        if (req->requested < req->items_used - 1) {
            req->requested++;
        }
        req->line_old_offset = req->line_offset;
        if (req->requested > req->line_offset + req->vis_lines - 1) {
            req->line_offset++;
        }
    }

    if (g_InputDB.menu_up) {
        if (req->requested) {
            req->requested--;
        }
        req->line_old_offset = req->line_offset;
        if (req->requested < req->line_offset) {
            req->line_offset--;
        }
    }

    if (req->line_offset) {
        if (!req->moreup) {
            req->moreup =
                Text_Create(req->x, line_one_off - req->line_height + 2, "[");
            Text_SetScale(req->moreup, PHD_ONE * 2 / 3, PHD_ONE * 2 / 3);
            Text_CentreH(req->moreup, 1);
            Text_AlignBottom(req->moreup, 1);
        }
    } else {
        Text_Remove(req->moreup);
        req->moreup = NULL;
    }

    if (req->items_used > req->vis_lines + req->line_offset) {
        if (!req->moredown) {
            req->moredown = Text_Create(req->x, edge_y - 12, "]");
            Text_SetScale(req->moredown, PHD_ONE * 2 / 3, PHD_ONE * 2 / 3);
            Text_CentreH(req->moredown, 1);
            Text_AlignBottom(req->moredown, 1);
        }
    } else {
        Text_Remove(req->moredown);
        req->moredown = NULL;
    }

    for (int i = 0; i < line_qty; i++) {
        if (!req->items[i].content) {
            req->items[i].content = Text_Create(
                0, line_one_off + req->line_height * i,
                req->items[req->line_offset + i].content_text);
            Text_CentreH(req->items[i].content, 1);
            Text_AlignBottom(req->items[i].content, 1);
        }
        if (req->line_offset + i == req->requested) {
            Text_AddBackground(
                req->items[i].content,
                req->pix_width - BOX_PADDING - 1 * BOX_BORDER, 0, 0, 0,
                TS_REQUESTED);
            Text_AddOutline(req->items[i].content, true, TS_REQUESTED);
        } else {
            Text_RemoveBackground(req->items[i].content);
            Text_RemoveOutline(req->items[i].content);
        }
    }

    if (req->line_offset != req->line_old_offset) {
        for (int i = 0; i < line_qty; i++) {
            if (req->items[i].content) {
                Text_ChangeText(
                    req->items[i].content,
                    req->items[req->line_offset + i].content_text);
                ;
            }
        }
    }

    if (g_InputDB.menu_confirm) {
        if (req->is_blockable && req->items[req->requested].is_blocked) {
            g_Input = (INPUT_STATE) { 0 };
            return 0;
        } else {
            Requester_ClearTextstrings(req);
            return req->requested + 1;
        }
    } else if (g_InputDB.menu_back) {
        Requester_ClearTextstrings(req);
        return -1;
    }

    return 0;
}

void Requester_SetHeading(REQUEST_INFO *req, const char *string)
{
    Text_Remove(req->heading);
    req->heading = NULL;
    if (req->heading_text) {
        Memory_FreePointer(&req->heading_text);
    }

    const size_t out_size = snprintf(NULL, 0, "%s", string) + 1;
    req->heading_text = Memory_Alloc(sizeof(char) * out_size);
    snprintf(req->heading_text, out_size, "%s", string);
}

void Requester_ChangeItem(
    REQUEST_INFO *req, const int32_t idx, const bool is_blocked,
    const char *const fmt, ...)
{
    if (idx < 0 || idx >= req->max_items || !fmt) {
        return;
    }

    va_list va;
    va_start(va, fmt);
    M_SetItem(req, idx, is_blocked, fmt, va);
    va_end(va);
}

void Requester_AddItem(
    REQUEST_INFO *req, const bool is_blocked, const char *const fmt, ...)
{
    if (req->items_used >= req->max_items || !fmt) {
        return;
    }

    va_list va;
    va_start(va, fmt);
    M_SetItem(req, req->items_used, is_blocked, fmt, va);
    va_end(va);
    req->items_used++;
}

void Requester_SetSize(REQUEST_INFO *req, int32_t max_lines, int16_t y)
{
    req->y = y;
    req->vis_lines = Screen_GetResHeightDownscaled(RSR_TEXT) / 2 / MAX_REQLINES;
    if (req->vis_lines > max_lines) {
        req->vis_lines = max_lines;
    }
}
