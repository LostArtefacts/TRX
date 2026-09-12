#include <trx/core/file.h>
#include <trx/core/utils.h>
#include <trx/game/game_flow.h>
#include <trx/game/inject/types.h>
#include <trx/game/level.h>

#define INJECTION_MAGIC MKTAG('T', 'R', 'X', 'J')

void Inject_InitLevel(const GF_LEVEL *level, INJECTION_MODE mode);

// Manage the injections declared by scripts. They load after the injections
// named by the level's game flow and are collected for each level.
void Inject_SetDeclarationCollector(void (*collect)(void));
void Inject_CollectDeclarations(void);

// Add one declared injection. Search dir first for a plain file name. Use null
// when the declaration comes from a loose script.
void Inject_AddDeclaredInjection(const char *name, const char *dir);
int32_t Inject_GetDeclaredCount(void);
const char *Inject_GetDeclaredPath(int32_t idx);

void Inject_AppendInjection(TRX_FILE *file);
void Inject_AllInjections(void);
void Inject_Cleanup(void);

INJECTION_ROOM_META Inject_GetRoomMeta(int32_t room_index);
int32_t Inject_GetDataCount(INJECTION_DATA_TYPE type);
int32_t Inject_GetMaxStaticObject3DId(void);
int32_t Inject_GetMaxStaticObject2DId(void);
LEVEL_CONTEXT_INFO Inject_GetCachedInfo(void);

void Inject_RegisterPaletteMap(const uint16_t *palette_map, int32_t size);
uint16_t Inject_GetPaletteIndex(uint16_t index);

void Inject_RegisterTester(
    const INJECTION_TEST_TYPE type,
    bool (*test_func)(const INJECTION_CONTEXT *, const INJECTION *injection));
void Inject_RegisterHandler(
    INJECTION_CHUNK_TYPE type,
    void (*handle_func)(const INJECTION_CONTEXT *, INJECTION_CHUNK chunk));

#define REGISTER_INJECT_TESTER(test_type, test_func)                           \
    __attribute__((constructor)) static void                                   \
    M_RegisterInjectTester##test_type(void)                                    \
    {                                                                          \
        Inject_RegisterTester(test_type, test_func);                           \
    }

#define REGISTER_INJECTOR(chunk_type, handle_func)                             \
    __attribute__((constructor)) static void M_RegisterInjector##chunk_type(   \
        void)                                                                  \
    {                                                                          \
        Inject_RegisterHandler(chunk_type, handle_func);                       \
    }
