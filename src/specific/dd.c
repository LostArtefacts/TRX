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

void T1MInjectSpecificDD()
{
    INJECT(0x004077D0, DDError);
}
