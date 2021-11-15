#pragma once

#include "Unknown.hpp"
#include "ddraw.hpp"

namespace glrage {
namespace ddraw {

class DirectDrawClipper
    : public Unknown
    , public IDirectDrawClipper
{
public:
    DirectDrawClipper();
    virtual ~DirectDrawClipper();

    /*** IUnknown methods ***/
    virtual HRESULT WINAPI QueryInterface(REFIID riid, LPVOID* ppvObj);
    virtual ULONG WINAPI AddRef();
    virtual ULONG WINAPI Release();

    /*** IDirectDrawClipper methods ***/
    HRESULT WINAPI GetClipList(LPRECT lpRect,
        LPRGNDATA lpClipList,
        LPDWORD lpdwSize);
    HRESULT WINAPI GetHWnd(HWND* lphWnd);
    HRESULT WINAPI Initialize(LPDIRECTDRAW lpDD, DWORD dwFlags);
    HRESULT WINAPI IsClipListChanged(BOOL* lpbChanged);
    HRESULT WINAPI SetClipList(LPRGNDATA lpClipList, DWORD dwFlags);
    HRESULT WINAPI SetHWnd(DWORD dwFlags, HWND hWnd);

private:
    HWND m_hWnd = nullptr;
    RGNDATA m_clipList;
};

} // namespace ddraw
} // namespace glrage
