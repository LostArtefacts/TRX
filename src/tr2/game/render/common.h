#pragma once

#include "global/types.h"

#include <libtrx/engine/image.h>
#include <libtrx/gfx/2d/2d_surface.h>

#include <stdint.h>

// TODO: get rid of these consts
#define SW_DETAIL_LOW (0 * WALL_L << W2V_SHIFT) // = 0
#define SW_DETAIL_MEDIUM (3 * WALL_L << W2V_SHIFT) // = 50331648
#define SW_DETAIL_HIGH (6 * WALL_L << W2V_SHIFT) // = 100663296
#define SW_DETAIL_ULTRA (20 * WALL_L << W2V_SHIFT) // = 335544320

typedef enum {
    ST_AVG_Z = 0,
    ST_MAX_Z = 1,
    ST_FAR_Z = 2,
} SORT_TYPE;

typedef enum {
    // clang-format off
    RENDER_RESET_VIEWPORT = 1 << 0,
    RENDER_RESET_PALETTE  = 1 << 1,
    RENDER_RESET_TEXTURES = 1 << 2,
    RENDER_RESET_PARAMS   = 1 << 3,
    RENDER_RESET_UVS      = 1 << 4,
    RENDER_RESET_ALL      = INT32_MAX,
    // clang-format on
} RENDER_RESET_FLAGS;

void Render_Init(void);
void Render_Shutdown(void);

void Render_Reset(RENDER_RESET_FLAGS reset_flags);
void Render_SetupDisplay(
    int32_t window_border, int32_t window_width, int32_t window_height,
    int32_t screen_width, int32_t screen_height);

void Render_BeginScene(void);
void Render_EndScene(void);

void Render_LoadBackgroundFromTexture(
    const OBJECT_TEXTURE *texture, int32_t repeat_x, int32_t repeat_y);
void Render_LoadBackgroundFromImage(const IMAGE *image);
void Render_UnloadBackground(void);
void Render_DrawBackground(void);

void Render_DrawPolyList(void);
void Render_DrawBlackRectangle(int32_t opacity);

void Render_ClearZBuffer(void);
void Render_EnableZBuffer(bool z_write_enable, bool z_test_enable);
void Render_SetWet(bool is_wet);

// TODO: there's too much repetition for these
void Render_InsertFlatFace3s(
    const FACE3 *faces, int32_t num, SORT_TYPE sort_type);
void Render_InsertFlatFace4s(
    const FACE4 *faces, int32_t num, SORT_TYPE sort_type);
void Render_InsertTexturedFace3s(
    const FACE3 *faces, int32_t num, SORT_TYPE sort_type);
void Render_InsertTexturedFace4s(
    const FACE4 *faces, int32_t num, SORT_TYPE sort_type);
void Render_InsertLine(
    int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t z,
    uint8_t color_idx);
void Render_InsertFlatRect(
    int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t z,
    uint8_t color_idx);
void Render_InsertTransQuad(
    int32_t x, int32_t y, int32_t width, int32_t height, int32_t z,
    uint8_t alpha);
void Render_InsertTransOctagon(const PHD_VBUF *vbuf, int16_t shade);
void Render_InsertSprite(
    int32_t z, int32_t x0, int32_t y0, int32_t x1, int32_t y1,
    int32_t sprite_idx, int16_t shade);
