#pragma once

#include "global/types.h"

#include <libtrx/game/overlay.h>

void Overlay_HideGameInfo(void);
void Overlay_DrawGameInfo(void);

void Overlay_AddDisplayPickup(GAME_OBJECT_ID obj_id);

void Overlay_Animate(int32_t frames);
