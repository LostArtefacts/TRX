#include <trx/game/ui/text.h>

#include <trx/config.h>
#include <trx/core/enum_map.h>
#include <trx/core/log.h>
#include <trx/core/memory.h>
#include <trx/core/strings.h>
#include <trx/core/utils.h>
#include <trx/core/vector.h>
#include <trx/debug.h>
#include <trx/game/clock/common.h>
#include <trx/game/clock/const.h>
#include <trx/game/input/common.h>
#include <trx/game/objects.h>
#include <trx/game/objects/families.h>
#include <trx/game/output/textures.h>
#include <trx/game/ui/common.h>
#include <trx/game/ui/draw.h>
#include <trx/game/ui/scaler.h>
#include <trx/version.h>

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <uthash.h>

#define M_LETTER_SPACING 0.5f
#define M_WORD_SPACING 6.0f
#define M_TR4_SHADE_ROWS 16
#define M_TR4_BASE_ROW 11
#define M_TR4_PULSE_FRAMES 32
#define M_LINE_HEIGHT 15
#define M_TR4_LINE_HEIGHT 29
#define M_TR4_LINE_PITCH 24
#define M_TR4_BASELINE 24

typedef enum {
    M_FONT_DEFAULT = 0,
    M_FONT_SMALL = 1,
    M_FONT_COUNT,
} M_FONT;

typedef enum {
    // A text character.
    GLYPH_TEXT,
    // An icon.
    GLYPH_ICON,
    // Spacing between words.
    GLYPH_SPACE,
    // Line break.
    GLYPH_NEW_LINE,
    // Marker used in the examine item dialog and others to force a new page.
    GLYPH_NEW_PAGE,
    // Icon for collectible secrets, taking the sprite from O_SECRET
    GLYPH_SECRET,
    // Icon requesting translators to verify AI-translated text.
    GLYPH_REVIEW_MARKER,
    // Marker that toggles the visibility of the following text.
    GLYPH_VISIBILITY_MARKER,
    // Marker that changes the color of the following text.
    GLYPH_COLOR_MARKER,
    // Marker that changes the font of the following text.
    // - mesh_idx = 0: default font (O_ALPHABET).
    // - mesh_idx = 1: default font (O_ALPHABET_SMALL).
    GLYPH_FONT_MARKER,
    // Glyph that dynamically expands a key role to its current key icon.
    GLYPH_INPUT,
} M_GLYPH_ROLE;

typedef struct {
    const char *text;
    M_GLYPH_ROLE role;
    int32_t width[M_FONT_COUNT];
    union {
        int32_t mesh_idx;
        INPUT_ROLE input_role; // for role == GLYPH_INPUT
    };
    // Whether the glyph was made for a colour that a text names, rather than
    // declared, so that it is freed with the colours.
    bool is_made;
} M_GLYPH_INFO;

// A colour that text is drawn in: the shade at the top of a glyph, the shade at
// its bottom, and whether it pulses. In TR4, a channel of 0x80 draws a glyph
// as it is, as the original lights its font.
typedef struct {
    RGBA_8888 top;
    RGBA_8888 bottom;
    bool pulse;
} M_COLOR;

typedef struct {
    char *spec;
    int32_t idx;
    UT_hash_handle hh;
} M_COLOR_MAP_ENTRY;

typedef struct {
    M_GLYPH_INFO *glyph;
    UT_hash_handle hh;
} M_GLYPH_MAP_ENTRY;

typedef struct {
    char *text;
    const M_GLYPH_INFO **glyphs;
    size_t glyph_count;
    UT_hash_handle hh;
} M_TEXT_MAP_ENTRY;

typedef void (*M_DRAW_FUNC)(
    int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, const RGBA_F[4]);

static M_GLYPH_INFO m_Glyphs[] = {
#define X_GLYPH_DEFINE(text_, role_, mesh_idx_)                                \
    { .text = text_, .role = role_, .mesh_idx = mesh_idx_ },
#include <trx/game/ui/text.def>
    { .text = nullptr }, // guard
};

static M_GLYPH_MAP_ENTRY *m_GlyphMap = nullptr;
static M_TEXT_MAP_ENTRY *m_TextMap = nullptr;

OBJECT_ID m_FontObjects[M_FONT_COUNT] = {
    [M_FONT_DEFAULT] = O_ALPHABET,
    [M_FONT_SMALL] = O_ALPHABET_SMALL,
};

// The colours text has named, by the order they were first named in. The
// plain colour comes first.
static VECTOR *m_Colors = nullptr;
static M_COLOR_MAP_ENTRY *m_ColorMap = nullptr;
static int32_t m_DefaultColor = 0;
static const char *m_DefaultSpec = nullptr;

// The height of a capital in each font, in sprite units above the baseline,
// which the TR4 shades are spread over.
static int32_t m_CapHeight[M_FONT_COUNT] = {};

