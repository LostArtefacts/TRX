#include "game/text.h"

#include "config.h"
#include "game/output.h"
#include "game/screen.h"
#include "global/const.h"
#include "global/types.h"
#include "global/vars.h"

#include <stdio.h>
#include <string.h>

#define TEXT_BOX_OFFSET 2
#define TEXT_MAX_STRING_SIZE 100
#define TEXT_MAX_STRINGS 64
#define LEFT_ARROW_SYM 108
#define RIGHT_ARROW_SYM 109

static int16_t m_TextstringCount = 0;
static TEXTSTRING m_TextstringTable[TEXT_MAX_STRINGS] = { 0 };
static char m_TextstringBuffers[TEXT_MAX_STRINGS][TEXT_MAX_STRING_SIZE] = { 0 };

static int8_t m_TextSpacing[110] = {
    14 /*A*/,  11 /*B*/, 11 /*C*/, 11 /*D*/, 11 /*E*/, 11 /*F*/, 11 /*G*/,
    13 /*H*/,  8 /*I*/,  11 /*J*/, 12 /*K*/, 11 /*L*/, 13 /*M*/, 13 /*N*/,
    12 /*O*/,  11 /*P*/, 12 /*Q*/, 12 /*R*/, 11 /*S*/, 12 /*T*/, 13 /*U*/,
    13 /*V*/,  13 /*W*/, 12 /*X*/, 12 /*Y*/, 11 /*Z*/, 9 /*a*/,  9 /*b*/,
    9 /*c*/,   9 /*d*/,  9 /*e*/,  9 /*f*/,  9 /*g*/,  9 /*h*/,  5 /*i*/,
    9 /*j*/,   9 /*k*/,  5 /*l*/,  12 /*m*/, 10 /*n*/, 9 /*o*/,  9 /*p*/,
    9 /*q*/,   8 /*r*/,  9 /*s*/,  8 /*t*/,  9 /*u*/,  9 /*v*/,  11 /*w*/,
    9 /*x*/,   9 /*y*/,  9 /*z*/,  12 /*0*/, 8 /*1*/,  10 /*2*/, 10 /*3*/,
    10 /*4*/,  10 /*5*/, 10 /*6*/, 9 /*7*/,  10 /*8*/, 10 /*9*/, 5 /*.*/,
    5 /*,*/,   5 /*!*/,  11 /*?*/, 9 /*"*/,  10 /*"*/, 8 /**/,   6 /*(*/,
    6 /*)*/,   7 /*-*/,  7 /*=*/,  3 /*:*/,  11 /*%*/, 8 /*+*/,  13 /*(c)*/,
    16 /*tm*/, 9 /*&*/,  4 /*'*/,  12,       12,       7,        5,
    7,         7,        7,        7,        7,        7,        7,
    7,         16,       14,       14,       14,       16,       16,
    16,        16,       16,       12,       14,       8,        8,
    8,         8,        8,        8,        8
};

static int8_t m_TextASCIIMap[95] = {
    0 /* */,   64 /*!*/,  66 /*"*/,  78 /*#*/, 77 /*$*/, 74 /*%*/, 78 /*&*/,
    79 /*'*/,  69 /*(*/,  70 /*)*/,  92 /***/, 72 /*+*/, 63 /*,*/, 71 /*-*/,
    62 /*.*/,  68 /**/,   52 /*0*/,  53 /*1*/, 54 /*2*/, 55 /*3*/, 56 /*4*/,
    57 /*5*/,  58 /*6*/,  59 /*7*/,  60 /*8*/, 61 /*9*/, 73 /*:*/, 73 /*;*/,
    66 /*<*/,  74 /*=*/,  75 /*>*/,  65 /*?*/, 0 /**/,   0 /*A*/,  1 /*B*/,
    2 /*C*/,   3 /*D*/,   4 /*E*/,   5 /*F*/,  6 /*G*/,  7 /*H*/,  8 /*I*/,
    9 /*J*/,   10 /*K*/,  11 /*L*/,  12 /*M*/, 13 /*N*/, 14 /*O*/, 15 /*P*/,
    16 /*Q*/,  17 /*R*/,  18 /*S*/,  19 /*T*/, 20 /*U*/, 21 /*V*/, 22 /*W*/,
    23 /*X*/,  24 /*Y*/,  25 /*Z*/,  80 /*[*/, 76 /*\*/, 81 /*]*/, 97 /*^*/,
    98 /*_*/,  77 /*`*/,  26 /*a*/,  27 /*b*/, 28 /*c*/, 29 /*d*/, 30 /*e*/,
    31 /*f*/,  32 /*g*/,  33 /*h*/,  34 /*i*/, 35 /*j*/, 36 /*k*/, 37 /*l*/,
    38 /*m*/,  39 /*n*/,  40 /*o*/,  41 /*p*/, 42 /*q*/, 43 /*r*/, 44 /*s*/,
    45 /*t*/,  46 /*u*/,  47 /*v*/,  48 /*w*/, 49 /*x*/, 50 /*y*/, 51 /*z*/,
    100 /*{*/, 101 /*|*/, 102 /*}*/, 67 /*~*/
};

