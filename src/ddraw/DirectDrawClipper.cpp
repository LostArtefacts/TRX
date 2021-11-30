#include "ddraw/DirectDrawClipper.hpp"

#include "glrage_util/Logger.hpp"

namespace glrage {
namespace ddraw {

DirectDrawClipper::DirectDrawClipper()
{
}

DirectDrawClipper::~DirectDrawClipper()
{
}

/*** IUnknown methods ***/
HRESULT WINAPI DirectDrawClipper::QueryInterface(REFIID riid, LPVOID *ppvObj)
{
    if (IsEqualGUID(riid, IID_IDirectDrawSurface)) {
        *ppvObj = static_cast<IDirectDrawClipper *>(this);
    } else {
        return Unknown::QueryInterface(riid, ppvObj);
    }

    Unknown::AddRef();
    return S_OK;
}

ULONG WINAPI DirectDrawClipper::AddRef()
{
    return Unknown::AddRef();
}

ULONG WINAPI DirectDrawClipper::Release()
{
    return Unknown::Release();
}

/*** IDirectDrawClipper methods ***/
HRESULT WINAPI DirectDrawClipper::GetClipList(
    LPRECT lpRect, LPRGNDATA lpClipList, LPDWORD lpdwSize)
{
    *lpClipList = m_clipList;
    return DD_OK;
}

HRESULT WINAPI DirectDrawClipper::GetHWnd(HWND *lphWnd)
{
    *lphWnd = m_hWnd;
    return DD_OK;
}

HRESULT WINAPI DirectDrawClipper::Initialize(LPDIRECTDRAW lpDD, DWORD dwFlags)
{
    return DD_OK;
}

HRESULT WINAPI DirectDrawClipper::IsClipListChanged(BOOL *lpbChanged)
{
    *lpbChanged = FALSE;
    return DD_OK;
}

HRESULT WINAPI
DirectDrawClipper::SetClipList(LPRGNDATA lpClipList, DWORD dwFlags)
{
    m_clipList = *lpClipList;
    return DD_OK;
}

HRESULT WINAPI DirectDrawClipper::SetHWnd(DWORD dwFlags, HWND hWnd)
{
    m_hWnd = hWnd;
    return DD_OK;
}

}
}
