#pragma once

#include "ContextImpl.hpp"

#include <glrage_util/Config.hpp>

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
    static GLRAPI Config& getConfig();

private:
    static ContextImpl m_context;
};

} // namespace glrage
