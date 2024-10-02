#include "game/requester.h"

#include "game/input.h"
#include "game/text.h"
#include "global/funcs.h"
#include "global/vars.h"

#include <libtrx/utils.h>

static void M_ClearTextStrings(REQUEST_INFO *req);

static void M_ClearTextStrings(REQUEST_INFO *const req)
{
    Text_Remove(req->heading_text1);
    req->heading_text1 = NULL;

    Text_Remove(req->heading_text2);
    req->heading_text2 = NULL;

    Text_Remove(req->background_text);
    req->background_text = NULL;

    Text_Remove(req->moreup_text);
    req->moreup_text = NULL;

    Text_Remove(req->moredown_text);
    req->moredown_text = NULL;

    for (int32_t i = 0; i < MAX_REQUESTER_ITEMS; i++) {
        Text_Remove(req->item_texts1[i]);
        req->item_texts1[i] = NULL;
        Text_Remove(req->item_texts2[i]);
        req->item_texts2[i] = NULL;
    }
}

void __cdecl Requester_Init(REQUEST_INFO *const req)
{
    req->background_flags = 1;
    req->moreup_flags = 1;
    req->moredown_flags = 1;
    req->items_count = 0;

    req->heading_text1 = NULL;
    req->heading_text2 = NULL;
    req->heading_flags1 = 0;
    req->heading_flags2 = 0;
    req->background_text = NULL;
    req->moreup_text = NULL;
    req->moredown_text = NULL;

    for (int32_t i = 0; i < MAX_REQUESTER_ITEMS; i++) {
        req->item_texts1[i] = NULL;
        req->item_texts2[i] = NULL;
        req->item_flags1[i] = 0;
        req->item_flags2[i] = 0;
    }

    req->pitem_flags1 = g_RequesterFlags1;
    req->pitem_flags2 = g_RequesterFlags2;

    req->render_width = GetRenderWidth();
    req->render_height = GetRenderHeight();
}

void __cdecl Requester_Shutdown(REQUEST_INFO *const req)
{
    M_ClearTextStrings(req);
    req->ready = 0;
}

