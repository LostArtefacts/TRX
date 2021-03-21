#include "game/requester.h"

#include "game/const.h"
#include "game/text.h"
#include "game/types.h"
#include "game/vars.h"
#include "specific/output.h"
#include "util.h"

#define BOX_PADDING 10
#define BOX_BORDER 2

#include <string.h>

// original name: Init_Requester
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

// original name: Remove_Requester
void RemoveRequester(REQUEST_INFO *req)
{
    T_RemovePrint(req->heading);
    req->heading = NULL;
    T_RemovePrint(req->background);
    req->background = NULL;
    T_RemovePrint(req->moreup);
    req->moreup = NULL;
    T_RemovePrint(req->moredown);
    req->moredown = NULL;
    for (int i = 0; i < MAX_REQLINES; i++) {
        T_RemovePrint(req->texts[i]);
        req->texts[i] = NULL;
    }
}

// original name: Display_Requester
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
        req->heading = T_Print(
            req->x, line_one_off - req->line_height - BOX_PADDING, req->z,
            req->heading_text);
        T_CentreH(req->heading, 1);
        T_BottomAlign(req->heading, 1);
        T_AddBackground(
            req->heading, req->pix_width - 2 * BOX_BORDER, 0, 0, 0, 8, IC_BLACK,
            ReqMainGour1, D_TRANS2);
        T_AddOutline(req->heading, 1, IC_ORANGE, ReqMainGour2, 0);
    }

    if (!req->background) {
        req->background = T_Print(req->x, box_y, 0, " ");
        T_CentreH(req->background, 1);
        T_BottomAlign(req->background, 1);
        T_AddBackground(
            req->background, box_width, box_height, 0, 0, 48, IC_BLACK,
            ReqBgndGour1, D_TRANS1);
        T_AddOutline(req->background, 1, IC_BLUE, ReqBgndGour2, 0);
    }

    if (req->line_offset) {
        if (!req->moreup) {
            req->moreup =
                T_Print(req->x, line_one_off - req->line_height + 2, 0, "[");
            T_SetScale(req->moreup, PHD_ONE * 2 / 3, PHD_ONE * 2 / 3);
            T_CentreH(req->moreup, 1);
            T_BottomAlign(req->moreup, 1);
            T_AddBackground(
                req->moreup, 16, 6, 0, 8, 8, IC_BLACK, ReqBgndMoreUp, D_TRANS1);
        }
    } else {
        T_RemovePrint(req->moreup);
        req->moreup = NULL;
    }

    if (req->items > req->vis_lines + req->line_offset) {
        if (!req->moredown) {
            req->moredown = T_Print(req->x, edge_y - 12, 0, "]");
            T_SetScale(req->moredown, PHD_ONE * 2 / 3, PHD_ONE * 2 / 3);
            T_CentreH(req->moredown, 1);
            T_BottomAlign(req->moredown, 1);
            T_AddBackground(
                req->moredown, 16, 6, 0, 0, 8, IC_BLACK, ReqBgndMoreDown,
                D_TRANS1);
        }
    } else {
        T_RemovePrint(req->moredown);
        req->moredown = NULL;
    }

    for (int i = 0; i < line_qty; i++) {
        if (!req->texts[i]) {
            req->texts[i] = T_Print(
                0, line_one_off + req->line_height * i, 0,
                &req->item_texts[req->item_text_len * (req->line_offset + i)]);
            T_CentreH(req->texts[i], 1);
            T_BottomAlign(req->texts[i], 1);
        }
        if (req->line_offset + i == req->requested) {
            T_AddBackground(
                req->texts[i], req->pix_width - BOX_PADDING - 2 * BOX_BORDER, 0,
                0, 0, 16, IC_BLACK, ReqUnselGour1, D_TRANS1);
            T_AddOutline(req->texts[i], 1, IC_ORANGE, ReqUnselGour2, 0);
        } else {
            T_RemoveBackground(req->texts[i]);
            T_RemoveOutline(req->texts[i]);
        }
    }

    if (req->line_offset != req->line_old_offset) {
        for (int i = 0; i < line_qty; i++) {
            if (req->texts[i]) {
                T_ChangeText(
                    req->texts[i],
                    &req->item_texts
                         [req->item_text_len * (req->line_offset + i)]);
            }
        }
    }

    if (CHK_ANY(InputDB, IN_BACK)) {
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

    if (CHK_ANY(InputDB, IN_FORWARD)) {
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

    if (CHK_ANY(InputDB, IN_SELECT)) {
        if ((req->item_flags[req->requested] & RIF_BLOCKED)
            && (req->flags & RIF_BLOCKABLE)) {
            Input = 0;
            return 0;
        } else {
            RemoveRequester(req);
            return req->requested + 1;
        }
    } else if (InputDB & IN_DESELECT) {
        RemoveRequester(req);
        return -1;
    }

    return 0;
}

void SetRequesterHeading(REQUEST_INFO *req, const char *string)
{
    T_RemovePrint(req->heading);
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
    req->vis_lines = GetRenderHeightDownscaled() / 2 / MAX_REQLINES;
    if (req->vis_lines > max_lines) {
        req->vis_lines = max_lines;
    }
}
