#include "game/background.h"

#include "game/hwr.h"
#include "global/const.h"
#include "global/funcs.h"
#include "global/vars.h"

#include <libtrx/utils.h>

#include <math.h>

#define TEXTURE_WIDTH 256
#define TEXTURE_HEIGHT 256

void __cdecl BGND_Make640x480(uint8_t *bitmap, RGB_888 *palette)
{
    if (g_TextureFormat.bpp >= 16) {
        g_BGND_PaletteIndex = -1;
    } else {
        g_BGND_PaletteIndex = CreateTexturePalette(palette);
    }

    const int32_t buf_size = 640 * 480 * 2;
    uint8_t *buf = game_malloc(buf_size, GBUF_TEMP_ALLOC);
    UT_MemBlt(buf, 0, 0, 256, 256, 256, bitmap, 0, 0, 640);
    BGND_AddTexture(0, buf, g_BGND_PaletteIndex, palette);

    UT_MemBlt(buf, 0, 0, 256, 256, 256, bitmap, 256, 0, 640);
    BGND_AddTexture(1, buf, g_BGND_PaletteIndex, palette);

    UT_MemBlt(buf, 0, 0, 128, 256, 256, bitmap, 512, 0, 640);
    UT_MemBlt(buf, 128, 0, 128, 224, 256, bitmap, 512, 256, 640);
    BGND_AddTexture(2, buf, g_BGND_PaletteIndex, palette);

    UT_MemBlt(buf, 0, 0, 256, 224, 256, bitmap, 0, 256, 640);
    BGND_AddTexture(3, buf, g_BGND_PaletteIndex, palette);

    UT_MemBlt(buf, 0, 0, 256, 224, 256, bitmap, 256, 256, 640);
    BGND_AddTexture(4, buf, g_BGND_PaletteIndex, palette);

    game_free(buf_size);

    BGND_GetPageHandles();

    g_BGND_PictureIsReady = true;
}

int32_t __cdecl BGND_AddTexture(
    const int32_t tile_idx, uint8_t *const bitmap, const int32_t pal_index,
    const RGB_888 *const bmp_pal)
{
    int32_t page_index;
    if (pal_index < 0) {
        uint8_t *bmp_src = &bitmap[TEXTURE_WIDTH * TEXTURE_HEIGHT];
        uint16_t *bmp_dst =
            &((uint16_t *)bitmap)[TEXTURE_WIDTH * TEXTURE_HEIGHT];
        for (int32_t i = 0; i < TEXTURE_WIDTH * TEXTURE_HEIGHT; i++) {
            bmp_src--;
            bmp_dst--;

            const RGB_888 *const color = &bmp_pal[*bmp_src];

            *bmp_dst = (1 << 15) | (((uint16_t)color->red >> 3) << 10)
                | (((uint16_t)color->green >> 3) << 5)
                | (((uint16_t)color->blue >> 3));
        }

        page_index = AddTexturePage16(256, 256, bmp_src);
    } else {
        page_index = AddTexturePage8(256, 256, bitmap, pal_index);
    }

    g_BGND_TexturePageIndexes[tile_idx] = page_index >= 0 ? page_index : -1;
    return page_index;
}

void __cdecl BGND_GetPageHandles(void)
{
    for (int32_t i = 0; i < BGND_MAX_TEXTURE_PAGES; i++) {
        g_BGND_PageHandles[i] = g_BGND_TexturePageIndexes[i] >= 0
            ? GetTexturePageHandle(g_BGND_TexturePageIndexes[i])
            : 0;
    }
}

void __cdecl BGND_DrawInGameBlack(void)
{
    HWR_EnableZBuffer(false, false);
    DrawQuad(
        (float)g_PhdWinMinX, (float)g_PhdWinMinY, (float)g_PhdWinWidth,
        (float)g_PhdWinHeight, 0);
    HWR_EnableZBuffer(true, true);
}

void __cdecl DrawQuad(
    const float sx, const float sy, const float width, const float height,
    const D3DCOLOR color)
{
    D3DTLVERTEX vertex[4];

    vertex[0].sx = sx;
    vertex[0].sy = sy;

    vertex[1].sx = sx + width;
    vertex[1].sy = sy;

    vertex[2].sx = sx;
    vertex[2].sy = sy + height;

    vertex[3].sx = sx + width;
    vertex[3].sy = sy + height;

    for (int32_t i = 0; i < 4; i++) {
        vertex[i].sz = 0;
        vertex[i].rhw = g_FltRhwONearZ;
        vertex[i].color = RGBA_SETALPHA(color, 0xFF);
        vertex[i].specular = 0;
    }

    HWR_TexSource(0);
    HWR_EnableColorKey(false);
    HWR_DrawPrimitive(D3DPT_TRIANGLESTRIP, &vertex, 4, true);
}

