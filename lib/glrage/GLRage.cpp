#include "GLRage.hpp"

namespace glrage {

GLRAPI Context& GLRage::getContext()
{
    return ContextImpl::instance();
}

GLRAPI RuntimePatcher& GLRage::getPatcher()
{
    return RuntimePatcher::instance();
}

GLRAPI Config& GLRage::getConfig()
{
    return Config::instance();
}

} // namespace glrage
