#pragma once

#include "./math/types.h"
#include "./output/draw.h"

#include <stddef.h>
#include <stdint.h>

// TODO: rename this
#define TEXT_HEIGHT_FIXED 15

#define TEXT_MAX_STRINGS 128
#define TEXT_BASE_SCALE 0x10000

typedef enum {
    // special non-printable glyph roles
    GLYPH_NORMAL,
    GLYPH_SPACE,
    GLYPH_NEWLINE,
    GLYPH_COMBINING,
    GLYPH_COMPOUND,
    GLYPH_SECRET,
} GLYPH_ROLE;

typedef struct {
    const char *text;
    GLYPH_ROLE role;
    int32_t width;
    int32_t mesh_idx;

    struct {
        int32_t mesh_idx;
        int32_t offset_x;
        int32_t offset_y;
    } combine_with;
} GLYPH_INFO;

typedef struct {
    XYZ_32 pos;

    int16_t letter_spacing;
    int16_t word_spacing;

    struct {
        int32_t h;
        int32_t v;
    } scale;

    size_t content_cap;
    char *content;

    size_t glyphs_cap;
    const GLYPH_INFO **glyphs;
} TEXTSTRING;

extern int32_t Text_GetMaxLineLength(void);
extern void Text_DrawText(TEXTSTRING *text);

void Text_Init(void);
void Text_Shutdown(void);

TEXTSTRING *Text_Create(int16_t x, int16_t y, const char *text);
void Text_Remove(TEXTSTRING *text);

void Text_ChangeText(TEXTSTRING *text, const char *content);
void Text_SetPos(TEXTSTRING *text, int16_t x, int16_t y);
void Text_SetScale(TEXTSTRING *text, int32_t scale_h, int32_t scale_v);

int32_t Text_GetWidth(const TEXTSTRING *text);
int32_t Text_GetHeight(const TEXTSTRING *text);
