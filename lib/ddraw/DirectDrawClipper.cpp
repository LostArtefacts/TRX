#include "DirectDrawClipper.hpp"

#include <glrage_util/Logger.hpp>

namespace glrage {
namespace ddraw {

DirectDrawClipper::DirectDrawClipper()
{
    LOG_TRACE("");
}

DirectDrawClipper::~DirectDrawClipper()
{
    LOG_TRACE("");
}

/*** IUnknown methods ***/
HRESULT WINAPI DirectDrawClipper::QueryInterface(REFIID riid, LPVOID* ppvObj)
{
    LOG_TRACE("");

    if (IsEqualGUID(riid, IID_IDirectDrawSurface)) {
        *ppvObj = static_cast<IDirectDrawClipper*>(this);
    } else {
        return Unknown::QueryInterface(riid, ppvObj);
    }

    Unknown::AddRef();
    return S_OK;
}

ULONG WINAPI DirectDrawClipper::AddRef()
{
    LOG_TRACE("");

    return Unknown::AddRef();
}

ULONG WINAPI DirectDrawClipper::Release()
{
    LOG_TRACE("");

    return Unknown::Release();
}

/*** IDirectDrawClipper methods ***/
HRESULT WINAPI DirectDrawClipper::GetClipList(LPRECT lpRect,
    LPRGNDATA lpClipList,
    LPDWORD lpdwSize)
{
    LOG_TRACE("");

    *lpClipList = m_clipList;
    return DD_OK;
}

HRESULT WINAPI DirectDrawClipper::GetHWnd(HWND* lphWnd)
{
    LOG_TRACE("");

    *lphWnd = m_hWnd;
    return DD_OK;
}

HRESULT WINAPI DirectDrawClipper::Initialize(LPDIRECTDRAW lpDD, DWORD dwFlags)
{
    LOG_TRACE("");

    return DD_OK;
}

HRESULT WINAPI DirectDrawClipper::IsClipListChanged(BOOL* lpbChanged)
{
    LOG_TRACE("");

    *lpbChanged = FALSE;
    return DD_OK;
}

HRESULT WINAPI DirectDrawClipper::SetClipList(LPRGNDATA lpClipList,
    DWORD dwFlags)
{
    LOG_TRACE("");

    m_clipList = *lpClipList;
    return DD_OK;
}

HRESULT WINAPI DirectDrawClipper::SetHWnd(DWORD dwFlags, HWND hWnd)
{
    LOG_TRACE("");

    m_hWnd = hWnd;
    return DD_OK;
}

} // namespace ddraw
} // namespace glrage
