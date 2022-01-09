#include "specific/s_output.h"

#include "3dsystem/3d_gen.h"
#include "config.h"
#include "game/output.h"
#include "game/screen.h"
#include "game/shell.h"
#include "game/viewport.h"
#include "gfx/2d/2d_surface.h"
#include "gfx/3d/3d_renderer.h"
#include "gfx/context.h"
#include "global/vars.h"
#include "global/vars_platform.h"
#include "log.h"

#include <assert.h>

#define CLIP_VERTCOUNT_SCALE 4

#define S_Output_CheckError(result)                                            \
    {                                                                          \
        if (!result) {                                                         \
            Shell_ExitSystem("Fatal 2D renderer error!");                      \
        }                                                                      \
    }

static int m_TextureMap[GFX_MAX_TEXTURES];
static RGB888 m_ATIPalette[256];

static GFX_3D_Renderer *m_Renderer3D;
static RGB888 m_GamePalette[256];
static bool m_IsPaletteActive = false;
static bool m_IsRendering = false;
static bool m_IsRenderingOld = false;
static bool m_IsTextureMode = false;
static int32_t m_SelectedTexture = -1;

static int32_t m_SurfaceWidth = 0;
static int32_t m_SurfaceHeight = 0;
static float m_SurfaceMinX = 0.0f;
static float m_SurfaceMinY = 0.0f;
static float m_SurfaceMaxX = 0.0f;
static float m_SurfaceMaxY = 0.0f;
static GFX_2D_Surface *m_PrimarySurface = NULL;
static GFX_2D_Surface *m_BackSurface = NULL;
static GFX_2D_Surface *m_PictureSurface = NULL;
static GFX_2D_Surface *m_TextureSurfaces[GFX_MAX_TEXTURES] = { NULL };

static void S_Output_SetHardwareVideoMode();
static void S_Output_SetupRenderContextAndRender();
static void S_Output_ReleaseTextures();
static void S_Output_ReleaseSurfaces();
static void S_Output_BlitSurface(
    GFX_2D_Surface *source, GFX_2D_Surface *target);
static void S_Output_FlipPrimaryBuffer();
static void S_Output_ClearSurface(GFX_2D_Surface *surface);
static void S_Output_DrawTriangleStrip(GFX_3D_Vertex *vertices, int num);
static int32_t S_Output_ClipVertices(int32_t num, GFX_3D_Vertex *source);
static int32_t S_Output_ClipVertices2(int32_t num, GFX_3D_Vertex *source);
static int32_t S_Output_ZedClipper(
    int32_t vertex_count, POINT_INFO *pts, GFX_3D_Vertex *vertices);

static void S_Output_ReleaseTextures()
{
    for (int i = 0; i < GFX_MAX_TEXTURES; i++) {
        if (m_TextureMap[i] != GFX_NO_TEXTURE) {
            GFX_3D_Renderer_TextureUnreg(m_Renderer3D, m_TextureMap[i]);
            m_TextureMap[i] = GFX_NO_TEXTURE;
        }
    }
}

static void S_Output_SetHardwareVideoMode()
{
    S_Output_ReleaseSurfaces();

    m_SurfaceWidth = Screen_GetResWidth();
    m_SurfaceHeight = Screen_GetResHeight();
    m_SurfaceMinX = 0.0f;
    m_SurfaceMinY = 0.0f;
    m_SurfaceMaxX = Screen_GetResWidth() - 1.0f;
    m_SurfaceMaxY = Screen_GetResHeight() - 1.0f;

    GFX_Context_SetDisplaySize(m_SurfaceWidth, m_SurfaceHeight);

    {
        GFX_2D_SurfaceDesc surface_desc = {
            .flags = {
                .primary = 1,
                .flip = 1,
            },
            .has_back_buffer = 1,
        };
        m_PrimarySurface = GFX_2D_Surface_Create(&surface_desc);
        m_BackSurface = GFX_2D_Surface_GetAttachedSurface(m_PrimarySurface);

        S_Output_ClearSurface(m_PrimarySurface);
    }

    for (int i = 0; i < GFX_MAX_TEXTURES; i++) {
        GFX_2D_SurfaceDesc surface_desc = {
            .width = 256,
            .height = 256,
        };
        m_TextureSurfaces[i] = GFX_2D_Surface_Create(&surface_desc);
    }

    S_Output_SetupRenderContextAndRender();
}

static void S_Output_SetupRenderContextAndRender()
{
    S_Output_RenderBegin();
    GFX_3D_Renderer_SetSmoothingEnabled(
        m_Renderer3D, g_Config.rendering.enable_bilinear_filter);
    S_Output_RenderToggle();
}

static void S_Output_ReleaseSurfaces()
{
    int i;

    if (m_PrimarySurface) {
        S_Output_ClearSurface(m_PrimarySurface);
        S_Output_ClearSurface(m_BackSurface);

        GFX_2D_Surface_Free(m_PrimarySurface);
        m_PrimarySurface = NULL;
        m_BackSurface = NULL;
    }

    for (i = 0; i < GFX_MAX_TEXTURES; i++) {
        if (m_TextureSurfaces[i]) {
            GFX_2D_Surface_Free(m_TextureSurfaces[i]);
            m_TextureSurfaces[i] = NULL;
        }
    }

    if (m_PictureSurface) {
        GFX_2D_Surface_Free(m_PictureSurface);
        m_PictureSurface = NULL;
    }
}

static void S_Output_BlitSurface(GFX_2D_Surface *source, GFX_2D_Surface *target)
{
    GFX_BlitterRect rect = {
        .left = 0,
        .top = 0,
        .right = m_SurfaceWidth,
        .bottom = m_SurfaceHeight,
    };
    bool result = GFX_2D_Surface_Blt(target, &rect, source, &rect);
    S_Output_CheckError(result);
}

static void S_Output_FlipPrimaryBuffer()
{
    S_Output_RenderEnd();
    bool result = GFX_2D_Surface_Flip(m_PrimarySurface);
    S_Output_CheckError(result);
    S_Output_RenderToggle();

    S_Output_SetupRenderContextAndRender();
}

static void S_Output_ClearSurface(GFX_2D_Surface *surface)
{
    bool result = GFX_2D_Surface_Clear(surface);
    S_Output_CheckError(result);
}

static void S_Output_DrawTriangleStrip(GFX_3D_Vertex *vertices, int num)
{
    GFX_3D_Renderer_RenderPrimStrip(m_Renderer3D, vertices, 3);
    int left = num - 2;
    for (int i = num - 3; i > 0; i--) {
        memcpy(&vertices[1], &vertices[2], left * sizeof(GFX_3D_Vertex));
        GFX_3D_Renderer_RenderPrimStrip(m_Renderer3D, vertices, 3);
        left--;
    }
}

