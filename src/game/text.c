#include "game/text.h"

#ifdef DEBUG_TEXT_SPRITES
    #include "3dsystem/3d_insert.h"
#endif
#include "global/const.h"
#include "global/types.h"
#include "global/vars.h"
#include "specific/frontend.h"
#include "specific/output.h"
#include "util.h"

#include <stdio.h>
#include <string.h>

#define TEXT_BOX_OFFSET 2

static int16_t TextStringCount = 0;
static TEXTSTRING TextInfoTable[MAX_TEXT_STRINGS];
static char TextStrings[MAX_TEXT_STRINGS][MAX_STRING_SIZE];

static int8_t TextSpacing[110] = {
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

static int8_t TextRemapASCII[95] = {
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

int T_GetStringLen(const char *string)
{
    int len = 1;
    while (*string++) {
        len++;
    }
    return len;
}

void T_InitPrint()
{
    for (int i = 0; i < MAX_TEXT_STRINGS; i++) {
        TextInfoTable[i].flags = 0;
    }
    TextStringCount = 0;
}

TEXTSTRING *
T_Print(int16_t xpos, int16_t ypos, int16_t zpos, const char *string)
{
    if (TextStringCount == MAX_TEXT_STRINGS) {
        return NULL;
    }

    TEXTSTRING *result = &TextInfoTable[0];
    int n;
    for (n = 0; n < MAX_TEXT_STRINGS; n++) {
        if (!(result->flags & TF_ACTIVE)) {
            break;
        }
        result++;
    }
    if (n >= MAX_TEXT_STRINGS) {
        return NULL;
    }

    if (!string) {
        return NULL;
    }

    int length = T_GetStringLen(string);
    if (length >= MAX_STRING_SIZE) {
        length = MAX_STRING_SIZE - 1;
    }

    result->xpos = xpos;
    result->ypos = ypos;
    result->zpos = zpos;
    result->letter_spacing = 1;
    result->word_spacing = 6;
    result->scale_h = PHD_ONE;
    result->scale_v = PHD_ONE;

    result->string = TextStrings[n];
    memcpy(result->string, string, length + 1);

    result->flags = TF_ACTIVE;
    result->text_flags = 0;
    result->outl_flags = 0;
    result->bgnd_flags = 0;

    result->bgnd_size_x = 0;
    result->bgnd_size_y = 0;
    result->bgnd_off_x = 0;
    result->bgnd_off_y = 0;
    result->bgnd_off_z = 0;

    TextStringCount++;

    return result;
}

void T_ChangeText(TEXTSTRING *textstring, const char *string)
{
    if (!textstring) {
        return;
    }
    if (!(textstring->flags & TF_ACTIVE)) {
        return;
    }
    strncpy(textstring->string, string, MAX_STRING_SIZE);
    if (T_GetStringLen(string) > MAX_STRING_SIZE) {
        textstring->string[MAX_STRING_SIZE - 1] = '\0';
    }
}

void T_SetScale(TEXTSTRING *textstring, int32_t scale_h, int32_t scale_v)
{
    if (!textstring) {
        return;
    }
    textstring->scale_h = scale_h;
    textstring->scale_v = scale_v;
}

void T_FlashText(TEXTSTRING *textstring, int16_t b, int16_t rate)
{
    if (!textstring) {
        return;
    }
    if (b) {
        textstring->flags |= TF_FLASH;
        textstring->flash_rate = rate;
        textstring->flash_count = rate;
    } else {
        textstring->flags &= ~TF_FLASH;
    }
}

void T_AddBackground(
    TEXTSTRING *textstring, int16_t xsize, int16_t ysize, int16_t xoff,
    int16_t yoff, int16_t zoff, int16_t colour, SG_COL *gourptr, int16_t flags)
{
    if (!textstring) {
        return;
    }
    textstring->flags |= TF_BGND;
    textstring->bgnd_size_x = xsize;
    textstring->bgnd_size_y = ysize;
    textstring->bgnd_off_x = xoff;
    textstring->bgnd_off_y = yoff;
    textstring->bgnd_off_z = zoff;
    textstring->bgnd_colour = colour;
    textstring->bgnd_gour = gourptr;
    textstring->bgnd_flags = flags;
}

void T_RemoveBackground(TEXTSTRING *textstring)
{
    if (!textstring) {
        return;
    }
    textstring->flags &= ~TF_BGND;
}

void T_AddOutline(
    TEXTSTRING *textstring, int16_t b, int16_t colour, SG_COL *gourptr,
    int16_t flags)
{
    if (!textstring) {
        return;
    }
    textstring->flags |= TF_OUTLINE;
    textstring->outl_gour = gourptr;
    textstring->outl_colour = colour;
    textstring->outl_flags = flags;
}

void T_RemoveOutline(TEXTSTRING *textstring)
{
    if (!textstring) {
        return;
    }
    textstring->flags &= ~TF_OUTLINE;
}

void T_CentreH(TEXTSTRING *textstring, int16_t b)
{
    if (!textstring) {
        return;
    }
    if (b) {
        textstring->flags |= TF_CENTRE_H;
    } else {
        textstring->flags &= ~TF_CENTRE_H;
    }
}

void T_CentreV(TEXTSTRING *textstring, int16_t b)
{
    if (!textstring) {
        return;
    }
    if (b) {
        textstring->flags |= TF_CENTRE_V;
    } else {
        textstring->flags &= ~TF_CENTRE_V;
    }
}

void T_RightAlign(TEXTSTRING *textstring, int16_t b)
{
    if (!textstring) {
        return;
    }
    if (b) {
        textstring->flags |= TF_RIGHT;
    } else {
        textstring->flags &= ~TF_RIGHT;
    }
}

void T_BottomAlign(TEXTSTRING *textstring, int16_t b)
{
    if (!textstring) {
        return;
    }
    if (b) {
        textstring->flags |= TF_BOTTOM;
    } else {
        textstring->flags &= ~TF_BOTTOM;
    }
}

int32_t T_GetTextWidth(TEXTSTRING *textstring)
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
            width += textstring->word_spacing * textstring->scale_h / PHD_ONE;
            continue;
        }

        if (letter >= 16) {
            letter = TextRemapASCII[letter - 32];
        } else if (letter >= 11) {
            letter = letter + 91;
        } else {
            letter = letter + 81;
        }

        width += ((TextSpacing[(uint8_t)letter] + textstring->letter_spacing)
                  * textstring->scale_h)
            / PHD_ONE;
    }
    width -= textstring->letter_spacing;
    width &= 0xFFFE;
    return width;
}

