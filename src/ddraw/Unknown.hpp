#pragma once

#include <unknwnbase.h>
#include <windows.h>

namespace glrage {
namespace ddraw {

class Unknown : public IUnknown {
public:
    Unknown();
    virtual ~Unknown();

    virtual HRESULT WINAPI QueryInterface(REFIID riid, LPVOID *ppvObj);
    virtual ULONG WINAPI AddRef();
    virtual ULONG WINAPI Release();

private:
    volatile ULONG m_refCount = 1;
};

}
}