static int32_t S_Output_ClipVertices(int32_t num, GFX_3D_Vertex *source)
{
    float scale;
    GFX_3D_Vertex vertices[num * CLIP_VERTCOUNT_SCALE];

    GFX_3D_Vertex *l = &source[num - 1];
    int j = 0;
    int i;

    for (i = 0; i < num; i++) {
        assert(j < num * CLIP_VERTCOUNT_SCALE);
        GFX_3D_Vertex *v1 = &vertices[j];
        GFX_3D_Vertex *v2 = l;
        l = &source[i];

        if (v2->x < m_SurfaceMinX) {
            if (l->x < m_SurfaceMinX) {
                continue;
            }
            scale = (m_SurfaceMinX - l->x) / (v2->x - l->x);
            v1->x = m_SurfaceMinX;
            v1->y = (v2->y - l->y) * scale + l->y;
            v1->z = (v2->z - l->z) * scale + l->z;
            v1->r = (v2->r - l->r) * scale + l->r;
            v1->g = (v2->g - l->g) * scale + l->g;
            v1->b = (v2->b - l->b) * scale + l->b;
            v1->a = (v2->a - l->a) * scale + l->a;
            v1 = &vertices[++j];
        } else if (v2->x > m_SurfaceMaxX) {
            if (l->x > m_SurfaceMaxX) {
                continue;
            }
            scale = (m_SurfaceMaxX - l->x) / (v2->x - l->x);
            v1->x = m_SurfaceMaxX;
            v1->y = (v2->y - l->y) * scale + l->y;
            v1->z = (v2->z - l->z) * scale + l->z;
            v1->r = (v2->r - l->r) * scale + l->r;
            v1->g = (v2->g - l->g) * scale + l->g;
            v1->b = (v2->b - l->b) * scale + l->b;
            v1->a = (v2->a - l->a) * scale + l->a;
            v1 = &vertices[++j];
        }

        if (l->x < m_SurfaceMinX) {
            scale = (m_SurfaceMinX - l->x) / (v2->x - l->x);
            v1->x = m_SurfaceMinX;
            v1->y = (v2->y - l->y) * scale + l->y;
            v1->z = (v2->z - l->z) * scale + l->z;
            v1->r = (v2->r - l->r) * scale + l->r;
            v1->g = (v2->g - l->g) * scale + l->g;
            v1->b = (v2->b - l->b) * scale + l->b;
            v1->a = (v2->a - l->a) * scale + l->a;
            v1 = &vertices[++j];
        } else if (l->x > m_SurfaceMaxX) {
            scale = (m_SurfaceMaxX - l->x) / (v2->x - l->x);
            v1->x = m_SurfaceMaxX;
            v1->y = (v2->y - l->y) * scale + l->y;
            v1->z = (v2->z - l->z) * scale + l->z;
            v1->r = (v2->r - l->r) * scale + l->r;
            v1->g = (v2->g - l->g) * scale + l->g;
            v1->b = (v2->b - l->b) * scale + l->b;
            v1->a = (v2->a - l->a) * scale + l->a;
            v1 = &vertices[++j];
        } else {
            v1->x = l->x;
            v1->y = l->y;
            v1->z = l->z;
            v1->r = l->r;
            v1->g = l->g;
            v1->b = l->b;
            v1->a = l->a;
            v1 = &vertices[++j];
        }
    }

    if (j < 3) {
        return 0;
    }

    num = j;
    l = &vertices[j - 1];
    j = 0;

    for (i = 0; i < num; i++) {
        GFX_3D_Vertex *v1 = &source[j];
        GFX_3D_Vertex *v2 = l;
        l = &vertices[i];

        if (v2->y < m_SurfaceMinY) {
            if (l->y < m_SurfaceMinY) {
                continue;
            }
            scale = (m_SurfaceMinY - l->y) / (v2->y - l->y);
            v1->x = (v2->x - l->x) * scale + l->x;
            v1->y = m_SurfaceMinY;
            v1->z = (v2->z - l->z) * scale + l->z;
            v1->r = (v2->r - l->r) * scale + l->r;
            v1->g = (v2->g - l->g) * scale + l->g;
            v1->b = (v2->b - l->b) * scale + l->b;
            v1->a = (v2->a - l->a) * scale + l->a;
            v1 = &source[++j];
        } else if (v2->y > m_SurfaceMaxY) {
            if (l->y > m_SurfaceMaxY) {
                continue;
            }
            scale = (m_SurfaceMaxY - l->y) / (v2->y - l->y);
            v1->x = (v2->x - l->x) * scale + l->x;
            v1->y = m_SurfaceMaxY;
            v1->z = (v2->z - l->z) * scale + l->z;
            v1->r = (v2->r - l->r) * scale + l->r;
            v1->g = (v2->g - l->g) * scale + l->g;
            v1->b = (v2->b - l->b) * scale + l->b;
            v1->a = (v2->a - l->a) * scale + l->a;
            v1 = &source[++j];
        }

        if (l->y < m_SurfaceMinY) {
            scale = (m_SurfaceMinY - l->y) / (v2->y - l->y);
            v1->x = (v2->x - l->x) * scale + l->x;
            v1->y = m_SurfaceMinY;
            v1->z = (v2->z - l->z) * scale + l->z;
            v1->r = (v2->r - l->r) * scale + l->r;
            v1->g = (v2->g - l->g) * scale + l->g;
            v1->b = (v2->b - l->b) * scale + l->b;
            v1->a = (v2->a - l->a) * scale + l->a;
            v1 = &source[++j];
        } else if (l->y > m_SurfaceMaxY) {
            scale = (m_SurfaceMaxY - l->y) / (v2->y - l->y);
            v1->x = (v2->x - l->x) * scale + l->x;
            v1->y = m_SurfaceMaxY;
            v1->z = (v2->z - l->z) * scale + l->z;
            v1->r = (v2->r - l->r) * scale + l->r;
            v1->g = (v2->g - l->g) * scale + l->g;
            v1->b = (v2->b - l->b) * scale + l->b;
            v1->a = (v2->a - l->a) * scale + l->a;
            v1 = &source[++j];
        } else {
            v1->x = l->x;
            v1->y = l->y;
            v1->z = l->z;
            v1->r = l->r;
            v1->g = l->g;
            v1->b = l->b;
            v1->a = l->a;
            v1 = &source[++j];
        }
    }

    if (j < 3) {
        return 0;
    }

    return j;
}

