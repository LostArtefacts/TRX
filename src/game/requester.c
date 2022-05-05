#include "game/requester.h"

#include "game/input.h"
#include "game/screen.h"
#include "game/text.h"
#include "global/const.h"
#include "global/types.h"
#include "global/vars.h"

#include <string.h>

#define BOX_BORDER 2
#define BOX_PADDING 10

static RGBA8888 m_CenterColor = { 70, 30, 107, 255 };
static RGBA8888 m_EdgeColor = { 26, 10, 20, 155 };

void InitRequester(REQUEST_INFO *req)
{
    req->heading = NULL;
    req->background = NULL;
    req->moreup = NULL;
    req->moredown = NULL;
    for (int i = 0; i < MAX_REQLINES; i++) {
        req->texts[i] = NULL;
        req->item_flags[i] = 0;
    }

    req->items = 0;
}

void RemoveRequester(REQUEST_INFO *req)
{
    Text_Remove(req->heading);
    req->heading = NULL;
    Text_Remove(req->background);
    req->background = NULL;
    Text_Remove(req->moreup);
    req->moreup = NULL;
    Text_Remove(req->moredown);
    req->moredown = NULL;
    for (int i = 0; i < MAX_REQLINES; i++) {
        Text_Remove(req->texts[i]);
        req->texts[i] = NULL;
    }
}

int32_t DisplayRequester(REQUEST_INFO *req)
{
    int32_t edge_y = req->y;
    int32_t lines_height = req->vis_lines * req->line_height;
    int32_t box_width = req->pix_width;
    int32_t box_height =
        req->line_height + lines_height + BOX_PADDING * 2 + BOX_BORDER * 2;

    int32_t line_one_off = edge_y - lines_height - BOX_PADDING;
    int32_t box_y = line_one_off - req->line_height - BOX_PADDING - BOX_BORDER;
    int32_t line_qty = req->vis_lines;
    if (req->items < req->vis_lines) {
        line_qty = req->items;
    }

    if (!req->heading) {
        req->heading = Text_Create(
            req->x, line_one_off - req->line_height - BOX_PADDING,
            req->heading_text);
        Text_CentreH(req->heading, 1);
        Text_AlignBottom(req->heading, 1);
        Text_AddBackground(
            req->heading, req->pix_width - 2 * BOX_BORDER, 0, 0, 0);
        Text_AddOutline(req->heading, 1);
    }

    if (!req->background) {
        req->background = Text_Create(req->x, box_y, " ");
        Text_CentreH(req->background, 1);
        Text_AlignBottom(req->background, 1);
        Text_AddBackground(req->background, box_width, box_height, 0, 0);
        Text_AddOutline(req->background, 1);
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

    if (req->items > req->vis_lines + req->line_offset) {
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
        if (!req->texts[i]) {
            req->texts[i] = Text_Create(
                0, line_one_off + req->line_height * i,
                &req->item_texts[req->item_text_len * (req->line_offset + i)]);
            Text_CentreH(req->texts[i], 1);
            Text_AlignBottom(req->texts[i], 1);
        }
        if (req->line_offset + i == req->requested) {
            Text_AddBackground(
                req->texts[i], req->pix_width - BOX_PADDING - 1 * BOX_BORDER, 0,
                0, 0);
            Text_CentreVGradient(req->texts[i], m_CenterColor, m_EdgeColor);
            Text_AddOutline(req->texts[i], 1);
        } else {
            Text_RemoveBackground(req->texts[i]);
            Text_RemoveOutline(req->texts[i]);
        }
    }

    if (req->line_offset != req->line_old_offset) {
        for (int i = 0; i < line_qty; i++) {
            if (req->texts[i]) {
                Text_ChangeText(
                    req->texts[i],
                    &req->item_texts
                         [req->item_text_len * (req->line_offset + i)]);
            }
        }
    }

    if (g_InputDB.back) {
        if (req->requested < req->items - 1) {
            req->requested++;
        }
        req->line_old_offset = req->line_offset;
        if (req->requested > req->line_offset + req->vis_lines - 1) {
            req->line_offset++;
            return 0;
        }
        return 0;
    }

    if (g_InputDB.forward) {
        if (req->requested) {
            req->requested--;
        }
        req->line_old_offset = req->line_offset;
        if (req->requested < req->line_offset) {
            req->line_offset--;
            return 0;
        }
        return 0;
    }

    if (g_InputDB.select) {
        if ((req->item_flags[req->requested] & RIF_BLOCKED)
            && (req->flags & RIF_BLOCKABLE)) {
            g_Input = (INPUT_STATE) { 0 };
            return 0;
        } else {
            RemoveRequester(req);
            return req->requested + 1;
        }
    } else if (g_InputDB.deselect) {
        RemoveRequester(req);
        return -1;
    }

    return 0;
}

void SetRequesterHeading(REQUEST_INFO *req, const char *string)
{
    Text_Remove(req->heading);
    req->heading = NULL;

    if (string) {
        req->heading_text = string; // unsafe
    } else {
        req->heading_text = NULL;
    }
}

void ChangeRequesterItem(
    REQUEST_INFO *req, int32_t idx, const char *string, uint16_t flag)
{
    if (string) {
        strcpy(&req->item_texts[idx * req->item_text_len], string);
        req->item_flags[idx] = flag;
    } else {
        req->item_flags[idx] = 0;
    }
}

void AddRequesterItem(REQUEST_INFO *req, const char *string, uint16_t flag)
{
    if (string) {
        strcpy(&req->item_texts[req->items * req->item_text_len], string);
        req->item_flags[req->items] = flag;
    } else {
        req->item_flags[req->items] = 0;
    }

    req->items++;
}

void SetRequesterSize(REQUEST_INFO *req, int32_t max_lines, int16_t y)
{
    req->y = y;
    req->vis_lines = Screen_GetResHeightDownscaled() / 2 / MAX_REQLINES;
    if (req->vis_lines > max_lines) {
        req->vis_lines = max_lines;
    }
}
