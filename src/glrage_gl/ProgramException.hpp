#pragma once

#include <stdexcept>

namespace glrage {
namespace gl {

class ProgramException : public std::runtime_error {
public:
    ProgramException(const std::string &msg)
        : std::runtime_error(msg)
    {
    }
    ProgramException(const char *msg)
        : std::runtime_error(msg)
    {
    }
};

}
}
