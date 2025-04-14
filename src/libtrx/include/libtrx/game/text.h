#pragma once

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

typedef enum {
    TS_HEADING = 0,
    TS_BACKGROUND = 1,
    TS_REQUESTED = 2,
} TEXT_STYLE;

typedef struct {
    union {
        uint32_t all;
        struct {
            uint32_t active : 1;
            uint32_t flash : 1;
            uint32_t rotate_h : 1;
            uint32_t centre_h : 1;
            uint32_t centre_v : 1;
            uint32_t right : 1;
            uint32_t bottom : 1;
            uint32_t background : 1;
            uint32_t outline : 1;
            uint32_t hide : 1;
            uint32_t multiline : 1;

            uint32_t manual_draw : 1;
            uint32_t drawn : 1;
        };
    } flags;

    struct {
        int32_t x;
        int32_t y;
        int32_t z;
    } pos;

    int16_t letter_spacing;
    int16_t word_spacing;

    struct {
        int16_t rate;
        int16_t count;
    } flash;

    struct {
        int32_t h;
        int32_t v;
    } scale;

    struct {
        TEXT_STYLE style;
        struct {
            int32_t x;
            int32_t y;
            int32_t z;
        } offset;
        struct {
            int16_t x;
            int16_t y;
        } size;
    } background;

    struct {
        TEXT_STYLE style;
    } outline;

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
void Text_Flash(TEXTSTRING *text, bool enable, int16_t rate);
void Text_Hide(TEXTSTRING *text, bool enable);

void Text_AddBackground(
    TEXTSTRING *text, int16_t w, int16_t h, int16_t x, int16_t y,
    TEXT_STYLE style);
void Text_RemoveBackground(TEXTSTRING *text);
void Text_AddOutline(TEXTSTRING *text, TEXT_STYLE style);
void Text_RemoveOutline(TEXTSTRING *text);

void Text_CentreH(TEXTSTRING *text, bool enable);
void Text_CentreV(TEXTSTRING *text, bool enable);
void Text_AlignRight(TEXTSTRING *text, bool enable);
void Text_AlignBottom(TEXTSTRING *text, bool enable);
void Text_SetMultiline(TEXTSTRING *text, bool enable);

int32_t Text_GetWidth(const TEXTSTRING *text);
int32_t Text_GetHeight(const TEXTSTRING *text);

void Text_DrawReset(void);
void Text_Draw(void);
