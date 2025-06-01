#include "game/ui/text.h"

#include "config.h"
#include "debug.h"
#include "game/objects.h"
#include "game/output.h"
#include "game/scaler.h"
#include "log.h"
#include "memory.h"
#include "utils.h"

#include <ctype.h>
#include <uthash.h>

#define M_LETTER_SPACING 0.5f
#define M_WORD_SPACING 6.0f

typedef enum {
    // special non-printable glyph roles
    GLYPH_NORMAL,
    GLYPH_SPACE,
    GLYPH_NEW_LINE,
    GLYPH_NEW_PAGE,
    GLYPH_COMBINING,
    GLYPH_COMPOUND,
    GLYPH_SECRET,
} M_GLYPH_ROLE;

typedef struct {
    const char *text;
    M_GLYPH_ROLE role;
    int32_t width;
    int32_t mesh_idx;

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
#define GLYPH_DEFINE(text_, role_, width_, mesh_idx_, ...)                     \
    { .text = text_,                                                           \
      .role = role_,                                                           \
      .width = width_,                                                         \
      .mesh_idx = mesh_idx_,                                                   \
      __VA_ARGS__ },
#include "./text.def"
    { .text = nullptr }, // guard
};

static size_t m_GlyphLookupKeyCap = 0;
static char *m_GlyphLookupKey = nullptr;
static M_GLYPH_MAP_ENTRY *m_GlyphMap = nullptr;
static M_TEXT_MAP_ENTRY *m_TextMap = nullptr;

static int32_t M_Scale(const int32_t value);
static size_t M_GetGlyphByteSize(const char *ptr);

static int32_t M_Scale(const int32_t value)
{
    return Scaler_Calc(value, SCALER_TARGET_TEXT);
}

static size_t M_GetGlyphByteSize(const char *const ptr)
{
    // Check for named escape sequence.
    if (*ptr == '\\' && *(ptr + 1) == '{') {
        const char *end = strchr(ptr + 2, '}');
        if (end != nullptr) {
            return end + 1 - ptr;
        }
        return 1;
    }

    // clang-format off
    // UTF-8 sequence lengths
    if ((*ptr & 0x80) == 0x00) { return 1; } // 1-byte sequence
    if ((*ptr & 0xE0) == 0xC0) { return 2; } // 2-byte sequence
    if ((*ptr & 0xF0) == 0xE0) { return 3; } // 3-byte sequence
    if ((*ptr & 0xF8) == 0xF0) { return 4; } // 4-byte sequence
    // clang-format on

    // Fallback to 1
    return 1;
}