int32_t __cdecl Requester_Display(
    REQUEST_INFO *const req, const bool destroy, const bool backgrounds)
{
    int32_t line_qty = req->visible_count;
    const int32_t lines_height = line_qty * req->line_height + 10;
    const int32_t line_one_off = req->y_pos - lines_height;

    const uint32_t render_width = GetRenderWidth();
    const uint32_t render_height = GetRenderHeight();
    if (render_width != req->render_width
        || render_height != req->render_height) {
        Requester_Shutdown(req);
        req->render_width = render_width;
        req->render_height = render_height;
    }

    req->pitem_flags1 = g_RequesterFlags1;
    req->pitem_flags2 = g_RequesterFlags2;
    if (req->items_count < req->visible_count) {
        line_qty = req->items_count;
    }

    /* heading */
    if (req->heading_flags1 & REQ_USE) {
        if (req->heading_text1 == NULL) {
            req->heading_text1 = Text_Create(
                req->x_pos, line_one_off - req->line_height - 10, req->z_pos,
                req->heading_string1);
            Text_CentreH(req->heading_text1, true);
            Text_AlignBottom(req->heading_text1, true);
            if (backgrounds) {
                Text_AddBackground(
                    req->heading_text1, req->pix_width - 4, 0, 0, 0, 8,
                    INV_COLOR_BLACK, g_Requester_MainGour1, 2);
                Text_AddOutline(
                    req->heading_text1, 1, INV_COLOR_ORANGE,
                    g_Requester_MainGour2, 0);
            }
        }

        if (req->heading_flags1 & REQ_ALIGN_LEFT) {
            Requester_Item_LeftAlign(req, req->heading_text1);
        }
        if (req->heading_flags1 & REQ_ALIGN_RIGHT) {
            Requester_Item_RightAlign(req, req->heading_text1);
        }
    }

    /* heading 2 */
    if (req->heading_flags2 & REQ_USE) {
        if (req->heading_text2) {
        } else {
            req->heading_text2 = Text_Create(
                req->x_pos, line_one_off - req->line_height - 10, req->z_pos,
                req->heading_string2);
            Text_CentreH(req->heading_text2, true);
            Text_AlignBottom(req->heading_text2, true);
            if (backgrounds) {
                Text_AddBackground(
                    req->heading_text2, req->pix_width - 4, 0, 0, 0, 8,
                    INV_COLOR_BLACK, g_Requester_MainGour1, 2);
                Text_AddOutline(
                    req->heading_text2, 1, INV_COLOR_ORANGE,
                    g_Requester_MainGour2, 0);
            }
        }

        if (req->heading_flags2 & REQ_ALIGN_LEFT) {
            Requester_Item_LeftAlign(req, req->heading_text2);
        }

        if (req->heading_flags2 & REQ_ALIGN_RIGHT) {
            Requester_Item_RightAlign(req, req->heading_text2);
        }
    }

    /* background */
    if (backgrounds && !req->background_text
        && (req->background_flags & 1) != 0) {
        req->background_text = Text_Create(
            req->x_pos, line_one_off - req->line_height - 12, 0, " ");
        Text_CentreH(req->background_text, true);
        Text_AlignBottom(req->background_text, true);
        Text_AddBackground(
            req->background_text, req->pix_width,
            req->line_height + lines_height + 12, 0, 0, 48, INV_COLOR_BLACK,
            g_Requester_BackgroundGour1, 1);
        Text_AddOutline(
            req->background_text, 1, INV_COLOR_BLUE,
            g_Requester_BackgroundGour2, 0);
    }

    /* arrows */
    if (req->line_offset) {
        if (!req->moreup_text && (req->moreup_flags & 1) != 0) {
            Text_CentreH(req->moreup_text, true);
            Text_AlignBottom(req->moreup_text, true);
        }
    } else {
        Text_Remove(req->moreup_text);
        req->moreup_text = NULL;
    }

    if (req->items_count <= req->visible_count + req->line_offset) {
        Text_Remove(req->moredown_text);
        req->moredown_text = NULL;
    } else if (!req->moredown_text && (req->moredown_flags & 1) != 0) {
        Text_CentreH(req->moredown_text, true);
        Text_AlignBottom(req->moredown_text, true);
    }

    /* item texts */
    for (int32_t i = 0; i < line_qty; i++) {
        const int32_t n = i + req->line_offset;

        TEXTSTRING **const text1 = &req->item_texts1[i];
        if (req->pitem_flags1[n] & REQ_USE) {
            if (*text1 == NULL) {
                *text1 = Text_Create(
                    0, line_one_off + req->line_height * i, 0,
                    &req->pitem_strings1[n * req->item_string_len]);
                Text_CentreH(*text1, true);
                Text_AlignBottom(*text1, true);
            }
            if (req->no_selector || n != req->selected) {
                Text_RemoveBackground(*text1);
                Text_RemoveOutline(*text1);
            } else {
                Text_AddBackground(
                    *text1, req->pix_width - 12, 0, 0, 0, 16, INV_COLOR_BLACK,
                    g_Requester_UnselectionGour1, 1);
                Text_AddOutline(
                    *text1, 1, INV_COLOR_ORANGE, g_Requester_SelectionGour2, 0);
            }

            if (req->pitem_flags1[n] & REQ_ALIGN_LEFT) {
                Requester_Item_LeftAlign(req, *text1);
            } else if (req->pitem_flags1[n] & REQ_ALIGN_RIGHT) {
                Requester_Item_RightAlign(req, *text1);
            } else {
                Requester_Item_CenterAlign(req, *text1);
            }
        } else {
            Text_Remove(*text1);
            Text_RemoveBackground(*text1);
            Text_RemoveOutline(*text1);
            *text1 = NULL;
        }

        TEXTSTRING **const text2 = &req->item_texts2[i];
        if (req->pitem_flags2[n] & REQ_USE) {
            if (*text2 == NULL) {
                *text2 = Text_Create(
                    0, line_one_off + req->line_height * i, 0,
                    &req->pitem_strings2[n * req->item_string_len]);
                Text_CentreH(*text2, true);
                Text_AlignBottom(*text2, true);
            }
            if (req->pitem_flags2[n] & REQ_ALIGN_LEFT) {
                Requester_Item_LeftAlign(req, *text2);
            } else if (req->pitem_flags2[n] & REQ_ALIGN_RIGHT) {
                Requester_Item_RightAlign(req, *text2);
            } else {
                Requester_Item_CenterAlign(req, *text2);
            }
        } else {
            Text_Remove(*text2);
            Text_RemoveBackground(*text2);
            Text_RemoveOutline(*text2);
            *text2 = NULL;
        }
    }

    if (req->line_offset != req->line_old_offset) {
        for (int32_t i = 0; i < line_qty; i++) {
            const int32_t n = req->line_offset + i;

            TEXTSTRING **text1 = &req->item_texts1[i];
            if (*text1 != NULL && (req->pitem_flags1[n] & REQ_USE)) {
                Text_ChangeText(
                    *text1, &req->pitem_strings1[n * req->item_string_len]);
            }
            if (req->pitem_flags1[n] & REQ_ALIGN_LEFT) {
                Requester_Item_LeftAlign(req, *text1);
            } else if (req->pitem_flags1[n] & REQ_ALIGN_RIGHT) {
                Requester_Item_RightAlign(req, *text1);
            } else {
                Requester_Item_CenterAlign(req, *text1);
            }

            TEXTSTRING **text2 = &req->item_texts2[i];
            if (*text2 != NULL && (req->pitem_flags2[n] & REQ_USE)) {
                Text_ChangeText(
                    *text2, &req->pitem_strings2[n * req->item_string_len]);
            }
            if (req->pitem_flags2[n] & REQ_ALIGN_LEFT) {
                Requester_Item_LeftAlign(req, *text2);
            } else if (req->pitem_flags2[n] & REQ_ALIGN_RIGHT) {
                Requester_Item_RightAlign(req, *text2);
            } else {
                Requester_Item_CenterAlign(req, *text2);
            }
        }
    }

    if (g_InputDB & IN_BACK) {
        if (req->no_selector) {
            req->line_old_offset = req->line_offset;
            if (req->visible_count + req->line_offset < req->items_count) {
                req->line_offset++;
                return 0;
            }
        } else {
            if (req->selected < req->items_count - 1) {
                req->selected++;
            }
            req->line_old_offset = req->line_offset;
            if (req->selected > req->line_offset + req->visible_count - 1) {
                req->line_offset++;
                return 0;
            }
        }
        return 0;
    }

    if (g_InputDB & IN_FORWARD) {
        if (req->no_selector) {
            req->line_old_offset = req->line_offset;
            if (req->line_offset > 0) {
                req->line_offset--;
                return 0;
            }
        } else {
            if (req->selected > 0) {
                req->selected--;
            }
            req->line_old_offset = req->line_offset;
            if (req->selected < req->line_offset) {
                req->line_offset--;
                return 0;
            }
        }
        return 0;
    }

    if (!(g_InputDB & IN_SELECT)) {
        if ((g_InputDB & IN_DESELECT) && destroy) {
            Requester_Shutdown(req);
            return -1;
        }
        return 0;
    }

    // TODO: WTF!?
    if (!strncmp(
            req->item_texts1[req->selected - req->line_offset]->text,
            g_GF_PCStrings[GF_S_PC_EMPTY_SLOT], 12)
        && !strcmp(
            g_PasswordText1->text,
            g_GF_GameStrings[GF_S_GAME_PASSPORT_LOAD_GAME])) {
        g_Input = 0;
        return 0;
    }

    Requester_Shutdown(req);
    return req->selected + 1;
}

