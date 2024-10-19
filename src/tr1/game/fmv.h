#pragma once

#include <stdbool.h>

bool FMV_Play(const char *path);
bool FMV_IsPlaying(void);
void FMV_Mute(void);
void FMV_Unmute(void);