static int32_t S_Output_ClipVertices2(int32_t num, GFX_3D_Vertex *source)
{
    float scale;
    GFX_3D_Vertex vertices[8];

    GFX_3D_Vertex *l = &source[num - 1];
    int j = 0;

    for (int i = 0; i < num; i++) {
        GFX_3D_Vertex *v1 = &vertices[j];
        GFX_3D_Vertex *v2 = l;
        l = &source[i];

        if (v2->x < m_SurfaceMinX) {
            if (l->x < m_SurfaceMinX) {
                continue;
            }
            scale = (m_SurfaceMinX - l->x) / (v2->x - l->x);
            v1->x = m_SurfaceMinX;
            v1->y = (v2->y - l->y) * scale + l->y;
            v1->z = (v2->z - l->z) * scale + l->z;
            v1->r = (v2->r - l->r) * scale + l->r;
            v1->g = (v2->g - l->g) * scale + l->g;
            v1->b = (v2->b - l->b) * scale + l->b;
            v1->w = (v2->w - l->w) * scale + l->w;
            v1->s = (v2->s - l->s) * scale + l->s;
            v1->t = (v2->t - l->t) * scale + l->t;
            v1 = &vertices[++j];
        } else if (v2->x > m_SurfaceMaxX) {
            if (l->x > m_SurfaceMaxX) {
                continue;
            }
            scale = (m_SurfaceMaxX - l->x) / (v2->x - l->x);
            v1->x = m_SurfaceMaxX;
            v1->y = (v2->y - l->y) * scale + l->y;
            v1->z = (v2->z - l->z) * scale + l->z;
            v1->r = (v2->r - l->r) * scale + l->r;
            v1->g = (v2->g - l->g) * scale + l->g;
            v1->b = (v2->b - l->b) * scale + l->b;
            v1->w = (v2->w - l->w) * scale + l->w;
            v1->s = (v2->s - l->s) * scale + l->s;
            v1->t = (v2->t - l->t) * scale + l->t;
            v1 = &vertices[++j];
        }

        if (l->x < m_SurfaceMinX) {
            scale = (m_SurfaceMinX - l->x) / (v2->x - l->x);
            v1->x = m_SurfaceMinX;
            v1->y = (v2->y - l->y) * scale + l->y;
            v1->z = (v2->z - l->z) * scale + l->z;
            v1->r = (v2->r - l->r) * scale + l->r;
            v1->g = (v2->g - l->g) * scale + l->g;
            v1->b = (v2->b - l->b) * scale + l->b;
            v1->w = (v2->w - l->w) * scale + l->w;
            v1->s = (v2->s - l->s) * scale + l->s;
            v1->t = (v2->t - l->t) * scale + l->t;
            v1 = &vertices[++j];
        } else if (l->x > m_SurfaceMaxX) {
            scale = (m_SurfaceMaxX - l->x) / (v2->x - l->x);
            v1->x = m_SurfaceMaxX;
            v1->y = (v2->y - l->y) * scale + l->y;
            v1->z = (v2->z - l->z) * scale + l->z;
            v1->r = (v2->r - l->r) * scale + l->r;
            v1->g = (v2->g - l->g) * scale + l->g;
            v1->b = (v2->b - l->b) * scale + l->b;
            v1->w = (v2->w - l->w) * scale + l->w;
            v1->s = (v2->s - l->s) * scale + l->s;
            v1->t = (v2->t - l->t) * scale + l->t;
            v1 = &vertices[++j];
        } else {
            v1->x = l->x;
            v1->y = l->y;
            v1->z = l->z;
            v1->r = l->r;
            v1->g = l->g;
            v1->b = l->b;
            v1->w = l->w;
            v1->s = l->s;
            v1->t = l->t;
            v1 = &vertices[++j];
        }
    }

    if (j < 3) {
        return 0;
    }

    num = j;
    l = &vertices[j - 1];
    j = 0;

    for (int i = 0; i < num; i++) {
        GFX_3D_Vertex *v1 = &source[j];
        GFX_3D_Vertex *v2 = l;
        l = &vertices[i];

        if (v2->y < m_SurfaceMinY) {
            if (l->y < m_SurfaceMinY) {
                continue;
            }
            scale = (m_SurfaceMinY - l->y) / (v2->y - l->y);
            v1->x = (v2->x - l->x) * scale + l->x;
            v1->y = m_SurfaceMinY;
            v1->z = (v2->z - l->z) * scale + l->z;
            v1->r = (v2->r - l->r) * scale + l->r;
            v1->g = (v2->g - l->g) * scale + l->g;
            v1->b = (v2->b - l->b) * scale + l->b;
            v1->w = (v2->w - l->w) * scale + l->w;
            v1->s = (v2->s - l->s) * scale + l->s;
            v1->t = (v2->t - l->t) * scale + l->t;
            v1 = &source[++j];
        } else if (v2->y > m_SurfaceMaxY) {
            if (l->y > m_SurfaceMaxY) {
                continue;
            }
            scale = (m_SurfaceMaxY - l->y) / (v2->y - l->y);
            v1->x = (v2->x - l->x) * scale + l->x;
            v1->y = m_SurfaceMaxY;
            v1->z = (v2->z - l->z) * scale + l->z;
            v1->r = (v2->r - l->r) * scale + l->r;
            v1->g = (v2->g - l->g) * scale + l->g;
            v1->b = (v2->b - l->b) * scale + l->b;
            v1->w = (v2->w - l->w) * scale + l->w;
            v1->s = (v2->s - l->s) * scale + l->s;
            v1->t = (v2->t - l->t) * scale + l->t;
            v1 = &source[++j];
        }

        if (l->y < m_SurfaceMinY) {
            scale = (m_SurfaceMinY - l->y) / (v2->y - l->y);
            v1->x = (v2->x - l->x) * scale + l->x;
            v1->y = m_SurfaceMinY;
            v1->z = (v2->z - l->z) * scale + l->z;
            v1->r = (v2->r - l->r) * scale + l->r;
            v1->g = (v2->g - l->g) * scale + l->g;
            v1->b = (v2->b - l->b) * scale + l->b;
            v1->w = (v2->w - l->w) * scale + l->w;
            v1->s = (v2->s - l->s) * scale + l->s;
            v1->t = (v2->t - l->t) * scale + l->t;
            v1 = &source[++j];
        } else if (l->y > m_SurfaceMaxY) {
            scale = (m_SurfaceMaxY - l->y) / (v2->y - l->y);
            v1->x = (v2->x - l->x) * scale + l->x;
            v1->y = m_SurfaceMaxY;
            v1->z = (v2->z - l->z) * scale + l->z;
            v1->r = (v2->r - l->r) * scale + l->r;
            v1->g = (v2->g - l->g) * scale + l->g;
            v1->b = (v2->b - l->b) * scale + l->b;
            v1->w = (v2->w - l->w) * scale + l->w;
            v1->s = (v2->s - l->s) * scale + l->s;
            v1->t = (v2->t - l->t) * scale + l->t;
            v1 = &source[++j];
        } else {
            v1->x = l->x;
            v1->y = l->y;
            v1->z = l->z;
            v1->r = l->r;
            v1->g = l->g;
            v1->b = l->b;
            v1->w = l->w;
            v1->s = l->s;
            v1->t = l->t;
            v1 = &source[++j];
        }
    }

    if (j < 3) {
        return 0;
    }

    return j;
}

