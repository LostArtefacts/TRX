#include "specific/dd.h"

#include "global/vars_platform.h"
#include "specific/ati.h"
#include "specific/smain.h"
#include "util.h"

void DDError(HRESULT result)
{
    if (result) {
        LOG_ERROR("DirectDraw error code %x", result);
        ShowFatalError("Fatal DirectDraw error!");
    }
}

void DDRenderBegin()
{
    DDOldIsRendering = DDIsRendering;
    if (!DDIsRendering) {
        ATI3DCIF_RenderBegin(ATIRenderContext);
        DDIsRendering = 1;
    }
}

void DDRenderEnd()
{
    DDOldIsRendering = DDIsRendering;
    if (DDIsRendering) {
        ATI3DCIF_RenderEnd();
        DDIsRendering = 0;
    }
}

void DDClearSurface(LPDIRECTDRAWSURFACE surface)
{
    DDBLTFX blt_fx;
    blt_fx.dwSize = sizeof(DDBLTFX);
    blt_fx.dwFillColor = 0;
    HRESULT result = IDirectDrawSurface_Blt(
        surface, NULL, NULL, NULL, DDBLT_WAIT | DDBLT_COLORFILL, &blt_fx);
    if (result) {
        DDError(result);
    }
}

void DDBlitSurface(LPDIRECTDRAWSURFACE target, LPDIRECTDRAWSURFACE source)
{
    RECT rect;
    SetRect(&rect, 0, 0, DDrawSurfaceWidth, DDrawSurfaceHeight);
    HRESULT result =
        IDirectDrawSurface_Blt(source, &rect, target, &rect, DDBLT_WAIT, NULL);
    if (result) {
        DDError(result);
    }
}

void T1MInjectSpecificDD()
{
    INJECT(0x004077D0, DDError);
    INJECT(0x00407827, DDRenderBegin);
    INJECT(0x0040783B, DDRenderEnd);
    INJECT(0x00407A49, DDClearSurface);
    INJECT(0x00408B2C, DDBlitSurface);
}
