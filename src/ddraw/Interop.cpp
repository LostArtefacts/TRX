#include "ddraw/Interop.hpp"

#include "ddraw/DirectDraw.hpp"
#include "glrage/GLRage.hpp"
#include "glrage_util/ErrorUtils.hpp"
#include "glrage_util/Logger.hpp"

#include <string>

namespace glrage {
namespace ddraw {

extern "C" {
HRESULT MyDirectDrawCreate(
    GUID FAR *lpGUID, LPDIRECTDRAW FAR *lplpDD, IUnknown FAR *pUnkOuter)
{
    Context &context = GLRage::getContext();

    try {
        *lplpDD = new DirectDraw();
    } catch (const std::exception &ex) {
        ErrorUtils::warning(ex);
        return DDERR_GENERIC;
    }

    return DD_OK;
}
}

}
}