static float M_ScaleScreen(const float value)
{
    return UI_Scaler_Calc(value, UI_SCALER_TARGET_TEXT);
}

static float M_ScaleNeutral(const float value)
{
    return value * UI_Scaler_GetTextScale();
}

static const M_COLOR *M_GetColor(const int32_t color_idx)
{
    if (color_idx < 0 || color_idx >= m_Colors->count) {
        return Vector_Get(m_Colors, 0);
    }
    return Vector_Get(m_Colors, color_idx);
}

// Returns how bright a pulsing colour is now, from 0 to 128. It brightens and
// dims over 32 frames.
static int32_t M_GetPulse(void)
{
    const int32_t frame =
        (int32_t)(Clock_GetRealTime() * LOGIC_FPS) % M_TR4_PULSE_FRAMES;
    return (uint8_t)((frame > 16 ? (uint8_t)-frame : frame) << 3);
}

// Doubles a TR4 shade, as the original lights a glyph: 128 and above draw at
// full brightness.
static float M_TR4Light(const int32_t shade)
{
    return MIN(shade * 2, 255) / 255.0f;
}

// Returns a colour at a TR4 shade row, between its top shade at the first row
// and its bottom one sixteen rows down.
static RGBA_F M_GetTR4Shade(const M_COLOR *const color, const int32_t row)
{
    const int32_t rest = M_TR4_SHADE_ROWS - row;
    const int32_t pulse = color->pulse ? M_GetPulse() : 128;
    const RGBA_8888 from = color->top;
    const RGBA_8888 to = color->bottom;
#define L_SHADE(c)                                                             \
    ((((to.c * row) >> 4) + ((from.c * rest) >> 4)) * pulse / 128)
    const RGBA_F result = {
        .r = M_TR4Light(L_SHADE(r)),
        .g = M_TR4Light(L_SHADE(g)),
        .b = M_TR4Light(L_SHADE(b)),
        .a = (((to.a * row) >> 4) + ((from.a * rest) >> 4)) / 255.0f,
    };
#undef L_SHADE
    return result;
}

// Returns a colour at the top or the bottom of a glyph outside TR4.
static RGBA_F M_GetShade(const M_COLOR *const color, const bool bottom)
{
    const RGBA_8888 c = bottom ? color->bottom : color->top;
    const float light = color->pulse ? M_GetPulse() / 128.0f : 1.0f;
    return (RGBA_F) {
        .r = c.r * light / 255.0f,
        .g = c.g * light / 255.0f,
        .b = c.b * light / 255.0f,
        .a = c.a / 255.0f,
    };
}

// Returns the TR4 shade row at a height in a glyph's sprite. The original
// shades the top of a capital at the first row and the baseline at row 11.
static int32_t M_GetTR4Row(const M_FONT font, const int32_t y)
{
    const int32_t cap = m_CapHeight[font];
    if (cap <= 0) {
        return M_TR4_BASE_ROW;
    }
    const int32_t above = MAX(y + cap, 0);
    int32_t row = (M_TR4_BASE_ROW * above + cap / 2) / cap;
    CLAMP(row, 0, M_TR4_SHADE_ROWS - 1);
    return row;
}

// Returns the height of a line of text.
static int32_t M_GetLineHeight(void)
{
    return g_TRVersion == 4 ? M_TR4_LINE_HEIGHT : M_LINE_HEIGHT;
}

// Returns the distance from the top of a line of text to its baseline. In TR4,
// the baseline sits above the bottom of the line, so that the capital letters
// are centred in it.
static int32_t M_GetBaseline(void)
{
    return g_TRVersion == 4 ? M_TR4_BASELINE : M_LINE_HEIGHT;
}

// Returns the distance between the baselines of two lines of text. In TR4,
// the lines of a passage sit closer together than the height of one line.
static int32_t M_GetLinePitch(void)
{
    return g_TRVersion == 4 ? M_TR4_LINE_PITCH : M_LINE_HEIGHT;
}

// Colours the corners of a sprite in a colour. TR4 shades a glyph by the rows
// of the font that its sprite reaches, and the other games from the top of the
// sprite to its bottom. `top_row` and `bottom_row` are the TR4 rows.
static void M_ShadeCorners(
    const int32_t color_idx, const int32_t top_row, const int32_t bottom_row,
    RGBA_F out[4])
{
    const M_COLOR *const color = M_GetColor(color_idx);
    if (g_TRVersion == 4) {
        out[0] = M_GetTR4Shade(color, top_row);
        out[2] = M_GetTR4Shade(color, bottom_row);
    } else {
        out[0] = M_GetShade(color, false);
        out[2] = M_GetShade(color, true);
    }
    out[1] = out[0];
    out[3] = out[2];
}

