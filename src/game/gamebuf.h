#pragma once

#include <stdint.h>

// Internal game memory manager. It allocates its internal buffer once per
// level launch. All subsequent "allocation" requests operate with pointer
// arithmetic. This makes it fast and convenient to request more memory as we
// go, but it makes freeing memory really inconvenient which is why it is
// intentionally not implemented. To use more dynamic memory management, use
// Memory_Alloc / Memory_Free.

typedef enum GAME_BUFFER {
    GBUF_TEXTURE_PAGES,
    GBUF_MESH_POINTERS,
    GBUF_MESHES,
    GBUF_ANIMS,
    GBUF_ANIM_CHANGES,
    GBUF_ANIM_RANGES,
    GBUF_ANIM_COMMANDS,
    GBUF_ANIM_BONES,
    GBUF_ANIM_FRAMES,
    GBUF_ROOM_INFOS,
    GBUF_ROOM_MESH,
    GBUF_ROOM_DOOR,
    GBUF_ROOM_FLOOR,
    GBUF_ROOM_LIGHTS,
    GBUF_ROOM_STATIC_MESH_INFOS,
    GBUF_FLOOR_DATA,
    GBUF_ITEMS,
    GBUF_CAMERAS,
    GBUF_SOUND_FX,
    GBUF_BOXES,
    GBUF_OVERLAPS,
    GBUF_GROUNDZONE,
    GBUF_FLYZONE,
    GBUF_ANIMATING_TEXTURE_RANGES,
    GBUF_CINEMATIC_FRAMES,
    GBUF_LOADDEMO_BUFFER,
    GBUF_EXTRA_DOOR_STUFF,
    GBUF_EFFECTS,
    GBUF_CREATURE_INFO,
    GBUF_CREATURE_LOT,
    GBUF_SAMPLE_INFOS,
    GBUF_SAMPLES,
    GBUF_TRAP_DATA,
    GBUF_CREATURE_DATA,
} GAME_BUFFER;

void GameBuf_Init(void);
void *GameBuf_Alloc(int32_t alloc_size, GAME_BUFFER buffer);
void GameBuf_Shutdown(void);
