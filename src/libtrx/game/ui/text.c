#include "game/ui/text.h"

#include "config.h"
#include "debug.h"
#include "enum_map.h"
#include "game/input/common.h"
#include "game/objects.h"
#include "game/output.h"
#include "game/scaler.h"
#include "game/ui/common.h"
#include "game/ui/draw.h"
#include "log.h"
#include "memory.h"
#include "strings.h"
#include "utils.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <uthash.h>

#define M_LETTER_SPACING 0.5f
#define M_WORD_SPACING 6.0f

typedef enum {
    // Normal character.
    GLYPH_NORMAL,
    // Spacing between words.
    GLYPH_SPACE,
    // Line break.
    GLYPH_NEW_LINE,
    // Marker used in the examine item dialog and others to force a new page.
    GLYPH_NEW_PAGE,
    // Accent/diacritic modifier to another character.
    GLYPH_COMBINING,
    // Character that needs to be combined with an accent/diacritic.
    GLYPH_COMPOUND,
    // Icon for collectible secrets, taking the sprite from O_SECRET
    GLYPH_SECRET,
    // Icon requesting translators to verify AI-translated text.
    GLYPH_REVIEW_MARKER,
    // Marker that toggles the visibility of the following text.
    GLYPH_VISIBILITY_MARKER,
    // Marker that toggles the dimming of the following text.
    GLYPH_DIM_MARKER,
    // Glyph that dynamically expands a key role to its current key icon.
    GLYPH_INPUT,
} M_GLYPH_ROLE;

typedef struct {
    const char *text;
    M_GLYPH_ROLE role;
    int32_t width;

    union {
        int32_t mesh_idx;
        INPUT_ROLE input_role; // for role == GLYPH_INPUT
    };

    // For compound accents or combined glyphs
    struct {
        int32_t mesh_idx;
        int32_t offset_x;
        int32_t offset_y;
    } combine_with;
} M_GLYPH_INFO;

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

static M_GLYPH_INFO m_Glyphs[] = {
#define X_GLYPH_DEFINE(text_, role_, width_, mesh_idx_, ...)                   \
    { .text = text_,                                                           \
      .role = role_,                                                           \
      .width = width_,                                                         \
      .mesh_idx = mesh_idx_,                                                   \
      __VA_ARGS__ },
#include "./text.def"
    { .text = nullptr }, // guard
};

static M_GLYPH_MAP_ENTRY *m_GlyphMap = nullptr;
static M_TEXT_MAP_ENTRY *m_TextMap = nullptr;

static float M_ScaleScreen(const float value)
{
    return Scaler_Calc(value, SCALER_TARGET_TEXT);
}