static void M_ShadeGlyph(
    const M_FONT font, const int32_t sprite_idx, const int32_t color_idx,
    RGBA_F out[4])
{
    const SPRITE_TEXTURE *const sprite = Output_GetSpriteTexture(sprite_idx);
    M_ShadeCorners(
        color_idx, M_GetTR4Row(font, sprite->y0), M_GetTR4Row(font, sprite->y1),
        out);
}

static bool M_ParseHex(const char *const text, RGBA_8888 *const out)
{
    const size_t len = strlen(text);
    if (text[0] != '#' || (len != 7 && len != 9)) {
        return false;
    }
    for (size_t i = 1; i < len; i++) {
        if (!isxdigit((unsigned char)text[i])) {
            return false;
        }
    }
    uint32_t r, g, b, a = 0xFF;
    if (sscanf(text + 1, "%2x%2x%2x", &r, &g, &b) != 3) {
        return false;
    }
    if (len == 9 && sscanf(text + 7, "%2x", &a) != 1) {
        return false;
    }
    *out = (RGBA_8888) { .r = r, .g = g, .b = b, .a = a };
    return true;
}

// Reads a colour as a text names it: one colour, or the colour at the top of a
// glyph and the one at its bottom, each as `#rrggbb` or `#rrggbbaa`, followed
// by `pulse` for a colour that pulses.
static bool M_ParseColor(const char *const spec, M_COLOR *const out)
{
    char *const copy = Memory_DupStr(spec);
    int32_t count = 0;
    bool pulse = false;
    bool ok = true;
    RGBA_8888 colors[2] = {};
    for (char *token = strtok(copy, " "); token != nullptr;
         token = strtok(nullptr, " ")) {
        if (strcmp(token, "pulse") == 0 && !pulse) {
            pulse = true;
        } else if (count < 2 && !pulse && M_ParseHex(token, &colors[count])) {
            count++;
        } else {
            ok = false;
            break;
        }
    }
    Memory_Free(copy);
    if (!ok || count == 0) {
        return false;
    }
    *out = (M_COLOR) {
        .top = colors[0],
        .bottom = count == 2 ? colors[1] : colors[0],
        .pulse = pulse,
    };
    return true;
}

static M_COLOR_MAP_ENTRY *M_AddColorEntry(
    const char *const spec, const int32_t idx)
{
    M_COLOR_MAP_ENTRY *const entry = Memory_Alloc(sizeof(*entry));
    entry->spec = Memory_DupStr(spec);
    entry->idx = idx;
    HASH_ADD_KEYPTR(hh, m_ColorMap, entry->spec, strlen(entry->spec), entry);
    return entry;
}

// Returns the colour that a text names, adding it where no text has named it
// before, or nullptr for one that names no colour.
static const M_COLOR_MAP_ENTRY *M_InternColor(const char *const spec)
{
    M_COLOR_MAP_ENTRY *entry;
    HASH_FIND_STR(m_ColorMap, spec, entry);
    if (entry != nullptr) {
        return entry;
    }
    M_COLOR color;
    if (!M_ParseColor(spec, &color)) {
        return nullptr;
    }
    Vector_Add(m_Colors, &color);
    return M_AddColorEntry(spec, m_Colors->count - 1);
}

// Finds the glyph that a piece of text stands for. A colour marker that names
// its colour, as `\{color #c08040}` does, has a glyph made for it the first
// time a text holds it.
static const M_GLYPH_INFO *M_FindGlyph(const char *const key)
{
    M_GLYPH_MAP_ENTRY *entry;
    HASH_FIND_STR(m_GlyphMap, key, entry);
    if (entry != nullptr) {
        return entry->glyph;
    }

    static const char prefix[] = "\\{color ";
    const size_t prefix_len = sizeof(prefix) - 1;
    const size_t len = strlen(key);
    if (len <= prefix_len + 1 || strncmp(key, prefix, prefix_len) != 0
        || key[len - 1] != '}') {
        return nullptr;
    }
    char *const spec = String_Format(
        "%.*s", (int32_t)(len - prefix_len - 1), key + prefix_len);
    const M_COLOR_MAP_ENTRY *const color = M_InternColor(spec);
    Memory_Free(spec);
    if (color == nullptr) {
        return nullptr;
    }

    M_GLYPH_INFO *const glyph = Memory_Alloc(sizeof(*glyph));
    glyph->text = Memory_DupStr(key);
    glyph->role = GLYPH_COLOR_MARKER;
    glyph->mesh_idx = color->idx;
    glyph->is_made = true;
    entry = Memory_Alloc(sizeof(*entry));
    entry->glyph = glyph;
    HASH_ADD_KEYPTR(hh, m_GlyphMap, glyph->text, strlen(glyph->text), entry);
    return glyph;
}

static int32_t M_HasGlyph(const M_FONT font, const M_GLYPH_INFO *const glyph)
{
    return glyph->width[font] > 0;
}