static int32_t S_Output_ZedClipper(
    int32_t vertex_count, POINT_INFO *pts, GFX_3D_Vertex *vertices)
{
    int32_t i;
    int32_t count;
    POINT_INFO *pts0;
    POINT_INFO *pts1;
    GFX_3D_Vertex *v;
    float clip;
    float persp_o_near_z;
    float multiplier;

    multiplier = 0.0625f * g_Config.brightness;
    float near_z = Output_GetNearZ();
    persp_o_near_z = g_PhdPersp / near_z;

    v = &vertices[0];
    pts0 = &pts[vertex_count - 1];
    for (i = 0; i < vertex_count; i++) {
        pts1 = pts0;
        pts0 = &pts[i];
        if (near_z > pts1->zv) {
            if (near_z > pts0->zv) {
                continue;
            }

            clip = (near_z - pts0->zv) / (pts1->zv - pts0->zv);
            v->x = ((pts1->xv - pts0->xv) * clip + pts0->xv) * persp_o_near_z
                + ViewPort_GetCenterX();
            v->y = ((pts1->yv - pts0->yv) * clip + pts0->yv) * persp_o_near_z
                + ViewPort_GetCenterY();
            v->z = near_z * 0.0001f;

            v->w = 65536.0f / near_z;
            v->s = v->w * ((pts1->u - pts0->u) * clip + pts0->u) * 0.00390625f;
            v->t = v->w * ((pts1->v - pts0->v) * clip + pts0->v) * 0.00390625f;

            v->r = v->g = v->b =
                (8192.0f - ((pts1->g - pts0->g) * clip + pts0->g)) * multiplier;
            Output_ApplyWaterEffect(&v->r, &v->g, &v->b);

            v++;
        }

        if (near_z > pts0->zv) {
            clip = (near_z - pts0->zv) / (pts1->zv - pts0->zv);
            v->x = ((pts1->xv - pts0->xv) * clip + pts0->xv) * persp_o_near_z
                + ViewPort_GetCenterX();
            v->y = ((pts1->yv - pts0->yv) * clip + pts0->yv) * persp_o_near_z
                + ViewPort_GetCenterY();
            v->z = near_z * 0.0001f;

            v->w = 65536.0f / near_z;
            v->s = v->w * ((pts1->u - pts0->u) * clip + pts0->u) * 0.00390625f;
            v->t = v->w * ((pts1->v - pts0->v) * clip + pts0->v) * 0.00390625f;

            v->r = v->g = v->b =
                (8192.0f - ((pts1->g - pts0->g) * clip + pts0->g)) * multiplier;
            Output_ApplyWaterEffect(&v->r, &v->g, &v->b);

            v++;
        } else {
            v->x = pts0->xs;
            v->y = pts0->ys;
            v->z = pts0->zv * 0.0001f;

            v->w = 65536.0f / pts0->zv;
            v->s = pts0->u * v->w * 0.00390625f;
            v->t = pts0->v * v->w * 0.00390625f;

            v->r = v->g = v->b = (8192.0f - pts0->g) * multiplier;
            Output_ApplyWaterEffect(&v->r, &v->g, &v->b);

            v++;
        }
    }

    count = v - vertices;
    return count < 3 ? 0 : count;
}

void S_Output_EnableTextureMode(void)
{
    if (m_IsTextureMode) {
        return;
    }

    m_IsTextureMode = true;
    GFX_3D_Renderer_SetTexturingEnabled(m_Renderer3D, m_IsTextureMode);
}

void S_Output_DisableTextureMode(void)
{
    if (!m_IsTextureMode) {
        return;
    }

    m_IsTextureMode = false;
    GFX_3D_Renderer_SetTexturingEnabled(m_Renderer3D, m_IsTextureMode);
}

void S_Output_RenderBegin()
{
    m_IsRenderingOld = m_IsRendering;
    if (!m_IsRendering) {
        GFX_3D_Renderer_RenderBegin(m_Renderer3D);
        m_IsRendering = true;
    }
}

void S_Output_RenderEnd()
{
    m_IsRenderingOld = m_IsRendering;
    if (m_IsRendering) {
        GFX_3D_Renderer_RenderEnd(m_Renderer3D);
        m_IsRendering = false;
    }
}

void S_Output_RenderToggle()
{
    if (m_IsRenderingOld) {
        S_Output_RenderBegin();
    } else {
        S_Output_RenderEnd();
    }
}

void S_Output_SetPalette(RGB888 palette[256])
{
    for (int i = 0; i < 256; i++) {
        m_GamePalette[i] = palette[i];
        m_ATIPalette[i].r = 4 * palette[i].r;
        m_ATIPalette[i].g = 4 * palette[i].g;
        m_ATIPalette[i].b = 4 * palette[i].b;
    }

    m_IsPaletteActive = true;
}

RGB888 S_Output_GetPaletteColor(uint8_t idx)
{
    return m_ATIPalette[idx];
}

void S_Output_DumpScreen()
{
    S_Output_FlipPrimaryBuffer();
    m_SelectedTexture = -1;
}

void S_Output_ClearBackBuffer()
{
    S_Output_RenderEnd();
    S_Output_ClearSurface(m_BackSurface);
    S_Output_RenderToggle();
}

void S_Output_CopyToPicture()
{
    if (!m_PictureSurface) {
        GFX_2D_SurfaceDesc surface_desc = {
            .width = m_SurfaceWidth,
            .height = m_SurfaceHeight,
        };
        m_PictureSurface = GFX_2D_Surface_Create(&surface_desc);
    }

    S_Output_RenderEnd();
    S_Output_BlitSurface(m_BackSurface, m_PictureSurface);
    S_Output_RenderToggle();
}

