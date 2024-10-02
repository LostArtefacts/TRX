#pragma once

typedef enum {
    GFX_TF_FIRST = 0,
    GFX_TF_NN = GFX_TF_FIRST,
    GFX_TF_BILINEAR,
    GFX_TF_LAST = GFX_TF_BILINEAR,
    GFX_TF_NUMBER_OF,
} GFX_TEXTURE_FILTER;

typedef enum {
    GFX_RM_LEGACY,
    GFX_RM_FRAMEBUFFER,
} GFX_RENDER_MODE;

typedef enum {
    GFX_GL_21,
    GFX_GL_33C,
} GFX_GL_BACKEND;
