#include "ati3dcif/Interop.hpp"

#include "ati3dcif/ATI3DCIF.h"
#include "ati3dcif/Renderer.hpp"
#include "log.h"

#include <cassert>
#include <memory>
#include <stdexcept>

namespace glrage {
namespace cif {

static std::unique_ptr<Renderer> renderer = NULL;

extern "C" {

void ATI3DCIF_Term(void)
{
    if (renderer) {
        renderer.reset();
    }
}

void ATI3DCIF_Init(void)
{
    if (renderer) {
        LOG_ERROR("Previous instance was not terminated by ATI3DCIF_Term!");
        ATI3DCIF_Term();
    }

    renderer = std::make_unique<Renderer>();
}

bool ATI3DCIF_TextureReg(C3D_PTMAP ptmapToReg, C3D_PHTX phtmap)
{
    assert(renderer);
    return renderer->textureReg(ptmapToReg, phtmap);
}

bool ATI3DCIF_TextureUnreg(C3D_HTX htxToUnreg)
{
    assert(renderer);
    return renderer->textureUnreg(htxToUnreg);
}

bool ATI3DCIF_SetState(C3D_ERSID eRStateID, C3D_PRSDATA pRStateData)
{
    assert(renderer);
    return renderer->setState(eRStateID, pRStateData);
}

void ATI3DCIF_RenderBegin(void)
{
    assert(renderer);
    renderer->renderBegin();
}

void ATI3DCIF_RenderEnd(void)
{
    assert(renderer);
    renderer->renderEnd();
}

void ATI3DCIF_RenderPrimStrip(C3D_VSTRIP vStrip, int u32NumVert)
{
    assert(renderer);
    renderer->renderPrimStrip(vStrip, u32NumVert);
}

void ATI3DCIF_RenderPrimList(C3D_VLIST vList, int u32NumVert)
{
    assert(renderer);
    renderer->renderPrimList(vList, u32NumVert);
}
}

}
}