static int32_t M_GetGlyphWidth(
    const M_FONT font, const M_GLYPH_INFO *const glyph)
{
    // Non-breaking space
    if (strcmp(glyph->text, " ") == 0) {
        return M_WORD_SPACING;
    }

    if (glyph->role == GLYPH_SECRET) {
        return 16;
    }

    if (glyph->mesh_idx != -1
        && (glyph->role == GLYPH_TEXT || glyph->role == GLYPH_ICON
            || glyph->role == GLYPH_REVIEW_MARKER)) {
        const OBJECT *const object = Object_Get(m_FontObjects[font]);
        if (!object->loaded) {
            return -1;
        }
        if (glyph->mesh_idx >= ABS(object->mesh_count)) {
            return -1;
        }
        const SPRITE_TEXTURE *const sprite =
            Output_GetSpriteTexture(object->mesh_idx + glyph->mesh_idx);
        if (sprite == nullptr) {
            return -1;
        }
        if (sprite->x1 - sprite->x0 == 0 && sprite->width / 255 == 1) {
            // Just a placeholder glyph necessary for indexing of other glyphs
            return -1;
        }
        return sprite->width / 255;
    }

    return 0;
}

static const M_GLYPH_INFO **M_Decompose(
    const char *const content, size_t *const out_glyph_count)
{
    // Count number of characters
    size_t glyph_count = 0;
    const char *content_ptr = content;
    while (*content_ptr != '\0') {
        const size_t glyph_size = String_GetCharByteSize(content_ptr);
        content_ptr += glyph_size;
        glyph_count++;
    }

    // Assign glyphs using hash table
    const M_GLYPH_INFO **glyphs =
        Memory_Alloc((glyph_count + 1) * sizeof(M_GLYPH_INFO *));
    content_ptr = content;
    const M_GLYPH_INFO **glyph_ptr = glyphs;
    while (*content_ptr != '\0') {
        const size_t glyph_size = String_GetCharByteSize(content_ptr);
        const char *const key_buf =
            String_FormatStatic("%.*s", (int)glyph_size, content_ptr);
        const M_GLYPH_INFO *const glyph = M_FindGlyph(key_buf);

        if (glyph != nullptr) {
            *glyph_ptr++ = glyph;
        } else {
            LOG_WARNING("Unknown glyph: %s", key_buf);
            glyph_count--;
        }

        content_ptr += glyph_size;
    }

    if (out_glyph_count != nullptr) {
        *out_glyph_count = glyph_count;
    }

    // guard
    *glyph_ptr++ = nullptr;
    return glyphs;
}

static const M_GLYPH_INFO **M_DecomposeWithCache(
    const char *const content, size_t *const out_glyph_count)
{
    M_TEXT_MAP_ENTRY *entry;
    HASH_FIND_STR(m_TextMap, content, entry);
    if (entry == nullptr) {
        entry = Memory_Alloc(sizeof(M_TEXT_MAP_ENTRY));
        entry->text = Memory_DupStr(content);
        entry->glyphs = M_Decompose(content, &entry->glyph_count);
        HASH_ADD_STR(m_TextMap, text, entry);
    }
    if (out_glyph_count != nullptr) {
        *out_glyph_count = entry->glyph_count;
    }
    return entry->glyphs;
}

// The keys an input placeholder stands for, as the glyphs that spell them out.
// A combo such as Alt+Enter names several keys at once, so a placeholder can
// expand to more than one glyph.
static const M_GLYPH_INFO **M_GetInputGlyphs(
    const M_GLYPH_INFO *const glyph, size_t *const out_count)
{
    const INPUT_BACKEND backend = g_Config.input.backend;
    const INPUT_LAYOUT layout = g_Config.input.layout[backend];
    const char *const key_name =
        Input_GetKeyName(backend, layout, glyph->input_role, 0);
    if (key_name == nullptr) {
        return M_DecomposeWithCache("?", out_count);
    }

    const M_GLYPH_INFO **const parts =
        M_DecomposeWithCache(key_name, out_count);
    return *out_count > 0 ? parts : M_DecomposeWithCache("?", out_count);
}

static M_FONT M_GetGlyphFont(const M_GLYPH_INFO *const glyph, const M_FONT font)
{
    if (font == M_FONT_SMALL && !M_HasGlyph(font, glyph)) {
        return M_FONT_DEFAULT;
    }
    return font;
}

// Width the glyph occupies when drawn, matching the expansion and font
// fallback that M_Process applies.
static float M_GetLayoutWidth(
    const M_GLYPH_INFO *const glyph, const M_FONT font)
{
    if (glyph->role != GLYPH_INPUT) {
        return glyph->width[M_GetGlyphFont(glyph, font)];
    }

    size_t count = 0;
    const M_GLYPH_INFO **const parts = M_GetInputGlyphs(glyph, &count);
    float width = 0.0f;
    for (size_t i = 0; i < count; i++) {
        if (i > 0) {
            width += M_LETTER_SPACING;
        }
        width += parts[i]->width[M_GetGlyphFont(parts[i], font)];
    }
    return width;
}

