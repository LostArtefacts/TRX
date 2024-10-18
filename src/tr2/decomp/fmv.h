#pragma once

#include <stdbool.h>

bool __cdecl PlayFMV(const char *file_name);
void __cdecl FmvBackToGame(void);
void __cdecl WinPlayFMV(const char *file_name, bool is_playback);
void __cdecl WinStopFMV(bool is_playback);
