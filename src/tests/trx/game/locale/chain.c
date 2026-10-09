#include <harness/harness.h>
#include <harness/stubs_paths.h>

#include <trx/core/memory.h>
#include <trx/core/vector.h>
#include <trx/game/locale/chain.h>
#include <trx/game/shell/args.h>

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

static void M_Touch(const char *const dir, const char *const rel)
{
    char path[512];
    snprintf(path, sizeof(path), "%s/%s", dir, rel);
    FILE *const file = fopen(path, "w");
    if (file != nullptr) {
        fclose(file);
    }
}

static void M_MakeDir(const char *const dir, const char *const rel)
{
    char path[512];
    snprintf(path, sizeof(path), "%s/%s", dir, rel);
    mkdir(path, 0755);
}

// An install with an Italian copy of one retail FMV and not of another.
static const char *M_MakeInstall(void)
{
    static char dir[64];
    snprintf(dir, sizeof(dir), "/tmp/trx-locale-chain-XXXXXX");
    if (mkdtemp(dir) == nullptr) {
        return nullptr;
    }
    M_MakeDir(dir, "fmv");
    M_MakeDir(dir, "fmv/it");
    M_Touch(dir, "fmv/intro.avi");
    M_Touch(dir, "fmv/it/intro.avi");
    M_Touch(dir, "fmv/ending.avi");

    static const SHELL_ARGS args = {};
    setenv("TRX_DIR", dir, 1);
    GamePath_Init(&args);
    return dir;
}

static void M_RemoveInstall(const char *const dir)
{
    static const char *const entries[] = {
        "fmv/ending.avi",
        "fmv/it/intro.avi",
        "fmv/intro.avi",
        "fmv/it",
        "fmv",
        "",
    };
    for (size_t i = 0; i < sizeof(entries) / sizeof(entries[0]); i++) {
        char path[512];
        snprintf(path, sizeof(path), "%s/%s", dir, entries[i]);
        remove(path);
    }
}

static char *M_Record(const char *const path, void *const user_data)
{
    VECTOR *const tried = user_data;
    char *const copy = Memory_DupStr(path);
    Vector_Add(tried, &copy);
    return nullptr;
}

static void M_FreeStrings(VECTOR *const strings)
{
    for (int32_t i = 0; i < strings->count; i++) {
        Memory_Free(*(char **)Vector_Get(strings, i));
    }
    Vector_Free(strings);
}

TEST(a_language_alone_makes_a_chain_of_one)
{
    const LOCALE_CHAIN chain = LocaleChain_FromCode("it");
    CHECK_EQ_INT(chain.count, 1);
    CHECK_EQ_STR(chain.codes[0], "it");
}

TEST(a_regional_code_falls_back_to_its_language)
{
    const LOCALE_CHAIN chain = LocaleChain_FromCode("pt-BR");
    CHECK_EQ_INT(chain.count, 2);
    CHECK_EQ_STR(chain.codes[0], "pt-br");
    CHECK_EQ_STR(chain.codes[1], "pt");
}

TEST(no_code_makes_an_empty_chain)
{
    CHECK_EQ_INT(LocaleChain_FromCode(nullptr).count, 0);
    CHECK_EQ_INT(LocaleChain_FromCode("").count, 0);
}

TEST(each_language_is_looked_for_beside_the_file_before_the_file)
{
    const LOCALE_CHAIN chain = LocaleChain_FromCode("pt-br");
    VECTOR *const tried = Vector_Create(sizeof(char *));
    CHECK_NULL(LocaleChain_Find(&chain, "audio/011.wav", M_Record, tried));
    CHECK_EQ_INT(tried->count, 3);
    CHECK_EQ_STR(*(char **)Vector_Get(tried, 0), "audio/pt-br/011.wav");
    CHECK_EQ_STR(*(char **)Vector_Get(tried, 1), "audio/pt/011.wav");
    CHECK_EQ_STR(*(char **)Vector_Get(tried, 2), "audio/011.wav");
    M_FreeStrings(tried);
}

TEST(a_bare_name_gets_a_language_folder_of_its_own)
{
    const LOCALE_CHAIN chain = LocaleChain_FromCode("it");
    VECTOR *const tried = Vector_Create(sizeof(char *));
    CHECK_NULL(LocaleChain_Find(&chain, "intro.avi", M_Record, tried));
    CHECK_EQ_INT(tried->count, 2);
    CHECK_EQ_STR(*(char **)Vector_Get(tried, 0), "it/intro.avi");
    M_FreeStrings(tried);
}

TEST(an_empty_chain_tries_the_file_alone)
{
    const LOCALE_CHAIN chain = {};
    VECTOR *const tried = Vector_Create(sizeof(char *));
    CHECK_NULL(LocaleChain_Find(&chain, "intro.avi", M_Record, tried));
    CHECK_EQ_INT(tried->count, 1);
    CHECK_EQ_STR(*(char **)Vector_Get(tried, 0), "intro.avi");
    M_FreeStrings(tried);
}

TEST(the_languages_copy_wins_over_the_retail_one)
{
    const char *const dir = M_MakeInstall();
    CHECK_NOT_NULL(dir);
    const LOCALE_CHAIN chain = LocaleChain_FromCode("it");
    char *const path =
        LocaleChain_Resolve(&chain, GAME_DYNAMIC_PATH_FMV_FILE, "intro.rpl");
    char expected[128];
    snprintf(expected, sizeof(expected), "%s/fmv/it/intro.avi", dir);
    CHECK_EQ_STR(path, expected);
    Memory_Free(path);
    M_RemoveInstall(dir);
}

TEST(what_a_language_leaves_out_comes_from_the_retail_files)
{
    const char *const dir = M_MakeInstall();
    CHECK_NOT_NULL(dir);
    const LOCALE_CHAIN chain = LocaleChain_FromCode("it");
    char *const path =
        LocaleChain_Resolve(&chain, GAME_DYNAMIC_PATH_FMV_FILE, "ending.avi");
    char expected[128];
    snprintf(expected, sizeof(expected), "%s/fmv/ending.avi", dir);
    CHECK_EQ_STR(path, expected);
    Memory_Free(path);
    M_RemoveInstall(dir);
}
