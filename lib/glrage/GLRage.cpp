#include "GLRage.hpp"

namespace glrage {

GLRAPI Context& GLRage::getContext()
{
    return ContextImpl::instance();
}

GLRAPI Config& GLRage::getConfig()
{
    return Config::instance();
}

} // namespace glrage
