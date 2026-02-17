#include <trx/gl/enum.h>

#include <trx/core/enum_map.h>

static __attribute__((constructor)) void M_Init(void)
{
    ENUM_MAP(TRX_GL_TEXTURE_FILTER, TRX_GL_TF_BILINEAR, "bilinear");
    ENUM_MAP(TRX_GL_TEXTURE_FILTER, TRX_GL_TF_POINT, "point");
}
