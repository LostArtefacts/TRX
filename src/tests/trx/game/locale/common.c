#include <harness/harness.h>
#include <harness/stubs_paths.h>

#include <trx/config/common.h>
#include <trx/config/vars.h>
#include <trx/core/memory.h>
#include <trx/core/subsystem.h>
#include <trx/core/vector.h>
#include <trx/game/game_flow/common.h>
#include <trx/game/game_strings/manager.h>
#include <trx/game/locale/common.h>
#include <trx/game/replay/test_replay.h>
#include <trx/game/shell/args.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

static LOCALE_PATH_ENTRY m_FrenchIntro = {
    .lang = "fr",
    .name_count = 1,
    .names = (char *[]) { "intr_fre.avi" },
};
static GF_FMV m_FMV = {
    .localized_path = { .entry_count = 1, .entries = &m_FrenchIntro },
};

static SUBSYSTEM *m_LocaleSubsystem = nullptr;
static EVENT_LISTENER m_ConfigListener = nullptr;
static bool m_Fired[LOCALE_ROLE_NUMBER_OF] = {};

static void M_SetLanguages(const char *const audio, const char *const text)
{
    g_ConfigStorage.audio.media_language = (char *)audio;
    g_ConfigStorage.language = (char *)text;
}

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

// An install with an Italian dub folder beside the retail FMVs, the French
// intro under the name the game flow gives it, a folder whose name is no
// language, and a file whose name is one.
static const char *M_MakeInstall(void)
{
    static char dir[64];
    snprintf(dir, sizeof(dir), "/tmp/trx-locale-XXXXXX");
    if (mkdtemp(dir) == nullptr) {
        return nullptr;
    }
    M_MakeDir(dir, "fmv");
    M_MakeDir(dir, "fmv/it");
    M_MakeDir(dir, "fmv/extras");
    M_Touch(dir, "fmv/de");
    M_Touch(dir, "fmv/intr_fre.avi");

    static const SHELL_ARGS args = {};
    setenv("TRX_DIR", dir, 1);
    GamePath_Init(&args);
    return dir;
}

static void M_RemoveInstall(const char *const dir)
{
    static const char *const entries[] = {
        "fmv/intr_fre.avi", "fmv/de", "fmv/extras", "fmv/it", "fmv", "",
    };
    for (size_t i = 0; i < sizeof(entries) / sizeof(entries[0]); i++) {
        char path[512];
        snprintf(path, sizeof(path), "%s/%s", dir, entries[i]);
        remove(path);
    }
}

static void M_FreeStrings(VECTOR *const strings)
{
    for (int32_t i = 0; i < strings->count; i++) {
        Memory_Free(*(char **)Vector_Get(strings, i));
    }
    Vector_Free(strings);
}

static void M_RecordChange(const EVENT *const event, void *const user_data)
{
    m_Fired[(LOCALE_ROLE)(intptr_t)event->data] = true;
}

// Moves one setting the way a settings write does, and reports which roles
// were told.
static void M_ChangeSetting(const void *const mirror)
{
    for (int32_t i = 0; i < LOCALE_ROLE_NUMBER_OF; i++) {
        m_Fired[i] = false;
    }
    const EVENT event = { .name = "change", .data = (void *)mirror };
    m_ConfigListener(&event, nullptr);
}

void Subsystem_Register(SUBSYSTEM *const subsystem)
{
    m_LocaleSubsystem = subsystem;
}

int32_t Config_SubscribeChanges(
    const EVENT_LISTENER listener, void *const user_data)
{
    m_ConfigListener = listener;
    return 0;
}

void Config_UnsubscribeChanges(const int32_t listener_id)
{
    m_ConfigListener = nullptr;
}

// What a change carries here is the address of the setting it moved.
bool Config_Change_HasMirror(
    const CONFIG_CHANGE *const change, const void *const mirror)
{
    return (const void *)change == mirror;
}

bool Config_IsFirstRun(void)
{
    return false;
}

bool Config_SetValue(const void *const mirror, const TRX_VALUE value)
{
    return false;
}

