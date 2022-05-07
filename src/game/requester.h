#pragma once

#include "global/types.h"

#include <stdint.h>

void Requester_Init(REQUEST_INFO *req);
void Requester_Remove(REQUEST_INFO *req);
int32_t Requester_Display(REQUEST_INFO *req);
void Requester_SetHeading(REQUEST_INFO *req, const char *string);
void Requester_ChangeItem(
    REQUEST_INFO *req, int32_t idx, const char *string, uint16_t flag);
void AddRequesterItem(REQUEST_INFO *req, const char *string, uint16_t flag);
void SetRequesterSize(REQUEST_INFO *req, int32_t max_lines, int16_t y);
