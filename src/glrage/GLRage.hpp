#pragma once

#include "glrage/ContextImpl.hpp"

namespace glrage {

class GLRage {
public:
    static Context &getContext();

private:
    static ContextImpl m_context;
};

}
