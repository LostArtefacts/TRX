#pragma once

#include "ati3dcif.hpp"

#include <stdexcept>
#include <string>

namespace glrage {
namespace cif {

class Error : public std::runtime_error
{
public:
    Error(const char* message, C3D_EC errorCode);
    Error(const std::string& message, C3D_EC errorCode);
    Error(const char* message);
    Error(const std::string& message);
    C3D_EC getErrorCode() const;
    const char* getErrorName() const;

private:
    C3D_EC m_errorCode;
};

} // namespace cif
} // namespace glrage