void S_Output_CopyFromPicture()
{
    S_Output_ClearBackBuffer();
    S_Output_RenderEnd();
    S_Output_BlitSurface(m_PictureSurface, m_BackSurface);
    S_Output_RenderToggle();
}

void S_Output_DownloadPicture(const PICTURE *pic)
{
    GFX_2D_Surface *picture_surface = NULL;

    // first, download the picture directly to a temporary surface
    {
        GFX_2D_SurfaceDesc surface_desc = {
            .width = pic->width,
            .height = pic->height,
        };
        picture_surface = GFX_2D_Surface_Create(&surface_desc);
    }

    {
        GFX_2D_SurfaceDesc surface_desc = { 0 };
        bool result = GFX_2D_Surface_Lock(picture_surface, &surface_desc);
        S_Output_CheckError(result);

        uint32_t *output_ptr = surface_desc.pixels;
        RGB888 *input_ptr = pic->data;
        for (int i = 0; i < pic->width * pic->height; i++) {
            uint8_t r = input_ptr->r;
            uint8_t g = input_ptr->g;
            uint8_t b = input_ptr->b;
            input_ptr++;
            *output_ptr++ = b | (g << 8) | (r << 16);
        }

        result = GFX_2D_Surface_Unlock(picture_surface, surface_desc.pixels);
        S_Output_CheckError(result);
    }

    if (!m_PictureSurface) {
        GFX_2D_SurfaceDesc surface_desc = {
            .width = m_SurfaceWidth,
            .height = m_SurfaceHeight,
        };
        m_PictureSurface = GFX_2D_Surface_Create(&surface_desc);
    }

    int32_t target_width = m_SurfaceWidth;
    int32_t target_height = m_SurfaceHeight;
    int32_t source_width = pic->width;
    int32_t source_height = pic->height;

    // keep aspect ratio and fit inside, adding black bars on the sides
    const float source_ratio = source_width / (float)source_height;
    const float target_ratio = target_width / (float)target_height;
    int32_t new_width = source_ratio < target_ratio
        ? target_height * source_ratio
        : target_width;
    int32_t new_height = source_ratio < target_ratio
        ? target_height
        : target_width / source_ratio;

    GFX_BlitterRect source_rect = {
        .left = 0,
        .top = 0,
        .right = pic->width,
        .bottom = pic->height,
    };
    GFX_BlitterRect target_rect = {
        .left = (target_width - new_width) / 2,
        .top = (target_height - new_height) / 2,
        .right = target_rect.left + new_width,
        .bottom = target_rect.top + new_height,
    };

    bool result = GFX_2D_Surface_Clear(m_PictureSurface);
    S_Output_CheckError(result);

    result = GFX_2D_Surface_Blt(
        m_PictureSurface, &target_rect, picture_surface, &source_rect);
    S_Output_CheckError(result);

    GFX_2D_Surface_Free(picture_surface);
}

void S_Output_SelectTexture(int tex_num)
{
    if (tex_num == m_SelectedTexture) {
        return;
    }

    if (m_TextureMap[tex_num] == GFX_NO_TEXTURE) {
        LOG_ERROR("ERROR: Attempt to select unloaded texture");
        return;
    }

    GFX_3D_Renderer_SelectTexture(m_Renderer3D, m_TextureMap[tex_num]);
    m_SelectedTexture = tex_num;
}

void S_Output_DrawSprite(
    int16_t x1, int16_t y1, int16_t x2, int y2, int z, int sprnum, int shade)
{
    float t1;
    float t2;
    float t3;
    float t4;
    float t5;
    float vz;
    float vshade;
    int32_t vertex_count;
    PHD_SPRITE *sprite;
    GFX_3D_Vertex vertices[10];
    float multiplier;

    multiplier = 0.0625f * g_Config.brightness;

    sprite = &g_PhdSpriteInfo[sprnum];
    vshade = (8192.0f - shade) * multiplier;
    if (vshade >= 256.0f) {
        vshade = 255.0f;
    }

    t1 = ((int)sprite->offset & 0xFF) + 0.5f;
    t2 = ((int)sprite->offset >> 8) + 0.5f;
    t3 = ((int)sprite->width >> 8) + t1;
    t4 = ((int)sprite->height >> 8) + t2;
    vz = z * 0.0001f;
    t5 = 65536.0f / z;

    vertices[0].x = x1;
    vertices[0].y = y1;
    vertices[0].z = vz;
    vertices[0].s = t1 * t5 * 0.00390625f;
    vertices[0].t = t2 * t5 * 0.00390625f;
    vertices[0].w = t5;
    vertices[0].r = vshade;
    vertices[0].g = vshade;
    vertices[0].b = vshade;

    vertices[1].x = x2;
    vertices[1].y = y1;
    vertices[1].z = vz;
    vertices[1].s = t3 * t5 * 0.00390625f;
    vertices[1].t = t2 * t5 * 0.00390625f;
    vertices[1].w = t5;
    vertices[1].r = vshade;
    vertices[1].g = vshade;
    vertices[1].b = vshade;

    vertices[2].x = x2;
    vertices[2].y = y2;
    vertices[2].z = vz;
    vertices[2].s = t3 * t5 * 0.00390625f;
    vertices[2].t = t4 * t5 * 0.00390625f;
    vertices[2].w = t5;
    vertices[2].r = vshade;
    vertices[2].g = vshade;
    vertices[2].b = vshade;

    vertices[3].x = x1;
    vertices[3].y = y2;
    vertices[3].z = vz;
    vertices[3].s = t1 * t5 * 0.00390625f;
    vertices[3].t = t4 * t5 * 0.00390625f;
    vertices[3].w = t5;
    vertices[3].r = vshade;
    vertices[3].g = vshade;
    vertices[3].b = vshade;

    vertex_count = 4;
    if (x1 < 0 || y1 < 0 || x2 > ViewPort_GetWidth()
        || y2 > ViewPort_GetHeight()) {
        vertex_count = S_Output_ClipVertices2(vertex_count, vertices);
    }

    if (!vertex_count) {
        return;
    }

    if (m_TextureMap[sprite->tpage] != GFX_NO_TEXTURE) {
        S_Output_EnableTextureMode();
        S_Output_SelectTexture(sprite->tpage);
        S_Output_DrawTriangleStrip(vertices, vertex_count);
    } else {
        S_Output_DisableTextureMode();
        S_Output_DrawTriangleStrip(vertices, vertex_count);
    }
}

