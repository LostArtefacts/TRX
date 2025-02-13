#include "game/game_buf.h"

#include "enum_map.h"
#include "memory.h"

static MEMORY_ARENA_ALLOCATOR m_Allocator = {
    .default_chunk_size = 1024 * 1024 * 5,
};

void GameBuf_Init(void)
{
}

void GameBuf_Reset(void)
{
    Memory_ArenaReset(&m_Allocator);
}

void GameBuf_Shutdown(void)
{
    Memory_ArenaFree(&m_Allocator);
}

void *GameBuf_Alloc(const size_t alloc_size, const GAME_BUFFER buffer)
{
    const size_t aligned_size = (alloc_size + 3) & ~3;
    return Memory_ArenaAlloc(&m_Allocator, aligned_size);
}
