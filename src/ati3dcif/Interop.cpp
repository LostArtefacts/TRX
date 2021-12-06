#include "ati3dcif/Interop.hpp"

#include "ati3dcif/ATI3DCIF.h"
#include "gfx/context.h"
#include "log.h"

#include <cassert>
#include <memory>
#include <stdexcept>

namespace glrage {
namespace cif {

extern "C" {

int ATI3DCIF_TextureReg(const void *data, int width, int height)
{
    GFX_3D_Renderer *renderer = GFX_Context_GetRenderer3D();
    assert(renderer);
    return GFX_3D_Renderer_TextureReg(renderer, data, width, height);
}

bool ATI3DCIF_TextureUnreg(int texture_num)
{
    GFX_3D_Renderer *renderer = GFX_Context_GetRenderer3D();
    assert(renderer);
    return GFX_3D_Renderer_TextureUnreg(renderer, texture_num);
}

bool ATI3DCIF_SetState(C3D_ERSID eRStateID, C3D_PRSDATA pRStateData)
{
    GFX_3D_Renderer *renderer = GFX_Context_GetRenderer3D();
    assert(renderer);
    return GFX_3D_Renderer_SetState(renderer, eRStateID, pRStateData);
}

void ATI3DCIF_RenderBegin(void)
{
    GFX_3D_Renderer *renderer = GFX_Context_GetRenderer3D();
    assert(renderer);
    GFX_3D_Renderer_RenderBegin(renderer);
}

void ATI3DCIF_RenderEnd(void)
{
    GFX_3D_Renderer *renderer = GFX_Context_GetRenderer3D();
    assert(renderer);
    GFX_3D_Renderer_RenderEnd(renderer);
}

void ATI3DCIF_RenderPrimStrip(C3D_VSTRIP vStrip, int u32NumVert)
{
    GFX_3D_Renderer *renderer = GFX_Context_GetRenderer3D();
    assert(renderer);
    GFX_3D_Renderer_RenderPrimStrip(renderer, vStrip, u32NumVert);
}

void ATI3DCIF_RenderPrimList(C3D_VLIST vList, int u32NumVert)
{
    GFX_3D_Renderer *renderer = GFX_Context_GetRenderer3D();
    assert(renderer);
    GFX_3D_Renderer_RenderPrimList(renderer, vList, u32NumVert);
}
}

}
}
