#include "glrage/GLRage.hpp"

namespace glrage {

Context &GLRage::getContext()
{
    return Context::instance();
}

}
