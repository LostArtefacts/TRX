#pragma once

#include <trx/core/result.h>

// Plays a video full screen. Returns when the video ends or when the player
// skips it. A video that the settings disable is no fault and plays nothing.
RESULT FMV_Play(const char *file_path);
bool FMV_IsPlaying(void);