void T_RemovePrint(TEXTSTRING *textstring)
{
    if (!textstring) {
        return;
    }
    if (textstring->flags & TF_ACTIVE) {
        textstring->flags &= ~TF_ACTIVE;
        TextStringCount--;
    }
}

void T_RemoveAllPrints()
{
    T_InitPrint();
}

void T_DrawText()
{
    // TombATI FPS counter, pretty pointless IMO as it always shows 30 for me.
    // Additionally, it's not present in TR2+.
    static TEXTSTRING *fps_text = NULL;
    static char fps_buf[20];
    static int fps_counter1 = 0;
    static int fps_counter2 = 0;

    // In case someone called T_RemoveAllPrints or similar.
    if (fps_text && !(fps_text->flags & TF_ACTIVE)) {
        fps_text = NULL;
    }

    if (AppSettings & ASF_FPS) {
        int fps = ++fps_counter1;
        int tmp = Camera.number_frames + fps_counter2;
        fps_counter2 += Camera.number_frames;
        if (tmp >= 60) {
            sprintf(fps_buf, "%d FPS", fps);
            if (fps_text) {
                T_ChangeText(fps_text, fps_buf);
                fps_counter1 = 0;
                fps_counter2 = 0;
            } else {
                fps_text = T_Print(10, 30, 0, fps_buf);
                fps_counter1 = 0;
                fps_counter2 = 0;
            }
        }
        if (Camera.number_frames > 30 && fps_text) {
            T_RemovePrint(fps_text);
            fps_text = NULL;
        }
    } else if (fps_text) {
        T_RemovePrint(fps_text);
        fps_text = NULL;
    }

    for (int i = 0; i < MAX_TEXT_STRINGS; i++) {
        TEXTSTRING *textstring = &TextInfoTable[i];
        if (textstring->flags & TF_ACTIVE) {
            T_DrawThisText(textstring);
        }
    }
}

