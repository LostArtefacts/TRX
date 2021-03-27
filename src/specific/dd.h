#ifndef T1M_SPECIFIC_DD_H
#define T1M_SPECIFIC_DD_H

#include <windows.h>
#include <ddraw.h>

void DDError(HRESULT result);
void DDRenderEnd();
void DDClearSurface(LPDIRECTDRAWSURFACE surface);
void DDBlitSurface(LPDIRECTDRAWSURFACE target, LPDIRECTDRAWSURFACE source);

void T1MInjectSpecificDD();

#endif
