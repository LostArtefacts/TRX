#pragma once

#include "../../config/types.h"

extern void Shell_HandleConfigChange(const CONFIG *old, const CONFIG *new);

void Shell_SyncToWindow(void);
void Shell_SyncFromWindow(bool update_viewport);

void Shell_RefreshRendererViewport(void);
void Shell_HandleCommonConfigChange(const CONFIG *old, const CONFIG *new);