void T_DrawThisText(TEXTSTRING *textstring)
{
    int sx, sy, sh, sv;
    if (textstring->flags & TF_FLASH) {
        textstring->flash_count -= (int16_t)Camera.number_frames;
        if (textstring->flash_count <= -textstring->flash_rate) {
            textstring->flash_count = textstring->flash_rate;
        } else if (textstring->flash_count < 0) {
            return;
        }
    }

    char *string = textstring->string;
    int32_t xpos = textstring->xpos;
    int32_t ypos = textstring->ypos;
    int32_t zpos = textstring->zpos;
    int32_t textwidth = T_GetTextWidth(textstring);

    if (textstring->flags & TF_CENTRE_H) {
        xpos += (GetRenderWidthDownscaled() - textwidth) / 2;
    } else if (textstring->flags & TF_RIGHT) {
        xpos += GetRenderWidthDownscaled() - textwidth;
    }

    if (textstring->flags & TF_CENTRE_V) {
        ypos += GetRenderHeightDownscaled() / 2;
    } else if (textstring->flags & TF_BOTTOM) {
        ypos += GetRenderHeightDownscaled();
    }

    int32_t bxpos = textstring->bgnd_off_x + xpos - TEXT_BOX_OFFSET;
    int32_t bypos =
        textstring->bgnd_off_y + ypos - TEXT_BOX_OFFSET * 2 - TEXT_HEIGHT;

#ifdef DEBUG_TEXT_SPRITES
    int32_t fx = GetRenderScale(xpos);
    int32_t fy = GetRenderScale(ypos);
#endif

    int32_t letter = '\0';
    while (*string) {
        letter = *string++;
        if (letter > 15 && letter < 32) {
            continue;
        }

        if (letter == ' ') {
            xpos += (textstring->word_spacing * textstring->scale_h) / PHD_ONE;
            continue;
        }

        int32_t sprite_num = letter;
        if (letter >= 16) {
            sprite_num = TextRemapASCII[letter - 32];
        } else if (letter >= 11) {
            sprite_num = letter + 91;
        } else {
            sprite_num = letter + 81;
        }

        sx = GetRenderScale(xpos);
        sy = GetRenderScale(ypos);
        sh = GetRenderScale(textstring->scale_h);
        sv = GetRenderScale(textstring->scale_v);

#ifdef DEBUG_TEXT_SPRITES
        PHD_SPRITE *sprite =
            &PhdSpriteInfo[0][0] + Objects[O_ALPHABET].mesh_index + sprite_num;
        int32_t fx1 = sx + ((sh * sprite->x1) >> 16);
        int32_t fx2 = sx + ((sh * sprite->x2) >> 16);
        int32_t fy1 = sy + ((sv * sprite->y1) >> 16);
        int32_t fy2 = sy + ((sv * sprite->y2) >> 16);
        Insert2DLine(fx1, fy1, fx2, fy1, -100, 0);
        Insert2DLine(fx1, fy2, fx2, fy2, -100, 0);
        Insert2DLine(fx1, fy1, fx1, fy2, -100, 0);
        Insert2DLine(fx2, fy1, fx2, fy2, -100, 0);
#endif

        S_DrawScreenSprite2d(
            sx, sy, zpos, sh, sv, Objects[O_ALPHABET].mesh_index + sprite_num,
            16 << 8, textstring->text_flags, 0);

        if (letter == '(' || letter == ')' || letter == '$' || letter == '~') {
            continue;
        }

        xpos += (((int32_t)textstring->letter_spacing + TextSpacing[sprite_num])
                 * textstring->scale_h)
            / PHD_ONE;
    }

#ifdef DEBUG_TEXT_SPRITES
    Insert2DLine(fx - 5, fy, fx + 5, fy, -100, 112);
    Insert2DLine(fx, fy - 5, fx, fy + 5, -100, 112);
#endif

    int32_t bwidth = 0;
    int32_t bheight = 0;
    if ((textstring->flags & TF_BGND) || (textstring->flags & TF_OUTLINE)) {
        if (textstring->bgnd_size_x) {
            bxpos += textwidth / 2;
            bxpos -= textstring->bgnd_size_x / 2;
            bwidth = textstring->bgnd_size_x + TEXT_BOX_OFFSET * 2;
        } else {
            bwidth = textwidth + TEXT_BOX_OFFSET * 2;
        }
        if (textstring->bgnd_size_y) {
            bheight = textstring->bgnd_size_y;
        } else {
            bheight = TEXT_HEIGHT + 7;
        }
    }

    if (textstring->flags & TF_BGND) {
        sx = GetRenderScale(bxpos);
        sy = GetRenderScale(bypos);
        sh = GetRenderScale(bwidth);
        sv = GetRenderScale(bheight);

        if (textstring->bgnd_gour) {
            int32_t bhw = sh / 2;
            int32_t bhh = sv / 2;
            int32_t bhw2 = sh - bhw;
            int32_t bhh2 = sv - bhh;

            S_DrawScreenFBox(
                sx, sy, zpos + textstring->bgnd_off_z + 8, bhw, bhh,
                textstring->bgnd_colour, textstring->bgnd_gour,
                textstring->bgnd_flags);

            S_DrawScreenFBox(
                sx + bhw, sy, zpos + textstring->bgnd_off_z + 8, bhw2, bhh,
                textstring->bgnd_colour, textstring->bgnd_gour + 4,
                textstring->bgnd_flags);

            S_DrawScreenFBox(
                sx + bhw, sy + bhh, zpos + textstring->bgnd_off_z + 8, bhw,
                bhh2, textstring->bgnd_colour, textstring->bgnd_gour + 8,
                textstring->bgnd_flags);

            S_DrawScreenFBox(
                sx, sy + bhh, zpos + textstring->bgnd_off_z + 8, bhw2, bhh2,
                textstring->bgnd_colour, textstring->bgnd_gour + 12,
                textstring->bgnd_flags);
        } else {
            S_DrawScreenFBox(
                sx, sy, zpos + textstring->bgnd_off_z + 8, sh, sv,
                textstring->bgnd_colour, 0, textstring->bgnd_flags);
            S_DrawScreenBox(
                sx, sy, textstring->bgnd_off_z + zpos, sh, sv, 0, 0, 0);
        }
    }

    if (textstring->flags & TF_OUTLINE) {
        sx = GetRenderScale(bxpos);
        sy = GetRenderScale(bypos);
        sh = GetRenderScale(bwidth);
        sv = GetRenderScale(bheight);
        S_DrawScreenBox(
            sx, sy, zpos + textstring->bgnd_off_z, sh, sv,
            textstring->outl_colour, textstring->outl_gour,
            textstring->outl_flags);
    }
}

void T1MInjectGameText()
{
    INJECT(0x00439750, T_InitPrint);
    INJECT(0x00439780, T_Print);
    INJECT(0x00439860, T_ChangeText);
    INJECT(0x004398A0, T_SetScale);
    INJECT(0x004398C0, T_FlashText);
    INJECT(0x004398F0, T_AddBackground);
    INJECT(0x00439950, T_RemoveBackground);
    INJECT(0x00439960, T_AddOutline);
    INJECT(0x00439990, T_RemoveOutline);
    INJECT(0x004399A0, T_CentreH);
    INJECT(0x004399C0, T_CentreV);
    INJECT(0x004399E0, T_RightAlign);
    INJECT(0x00439A00, T_BottomAlign);
    INJECT(0x00439A20, T_GetTextWidth);
    INJECT(0x00439AD0, T_RemovePrint);
    INJECT(0x00439B00, T_DrawText);
    INJECT(0x00439C60, T_DrawThisText);
}