void S_Output_Draw2DLine(
    int32_t x1, int32_t y1, int32_t x2, int32_t y2, RGB888 color1,
    RGB888 color2)
{
    GFX_3D_Vertex vertices[2];

    vertices[0].x = x1;
    vertices[0].y = y1;
    vertices[0].z = 0.0f;
    vertices[0].r = color1.r;
    vertices[0].g = color1.g;
    vertices[0].b = color1.b;

    vertices[1].x = x2;
    vertices[1].y = y2;
    vertices[1].z = 0.0f;
    vertices[1].r = color2.r;
    vertices[1].g = color2.g;
    vertices[1].b = color2.b;

    GFX_3D_Renderer_SetPrimType(m_Renderer3D, GFX_3D_PRIM_LINE);
    S_Output_DisableTextureMode();
    GFX_3D_Renderer_RenderPrimList(m_Renderer3D, vertices, 2);
    GFX_3D_Renderer_SetPrimType(m_Renderer3D, GFX_3D_PRIM_TRI);
}

void S_Output_Draw2DQuad(
    int32_t x1, int32_t y1, int32_t x2, int32_t y2, RGB888 tl, RGB888 tr,
    RGB888 bl, RGB888 br)
{
    GFX_3D_Vertex vertices[4];

    vertices[0].x = x1;
    vertices[0].y = y1;
    vertices[0].z = 1.0f;
    vertices[0].r = tl.r;
    vertices[0].g = tl.g;
    vertices[0].b = tl.b;

    vertices[1].x = x2;
    vertices[1].y = y1;
    vertices[1].z = 1.0f;
    vertices[1].r = tr.r;
    vertices[1].g = tr.g;
    vertices[1].b = tr.b;

    vertices[2].x = x2;
    vertices[2].y = y2;
    vertices[2].z = 1.0f;
    vertices[2].r = br.r;
    vertices[2].g = br.g;
    vertices[2].b = br.b;

    vertices[3].x = x1;
    vertices[3].y = y2;
    vertices[3].z = 1.0f;
    vertices[3].r = bl.r;
    vertices[3].g = bl.g;
    vertices[3].b = bl.b;

    S_Output_DisableTextureMode();

    S_Output_DrawTriangleStrip(vertices, 4);
}

void S_Output_DrawTranslucentQuad(
    int32_t x1, int32_t y1, int32_t x2, int32_t y2)
{
    GFX_3D_Vertex vertices[4];

    vertices[0].x = x1;
    vertices[0].y = y1;
    vertices[0].z = 1.0f;
    vertices[0].b = 0.0f;
    vertices[0].g = 0.0f;
    vertices[0].r = 0.0f;
    vertices[0].a = 128.0f;

    vertices[1].x = x2;
    vertices[1].y = y1;
    vertices[1].z = 1.0f;
    vertices[1].b = 0.0f;
    vertices[1].g = 0.0f;
    vertices[1].r = 0.0f;
    vertices[1].a = 128.0f;

    vertices[2].x = x2;
    vertices[2].y = y2;
    vertices[2].z = 1.0f;
    vertices[2].b = 0.0f;
    vertices[2].g = 0.0f;
    vertices[2].r = 0.0f;
    vertices[2].a = 128.0f;

    vertices[3].x = x1;
    vertices[3].y = y2;
    vertices[3].z = 1.0f;
    vertices[3].b = 0.0f;
    vertices[3].g = 0.0f;
    vertices[3].r = 0.0f;
    vertices[3].a = 128.0f;

    S_Output_DisableTextureMode();

    GFX_3D_Renderer_SetBlendingEnabled(m_Renderer3D, true);
    S_Output_DrawTriangleStrip(vertices, 4);
    GFX_3D_Renderer_SetBlendingEnabled(m_Renderer3D, false);
}

void S_Output_DrawLightningSegment(
    int x1, int y1, int z1, int thickness1, int x2, int y2, int z2,
    int thickness2)
{
    GFX_3D_Vertex vertices[4 * CLIP_VERTCOUNT_SCALE];

    S_Output_DisableTextureMode();

    GFX_3D_Renderer_SetBlendingEnabled(m_Renderer3D, true);
    vertices[0].x = x1;
    vertices[0].y = y1;
    vertices[0].z = z1 * 0.0001f;
    vertices[0].g = 0.0f;
    vertices[0].r = 0.0f;
    vertices[0].b = 255.0f;
    vertices[0].a = 128.0f;

    vertices[1].x = thickness1 / 2 + x1;
    vertices[1].y = vertices[0].y;
    vertices[1].z = vertices[0].z;
    vertices[1].b = 255.0f;
    vertices[1].g = 255.0f;
    vertices[1].r = 255.0f;
    vertices[1].a = 128.0f;

    vertices[2].x = thickness2 / 2 + x2;
    vertices[2].y = y2;
    vertices[2].z = z2 * 0.0001f;
    vertices[2].b = 255.0f;
    vertices[2].g = 255.0f;
    vertices[2].r = 255.0f;
    vertices[2].a = 128.0f;

    vertices[3].x = x2;
    vertices[3].y = vertices[2].y;
    vertices[3].z = vertices[2].z;
    vertices[3].g = 0.0f;
    vertices[3].r = 0.0f;
    vertices[3].b = 255.0f;
    vertices[3].a = 128.0f;

    int num = S_Output_ClipVertices(4, vertices);
    if (num) {
        S_Output_DrawTriangleStrip(vertices, num);
    }

    vertices[0].x = thickness1 / 2 + x1;
    vertices[0].y = y1;
    vertices[0].z = z1 * 0.0001f;
    vertices[0].b = 255.0f;
    vertices[0].g = 255.0f;
    vertices[0].r = 255.0f;
    vertices[0].a = 128.0f;

    vertices[1].x = thickness1 + x1;
    vertices[1].y = vertices[0].y;
    vertices[1].z = vertices[0].z;
    vertices[1].g = 0.0f;
    vertices[1].r = 0.0f;
    vertices[1].b = 255.0f;
    vertices[1].a = 128.0f;

    vertices[2].x = (thickness2 + x2);
    vertices[2].y = y2;
    vertices[2].z = z2 * 0.0001f;
    vertices[2].g = 0.0f;
    vertices[2].r = 0.0f;
    vertices[2].b = 255.0f;
    vertices[2].a = 128.0f;

    vertices[3].x = (thickness2 / 2 + x2);
    vertices[3].y = vertices[2].y;
    vertices[3].z = vertices[2].z;
    vertices[3].b = 255.0f;
    vertices[3].g = 255.0f;
    vertices[3].r = 255.0f;
    vertices[3].a = 128.0f;

    num = S_Output_ClipVertices(4, vertices);
    if (num) {
        S_Output_DrawTriangleStrip(vertices, num);
    }
    GFX_3D_Renderer_SetBlendingEnabled(m_Renderer3D, false);
}

