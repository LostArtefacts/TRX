// Enough of the engine for the UI to measure and lay out a scene: the font
// metrics, the viewport it sizes itself against, and no-op draw scheduling.
//
// The metrics come out of the game's own font.bin, so text measures here
// exactly as it does in the game. Each game ships its own font, hence one per
// version.

#include <fakes/ui.h>

#include <harness/font_bin.h>

#include <trx/core/strings.h>
#include <trx/debug.h>
#include <trx/game/input/common.h>
#include <trx/game/objects/common.h>
#include <trx/game/output/textures.h>
#include <trx/game/viewport.h>
#include <trx/version.h>

static FONT_BIN m_Fonts[TR_VERSION_COUNT];
static FONT_BIN *m_Font = nullptr;
static OBJECT m_Objects[O_NUMBER_OF];
static int32_t m_ViewportWidth = 640;
static int32_t m_ViewportHeight = 480;

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

// The font resolves "\{input <role>}" through the current binding. Nothing here
// binds keys, so every role reports the widest keycap in the sheet: a check
// built on these metrics then holds for any binding a player can make.
const char *Input_GetKeyName(
    const INPUT_BACKEND backend, const INPUT_LAYOUT layout,
    const INPUT_ROLE role, const int32_t slot)
{
    return "\\{keyboard backspace}";
}
