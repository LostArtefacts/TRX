#pragma once

#include "glrage/Context.hpp"

namespace glrage {

class GLRage {
public:
    static Context &getContext();

private:
    static Context m_context;
};

}
