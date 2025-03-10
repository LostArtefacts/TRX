#include "../game_flow.h"
#include "../level.h"
#include "./types.h"

void Inject_InitLevel(const GF_LEVEL *level);
void Inject_AllInjections(void);
void Inject_Cleanup(void);

INJECTION_MESH_META Inject_GetRoomMeshMeta(int32_t room_index);
int32_t Inject_GetDataCount(INJECTION_DATA_TYPE type);
LEVEL_INFO Inject_GetCachedInfo(void);

void Inject_RegisterPaletteMap(const uint16_t *palette_map, int32_t size);
uint16_t Inject_GetPaletteIndex(uint16_t index);

void Inject_RegisterTester(
    const INJECTION_TEST_TYPE type,
    bool (*test_func)(const INJECTION *injection));
void Inject_RegisterHandler(
    INJECTION_CHUNK_TYPE type, void (*handle_func)(INJECTION_CHUNK chunk));

#define REGISTER_INJECT_TESTER(test_type, test_func)                           \
    __attribute__((                                                            \
        constructor)) static void M_RegisterInjectTester##test_type(void)      \
    {                                                                          \
        Inject_RegisterTester(test_type, test_func);                           \
    }

#define REGISTER_INJECTOR(chunk_type, handle_func)                             \
    __attribute__((constructor)) static void M_RegisterInjector##chunk_type(   \
        void)                                                                  \
    {                                                                          \
        Inject_RegisterHandler(chunk_type, handle_func);                       \
    }