static RGBA8888 m_MenuColorMap[] = {
    // Purple background centre
    { 70, 30, 107, 230 },
    // Purple background edge
    { 70, 30, 107, 0 },
    // Brown background centre
    { 91, 46, 9, 255 },
    // Brown background edge
    { 91, 46, 9, 0 },
    // Grey outline centre
    { 197, 197, 197, 255 },
    // Grey outline edge
    { 45, 45, 45, 255 },
    // Grey outline top left
    { 96, 96, 96, 255 },
    // Grey outline top right
    { 32, 32, 32, 255 },
    // Grey outline bottom left
    { 63, 63, 63, 255 },
    // Grey outline bottom right
    { 0, 0, 0, 255 },
    // Black outline
    { 0, 0, 0, 255 },
    // Gold light
    { 232, 192, 112, 255 },
    // Gold dark
    { 140, 112, 56, 255 },
};

typedef struct QUAD_INFO {
    int32_t x;
    int32_t y;
    int32_t w;
    int32_t h;
} QUAD_INFO;

static void Text_DrawText(TEXTSTRING *textstring);
static uint8_t Text_MapLetterToSpriteNum(char letter);
static void Text_DrawTextBackground(
    UI_STYLE ui_style, int32_t sx, int32_t sy, int32_t w, int32_t h,
    TEXT_STYLE text_style);
static void Text_DrawTextOutline(
    UI_STYLE ui_style, int32_t sx, int32_t sy, int32_t w, int32_t h,
    TEXT_STYLE text_style);

static void Text_DrawTextBackground(
    UI_STYLE ui_style, int32_t sx, int32_t sy, int32_t w, int32_t h,
    TEXT_STYLE text_style)
{
    if (ui_style == UI_STYLE_PS1) {
        // Make sure height and width divisible by 2.
        w = 2 * ((w + 1) / 2);
        h = 2 * ((h + 1) / 2);
        Output_DrawScreenFBox(sx, sy, w, h);

        QUAD_INFO gradient_quads[4] = { { sx, sy, w / 2, h / 2 },
                                        { sx + w, sy, -w / 2, h / 2 },
                                        { sx, sy + h, w / 2, -h / 2 },
                                        { sx + w, sy + h, -w / 2, -h / 2 } };

        if (text_style == TS_HEADING) {
            for (int i = 0; i < 4; i++) {
                Output_DrawScreenGradientQuad(
                    gradient_quads[i].x, gradient_quads[i].y,
                    gradient_quads[i].w, gradient_quads[i].h,
                    Text_GetMenuColor(MC_BROWN_E),
                    Text_GetMenuColor(MC_BROWN_E),
                    Text_GetMenuColor(MC_BROWN_E),
                    Text_GetMenuColor(MC_BROWN_C));
            }
        } else if (text_style == TS_REQUESTED) {
            for (int i = 0; i < 4; i++) {
                Output_DrawScreenGradientQuad(
                    gradient_quads[i].x, gradient_quads[i].y,
                    gradient_quads[i].w, gradient_quads[i].h,
                    Text_GetMenuColor(MC_PURPLE_E),
                    Text_GetMenuColor(MC_PURPLE_E),
                    Text_GetMenuColor(MC_PURPLE_E),
                    Text_GetMenuColor(MC_PURPLE_C));
            }
        }
    } else {
        Output_DrawScreenFBox(sx, sy, w, h);
    }
}