static float M_ScaleNeutral(const float value)
{
    return value * g_Config.ui.text_scale;
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
        M_GLYPH_MAP_ENTRY *entry;
        HASH_FIND_STR(m_GlyphMap, key_buf, entry);

        if (entry != nullptr) {
            *glyph_ptr++ = entry->glyph;
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

// Replace input placeholder glyph with the actual keyboard glyph for the
// current binding
static const M_GLYPH_INFO *M_GetResolvedGlyph(const M_GLYPH_INFO *glyph)
{
    if (glyph->role != GLYPH_INPUT) {
        return glyph;
    }
    const char *const key_name = Input_GetKeyName(
        g_Config.input.backend, g_Config.input.layout[g_Config.input.backend],
        glyph->input_role);
    // NOTE: this aliasing approach assumes that Input_GetKeyName returns
    // text that resolves to a single glyph.
    M_GLYPH_MAP_ENTRY *entry = nullptr;
    if (key_name != nullptr) {
        HASH_FIND_STR(m_GlyphMap, key_name, entry);
    }
    if (entry == nullptr) {
        HASH_FIND_STR(m_GlyphMap, "?", entry);
    }
    return entry != nullptr ? entry->glyph : nullptr;
}

static size_t M_WordWrap(
    const M_GLYPH_INFO **glyphs, const size_t glyph_count, const float scale_f,
    const float max_width, char *const dst)
{
    size_t out_len = 0;
    float cur_width = 0.0f;
    bool in_bullet = false;

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

    // Iterate glyphs for wrapping
    for (size_t i = 0; i < glyph_count; i++) {
        const M_GLYPH_INFO *const glyph = M_GetResolvedGlyph(glyphs[i]);
        if (glyph == nullptr) {
            continue;
        }

        if (glyph->role == GLYPH_NEW_LINE) {
            L_CONCAT_CHAR('\n')
            cur_width = 0.0f;
            in_bullet = false;
        } else if (glyph->role == GLYPH_NEW_PAGE) {
            L_CONCAT_CHAR('\f')
            cur_width = 0.0f;
            in_bullet = false;
        } else if (glyph->role == GLYPH_SPACE) {
            if (cur_width > 0.0f) {
                const float w = M_WORD_SPACING * scale_f;
                if (cur_width + w > max_width) {
                    L_CONCAT_CHAR('\n')
                    cur_width = 0.0f;
                    if (in_bullet) {
                        L_CONCAT_CHAR(' ')
                        L_CONCAT_CHAR(' ')
                        cur_width += 2 * space_width;
                    }
                } else {
                    L_CONCAT_CHAR(' ')
                    cur_width += w;
                }
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

            // Detect bullet start marker ("- " at line start)
            if (cur_width == 0.0f && word_len == 1 && glyphs[i]->text[0] == '-'
                && (i + 1 < glyph_count
                    && glyphs[i + 1]->role == GLYPH_SPACE)) {
                in_bullet = true;
            }

            // Compute width (sum widths + spacing)
            float word_width = 0.0f;
            for (size_t j = i; j < i + word_len; j++) {
                word_width += glyphs[j]->width + M_LETTER_SPACING;
            }
            if (word_width > 0) {
                word_width -= M_LETTER_SPACING;
            }
            word_width *= scale_f;

            // Wrap line if needed
            if (cur_width + word_width > max_width) {
                if (cur_width > 0.0f) {
                    L_CONCAT_CHAR('\n')
                    cur_width = 0.0f;
                    if (in_bullet) {
                        L_CONCAT_CHAR(' ')
                        L_CONCAT_CHAR(' ')
                        cur_width += 2 * space_width;
                    }
                }

                // Break word if longer than line
                if (word_width > max_width) {
                    for (size_t j = i; j < i + word_len; j++) {
                        const M_GLYPH_INFO *const glyph = glyphs[j];
                        const float glyph_width =
                            (glyph->width + M_LETTER_SPACING) * scale_f;
                        if (cur_width + glyph_width > max_width) {
                            L_CONCAT_CHAR('\n')
                            cur_width = 0.0f;
                            if (in_bullet) {
                                L_CONCAT_CHAR(' ')
                                L_CONCAT_CHAR(' ')
                                cur_width += 2 * space_width;
                            }
                        }
                        L_CONCAT_STR(glyph->text)
                        cur_width += glyph_width;
                    }
                } else {
                    for (size_t j = i; j < i + word_len; j++) {
                        const M_GLYPH_INFO *const glyph = glyphs[j];
                        L_CONCAT_STR(glyph->text)
                    }
                    cur_width = word_width;
                }
            } else {
                // Copy word as is
                for (size_t j = i; j < i + word_len; j++) {
                    const M_GLYPH_INFO *const glyph = glyphs[j];
                    L_CONCAT_STR(glyph->text)
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

static void M_Process(
    const char *const text, float *const out_w, float *const out_h,
    const UI_TEXT_SETTINGS settings, const float base_x, const float base_y,
    float (*const scale_func)(float),
    void (*const draw_func)(
        int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, int16_t))
{
    if (text == nullptr) {
        return;
    }

    const M_GLYPH_INFO **glyphs = M_DecomposeWithCache(text, nullptr);
    ASSERT(glyphs != nullptr);

    const OBJECT *const obj = Object_Get(O_ALPHABET);
    const float scale = scale_func(UI_TEXT_BASE_SCALE * settings.scale);

    float x = scale_func(base_x / g_Config.ui.text_scale);
    float y = scale_func(
        base_y / g_Config.ui.text_scale + settings.scale * UI_TEXT_HEIGHT);
    int32_t z = settings.z;

    float max_width = 0.0f;
    const float start_x = x;

    bool visible = true;
    bool dimmed = false;
    const M_GLYPH_INFO **glyph_ptr = glyphs;
    while (*glyph_ptr != nullptr) {
        const M_GLYPH_INFO *const glyph = M_GetResolvedGlyph(*glyph_ptr);
        if (glyph == nullptr) {
            goto loop_end;
        }

        if (glyph->role == GLYPH_REVIEW_MARKER
            && !g_Config.debug.enable_review_markers) {
            goto loop_end;
        }

        if (glyph->role == GLYPH_VISIBILITY_MARKER) {
            visible = glyph->mesh_idx;
            goto loop_end;
        }

        if (glyph->role == GLYPH_DIM_MARKER) {
            dimmed = glyph->mesh_idx;
            goto loop_end;
        }

        if (glyph->role == GLYPH_NEW_LINE || glyph->role == GLYPH_NEW_PAGE) {
            y += UI_TEXT_HEIGHT * scale / UI_TEXT_BASE_SCALE;
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

        const int16_t shade = dimmed ? 0x1600 : SHADE_NEUTRAL;

        if (glyph->role == GLYPH_SECRET) {
            const int16_t sprite_idx =
                Object_Get(O_SECRET_1 + glyph->mesh_idx)->mesh_idx;
            const SPRITE_TEXTURE *const sprite =
                Output_GetSpriteTexture(sprite_idx);
            const float input_scale_h =
                settings.scale / (sprite->x1 - sprite->x0);
            const float input_scale_v =
                settings.scale / (sprite->y1 - sprite->y0);
            const float input_scale = MIN(input_scale_h, input_scale_v);
            const float output_scale =
                scale_func(UI_TEXT_BASE_SCALE * glyph->width * input_scale);
            if (visible && draw_func != nullptr) {
                draw_func(
                    x + scale_func(10), y, z, output_scale, output_scale,
                    sprite_idx, shade);
            }
            x += glyph->width * scale / UI_TEXT_BASE_SCALE;
            goto loop_end;
        }

        if (glyph->role == GLYPH_COMPOUND && obj->loaded
            && glyph->combine_with.mesh_idx >= 0
            && glyph->combine_with.mesh_idx < ABS(obj->mesh_count) && visible
            && draw_func != nullptr) {
            const float cx =
                x + (glyph->combine_with.offset_x * scale / UI_TEXT_BASE_SCALE);
            const float cy =
                y + (glyph->combine_with.offset_y * scale / UI_TEXT_BASE_SCALE);
            draw_func(
                cx, cy, 0, scale, scale,
                obj->mesh_idx + glyph->combine_with.mesh_idx, shade);
        }

        if (obj->loaded && glyph->mesh_idx >= 0
            && glyph->mesh_idx < ABS(obj->mesh_count) && visible
            && draw_func != nullptr) {
            draw_func(
                x, y, z, scale, scale, obj->mesh_idx + glyph->mesh_idx, shade);
        }

        if (glyph->role != GLYPH_COMBINING) {
            float spacing = glyph->width;
            if (glyph_ptr[1] != nullptr && glyph_ptr[1]->role != GLYPH_NEW_LINE
                && glyph_ptr[1]->role != GLYPH_NEW_PAGE) {
                spacing += M_LETTER_SPACING;
            }
            x += spacing * scale / UI_TEXT_BASE_SCALE;
        }

    loop_end:
        max_width = MAX(max_width, x);
        glyph_ptr++;
    }

    if (out_w != nullptr) {
        *out_w = max_width;
    }

    if (out_h != nullptr) {
        *out_h = y;
    }
}

void UI_InitText(void)
{
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
        input_glyph->width = 0;
        input_glyph->input_role = role;
        input_glyph->combine_with.mesh_idx = -1;
        input_glyph->combine_with.offset_x = 0;
        input_glyph->combine_with.offset_y = 0;
        M_GLYPH_MAP_ENTRY *entry = Memory_Alloc(sizeof(*entry));
        entry->glyph = input_glyph;
        HASH_ADD_KEYPTR(
            hh, m_GlyphMap, input_glyph->text, strlen(input_glyph->text),
            entry);
    }
}

void UI_ShutdownText(void)
{
    {
        M_GLYPH_MAP_ENTRY *current, *tmp;
        HASH_ITER(hh, m_GlyphMap, current, tmp)
        {
            if (current->glyph->role == GLYPH_INPUT) {
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
}

void UI_Text_Measure(
    const char *const text, float *const out_w, float *const out_h,
    const UI_TEXT_SETTINGS settings)
{
    M_Process(
        text, out_w, out_h, settings, 0.0f, 0.0f, M_ScaleNeutral, nullptr);
}

void UI_Text_Draw(
    const char *const text, const float base_x, const float base_y,
    const UI_TEXT_SETTINGS settings)
{
    M_Process(
        text, nullptr, nullptr, settings, base_x,
        base_y - g_Config.ui.text_scale, M_ScaleScreen,
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

    const float scale_f = scale * g_Config.ui.text_scale;
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
        M_GLYPH_MAP_ENTRY *entry = nullptr;
        HASH_FIND_STR(m_GlyphMap, key_buf, entry);
        if (entry != nullptr) {
            memcpy(out + out_len, p, sz);
            out_len += sz;
        }
        p += sz;
    }
    out[out_len] = '\0';
    return out;
}
