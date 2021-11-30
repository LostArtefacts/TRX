#pragma once

#include "ddraw/ddraw.hpp"

#ifdef __cplusplus
extern "C" {
#endif

HRESULT MyDirectDrawCreate(
    GUID FAR *lpGUID, LPDIRECTDRAW FAR *lplpDD, IUnknown FAR *pUnkOuter);

#ifdef __cplusplus
}
#endif