// How far the continuations of a line are indented, so that a line broken in
// two still reads as one entry rather than as two at different depths.
static int32_t M_DetectHangingIndent(
    const M_GLYPH_INFO **glyphs, const size_t glyph_count, const size_t idx)
{
    size_t scan = idx;
    int32_t leading_spaces = 0;
    while (scan < glyph_count && glyphs[scan]->role == GLYPH_SPACE) {
        leading_spaces++;
        scan++;
    }
    if (scan + 1 < glyph_count && glyphs[scan]->role == GLYPH_TEXT
        && glyphs[scan]->text[0] == '-' && glyphs[scan]->text[1] == '\0'
        && glyphs[scan + 1]->role == GLYPH_SPACE) {
        return leading_spaces + 2;
    }
    return leading_spaces;
}

static void M_EmitIndent(
    char *const dst, size_t *const out_len, const int32_t indent,
    const float space_width, float *const cur_width)
{
    for (int32_t s = 0; s < indent; s++) {
        if (dst != nullptr) {
            dst[*out_len] = ' ';
        }
        (*out_len)++;
    }
    *cur_width += indent * space_width;
}

static void M_EmitNewline(
    char *const dst, size_t *const out_len, const int32_t indent,
    const float space_width, float *const cur_width)
{
    if (dst != nullptr) {
        dst[*out_len] = '\n';
    }
    (*out_len)++;
    *cur_width = 0.0f;
    if (indent > 0) {
        M_EmitIndent(dst, out_len, indent, space_width, cur_width);
    }
}

static size_t M_WordWrap(
    const M_GLYPH_INFO **glyphs, const size_t glyph_count, const float scale_f,
    const float max_width, char *const dst)
{
    size_t out_len = 0;
    float cur_width = 0.0f;
    int32_t hanging_indent = 0;

    const float space_width = M_WORD_SPACING * scale_f;

#define L_CONCAT_CHAR(part)                                                    \
    if (dst != nullptr) {                                                      \
        dst[out_len] = part;                                                   \
    }                                                                          \
    out_len++;
#define L_CONCAT_STR(part)                                                     \
    if (dst != nullptr) {                                                      \
        strcpy(dst + out_len, part);                                           \
    }                                                                          \
    out_len += strlen(part);

    M_FONT current_font = M_FONT_DEFAULT;

    // Iterate glyphs for wrapping
    for (size_t i = 0; i < glyph_count; i++) {
        const M_GLYPH_INFO *const glyph = glyphs[i];

        if (cur_width == 0.0f && hanging_indent == 0) {
            hanging_indent = M_DetectHangingIndent(glyphs, glyph_count, i);
        }

        if (glyph->role == GLYPH_FONT_MARKER) {
            current_font = glyph->mesh_idx;
        } else if (glyph->role == GLYPH_NEW_LINE) {
            L_CONCAT_CHAR('\n')
            cur_width = 0.0f;
            hanging_indent = 0;
        } else if (glyph->role == GLYPH_NEW_PAGE) {
            L_CONCAT_CHAR('\f')
            cur_width = 0.0f;
            hanging_indent = 0;
        } else if (glyph->role == GLYPH_SPACE) {
            const float w = M_WORD_SPACING * scale_f;
            if (cur_width + w > max_width) {
                M_EmitNewline(
                    dst, &out_len, hanging_indent, space_width, &cur_width);
            } else {
                L_CONCAT_CHAR(' ')
                cur_width += w;
            }
        } else if (
            glyph->role == GLYPH_REVIEW_MARKER
            && !g_Config.debug.enable_review_markers) {
            continue;
        } else {
            // Gather next word glyphs
            size_t word_len = 0;
            for (size_t j = i; j < glyph_count; j++) {
                if (glyphs[i + word_len]->role == GLYPH_SPACE
                    || glyphs[i + word_len]->role == GLYPH_NEW_LINE
                    || glyphs[i + word_len]->role == GLYPH_NEW_PAGE) {
                    break;
                }
                word_len++;
            }

            // Compute width (sum widths + spacing). The spacing after the last
            // glyph counts too: what follows the word is a space, which
            // M_Process draws after that spacing.
            float word_width = 0.0f;
            for (size_t j = i; j < i + word_len; j++) {
                word_width += M_LETTER_SPACING;
                word_width += M_GetLayoutWidth(glyphs[j], current_font);
            }
            word_width *= scale_f;

            // Wrap line if needed
            if (cur_width + word_width > max_width) {
                if (cur_width > 0.0f) {
                    M_EmitNewline(
                        dst, &out_len, hanging_indent, space_width, &cur_width);
                }

                // Break word if longer than line
                if (word_width > max_width) {
                    for (size_t j = i; j < i + word_len; j++) {
                        const M_GLYPH_INFO *const next_glyph = glyphs[j];
                        const float glyph_width =
                            (M_GetLayoutWidth(next_glyph, current_font)
                             + M_LETTER_SPACING)
                            * scale_f;
                        if (cur_width + glyph_width > max_width) {
                            M_EmitNewline(
                                dst, &out_len, hanging_indent, space_width,
                                &cur_width);
                        }
                        L_CONCAT_STR(next_glyph->text)
                        cur_width += glyph_width;
                    }
                } else {
                    for (size_t j = i; j < i + word_len; j++) {
                        const M_GLYPH_INFO *const next_glyph = glyphs[j];
                        L_CONCAT_STR(next_glyph->text)
                    }
                    cur_width = word_width;
                }
            } else {
                // Copy word as is
                for (size_t j = i; j < i + word_len; j++) {
                    const M_GLYPH_INFO *const next_glyph = glyphs[j];
                    L_CONCAT_STR(next_glyph->text)
                }
                cur_width += word_width;
            }

            // Skip forward the characters, respecting the default loop
            // accumulator
            i += word_len - 1;
        }
    }

    L_CONCAT_CHAR('\0')

#undef L_CONCAT_CHAR
#undef L_CONCAT_STR
    return out_len;
}