void S_Output_DrawShadow(PHD_VBUF *vbufs, int clip, int vertex_count)
{
    // needs to be more than 8 cause clipping might return more polygons.
    GFX_3D_Vertex vertices[vertex_count * CLIP_VERTCOUNT_SCALE];
    int i;
    int32_t tmp;

    for (i = 0; i < vertex_count; i++) {
        GFX_3D_Vertex *vertex = &vertices[i];
        PHD_VBUF *vbuf = &vbufs[i];
        vertex->x = vbuf->xs;
        vertex->y = vbuf->ys;
        vertex->z = vbuf->zv * 0.0001f - 16.0f;
        vertex->b = 0.0f;
        vertex->g = 0.0f;
        vertex->r = 0.0f;
        vertex->a = 128.0f;
    }

    if (clip) {
        int original = vertex_count;
        vertex_count = S_Output_ClipVertices(vertex_count, vertices);
        assert(vertex_count < original * CLIP_VERTCOUNT_SCALE);
    }

    if (!vertex_count) {
        return;
    }

    S_Output_DisableTextureMode();

    GFX_3D_Renderer_SetBlendingEnabled(m_Renderer3D, true);
    S_Output_DrawTriangleStrip(vertices, vertex_count);
    GFX_3D_Renderer_SetBlendingEnabled(m_Renderer3D, false);
}

void S_Output_FadeToBlack()
{
    S_Output_ClearBackBuffer();
    S_Output_DumpScreen();
}

void S_Output_ApplyResolution()
{
    S_Output_SetHardwareVideoMode();
}

void S_Output_SetViewport(int width, int height)
{
    GFX_Context_SetWindowSize(width, height);
}

void S_Output_SetFullscreen(bool fullscreen)
{
    GFX_Context_SetFullscreen(fullscreen);
}

bool S_Output_Init()
{
    GFX_Context_Attach(g_TombHWND);
    m_Renderer3D = GFX_Context_GetRenderer3D();

    for (int i = 0; i < GFX_MAX_TEXTURES; i++) {
        m_TextureMap[i] = GFX_NO_TEXTURE;
        m_TextureSurfaces[i] = NULL;
    }

    S_Output_SetHardwareVideoMode();

    GFX_3D_Renderer_SetPrimType(m_Renderer3D, GFX_3D_PRIM_TRI);

    return true;
}

void S_Output_Shutdown()
{
    S_Output_ReleaseTextures();
    S_Output_ReleaseSurfaces();
    GFX_Context_Detach();
    m_Renderer3D = NULL;
}

void S_Output_DrawFlatTriangle(
    PHD_VBUF *vn1, PHD_VBUF *vn2, PHD_VBUF *vn3, int32_t color)
{
    int32_t vertex_count;
    GFX_3D_Vertex vertices[8];
    float r;
    float g;
    float b;
    float light;
    float divisor;

    if (!((vn3->clip & vn2->clip & vn1->clip) == 0 && vn1->clip >= 0
          && vn2->clip >= 0 && vn3->clip >= 0
          && (vn1->ys - vn2->ys) * (vn3->xs - vn2->xs)
                  - (vn3->ys - vn2->ys) * (vn1->xs - vn2->xs)
              >= 0)) {
        return;
    }

    r = m_GamePalette[color].r;
    g = m_GamePalette[color].g;
    b = m_GamePalette[color].b;

    Output_ApplyWaterEffect(&r, &g, &b);

    divisor = (1.0f / g_Config.brightness) * 1024.0f;

    light = (8192.0f - vn1->g) / divisor;
    vertices[0].x = vn1->xs;
    vertices[0].y = vn1->ys;
    vertices[0].z = vn1->zv * 0.0001f;
    vertices[0].r = r * light;
    vertices[0].g = g * light;
    vertices[0].b = b * light;

    light = (8192.0f - vn2->g) / divisor;
    vertices[1].x = vn2->xs;
    vertices[1].y = vn2->ys;
    vertices[1].z = vn2->zv * 0.0001f;
    vertices[1].r = r * light;
    vertices[1].g = g * light;
    vertices[1].b = b * light;

    light = (8192.0f - vn3->g) / divisor;
    vertices[2].x = vn3->xs;
    vertices[2].y = vn3->ys;
    vertices[2].z = vn3->zv * 0.0001f;
    vertices[2].r = r * light;
    vertices[2].g = g * light;
    vertices[2].b = b * light;

    vertex_count = 3;
    if (vn1->clip || vn2->clip || vn3->clip) {
        vertex_count = S_Output_ClipVertices(vertex_count, vertices);
    }
    if (!vertex_count) {
        return;
    }

    S_Output_DrawTriangleStrip(vertices, vertex_count);
}

