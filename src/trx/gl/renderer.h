#pragma once

#include <trx/core/result.h>
#include <trx/gl/config.h>
#include <trx/gl/texture.h>

#include <GL/glew.h>
#include <stdint.h>

typedef struct TRX_GL_Renderer {
    RESULT (*init)(
        struct TRX_GL_Renderer *renderer, const TRX_GL_CONFIG *config);
    void (*shutdown)(struct TRX_GL_Renderer *renderer);
    void (*swap_buffers)(struct TRX_GL_Renderer *renderer);
    void *priv;
} TRX_GL_RENDERER;

extern TRX_GL_RENDERER g_TRX_GL_Renderer;

// Bind the geometry framebuffer for rendering the 3D scene.
void TRX_GL_Renderer_BindGeometryFbo(void);

// Marks what was resolved from the scene stale, so that the next resolve
// reads the geometry framebuffer again. Anything that draws into the scene
// after a resolve has to call this.
void TRX_GL_Renderer_InvalidateScene(void);

// Bind the UI framebuffer for rendering the UI overlay.
void TRX_GL_Renderer_BindUiFbo(void);

// Returns the GL object id of the geometry framebuffer, for use as a blit
// source.
GLuint TRX_GL_Renderer_GetGeometryFboId(void);

// Starts drawing over the interface at the scene resolution, so that the
// upscaling factor applies to what is drawn as it applies to the scene. Used
// for models in a world of their own and for backdrops. Returns false, having
// changed nothing, where the scene is not upscaled or the UI framebuffer is not
// the one bound: drawing then goes where it is.
bool TRX_GL_Renderer_BeginUiScenePass(void);

// Stretches what was drawn since TRX_GL_Renderer_BeginUiScenePass onto the UI
// framebuffer, over what is there, with the upscaling filter. Leaves the UI
// framebuffer bound with its viewport and premultiplied blending on; the
// program, vertex array, sampler and texture of unit 0 are the renderer's
// own, and depth testing and culling are off.
void TRX_GL_Renderer_EndUiScenePass(void);

// Resize the framebuffers to the current viewport sizes. The renderer does
// this itself once the frame has been presented; call it when the viewports
// change between frames, so the next frame is not drawn into framebuffers of
// the old size.
void TRX_GL_Renderer_SyncFboSizes(void);

// Paints every buffer the window rotates through black. A window that
// changes size gets fresh buffers from the driver, and those hold the pixels
// the memory carried before; whichever of them the driver presents next then
// shows an older frame.
void TRX_GL_Renderer_BlankWindow(void);

// Makes each frame present black while it is set. The frame is still drawn
// and put together in full, so the graphics card does the same work.
void TRX_GL_Renderer_SetOutputBlank(bool is_blank);

// Resolve the geometry framebuffer down to VIEWPORT_SCENE and return the GL
// object id holding the result (3D scene only, no UI). Multisampling and
// supersampling both need this before the scene can be sampled or read back;
// with neither of them on, the geometry framebuffer already is that result and
// is returned as is. Resolving twice in a frame costs nothing.
GLuint TRX_GL_Renderer_ResolveSceneFbo(void);

// Uploads a color cube that maps rendered colors to the nearest colors in the
// level palette. Each texel stores the color in RGB and its distance in alpha.
// Passing nullptr disables the mapping.
void TRX_GL_Renderer_SetPaletteLut(const void *rgba, int32_t size);

// Draw the scene and the UI over it into the given texture, the same way the
// frame is put together for the window. Called between frames, before anything
// has drawn over the framebuffers, this reproduces the frame the player is
// looking at; the texture is written in full, at its own size.
void TRX_GL_Renderer_CompositeToTexture(
    const TRX_GL_TEXTURE *texture, int32_t width, int32_t height);
