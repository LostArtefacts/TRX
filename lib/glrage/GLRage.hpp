#pragma once

#include "ContextImpl.hpp"

namespace glrage {

#ifdef _LIB
#define GLRAPI
#elif defined(GLR_EXPORTS)
#define GLRAPI __declspec(dllexport)
#else
#define GLRAPI __declspec(dllimport)
#endif

class GLRage
{
public:
    static GLRAPI Context& getContext();

private:
    static ContextImpl m_context;
};

} // namespace glrage
