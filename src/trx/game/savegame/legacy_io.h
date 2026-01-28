#pragma once

#include <stddef.h>
#include <stdint.h>

typedef struct SAVEGAME_LEGACY_IO {
    int32_t (*read_s32)(const struct SAVEGAME_LEGACY_IO *io);
    int16_t (*read_s16)(const struct SAVEGAME_LEGACY_IO *io);
    void (*skip)(const struct SAVEGAME_LEGACY_IO *io, size_t size);
    int32_t tr_version;
    void *priv;
} SAVEGAME_LEGACY_IO;