bool TestReplay_IsOpened(void)
{
    return false;
}

VECTOR *GameStringManager_GetAvailableLanguages(void)
{
    return nullptr;
}

const char *GameStringManager_GetLanguageName(const char *const code)
{
    return nullptr;
}

const char *GameStringLang_MatchPreferred(
    const VECTOR *const available, const VECTOR *const preferred)
{
    return nullptr;
}

int32_t GF_GetFMVCount(void)
{
    return 1;
}

const GF_FMV *GF_GetFMV(const int32_t num)
{
    return num == 1 ? &m_FMV : nullptr;
}

TEST(auto_hears_the_text_language)
{
    M_SetLanguages(LOCALE_AUDIO_AUTO, "it");
    const LOCALE_CHAIN chain = Locale_GetChain(LOCALE_ROLE_AUDIO);
    CHECK_EQ_INT(chain.count, 1);
    CHECK_EQ_STR(chain.codes[0], "it");
}

TEST(retail_hears_no_language_and_leaves_the_text_alone)
{
    M_SetLanguages(LOCALE_AUDIO_RETAIL, "it");
    CHECK_EQ_INT(Locale_GetChain(LOCALE_ROLE_AUDIO).count, 0);
    CHECK_NULL(Locale_GetCode(LOCALE_ROLE_AUDIO));
    CHECK_EQ_STR(Locale_GetCode(LOCALE_ROLE_TEXT), "it");
}

TEST(a_chosen_dub_differs_from_the_text)
{
    M_SetLanguages("de", "it");
    CHECK_EQ_STR(Locale_GetCode(LOCALE_ROLE_AUDIO), "de");
    CHECK_EQ_STR(Locale_GetCode(LOCALE_ROLE_TEXT), "it");
}

TEST(subtitles_follow_the_text)
{
    M_SetLanguages("de", "pt-br");
    const LOCALE_CHAIN chain = Locale_GetChain(LOCALE_ROLE_SUBTITLES);
    CHECK_EQ_INT(chain.count, 2);
    CHECK_EQ_STR(chain.codes[0], "pt-br");
}

TEST(dubs_come_from_folders_and_the_game_flow)
{
    const char *const dir = M_MakeInstall();
    CHECK_NOT_NULL(dir);
    VECTOR *const codes = Locale_GetAvailable(LOCALE_ROLE_AUDIO);
    CHECK_EQ_INT(codes->count, 2);
    if (codes->count == 2) {
        CHECK_EQ_STR(*(char **)Vector_Get(codes, 0), "it");
        CHECK_EQ_STR(*(char **)Vector_Get(codes, 1), "fr");
    }
    M_FreeStrings(codes);
    M_RemoveInstall(dir);
}

TEST(changing_the_text_language_tells_every_role)
{
    CHECK_NOT_NULL(m_LocaleSubsystem);
    m_LocaleSubsystem->init();
    const int32_t listener = Locale_SubscribeChanges(M_RecordChange, nullptr);
    M_ChangeSetting(&g_ConfigStorage.language);
    CHECK(m_Fired[LOCALE_ROLE_TEXT]);
    CHECK(m_Fired[LOCALE_ROLE_AUDIO]);
    CHECK(m_Fired[LOCALE_ROLE_SUBTITLES]);
    Locale_UnsubscribeChanges(listener);
    m_LocaleSubsystem->shutdown();
}

TEST(changing_the_dub_tells_the_audio_alone)
{
    CHECK_NOT_NULL(m_LocaleSubsystem);
    m_LocaleSubsystem->init();
    const int32_t listener = Locale_SubscribeChanges(M_RecordChange, nullptr);
    M_ChangeSetting(&g_ConfigStorage.audio.media_language);
    CHECK(!m_Fired[LOCALE_ROLE_TEXT]);
    CHECK(m_Fired[LOCALE_ROLE_AUDIO]);
    CHECK(!m_Fired[LOCALE_ROLE_SUBTITLES]);
    Locale_UnsubscribeChanges(listener);
    m_LocaleSubsystem->shutdown();
}