static void Text_DrawTextOutline(
    UI_STYLE ui_style, int32_t sx, int32_t sy, int32_t w, int32_t h,
    TEXT_STYLE text_style)
{
    if (ui_style == UI_STYLE_PS1) {
        if (text_style == TS_HEADING) {
            Output_DrawGradientScreenBox(
                sx, sy, w, h, Text_GetMenuColor(MC_BLACK),
                Text_GetMenuColor(MC_BLACK), Text_GetMenuColor(MC_BLACK),
                Text_GetMenuColor(MC_BLACK), OUTLINE_THICKNESS);
        } else if (text_style == TS_BACKGROUND) {
            Output_DrawGradientScreenBox(
                sx, sy, w, h, Text_GetMenuColor(MC_GREY_TL),
                Text_GetMenuColor(MC_GREY_TR), Text_GetMenuColor(MC_GREY_BL),
                Text_GetMenuColor(MC_GREY_BR), OUTLINE_THICKNESS);
        } else if (text_style == TS_REQUESTED) {
            // Make sure height and width divisible by 2.
            w = 2 * ((w + 1) / 2);
            h = 2 * ((h + 1) / 2);
            Output_DrawCentreGradientScreenBox(
                sx, sy, w, h, Text_GetMenuColor(MC_GREY_E),
                Text_GetMenuColor(MC_GREY_C), OUTLINE_THICKNESS);
        }
    } else {
        Output_DrawScreenBox(
            sx, sy, w, h, Text_GetMenuColor(MC_GOLD_LIGHT), OUTLINE_THICKNESS);
        Output_DrawScreenBox(
            sx - 1, sy - 1, w, h, Text_GetMenuColor(MC_GOLD_DARK),
            OUTLINE_THICKNESS);
    }
}

static uint8_t Text_MapLetterToSpriteNum(char letter)
{
    if (letter >= 16) {
        return m_TextASCIIMap[letter - 32];
    } else if (letter == '\200') {
        return LEFT_ARROW_SYM;
    } else if (letter == '\201') {
        return RIGHT_ARROW_SYM;
    } else if (letter >= 11) {
        return letter + 91;
    } else {
        return letter + 81;
    }
}

RGBA8888 Text_GetMenuColor(MENU_COLOR color)
{
    return m_MenuColorMap[color];
}

void Text_Init(void)
{
    for (int i = 0; i < TEXT_MAX_STRINGS; i++) {
        m_TextstringTable[i].flags.all = 0;
    }
    m_TextstringCount = 0;
}

TEXTSTRING *Text_Create(int16_t x, int16_t y, const char *string)
{
    if (m_TextstringCount == TEXT_MAX_STRINGS) {
        return NULL;
    }

    TEXTSTRING *result = &m_TextstringTable[0];
    int n;
    for (n = 0; n < TEXT_MAX_STRINGS; n++) {
        if (!result->flags.active) {
            break;
        }
        result++;
    }
    if (n >= TEXT_MAX_STRINGS) {
        return NULL;
    }

    if (!string) {
        return NULL;
    }

    result->string = m_TextstringBuffers[n];
    result->pos.x = x;
    result->pos.y = y;
    result->letter_spacing = 1;
    result->word_spacing = 6;
    result->scale.h = PHD_ONE;
    result->scale.v = PHD_ONE;

    result->flags.all = 0;
    result->flags.active = 1;

    result->bgnd_size.x = 0;
    result->bgnd_size.y = 0;
    result->bgnd_off.x = 0;
    result->bgnd_off.y = 0;

    result->on_remove = NULL;

    Text_ChangeText(result, string);

    m_TextstringCount++;

    return result;
}

