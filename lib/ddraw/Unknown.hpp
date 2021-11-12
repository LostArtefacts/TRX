#pragma once

#include <Unknwnbase.h>
#include <Windows.h>

namespace glrage {
namespace ddraw {

class Unknown : public IUnknown
{
public:
    Unknown();
    virtual ~Unknown();

    virtual HRESULT WINAPI QueryInterface(REFIID riid, LPVOID* ppvObj);
    virtual ULONG WINAPI AddRef();
    virtual ULONG WINAPI Release();

private:
    volatile ULONG m_refCount = 1;
};

} // namespace ddraw
} // namespace glrage