void __cdecl Requester_RemoveAllItems(REQUEST_INFO *const req)
{
    req->items_count = 0;
    req->line_offset = 0;
    req->selected = 0;
}

void __cdecl Requester_Item_CenterAlign(
    REQUEST_INFO *const req, TEXTSTRING *const text)
{
    if (text == NULL) {
        return;
    }
    text->bgnd_off.x = 0;
    text->pos.x = req->x_pos;
}

void __cdecl Requester_Item_LeftAlign(
    REQUEST_INFO *const req, TEXTSTRING *const text)
{
    if (text == NULL) {
        return;
    }
    const uint32_t scale_h = Text_GetScaleH(text->scale.h);
    const int32_t x = ((scale_h * req->pix_width) >> 16) / 2
        - Text_GetWidth(text) / 2 - ((8 * scale_h) >> 16);
    text->pos.x = req->x_pos - x;
    text->bgnd_off.x = x;
}

void __cdecl Requester_Item_RightAlign(
    REQUEST_INFO *const req, TEXTSTRING *const text)
{
    const uint32_t scale_h = Text_GetScaleH(text->scale.h);
    if (text == NULL) {
        return;
    }
    const int32_t x = ((scale_h * req->pix_width) >> 16) / 2
        - Text_GetWidth(text) / 2 - ((8 * scale_h) >> 16);
    text->pos.x = x + req->x_pos;
    text->bgnd_off.x = -x;
}