void __cdecl BGND_DrawInGameBackground(void)
{
    const OBJECT *const obj = &g_Objects[O_INV_BACKGROUND];
    if (!obj->loaded) {
        BGND_DrawInGameBlack();
        return;
    }

    const int16_t *mesh_ptr = g_Meshes[obj->mesh_idx];
    mesh_ptr += 5;
    const int32_t num_vertices = *mesh_ptr++;
    mesh_ptr += num_vertices * 3;

    const int32_t num_normals = *mesh_ptr++;
    if (num_normals >= 0) {
        mesh_ptr += num_normals * 3;
    } else {
        mesh_ptr -= num_normals;
    }
    const int32_t num_quads = *mesh_ptr++;
    if (num_quads < 1) {
        BGND_DrawInGameBlack();
        return;
    }

    mesh_ptr += 4;
    const int32_t texture_idx = *mesh_ptr++;

    const PHD_TEXTURE *texture = &g_PhdTextureInfo[texture_idx];
    const HWR_TEXTURE_HANDLE tex_source = g_HWR_PageHandles[texture->tex_page];
    const int32_t tu = texture->uv[0].u / PHD_HALF;
    const int32_t tv = texture->uv[0].v / PHD_HALF;
    const int32_t t_width = texture->uv[2].u / PHD_HALF - tu + 1;
    const int32_t t_height = texture->uv[2].v / PHD_HALF - tv + 1;

    HWR_EnableZBuffer(false, false);

    const int32_t y_count = 6;
    const int32_t x_count = 8;

    int32_t y_current = 0;
    for (int32_t y = 0; y < y_count; y++) {
        const int32_t y_next = g_PhdWinHeight + y_current;

        int32_t x_current = 0;
        for (int32_t x = 0; x < x_count; x++) {
            const int32_t x_next = x_current + g_PhdWinWidth;

            const int32_t y0 = g_PhdWinMinY + y_current / y_count;
            const int32_t x0 = g_PhdWinMinX + x_current / x_count;
            const int32_t x1 = g_PhdWinMinX + x_next / x_count;
            const int32_t y1 = g_PhdWinMinY + y_next / y_count;

            const D3DCOLOR color[4] = {
                BGND_CenterLighting(x0, y0, g_PhdWinWidth, g_PhdWinHeight),
                BGND_CenterLighting(x1, y0, g_PhdWinWidth, g_PhdWinHeight),
                BGND_CenterLighting(x0, y1, g_PhdWinWidth, g_PhdWinHeight),
                BGND_CenterLighting(x1, y1, g_PhdWinWidth, g_PhdWinHeight),
            };

            DrawTextureTile(
                x0, y0, x1 - x0, y1 - y0, tex_source, tu, tv, t_width, t_height,
                color[0], color[1], color[2], color[3]);

            x_current = x_next;
        }
        y_current = y_next;
    }

    HWR_EnableZBuffer(true, true);
}

void __cdecl DrawTextureTile(
    const int32_t sx, const int32_t sy, const int32_t width,
    const int32_t height, const HWR_TEXTURE_HANDLE tex_source, const int32_t tu,
    const int32_t tv, const int32_t t_width, const int32_t t_height,
    const D3DCOLOR color0, const D3DCOLOR color1, const D3DCOLOR color2,
    const D3DCOLOR color3)
{
    const D3DVALUE sx0 = sx;
    const D3DVALUE sy0 = sy;
    const D3DVALUE sx1 = sx + width;
    const D3DVALUE sy1 = sy + height;

    const double uv_adjust = g_UVAdd / (double)PHD_ONE;
    const D3DVALUE tu0 = tu / 256.0 + uv_adjust;
    const D3DVALUE tv0 = tv / 256.0 + uv_adjust;
    const D3DVALUE tu1 = (tu + t_width) / 256.0 - uv_adjust;
    const D3DVALUE tv1 = (tv + t_height) / 256.0 - uv_adjust;

    D3DTLVERTEX vertex[4] = {
        {
            .sx = sx0,
            .sy = sy0,
            .tu = tu0,
            .tv = tv0,
            .color = color0,
        },
        {
            .sx = sx1,
            .sy = sy0,
            .tu = tu1,
            .tv = tv0,
            .color = color1,
        },
        {
            .sx = sx0,
            .sy = sy1,
            .tu = tu0,
            .tv = tv1,
            .color = color2,
        },
        {
            .sx = sx1,
            .sy = sy1,
            .tu = tu1,
            .tv = tv1,
            .color = color3,
        },
    };

    for (int32_t i = 0; i < 4; i++) {
        vertex[i].sz = 0.995;
        vertex[i].rhw = g_RhwFactor / g_FltFarZ;
        vertex[i].specular = 0;
    }

    HWR_TexSource(tex_source);
    HWR_EnableColorKey(0);
    HWR_DrawPrimitive(D3DPT_TRIANGLESTRIP, &vertex, 4, true);
}

D3DCOLOR __cdecl BGND_CenterLighting(
    const int32_t x, const int32_t y, const int32_t width, const int32_t height)
{
    const double x_dist = (double)(x - (width / 2)) / (double)width;
    const double y_dist = (double)(y - (height / 2)) / (double)height;

    int32_t light = 256 - (sqrt(x_dist * x_dist + y_dist * y_dist) * 300.0);
    CLAMP(light, 0, 255);

    return RGBA_MAKE(light, light, light, 0xFF);
}

void __cdecl BGND_Free(void)
{
    for (int32_t i = 0; i < BGND_MAX_TEXTURE_PAGES; i++) {
        if (g_BGND_TexturePageIndexes[i] >= 0) {
            SafeFreeTexturePage(g_BGND_TexturePageIndexes[i]);
            g_BGND_TexturePageIndexes[i] = -1;
        }
        g_BGND_PageHandles[i] = 0;
    }

    if (g_BGND_PaletteIndex >= 0) {
        SafeFreePalette(g_BGND_PaletteIndex);
        g_BGND_PaletteIndex = -1;
    }
}

bool __cdecl BGND_Init(void)
{
    g_BGND_PictureIsReady = false;
    g_BGND_PaletteIndex = -1;
    for (int32_t i = 0; i < BGND_MAX_TEXTURE_PAGES; i++) {
        g_BGND_TexturePageIndexes[i] = -1;
    }
    return true;
}