// Draws one font glyph and returns how far the cursor moves past it.
static float M_DrawGlyph(
    const M_GLYPH_INFO *const glyph, const M_FONT font, const float x,
    const float y, const int32_t z, const float scale, const bool spaced,
    const bool visible, const int32_t color_idx, const M_DRAW_FUNC draw_func)
{
    const M_FONT glyph_font = M_GetGlyphFont(glyph, font);
    float spacing = glyph->width[glyph_font];
    if (spaced) {
        spacing += M_LETTER_SPACING;
    }

    // A non-breaking space and its like take up room without drawing anything.
    const bool renders = glyph->role != GLYPH_TEXT || glyph->mesh_idx >= 0;
    if (renders && visible && draw_func != nullptr) {
        const OBJECT *const object = Object_Get(m_FontObjects[glyph_font]);
        const int32_t sprite_idx = object->mesh_idx + glyph->mesh_idx;
        RGBA_F colors[4];
        M_ShadeGlyph(glyph_font, sprite_idx, color_idx, colors);
        draw_func(x, y, z, scale, scale, sprite_idx, colors);
    }

    return spacing * scale / UI_TEXT_BASE_SCALE;
}

static void M_Process(
    const char *const text, float *const out_w, float *const out_h,
    const UI_TEXT_SETTINGS settings, const float base_x, const float base_y,
    float (*const scale_func)(float),
    void (*const draw_func)(
        int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, const RGBA_F[4]))
{
    if (text == nullptr) {
        return;
    }

    const M_GLYPH_INFO **glyphs = M_DecomposeWithCache(text, nullptr);
    ASSERT(glyphs != nullptr);

    const float scale = scale_func(UI_TEXT_BASE_SCALE * settings.scale);

    float x = scale_func(base_x / UI_Scaler_GetTextScale());
    float y = scale_func(
        base_y / UI_Scaler_GetTextScale() + settings.scale * M_GetBaseline());
    int32_t z = settings.z;

    float max_width = 0.0f;
    const float start_x = x;

    M_FONT current_font = M_FONT_DEFAULT;
    int32_t color_idx = m_DefaultColor;
    int32_t prev_color_idx = color_idx;
    bool visible = true;

    const M_GLYPH_INFO **glyph_ptr = glyphs;
    while (*glyph_ptr != nullptr) {
        const M_GLYPH_INFO *const glyph = *glyph_ptr;

        if (glyph->role == GLYPH_REVIEW_MARKER
            && !g_Config.debug.enable_review_markers) {
            goto loop_end;
        }

        if (glyph->role == GLYPH_VISIBILITY_MARKER) {
            visible = glyph->mesh_idx;
            goto loop_end;
        }

        if (glyph->role == GLYPH_FONT_MARKER) {
            current_font = glyph->mesh_idx;
            goto loop_end;
        }

        if (glyph->role == GLYPH_COLOR_MARKER) {
            if (glyph->mesh_idx != -1) {
                prev_color_idx = color_idx;
                color_idx = glyph->mesh_idx;
            } else {
                color_idx = prev_color_idx;
            }
            goto loop_end;
        }

        if (glyph->role == GLYPH_NEW_LINE || glyph->role == GLYPH_NEW_PAGE) {
            y += M_GetLinePitch() * scale / UI_TEXT_BASE_SCALE;
            x = start_x;
            goto loop_end;
        }

        if (glyph->role == GLYPH_SPACE) {
            if (glyph_ptr[1] == nullptr
                || (glyph_ptr[1]->role != GLYPH_NEW_LINE
                    && glyph_ptr[1]->role != GLYPH_NEW_PAGE)) {
                x += M_WORD_SPACING * scale / UI_TEXT_BASE_SCALE;
            }
            goto loop_end;
        }

        if (glyph->role == GLYPH_SECRET) {
            const int16_t sprite_idx =
                Object_Get(
                    ObjectFamily_GetMember(OBJ_FAMILY_SECRET, glyph->mesh_idx))
                    ->mesh_idx;
            const SPRITE_TEXTURE *const sprite =
                Output_GetSpriteTexture(sprite_idx);
            const float input_scale_h =
                settings.scale / (sprite->x1 - sprite->x0);
            const float input_scale_v =
                settings.scale / (sprite->y1 - sprite->y0);
            const float input_scale = MIN(input_scale_h, input_scale_v);
            const float output_scale = scale_func(
                UI_TEXT_BASE_SCALE * glyph->width[current_font] * input_scale);
            if (visible && draw_func != nullptr) {
                RGBA_F colors[4];
                M_ShadeCorners(color_idx, 0, M_TR4_BASE_ROW, colors);
                draw_func(
                    x + scale_func(10), y, z, output_scale, output_scale,
                    sprite_idx, colors);
            }
            x += glyph->width[current_font] * scale / UI_TEXT_BASE_SCALE;
            goto loop_end;
        }

        const bool spaced = glyph_ptr[1] != nullptr
            && glyph_ptr[1]->role != GLYPH_NEW_LINE
            && glyph_ptr[1]->role != GLYPH_NEW_PAGE;

        if (glyph->role == GLYPH_INPUT) {
            size_t count = 0;
            const M_GLYPH_INFO **const parts = M_GetInputGlyphs(glyph, &count);
            for (size_t i = 0; i < count; i++) {
                x += M_DrawGlyph(
                    parts[i], current_font, x, y, z, scale,
                    i + 1 < count || spaced, visible, color_idx, draw_func);
            }
            goto loop_end;
        }

        x += M_DrawGlyph(
            glyph, current_font, x, y, z, scale, spaced, visible, color_idx,
            draw_func);

    loop_end:
        max_width = MAX(max_width, x);
        glyph_ptr++;
    }

    if (out_w != nullptr) {
        *out_w = max_width;
    }

    if (out_h != nullptr) {
        *out_h = y
            + scale_func(
                     settings.scale * (M_GetLineHeight() - M_GetBaseline()));
    }
}

