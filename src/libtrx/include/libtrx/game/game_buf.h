#pragma once

#include <stddef.h>

// Internal game memory manager. It allocates its internal buffer once per
// level launch. All subsequent "allocation" requests operate with pointer
// arithmetic. This makes it fast and convenient to request more memory as we
// go, but it makes freeing memory really inconvenient which is why it is
// intentionally not implemented. To use more dynamic memory management, use
// Memory_Alloc / Memory_Free.

typedef enum {
    // clang-format off
    GBUF_TEXTURE_PAGES,
    GBUF_PALETTES,
    GBUF_OBJECT_TEXTURES,
    GBUF_SPRITE_TEXTURES,
    GBUF_MESH_POINTERS,
    GBUF_MESHES,
    GBUF_ANIMS,
    GBUF_ANIM_CHANGES,
    GBUF_ANIM_RANGES,
    GBUF_ANIM_COMMANDS,
    GBUF_ANIM_BONES,
    GBUF_ANIM_FRAMES,
    GBUF_ROOMS,
    GBUF_ROOM_MESH,
    GBUF_ROOM_PORTALS,
    GBUF_ROOM_SECTORS,
    GBUF_ROOM_LIGHTS,
    GBUF_ROOM_STATIC_MESHES,
    GBUF_FLOOR_DATA,
    GBUF_ITEMS,
    GBUF_ITEM_DATA,
    GBUF_EFFECTS,
    GBUF_CAMERAS,
    GBUF_SOUND_SOURCES,
    GBUF_BOXES,
    GBUF_OVERLAPS,
    GBUF_GROUND_ZONE,
    GBUF_FLY_ZONE,
    GBUF_ANIMATED_TEXTURE_RANGES,
    GBUF_CINEMATIC_FRAMES,
    GBUF_CREATURE_DATA,
    GBUF_CREATURE_LOT,
    GBUF_SAMPLE_INFOS,
    GBUF_SAMPLES,
    GBUF_DEMO_BUFFER,
    GBUF_VERTEX_BUFFER,
    GBUF_NUM_MALLOC_TYPES,
    // clang-format on
} GAME_BUFFER;

void GameBuf_Init(size_t cap);
void GameBuf_Shutdown(void);
void GameBuf_Reset(void);

void *GameBuf_Alloc(size_t alloc_size, GAME_BUFFER buffer);
