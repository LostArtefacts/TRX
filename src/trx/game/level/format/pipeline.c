#include <trx/core/benchmark.h>
#include <trx/core/file.h>
#include <trx/core/vector.h>
#include <trx/debug.h>
#include <trx/game/game_buf.h>
#include <trx/game/level/context.h>
#include <trx/game/level/format/format.h>
#include <trx/game/level/format/priv.h>
#include <trx/game/shell.h>
#include <trx/version.h>

typedef struct {
    int32_t priority;
    const LEVEL_FORMAT_LOADER *loader;
} M_REGISTERED_LOADER;

static VECTOR *m_Loaders = nullptr;

__attribute__((destructor)) static void M_Shutdown(void)
{
    if (m_Loaders != nullptr) {
        Vector_Free(m_Loaders);
        m_Loaders = nullptr;
    }
}

static int32_t M_GetRegisteredLoaderCount(void)
{
    if (m_Loaders == nullptr) {
        return 0;
    }
    return m_Loaders->count;
}

static const LEVEL_FORMAT_LOADER *M_GetRegisteredLoader(const int32_t index)
{
    return ((M_REGISTERED_LOADER *)Vector_Get(m_Loaders, index))->loader;
}

void Level_Format_RegisterLoader(
    const int32_t priority, const LEVEL_FORMAT_LOADER *const loader)
{
    if (m_Loaders == nullptr) {
        m_Loaders = Vector_Create(sizeof(M_REGISTERED_LOADER));
    }

    M_REGISTERED_LOADER registered = {
        .priority = priority,
        .loader = loader,
    };

    int32_t insert_idx = m_Loaders->count;
    for (int32_t i = 0; i < m_Loaders->count; i++) {
        const M_REGISTERED_LOADER *const test = Vector_Get(m_Loaders, i);
        if (priority < test->priority) {
            insert_idx = i;
            break;
        }
    }
    Vector_Insert(m_Loaders, insert_idx, &registered);
}

const LEVEL_FORMAT_LOADER *Level_Format_GuessLoader(TRX_FILE *const file)
{
    const LEVEL_FORMAT_LOADER *result = nullptr;
    BENCHMARK benchmark = Benchmark_Start();
    const int32_t loader_count = M_GetRegisteredLoaderCount();
    for (int32_t i = 0; i < loader_count; i++) {
        const LEVEL_FORMAT_LOADER *const loader = M_GetRegisteredLoader(i);
        if (IS_OK(loader->probe(loader, file, LEVEL_FORMAT_PROBE_MINIMAL))) {
            result = loader;
            break;
        }
    }
    Benchmark_End(&benchmark, nullptr);
    return result;
}

LEVEL_FORMAT_LAYOUT Level_Format_GuessLayout(TRX_FILE *const file)
{
    const LEVEL_FORMAT_LOADER *const loader = Level_Format_GuessLoader(file);
    if (loader != nullptr) {
        return loader->layout;
    }
    return LEVEL_FORMAT_LAYOUT_UNKNOWN;
}

RESULT Level_Format_LoadFromFile(
    const GF_LEVEL *const level, const LEVEL_FORMAT_LOADER **const out_loader)
{
    *out_loader = nullptr;
    GameBuf_Reset();

    BENCHMARK benchmark = Benchmark_Start();
    TRX_FILE *file = nullptr;
    MUST(File_OpenPathInMemory(level->path, &file));

    const LEVEL_FORMAT_LOADER *const loader = Level_Format_GuessLoader(file);
    if (loader == nullptr) {
        File_Close(file);
        return FAIL("%s: the level is in no format TRX knows", level->path);
    }
    g_TRVersion = loader->game_version;
    Level_Context_Reset(loader);
    ASSERT(loader->load != nullptr);
    const RESULT result = loader->load(loader, file);

    File_Close(file);
    Benchmark_End(&benchmark, nullptr);
    MUST(result, "%s", level->path);
    *out_loader = loader;
    return OK;
}