void UI_InitText(void)
{
    m_Colors = Vector_Create(sizeof(M_COLOR));
    const M_COLOR plain = {
        .top = COLOR_RGBA_8888_WHITE,
        .bottom = COLOR_RGBA_8888_WHITE,
    };
    Vector_Add(m_Colors, &plain);
    m_DefaultColor = 0;
    m_DefaultSpec = nullptr;

    // Convert the linear array coming from the .def macros to a hash lookup
    // table for faster text-to-glyph resolution.
    for (M_GLYPH_INFO *glyph_ptr = m_Glyphs; glyph_ptr->text != nullptr;
         glyph_ptr++) {
        // mark static glyphs as non-input
        M_GLYPH_MAP_ENTRY *const hash_entry = Memory_Alloc(sizeof(*hash_entry));
        hash_entry->glyph = glyph_ptr;
        HASH_ADD_KEYPTR(
            hh, m_GlyphMap, glyph_ptr->text, strlen(glyph_ptr->text),
            hash_entry);
    }

    // Create dynamic glyphs for "{key <role>}" tokens; resolution happens when
    // drawing/wrapping
    for (INPUT_ROLE role = 0; role < INPUT_ROLE_NUMBER_OF; role++) {
        const char *role_str =
            EnumMap_ToString(ENUM_MAP_NAME(INPUT_ROLE), role);
        if (role_str == nullptr || *role_str == '\0') {
            continue;
        }
        M_GLYPH_INFO *input_glyph = Memory_Alloc(sizeof(*input_glyph));
        input_glyph->text = String_Format("\\{input %s}", role_str);
        input_glyph->role = GLYPH_INPUT;
        input_glyph->input_role = role;
        for (M_FONT font = 0; font < M_FONT_COUNT; font++) {
            input_glyph->width[font] = 0;
        }
        M_GLYPH_MAP_ENTRY *entry = Memory_Alloc(sizeof(*entry));
        entry->glyph = input_glyph;
        HASH_ADD_KEYPTR(
            hh, m_GlyphMap, input_glyph->text, strlen(input_glyph->text),
            entry);
    }
}