void Text_ChangeText(TEXTSTRING *textstring, const char *string)
{
    if (!textstring) {
        return;
    }
    if (!textstring->flags.active) {
        return;
    }
    size_t length = strlen(string) + 1;
    strncpy(textstring->string, string, TEXT_MAX_STRING_SIZE - 1);
    if (length >= TEXT_MAX_STRING_SIZE) {
        textstring->string[TEXT_MAX_STRING_SIZE - 1] = '\0';
    }
}

void Text_SetPos(TEXTSTRING *textstring, int16_t x, int16_t y)
{
    if (!textstring) {
        return;
    }
    textstring->pos.x = x;
    textstring->pos.y = y;
}

void Text_SetScale(TEXTSTRING *textstring, int32_t scale_h, int32_t scale_v)
{
    if (!textstring) {
        return;
    }
    textstring->scale.h = scale_h;
    textstring->scale.v = scale_v;
}

void Text_Flash(TEXTSTRING *textstring, bool enable, int16_t rate)
{
    if (!textstring) {
        return;
    }
    if (enable) {
        textstring->flags.flash = 1;
        textstring->flash.rate = rate;
        textstring->flash.count = rate;
    } else {
        textstring->flags.flash = 0;
    }
}

void Text_Hide(TEXTSTRING *textstring, bool enable)
{
    if (!textstring) {
        return;
    }
    textstring->flags.hide = enable;
}

void Text_AddBackground(
    TEXTSTRING *textstring, int16_t w, int16_t h, int16_t x, int16_t y,
    TEXT_STYLE style)
{
    if (!textstring) {
        return;
    }
    textstring->flags.background = 1;
    textstring->bgnd_size.x = w;
    textstring->bgnd_size.y = h;
    textstring->bgnd_off.x = x;
    textstring->bgnd_off.y = y;
    textstring->background.style = style;
}

void Text_RemoveBackground(TEXTSTRING *textstring)
{
    if (!textstring) {
        return;
    }
    textstring->flags.background = 0;
}

void Text_AddOutline(TEXTSTRING *textstring, bool enable, TEXT_STYLE style)
{
    if (!textstring) {
        return;
    }
    textstring->flags.outline = 1;
    textstring->outline.style = style;
}

void Text_RemoveOutline(TEXTSTRING *textstring)
{
    if (!textstring) {
        return;
    }
    textstring->flags.outline = 0;
}

void Text_CentreH(TEXTSTRING *textstring, bool enable)
{
    if (!textstring) {
        return;
    }
    textstring->flags.centre_h = enable;
}

void Text_CentreV(TEXTSTRING *textstring, bool enable)
{
    if (!textstring) {
        return;
    }
    textstring->flags.centre_v = enable;
}

void Text_AlignRight(TEXTSTRING *textstring, bool enable)
{
    if (!textstring) {
        return;
    }
    textstring->flags.right = enable;
}

void Text_AlignBottom(TEXTSTRING *textstring, bool enable)
{
    if (!textstring) {
        return;
    }
    textstring->flags.bottom = enable;
}

int32_t Text_GetWidth(TEXTSTRING *textstring)
{
    if (!textstring) {
        return 0;
    }
    int width = 0;
    char *ptr = textstring->string;
    for (char letter = *ptr; *ptr; letter = *ptr++) {
        if (letter == 0x7F || (letter > 10 && letter < 32)) {
            continue;
        }

        if (letter == 32) {
            width += textstring->word_spacing * textstring->scale.h / PHD_ONE;
            continue;
        }

        uint8_t sprite_num = Text_MapLetterToSpriteNum(letter);
        width += ((m_TextSpacing[sprite_num] + textstring->letter_spacing)
                  * textstring->scale.h)
            / PHD_ONE;
    }
    width -= textstring->letter_spacing;
    width &= 0xFFFE;
    return width;
}

void Text_Remove(TEXTSTRING *textstring)
{
    if (!textstring) {
        return;
    }
    if (textstring->flags.active) {
        if (textstring->on_remove) {
            textstring->on_remove(textstring);
        }
        textstring->flags.active = 0;
        m_TextstringCount--;
    }
}

void Text_RemoveAll(void)
{
    for (int i = 0; i < TEXT_MAX_STRINGS; i++) {
        TEXTSTRING *textstring = &m_TextstringTable[i];
        if (textstring->flags.active) {
            Text_Remove(textstring);
        }
    }
    Text_Init();
}

