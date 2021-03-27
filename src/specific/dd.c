#include "specific/dd.h"

#include "global/vars_platform.h"
#include "specific/smain.h"
#include "util.h"

void DDError(HRESULT result)
{
    if (result) {
        LOG_ERROR("DirectDraw error code %x", result);
        ShowFatalError("Fatal DirectDraw error!");
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
    INJECT(0x00408B2C, DDBlitSurface);
}
