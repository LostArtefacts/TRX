#pragma once

#include "global/types.h"

void __cdecl Requester_Init(REQUEST_INFO *req);
void __cdecl Requester_Shutdown(REQUEST_INFO *req);
int32_t __cdecl Requester_Display(
    REQUEST_INFO *req, bool destroy, bool backgrounds);
void __cdecl Requester_RemoveAllItems(REQUEST_INFO *req);
void __cdecl Requester_Item_CenterAlign(REQUEST_INFO *req, TEXTSTRING *text);
void __cdecl Requester_Item_LeftAlign(REQUEST_INFO *req, TEXTSTRING *text);
void __cdecl Requester_Item_RightAlign(REQUEST_INFO *req, TEXTSTRING *text);
void __cdecl Requester_SetHeading(
    REQUEST_INFO *req, const char *text1, uint32_t flags1, const char *text2,
    uint32_t flags2);
void __cdecl Requester_ChangeItem(
    REQUEST_INFO *req, int32_t item, const char *text1, uint32_t flags1,
    const char *text2, uint32_t flags2);
void __cdecl Requester_AddItem(
    REQUEST_INFO *req, const char *text1, uint32_t flags1, const char *text2,
    uint32_t flags2);
void __cdecl Requester_SetSize(
    REQUEST_INFO *req, int32_t max_lines, int32_t y_pos);
