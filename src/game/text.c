#include "game/text.h"

#include "global/const.h"
#include "global/types.h"
#include "global/vars.h"
#include "specific/clock.h"
#include "specific/frontend.h"
#include "specific/output.h"
#include "util.h"

#include <stdio.h>
#include <string.h>

#define TEXT_BOX_OFFSET 2
#define TEXT_MAX_STRING_SIZE 100
#define TEXT_MAX_STRINGS 64

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

static void Text_DrawText(TEXTSTRING *textstring);

void Text_Init()
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
    CLAMPG(length, TEXT_MAX_STRING_SIZE);
    strncpy(textstring->string, string, length);
    if (length >= TEXT_MAX_STRING_SIZE) {
        textstring->string[TEXT_MAX_STRING_SIZE - 1] = '\0';
    }
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

void Text_AddBackground(
    TEXTSTRING *textstring, int16_t w, int16_t h, int16_t x, int16_t y)
{
    if (!textstring) {
        return;
    }
    textstring->flags.background = 1;
    textstring->bgnd_size.x = w;
    textstring->bgnd_size.y = h;
    textstring->bgnd_off.x = x;
    textstring->bgnd_off.y = y;
}

void Text_RemoveBackground(TEXTSTRING *textstring)
{
    if (!textstring) {
        return;
    }
    textstring->flags.background = 0;
}

void Text_AddOutline(TEXTSTRING *textstring, bool enable)
{
    if (!textstring) {
        return;
    }
    textstring->flags.outline = 1;
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

        if (letter >= 16) {
            letter = m_TextASCIIMap[letter - 32];
        } else if (letter >= 11) {
            letter = letter + 91;
        } else {
            letter = letter + 81;
        }

        width += ((m_TextSpacing[(uint8_t)letter] + textstring->letter_spacing)
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

void Text_RemoveAll()
{
    for (int i = 0; i < TEXT_MAX_STRINGS; i++) {
        TEXTSTRING *textstring = &m_TextstringTable[i];
        if (textstring->flags.active) {
            Text_Remove(textstring);
        }
    }
    Text_Init();
}

void Text_Draw()
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
    if (textstring->flags.flash) {
        textstring->flash.count -= (int16_t)Camera.number_frames;
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
        x += (GetRenderWidthDownscaled() - textwidth) / 2;
    } else if (textstring->flags.right) {
        x += GetRenderWidthDownscaled() - textwidth;
    }

    if (textstring->flags.centre_v) {
        y += GetRenderHeightDownscaled() / 2;
    } else if (textstring->flags.bottom) {
        y += GetRenderHeightDownscaled();
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

        int32_t sprite_num = letter;
        if (letter >= 16) {
            sprite_num = m_TextASCIIMap[letter - 32];
        } else if (letter >= 11) {
            sprite_num = letter + 91;
        } else {
            sprite_num = letter + 81;
        }

        sx = GetRenderScale(x);
        sy = GetRenderScale(y);
        sh = GetRenderScale(textstring->scale.h);
        sv = GetRenderScale(textstring->scale.v);

        S_DrawScreenSprite2d(
            sx, sy, 0, sh, sv, Objects[O_ALPHABET].mesh_index + sprite_num,
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
        sx = GetRenderScale(bxpos);
        sy = GetRenderScale(bypos);
        sh = GetRenderScale(bwidth);
        sv = GetRenderScale(bheight);

        S_DrawScreenFBox(sx, sy, sh, sv);
    }

    if (textstring->flags.outline) {
        sx = GetRenderScale(bxpos);
        sy = GetRenderScale(bypos);
        sh = GetRenderScale(bwidth);
        sv = GetRenderScale(bheight);
        S_DrawScreenBox(sx, sy, sh, sv);
    }
}
