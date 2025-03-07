#pragma once

#include "global/types.h"

#include <libtrx/game/inject.h>

int32_t Inject_GetDataCount(INJECTION_DATA_TYPE type);
void Inject_Init(int32_t injection_count, char *filenames[]);
void Inject_AllInjections(void);
void Inject_Cleanup();