static const M_GLYPH_INFO **M_Decompose(
    const char *const content, size_t *out_glyph_count)
{
    // Count number of characters
    size_t glyph_count = 0;
    const char *content_ptr = content;
    while (*content_ptr != '\0') {
        const size_t glyph_size = M_GetGlyphByteSize(content_ptr);
        content_ptr += glyph_size;
        glyph_count++;
    }

    // Assign glyphs using hash table
    const M_GLYPH_INFO **glyphs =
        Memory_Alloc((glyph_count + 1) * sizeof(M_GLYPH_INFO *));
    content_ptr = content;
    const M_GLYPH_INFO **glyph_ptr = glyphs;
    while (*content_ptr != '\0') {
        const size_t glyph_size = M_GetGlyphByteSize(content_ptr);
        if (m_GlyphLookupKeyCap <= glyph_size) {
            m_GlyphLookupKeyCap = glyph_size + 10;
            m_GlyphLookupKey =
                Memory_Realloc(m_GlyphLookupKey, m_GlyphLookupKeyCap);
        }
        strncpy(m_GlyphLookupKey, content_ptr, glyph_size);
        m_GlyphLookupKey[glyph_size] = '\0';

        M_GLYPH_MAP_ENTRY *entry;
        HASH_FIND_STR(m_GlyphMap, m_GlyphLookupKey, entry);

        if (entry != nullptr) {
            *glyph_ptr++ = entry->glyph;
        } else {
            LOG_WARNING("Unknown glyph: %s", m_GlyphLookupKey);
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
    const char *const content, size_t *out_glyph_count)
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

void UI_InitText(void)
{
    // Convert the linear array coming from the .def macros to a hash lookup
    // table for faster text-to-glyph resolution.
    for (M_GLYPH_INFO *glyph_ptr = m_Glyphs; glyph_ptr->text != nullptr;
         glyph_ptr++) {
        M_GLYPH_MAP_ENTRY *const hash_entry =
            Memory_Alloc(sizeof(M_GLYPH_MAP_ENTRY));
        hash_entry->glyph = glyph_ptr;
        HASH_ADD_KEYPTR(
            hh, m_GlyphMap, glyph_ptr->text, strlen(glyph_ptr->text),
            hash_entry);
    }

    m_GlyphLookupKeyCap = 10;
    m_GlyphLookupKey = Memory_Alloc(m_GlyphLookupKeyCap);
}

void UI_ShutdownText(void)
{
    {
        M_GLYPH_MAP_ENTRY *current, *tmp;
        HASH_ITER(hh, m_GlyphMap, current, tmp)
        {
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

    Memory_FreePointer(&m_GlyphLookupKey);
    m_GlyphLookupKeyCap = 0;
}

void UI_Text_Measure(
    const char *const text, float *const out_w, float *const out_h,
    const UI_TEXT_SETTINGS settings)
{
    if (text == nullptr) {
        if (out_w != nullptr) {
            *out_w = 0.0f;
        }
        if (out_h != nullptr) {
            *out_h = 0.0f;
        }
        return;
    }

    const M_GLYPH_INFO **glyphs = M_DecomposeWithCache(text, nullptr);
    ASSERT(glyphs != nullptr);

    if (out_w != nullptr) {
        float width = 0.0f;
        float max_width = 0.0f;
        const M_GLYPH_INFO **glyph_ptr = glyphs;
        while (*glyph_ptr != nullptr) {
            if ((*glyph_ptr)->role == GLYPH_SPACE) {
                width += M_WORD_SPACING;
            } else if (
                (*glyph_ptr)->role == GLYPH_NEW_LINE
                || (*glyph_ptr)->role == GLYPH_NEW_PAGE) {
                max_width = MAX(max_width, width);
                width = 0;
            } else {
                width += (*glyph_ptr)->width + M_LETTER_SPACING;
            }
            glyph_ptr++;
        }

        width = MAX(max_width, width);
        width -= M_LETTER_SPACING;
        *out_w = width * settings.scale * g_Config.ui.text_scale;
    }

    if (out_h != nullptr) {
        int32_t height = UI_TEXT_HEIGHT;
        for (const char *ptr = text; *ptr != '\0'; ptr++) {
            if (*ptr == '\n') {
                height += UI_TEXT_HEIGHT;
            }
        }
        *out_h = height * settings.scale * g_Config.ui.text_scale;
    }
}

int32_t UI_Text_GetMaxLineLength(void)
{
    return 640 / (UI_TEXT_HEIGHT * 0.6);
}

void UI_Text_Draw(
    const char *const text, const float base_x, const float base_y,
    const UI_TEXT_SETTINGS settings)
{
    if (text == nullptr) {
        return;
    }

    const M_GLYPH_INFO **glyphs = M_DecomposeWithCache(text, nullptr);
    ASSERT(glyphs != nullptr);

    const OBJECT *const obj = Object_Get(O_ALPHABET);
    if (!obj->loaded) {
        return;
    }

    const int32_t scale = M_Scale(UI_TEXT_BASE_SCALE * settings.scale);

    float x = M_Scale(base_x / g_Config.ui.text_scale);
    float y = M_Scale(
        base_y / g_Config.ui.text_scale + settings.scale * UI_TEXT_HEIGHT - 1);
    int32_t z = settings.z;

    const float start_x = x;

    const M_GLYPH_INFO **glyph_ptr = glyphs;
    while (*glyph_ptr != nullptr) {
        const M_GLYPH_INFO *glyph = *glyph_ptr;
        if (glyph->role == GLYPH_NEW_LINE || glyph->role == GLYPH_NEW_PAGE) {
            y += UI_TEXT_HEIGHT * scale / UI_TEXT_BASE_SCALE;
            x = start_x;
            goto loop_end;
        }

        if (glyph->role == GLYPH_SPACE) {
            x += M_WORD_SPACING * scale / UI_TEXT_BASE_SCALE;
            goto loop_end;
        }

#if TR_VERSION == 2
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
                M_Scale(UI_TEXT_BASE_SCALE * glyph->width * input_scale);
            Output_DrawScreenSprite(
                x + M_Scale(10), y, z, output_scale, output_scale, sprite_idx,
                SHADE_NEUTRAL);
            x += glyph->width * scale / UI_TEXT_BASE_SCALE;
            goto loop_end;
        }
#endif

        if (glyph->role == GLYPH_COMPOUND) {
            const int32_t cx =
                x + (glyph->combine_with.offset_x * scale / UI_TEXT_BASE_SCALE);
            const int32_t cy =
                y + (glyph->combine_with.offset_y * scale / UI_TEXT_BASE_SCALE);
            if (glyph->combine_with.mesh_idx >= ABS(obj->mesh_count)) {
                goto loop_end;
            }

            Output_DrawScreenSprite(
                cx, cy, 0, scale, scale,
                obj->mesh_idx + glyph->combine_with.mesh_idx, SHADE_NEUTRAL);
        }

        if (glyph->mesh_idx >= ABS(obj->mesh_count)) {
            goto loop_end;
        }
        Output_DrawScreenSprite(
            x, y, z, scale, scale, obj->mesh_idx + glyph->mesh_idx,
            SHADE_NEUTRAL);

        if (glyph->role != GLYPH_COMBINING) {
            const float spacing = glyph->width + M_LETTER_SPACING;
            x += spacing * scale / UI_TEXT_BASE_SCALE;
        }

    loop_end:
        glyph_ptr++;
    }
}

char *UI_Text_WordWrap(
    const char *text, const float scale, const float max_width)
{
    if (text == nullptr || max_width <= 0) {
        return nullptr;
    }

    size_t glyph_count = 0;
    const M_GLYPH_INFO **glyphs = M_DecomposeWithCache(text, &glyph_count);

    // Prepare output buffer
    // Assume worst case scenario instead of using a growing buffer or
    // computing the exact capacity for the sake of simplicity
    char *const wrapped_text = Memory_Alloc(strlen(text) * 2 + 1);
    char *dst = wrapped_text;

    const float scale_f = scale * g_Config.ui.text_scale;
    float cur_width = 0.0f;

    // Iterate glyphs for wrapping
    for (size_t i = 0; i < glyph_count; i++) {
        const M_GLYPH_INFO *const glyph = glyphs[i];
        if (glyph->role == GLYPH_NEW_LINE) {
            *dst++ = '\n';
            cur_width = 0.0f;
        } else if (glyph->role == GLYPH_NEW_PAGE) {
            *dst++ = '\f';
            cur_width = 0.0f;
        } else if (glyph->role == GLYPH_SPACE) {
            if (cur_width > 0.0f) {
                const float w = M_WORD_SPACING * scale_f;
                if (cur_width + w > max_width) {
                    *dst++ = '\n';
                    cur_width = 0.0f;
                } else {
                    *dst++ = ' ';
                    cur_width += w;
                }
            }
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
                    *dst++ = '\n';
                    cur_width = 0.0f;
                }

                // Break word if longer than line
                if (word_width > max_width) {
                    for (size_t j = i; j < i + word_len; j++) {
                        const M_GLYPH_INFO *const glyph = glyphs[j];
                        const float glyph_width =
                            (glyph->width + M_LETTER_SPACING) * scale_f;
                        if (cur_width + glyph_width > max_width) {
                            *dst++ = '\n';
                            cur_width = 0.0f;
                        }
                        strcpy(dst, glyph->text);
                        dst += strlen(glyph->text);
                        cur_width += glyph_width;
                    }
                } else {
                    for (size_t j = i; j < i + word_len; j++) {
                        const M_GLYPH_INFO *const glyph = glyphs[j];
                        strcpy(dst, glyph->text);
                        dst += strlen(glyph->text);
                    }
                    cur_width = word_width;
                }
            } else {
                // Copy word as is
                for (size_t j = i; j < i + word_len; j++) {
                    const M_GLYPH_INFO *const glyph = glyphs[j];
                    strcpy(dst, glyph->text);
                    dst += strlen(glyph->text);
                }
                cur_width += word_width;
            }
            // Skip forward the characters, bearing in mind the default loop
            // accumulator
            i += word_len - 1;
        }
    }
    *dst = '\0';

    return wrapped_text;
}
