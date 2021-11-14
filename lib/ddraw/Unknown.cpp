#include "Unknown.hpp"

namespace glrage {
namespace ddraw {

Unknown::Unknown()
{}

Unknown::~Unknown()
{}

HRESULT WINAPI Unknown::QueryInterface(REFIID riid, LPVOID* ppvObj)
{
    if (IsEqualGUID(riid, IID_IUnknown)) {
        *ppvObj = static_cast<IUnknown*>(this);
        AddRef();
        return S_OK;
    } else {
        return E_NOINTERFACE;
    }
}

ULONG WINAPI Unknown::AddRef()
{
    return ++m_refCount;
}

ULONG WINAPI Unknown::Release()
{
    ULONG refCount = InterlockedDecrement(&m_refCount);
    if (refCount == 0) {
        delete this;
    }
    return refCount;
}

} // namespace ddraw
} // namespace glrage
