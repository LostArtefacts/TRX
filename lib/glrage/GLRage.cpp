#include "GLRage.hpp"

namespace glrage {

GLRAPI Context& GLRage::getContext()
{
    return ContextImpl::instance();
}

} // namespace glrage
