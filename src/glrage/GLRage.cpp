#include "glrage/GLRage.hpp"

namespace glrage {

Context &GLRage::getContext()
{
    return ContextImpl::instance();
}

}