void UI_LoadText(void)
{
    // The plain colour draws the font as it is: TR4 lights it at half.
    const RGBA_8888 plain = g_TRVersion == 4
        ? (RGBA_8888) { .r = 0x80, .g = 0x80, .b = 0x80, .a = 0xFF }
        : COLOR_RGBA_8888_WHITE;
    *(M_COLOR *)Vector_Get(m_Colors, 0) =
        (M_COLOR) { .top = plain, .bottom = plain };

    for (M_FONT font = 0; font < M_FONT_COUNT; font++) {
        m_CapHeight[font] = 0;
        for (M_GLYPH_INFO *glyph_ptr = m_Glyphs; glyph_ptr->text != nullptr;
             glyph_ptr++) {
            glyph_ptr->width[font] = M_GetGlyphWidth(font, glyph_ptr);
            if (glyph_ptr->width[font] > 0
                && strcmp(glyph_ptr->text, "A") == 0) {
                const OBJECT *const object = Object_Get(m_FontObjects[font]);
                const SPRITE_TEXTURE *const sprite = Output_GetSpriteTexture(
                    object->mesh_idx + glyph_ptr->mesh_idx);
                m_CapHeight[font] = -sprite->y0;
            }
        }
    }
}

void UI_ShutdownText(void)
{
    {
        M_GLYPH_MAP_ENTRY *current, *tmp;
        HASH_ITER(hh, m_GlyphMap, current, tmp)
        {
            if (current->glyph->role == GLYPH_INPUT
                || current->glyph->is_made) {
                Memory_FreePointer(&current->glyph->text);
                Memory_FreePointer(&current->glyph);
            }
            HASH_DEL(m_GlyphMap, current);
            Memory_Free(current);
        }
    }

    {
        M_TEXT_MAP_ENTRY *current, *tmp;
        HASH_ITER(hh, m_TextMap, current, tmp)
        {
            Memory_FreePointer(&current->text);
            Memory_FreePointer(&current->glyphs);
            HASH_DEL(m_TextMap, current);
            Memory_FreePointer(&current);
        }
    }

    {
        M_COLOR_MAP_ENTRY *current, *tmp;
        HASH_ITER(hh, m_ColorMap, current, tmp)
        {
            Memory_FreePointer(&current->spec);
            HASH_DEL(m_ColorMap, current);
            Memory_FreePointer(&current);
        }
    }
    Vector_Free(m_Colors);
    m_Colors = nullptr;
    m_DefaultColor = 0;
    m_DefaultSpec = nullptr;
}

void UI_Text_Measure(
    const char *const text, float *const out_w, float *const out_h,
    const UI_TEXT_SETTINGS settings)
{
    M_Process(
        text, out_w, out_h, settings, 0.0f, 0.0f, M_ScaleNeutral, nullptr);
}

float UI_Text_GetBaseline(const UI_TEXT_SETTINGS settings)
{
    return M_ScaleNeutral(settings.scale * M_GetBaseline());
}

bool UI_Text_IsColor(const char *const spec)
{
    return M_InternColor(spec) != nullptr;
}

const char *UI_Text_SetDefaultColor(const char *const spec)
{
    const char *const previous = m_DefaultSpec;
    const M_COLOR_MAP_ENTRY *const color =
        spec != nullptr ? M_InternColor(spec) : nullptr;
    m_DefaultColor = color != nullptr ? color->idx : 0;
    m_DefaultSpec = color != nullptr ? color->spec : nullptr;
    return previous;
}

void UI_Text_Draw(
    const char *const text, const float base_x, const float base_y,
    const UI_TEXT_SETTINGS settings)
{
    M_Process(
        text, nullptr, nullptr, settings, base_x,
        base_y - UI_Scaler_GetTextScale(), M_ScaleScreen,
        UI_ScheduleDrawScreenSprite);
}

char *UI_Text_WordWrap(
    const char *text, const float scale, const float max_width)
{
    if (text == nullptr || max_width <= 0) {
        return nullptr;
    }

    size_t glyph_count = 0;
    const M_GLYPH_INFO **glyphs = M_DecomposeWithCache(text, &glyph_count);

    const float scale_f = scale * UI_Scaler_GetTextScale();
    size_t len = M_WordWrap(glyphs, glyph_count, scale_f, max_width, nullptr);
    char *const wrapped_text = Memory_Alloc(len);
    M_WordWrap(glyphs, glyph_count, scale_f, max_width, wrapped_text);
    return wrapped_text;
}

char *UI_Text_FilterGlyphs(const char *const text)
{
    if (text == nullptr) {
        return nullptr;
    }
    const size_t in_len = strlen(text);
    char *out = Memory_Alloc(in_len + 1);
    size_t out_len = 0;
    const char *p = text;
    while (*p != '\0') {
        const size_t sz = String_GetCharByteSize(p);
        const char *const key_buf = String_FormatStatic("%.*s", (int32_t)sz, p);
        if (M_FindGlyph(key_buf) != nullptr) {
            memcpy(out + out_len, p, sz);
            out_len += sz;
        }
        p += sz;
    }
    out[out_len] = '\0';
    return out;
}
