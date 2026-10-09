#include <harness/harness.h>
#include <harness/stubs_paths.h>

#include <trx/core/memory.h>
#include <trx/game/locale/chain.h>
#include <trx/game/locale/path.h>
#include <trx/game/shell/args.h>

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

static char m_Dir[64];

// The intro under its English and its Italian retail name, with a French copy
// of the English one in a language folder, and the ending only in German.
static LOCALE_PATH_ENTRY m_IntroEntries[] = {
    { .lang = nullptr,
      .name_count = 1,
      .names = (char *[]) { "intr_eng.avi" } },
    { .lang = "it", .name_count = 1, .names = (char *[]) { "intr_ita.avi" } },
};
static const LOCALE_PATH m_Intro = { .entry_count = 2,
                                     .entries = m_IntroEntries };

static LOCALE_PATH_ENTRY m_EndingEntries[] = {
    { .lang = nullptr, .name_count = 1, .names = (char *[]) { "ending.avi" } },
    { .lang = "de", .name_count = 1, .names = (char *[]) { "ende.avi" } },
};
static const LOCALE_PATH m_Ending = {
    .entry_count = 2,
    .entries = m_EndingEntries,
};

static void M_Touch(const char *const rel)
{
    char path[512];
    snprintf(path, sizeof(path), "%s/%s", m_Dir, rel);
    FILE *const file = fopen(path, "w");
    if (file != nullptr) {
        fclose(file);
    }
}

static void M_MakeDir(const char *const rel)
{
    char path[512];
    snprintf(path, sizeof(path), "%s/%s", m_Dir, rel);
    mkdir(path, 0755);
}

static bool M_MakeInstall(void)
{
    snprintf(m_Dir, sizeof(m_Dir), "/tmp/trx-locale-path-XXXXXX");
    if (mkdtemp(m_Dir) == nullptr) {
        return false;
    }
    M_MakeDir("fmv");
    M_MakeDir("fmv/fr");
    M_Touch("fmv/intr_eng.avi");
    M_Touch("fmv/intr_ita.avi");
    M_Touch("fmv/fr/intr_eng.avi");
    M_Touch("fmv/ende.avi");

    static const SHELL_ARGS args = {};
    setenv("TRX_DIR", m_Dir, 1);
    GamePath_Init(&args);
    return true;
}

static void M_RemoveInstall(void)
{
    static const char *const entries[] = {
        "fmv/ende.avi",
        "fmv/fr/intr_eng.avi",
        "fmv/intr_ita.avi",
        "fmv/intr_eng.avi",
        "fmv/fr",
        "fmv",
        "",
    };
    for (size_t i = 0; i < sizeof(entries) / sizeof(entries[0]); i++) {
        char path[512];
        snprintf(path, sizeof(path), "%s/%s", m_Dir, entries[i]);
        remove(path);
    }
}

static void M_CheckResolves(
    const LOCALE_PATH *const path, const char *const code,
    const char *const expected_rel)
{
    const LOCALE_CHAIN chain = LocaleChain_FromCode(code);
    char *const resolved =
        LocalePath_Resolve(path, &chain, GAME_DYNAMIC_PATH_FMV_FILE);
    char expected[128];
    snprintf(expected, sizeof(expected), "%s/%s", m_Dir, expected_rel);
    CHECK_EQ_STR(resolved, expected);
    Memory_Free(resolved);
}

TEST(a_languages_own_name_comes_first)
{
    CHECK(M_MakeInstall());
    M_CheckResolves(&m_Intro, "it", "fmv/intr_ita.avi");
    M_RemoveInstall();
}

TEST(the_fallback_name_is_looked_for_in_the_language_folder)
{
    CHECK(M_MakeInstall());
    M_CheckResolves(&m_Intro, "fr", "fmv/fr/intr_eng.avi");
    M_RemoveInstall();
}

TEST(no_language_plays_the_fallback_name)
{
    CHECK(M_MakeInstall());
    M_CheckResolves(&m_Intro, nullptr, "fmv/intr_eng.avi");
    M_RemoveInstall();
}

TEST(another_languages_name_stands_in_when_nothing_else_is_installed)
{
    CHECK(M_MakeInstall());
    M_CheckResolves(&m_Ending, "it", "fmv/ende.avi");
    M_RemoveInstall();
}

TEST(freeing_leaves_the_path_empty)
{
    LOCALE_PATH path = {
        .entry_count = 1,
        .entries = Memory_Alloc(sizeof(LOCALE_PATH_ENTRY)),
    };
    path.entries[0].name_count = 1;
    path.entries[0].names = Memory_Alloc(sizeof(char *));
    path.entries[0].names[0] = Memory_DupStr("intro.avi");
    path.entries[0].lang = Memory_DupStr("it");
    LocalePath_Free(&path);
    CHECK_EQ_INT(path.entry_count, 0);
    CHECK_NULL(path.entries);
}
