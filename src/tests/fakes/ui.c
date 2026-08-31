#include <harness/fake_objects.h>
#include <fakes/ui.h>

#include <harness/font_bin.h>

#include <trx/core/enum_map.h>
#include <trx/core/strings.h>
#include <trx/debug.h>
#include <trx/game/game_strings/entries.h>
#include <trx/game/input/common.h>
#include <trx/game/objects/common.h>
#include <trx/game/objects/families.h>
#include <trx/game/objects/vars.h>
#include <trx/game/output/textures.h>
#include <trx/game/viewport.h>
#include <trx/version.h>

#define M_DEFAULT_KEY_NAME "\\{keyboard backspace}"

static FONT_BIN m_Fonts[TR_VERSION_COUNT];
static FONT_BIN *m_Font = nullptr;
static OBJECT m_Objects[FAKE_OBJ_COUNT];
static int32_t m_ViewportWidth = 640;
static int32_t m_ViewportHeight = 480;
static const char *m_KeyName = M_DEFAULT_KEY_NAME;

void FakeUI_SetGame(const int32_t tr_version)
{
    g_TRVersion = tr_version;

    FONT_BIN *const font = &m_Fonts[tr_version - 1];
    if (font->sprite_count == 0) {
        const char *const path = String_FormatStatic(
            "%s/tr%d/injections/font.bin", TEST_SHIP_GAMES_DIR, tr_version);
        // Without the font every measurement would come out zero, and a check
        // on whether text fits would pass no matter what.
        ASSERT(FontBin_Load(path, font));
    }
    m_Font = font;

    m_Objects[O_ALPHABET] = (OBJECT) {
        .loaded = true,
        .mesh_idx = font->font_base[0],
        .mesh_count = font->font_count[0],
    };
    m_Objects[O_ALPHABET_SMALL] = (OBJECT) {
        .loaded = true,
        .mesh_idx = font->font_base[1],
        .mesh_count = font->font_count[1],
    };
}

void FakeUI_SetViewport(const int32_t width, const int32_t height)
{
    m_ViewportWidth = width;
    m_ViewportHeight = height;
}

void FakeUI_Shutdown(void)
{
    for (int32_t i = 0; i < TR_VERSION_COUNT; i++) {
        FontBin_Free(&m_Fonts[i]);
    }
    m_Font = nullptr;
    m_KeyName = M_DEFAULT_KEY_NAME;
}

OBJECT *Object_Get(const OBJECT_ID object_id)
{
    return &m_Objects[object_id];
}

SPRITE_TEXTURE *Output_GetSpriteTexture(const int32_t texture_idx)
{
    if (m_Font == nullptr || texture_idx < 0
        || texture_idx >= m_Font->sprite_count) {
        return nullptr;
    }
    return &m_Font->sprites[texture_idx];
}

int32_t Viewport_GetWidth(const VIEWPORT_SPACE space)
{
    return m_ViewportWidth;
}

int32_t Viewport_GetHeight(const VIEWPORT_SPACE space)
{
    return m_ViewportHeight;
}

const char *Input_GetKeyName(
    const INPUT_BACKEND backend, const INPUT_LAYOUT layout,
    const INPUT_ROLE role, const int32_t slot)
{
    return m_KeyName;
}

void FakeUI_SetKeyName(const char *const key_name)
{
    m_KeyName = key_name;
}

void FakeUI_ResetKeyName(void)
{
    m_KeyName = M_DEFAULT_KEY_NAME;
}

const char *Input_GetRoleName(const INPUT_ROLE role)
{
    return EnumMap_GetLabel(ENUM_MAP_NAME(INPUT_ROLE), role);
}

const char *const *Input_GetLayoutNamePtr(const INPUT_LAYOUT layout)
{
    static const GAME_STRING_ID layout_names[INPUT_LAYOUT_NUMBER_OF] = {
        [INPUT_LAYOUT_DEFAULT] =
            GS_ID("general/settings/controls/layout/default"),
        [INPUT_LAYOUT_CUSTOM_1] =
            GS_ID("general/settings/controls/layout/custom_1"),
        [INPUT_LAYOUT_CUSTOM_2] =
            GS_ID("general/settings/controls/layout/custom_2"),
        [INPUT_LAYOUT_CUSTOM_3] =
            GS_ID("general/settings/controls/layout/custom_3"),
    };
    return GameString_GetPtr(layout_names[layout]);
}

void Input_Update(void)
{
}

bool Input_IsInListenMode(void)
{
    return false;
}

bool Input_IsRoleUnbindable(const INPUT_ROLE role)
{
    return true;
}

bool Input_IsKeyConflicted(
    const INPUT_BACKEND backend, const INPUT_LAYOUT layout,
    const INPUT_ROLE role)
{
    return false;
}

bool Input_ReadAndAssignRole(
    const INPUT_BACKEND backend, const INPUT_LAYOUT layout,
    const INPUT_ROLE role, const int32_t slot)
{
    return false;
}

void Input_UnassignRole(
    const INPUT_BACKEND backend, const INPUT_LAYOUT layout,
    const INPUT_ROLE role, const int32_t slot)
{
}

void Input_ResetLayout(const INPUT_BACKEND backend, const INPUT_LAYOUT layout)
{
}

bool InputState_IsAnyPressed(const INPUT_STATE state)
{
    return false;
}

void TouchOverlay_EnterSelectionMode(void)
{
}

void TouchOverlay_ExitSelectionMode(void)
{
}

// Include only the secret glyph family because the shipped game text defines
// names only for those glyphs.
OBJECT_ID ObjectFamily_GetMember(const OBJECT_FAMILY family, const int32_t idx)
{
    return g_SecretObjects[idx];
}
