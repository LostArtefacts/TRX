#pragma once

#include "global/types.h"

#include <stdint.h>

void Requester_Init(REQUEST_INFO *req, uint16_t num_items);
void Requester_Shutdown(REQUEST_INFO *req);
void Requester_ClearTextstrings(REQUEST_INFO *req);
int32_t Requester_Display(REQUEST_INFO *req);
void Requester_SetHeading(REQUEST_INFO *req, const char *string);
void Requester_ChangeItem(
    REQUEST_INFO *req, int32_t idx, bool is_blocked, const char *fmt, ...);
void Requester_AddItem(
    REQUEST_INFO *req, bool is_blocked, const char *fmt, ...);
void Requester_SetSize(REQUEST_INFO *req, int32_t max_lines, int16_t y);