void S_Output_DrawTexturedTriangle(
    PHD_VBUF *vn1, PHD_VBUF *vn2, PHD_VBUF *vn3, int16_t tpage, PHD_UV *uv1,
    PHD_UV *uv2, PHD_UV *uv3, uint16_t textype)
{
    int32_t i;
    int32_t vertex_count;
    GFX_3D_Vertex vertices[8];
    POINT_INFO points[3];
    PHD_VBUF *src_vbuf[4];
    PHD_UV *src_uv[4];
    float multiplier;

    multiplier = 0.0625f * g_Config.brightness;

    src_vbuf[0] = vn1;
    src_vbuf[1] = vn2;
    src_vbuf[2] = vn3;

    src_uv[0] = uv1;
    src_uv[1] = uv2;
    src_uv[2] = uv3;

    if (vn3->clip & vn2->clip & vn1->clip) {
        return;
    }

    if (vn1->clip >= 0 && vn2->clip >= 0 && vn3->clip >= 0) {
        if ((vn1->ys - vn2->ys) * (vn3->xs - vn2->xs)
                - (vn3->ys - vn2->ys) * (vn1->xs - vn2->xs)
            < 0) {
            return;
        }

        for (i = 0; i < 3; i++) {
            vertices[i].x = src_vbuf[i]->xs;
            vertices[i].y = src_vbuf[i]->ys;
            vertices[i].z = src_vbuf[i]->zv * 0.0001f;

            vertices[i].w = 65536.0f / src_vbuf[i]->zv;
            vertices[i].s = ((src_uv[i]->u1 & 0xFF00) + 127) * 0.00390625f
                * vertices[i].w * 0.00390625f;
            vertices[i].t = ((src_uv[i]->v1 & 0xFF00) + 127) * 0.00390625f
                * vertices[i].w * 0.00390625f;

            vertices[i].r = vertices[i].g = vertices[i].b =
                (8192.0f - src_vbuf[i]->g) * multiplier;

            Output_ApplyWaterEffect(
                &vertices[i].r, &vertices[i].g, &vertices[i].b);
        }

        vertex_count = 3;
        if (vn1->clip || vn2->clip || vn3->clip) {
            vertex_count = S_Output_ClipVertices2(vertex_count, vertices);
        }
    } else {
        if (!phd_VisibleZClip(vn1, vn2, vn3)) {
            return;
        }

        for (i = 0; i < 3; i++) {
            points[i].xv = src_vbuf[i]->xv;
            points[i].yv = src_vbuf[i]->yv;
            points[i].zv = src_vbuf[i]->zv;
            points[i].xs = src_vbuf[i]->xs;
            points[i].ys = src_vbuf[i]->ys;
            points[i].g = src_vbuf[i]->g;
            points[i].u = ((src_uv[i]->u1 & 0xFF00) + 127) * 0.00390625f;
            points[i].v = ((src_uv[i]->v1 & 0xFF00) + 127) * 0.00390625f;
        }

        vertex_count = S_Output_ZedClipper(3, points, vertices);
        if (!vertex_count) {
            return;
        }
        vertex_count = S_Output_ClipVertices2(vertex_count, vertices);
    }

    if (!vertex_count) {
        return;
    }

    if (m_TextureMap[tpage] != GFX_NO_TEXTURE) {
        S_Output_EnableTextureMode();
        S_Output_SelectTexture(tpage);
        S_Output_DrawTriangleStrip(vertices, vertex_count);
    } else {
        S_Output_DisableTextureMode();
        S_Output_DrawTriangleStrip(vertices, vertex_count);
    }
}

void S_Output_DrawTexturedQuad(
    PHD_VBUF *vn1, PHD_VBUF *vn2, PHD_VBUF *vn3, PHD_VBUF *vn4, uint16_t tpage,
    PHD_UV *uv1, PHD_UV *uv2, PHD_UV *uv3, PHD_UV *uv4, uint16_t textype)
{
    int32_t i;
    float multiplier;
    GFX_3D_Vertex vertices[4];
    PHD_VBUF *src_vbuf[4];
    PHD_UV *src_uv[4];

    if (vn4->clip | vn3->clip | vn2->clip | vn1->clip) {
        if ((vn4->clip & vn3->clip & vn2->clip & vn1->clip)) {
            return;
        }

        if (vn1->clip >= 0 && vn2->clip >= 0 && vn3->clip >= 0
            && vn4->clip >= 0) {
            if ((vn1->ys - vn2->ys) * (vn3->xs - vn2->xs)
                    - (vn3->ys - vn2->ys) * (vn1->xs - vn2->xs)
                < 0) {
                return;
            }
        } else if (!phd_VisibleZClip(vn1, vn2, vn3)) {
            return;
        }

        S_Output_DrawTexturedTriangle(
            vn1, vn2, vn3, tpage, uv1, uv2, uv3, textype);
        S_Output_DrawTexturedTriangle(
            vn3, vn4, vn1, tpage, uv3, uv4, uv1, textype);
        return;
    }

    if ((vn1->ys - vn2->ys) * (vn3->xs - vn2->xs)
            - (vn3->ys - vn2->ys) * (vn1->xs - vn2->xs)
        < 0) {
        return;
    }

    multiplier = 0.0625f * g_Config.brightness;

    src_vbuf[0] = vn2;
    src_vbuf[1] = vn1;
    src_vbuf[2] = vn3;
    src_vbuf[3] = vn4;

    src_uv[0] = uv2;
    src_uv[1] = uv1;
    src_uv[2] = uv3;
    src_uv[3] = uv4;

    for (i = 0; i < 4; i++) {
        vertices[i].x = src_vbuf[i]->xs;
        vertices[i].y = src_vbuf[i]->ys;
        vertices[i].z = src_vbuf[i]->zv * 0.0001f;

        vertices[i].w = 65536.0f / src_vbuf[i]->zv;
        vertices[i].s = ((src_uv[i]->u1 & 0xFF00) + 127) * 0.00390625f
            * vertices[i].w * 0.00390625f;
        vertices[i].t = ((src_uv[i]->v1 & 0xFF00) + 127) * 0.00390625f
            * vertices[i].w * 0.00390625f;

        vertices[i].r = vertices[i].g = vertices[i].b =
            (8192.0f - src_vbuf[i]->g) * multiplier;

        Output_ApplyWaterEffect(&vertices[i].r, &vertices[i].g, &vertices[i].b);
    }

    if (m_TextureMap[tpage] != GFX_NO_TEXTURE) {
        S_Output_EnableTextureMode();
        S_Output_SelectTexture(tpage);
    } else {
        S_Output_DisableTextureMode();
    }

    GFX_3D_Renderer_RenderPrimStrip(m_Renderer3D, vertices, 4);
}

void S_Output_DownloadTextures(int32_t pages)
{
    if (pages > GFX_MAX_TEXTURES) {
        Shell_ExitSystem("Attempt to download more than texture page limit");
    }

    S_Output_ReleaseTextures();

    for (int i = 0; i < pages; i++) {
        GFX_2D_SurfaceDesc surface_desc = { 0 };
        bool result = GFX_2D_Surface_Lock(m_TextureSurfaces[i], &surface_desc);
        S_Output_CheckError(result);

        uint32_t *output_ptr = surface_desc.pixels;
        uint8_t *input_ptr = g_TexturePagePtrs[i];
        for (int j = 0; j < surface_desc.width * surface_desc.height; j++) {
            uint8_t pal_idx = *input_ptr++;
            // first color in the palette is chroma key, make it transparent
            uint8_t alpha = pal_idx == 0 ? 0 : 0xFF;
            RGB888 pix = S_Output_GetPaletteColor(pal_idx);
            *output_ptr++ =
                pix.b | (pix.g << 8) | (pix.r << 16) | (alpha << 24);
        }

        result =
            GFX_2D_Surface_Unlock(m_TextureSurfaces[i], surface_desc.pixels);
        S_Output_CheckError(result);

        m_TextureMap[i] = GFX_3D_Renderer_TextureReg(
            m_Renderer3D, surface_desc.pixels, surface_desc.width,
            surface_desc.height);
    }

    m_SelectedTexture = -1;
}

bool S_Output_MakeScreenshot(const char *path)
{
    GFX_Context_ScheduleScreenshot(path);
    return true;
}
