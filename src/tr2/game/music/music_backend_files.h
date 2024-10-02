#pragma once

#include "game/music/music_backend.h"

MUSIC_BACKEND *Music_Backend_Files_Factory(const char *path);
void Music_Backend_Files_Destroy(MUSIC_BACKEND *backend);