void __cdecl Requester_SetHeading(
    REQUEST_INFO *const req, const char *const text1, const uint32_t flags1,
    const char *const text2, const uint32_t flags2)
{
    Text_Remove(req->heading_text1);
    req->heading_text1 = NULL;

    Text_Remove(req->heading_text2);
    req->heading_text2 = NULL;

    if (text1 != NULL) {
        strcpy(req->heading_string1, text1);
        req->heading_flags1 = flags1 | REQ_USE;
    } else {
        strcpy(req->heading_string1, "u");
        req->heading_flags1 = 0;
    }

    if (text2 != NULL) {
        strcpy(req->heading_string2, text2);
        req->heading_flags2 = flags2 | REQ_USE;
    } else {
        strcpy(req->heading_string2, "u");
        req->heading_flags2 = 0;
    }
}

void __cdecl Requester_ChangeItem(
    REQUEST_INFO *const req, const int32_t item, const char *const text1,
    const uint32_t flags1, const char *const text2, const uint32_t flags2)
{
    Text_Remove(req->item_texts1[item]);
    req->item_texts1[item] = NULL;

    Text_Remove(req->item_texts2[item]);
    req->item_texts2[item] = NULL;

    if (text1 != NULL) {
        strcpy(&req->pitem_strings1[item * req->item_string_len], text1);
        req->pitem_flags1[item] = flags1 | REQ_USE;
    } else {
        req->pitem_flags1[item] = 0;
    }

    if (text2 != NULL) {
        strcpy(&req->pitem_strings2[item * req->item_string_len], text2);
        req->pitem_flags2[item] = flags2 | REQ_USE;
    } else {
        req->pitem_flags2[item] = 0;
    }
}

void __cdecl Requester_AddItem(
    REQUEST_INFO *const req, const char *const text1, const uint32_t flags1,
    const char *const text2, const uint32_t flags2)
{
    req->pitem_flags1 = g_RequesterFlags1;
    req->pitem_flags2 = g_RequesterFlags2;

    if (text1 != NULL) {
        strcpy(
            &req->pitem_strings1[req->items_count * req->item_string_len],
            text1);
        req->pitem_flags1[req->items_count] = flags1 | REQ_USE;
    } else {
        g_RequesterFlags1[req->items_count] = 0;
    }

    if (text2 != NULL) {
        strcpy(
            &req->pitem_strings2[req->items_count * req->item_string_len],
            text2);
        req->pitem_flags2[req->items_count] = flags2 | REQ_USE;
    } else {
        req->pitem_flags2[req->items_count] = 0;
    }

    req->items_count++;
}

void __cdecl Requester_SetSize(
    REQUEST_INFO *const req, const int32_t max_lines, const int32_t y_pos)
{
    int32_t visible_lines = GetRenderHeight() / 2 / 18;
    CLAMPG(visible_lines, max_lines);
    req->y_pos = y_pos;
    req->visible_count = visible_lines;
}