void Text_Draw(void)
{
    for (int i = 0; i < TEXT_MAX_STRINGS; i++) {
        TEXTSTRING *textstring = &m_TextstringTable[i];
        if (textstring->flags.active) {
            Text_DrawText(textstring);
        }
    }
}

static void Text_DrawText(TEXTSTRING *textstring)
{
    int sx, sy, sh, sv;

    if (textstring->flags.hide) {
        return;
    }

    if (textstring->flags.flash) {
        textstring->flash.count -= (int16_t)g_Camera.number_frames;
        if (textstring->flash.count <= -textstring->flash.rate) {
            textstring->flash.count = textstring->flash.rate;
        } else if (textstring->flash.count < 0) {
            return;
        }
    }

    char *string = textstring->string;
    int32_t x = textstring->pos.x;
    int32_t y = textstring->pos.y;
    int32_t textwidth = Text_GetWidth(textstring);

    if (textstring->flags.centre_h) {
        x += (Screen_GetResWidthDownscaled() - textwidth) / 2;
    } else if (textstring->flags.right) {
        x += Screen_GetResWidthDownscaled() - textwidth;
    }

    if (textstring->flags.centre_v) {
        y += Screen_GetResHeightDownscaled() / 2;
    } else if (textstring->flags.bottom) {
        y += Screen_GetResHeightDownscaled();
    }

    int32_t bxpos = textstring->bgnd_off.x + x - TEXT_BOX_OFFSET;
    int32_t bypos =
        textstring->bgnd_off.y + y - TEXT_BOX_OFFSET * 2 - TEXT_HEIGHT;

    int32_t letter = '\0';
    while (*string) {
        letter = *string++;
        if (letter > 15 && letter < 32) {
            continue;
        }

        if (letter == ' ') {
            x += (textstring->word_spacing * textstring->scale.h) / PHD_ONE;
            continue;
        }

        uint8_t sprite_num = Text_MapLetterToSpriteNum(letter);
        sx = Screen_GetRenderScale(x);
        sy = Screen_GetRenderScale(y);
        sh = Screen_GetRenderScale(textstring->scale.h);
        sv = Screen_GetRenderScale(textstring->scale.v);

        Output_DrawScreenSprite2D(
            sx, sy, 0, sh, sv, g_Objects[O_ALPHABET].mesh_index + sprite_num,
            16 << 8, 0, 0);

        if (letter == '(' || letter == ')' || letter == '$' || letter == '~') {
            continue;
        }

        x += (((int32_t)textstring->letter_spacing + m_TextSpacing[sprite_num])
              * textstring->scale.h)
            / PHD_ONE;
    }

    int32_t bwidth = 0;
    int32_t bheight = 0;
    if (textstring->flags.background || textstring->flags.outline) {
        if (textstring->bgnd_size.x) {
            bxpos += textwidth / 2;
            bxpos -= textstring->bgnd_size.x / 2;
            bwidth = textstring->bgnd_size.x + TEXT_BOX_OFFSET * 2;
        } else {
            bwidth = textwidth + TEXT_BOX_OFFSET * 2;
        }
        if (textstring->bgnd_size.y) {
            bheight = textstring->bgnd_size.y;
        } else {
            bheight = TEXT_HEIGHT + 7;
        }
    }

    if (textstring->flags.background) {
        sx = Screen_GetRenderScale(bxpos);
        sy = Screen_GetRenderScale(bypos);
        sh = Screen_GetRenderScale(bwidth);
        sv = Screen_GetRenderScale(bheight);

        Text_DrawTextBackground(
            g_Config.ui.menu_style, sx, sy, sh, sv,
            textstring->background.style);
    }

    if (textstring->flags.outline) {
        sx = Screen_GetRenderScale(bxpos);
        sy = Screen_GetRenderScale(bypos);
        sh = Screen_GetRenderScale(bwidth);
        sv = Screen_GetRenderScale(bheight);

        Text_DrawTextOutline(
            g_Config.ui.menu_style, sx, sy, sh, sv, textstring->outline.style);
    }
}
