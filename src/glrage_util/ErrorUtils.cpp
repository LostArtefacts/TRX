#include "glrage_util/ErrorUtils.hpp"

namespace glrage {

void ErrorUtils::warning(const std::string &message)
{
    MessageBox(hwnd, message.c_str(), "Warning", MB_ICONWARNING | MB_OK);
}

void ErrorUtils::warning(
    const std::string &message, const std::exception &exception)
{
    warning(message, exception.what());
}

void ErrorUtils::warning(const std::string &message, const std::string &reason)
{
    if (reason.empty()) {
        warning(message);
    } else {
        warning(message + ": " + reason);
    }
}

void ErrorUtils::warning(const std::exception &exception)
{
    warning(exception.what());
}

void ErrorUtils::error(const std::string &message)
{
    MessageBox(hwnd, message.c_str(), "Error", MB_ICONERROR | MB_OK);
    ExitProcess(1);
}

void ErrorUtils::error(
    const std::string &message, const std::exception &exception)
{
    error(message, exception.what());
}

void ErrorUtils::error(const std::string &message, const std::string &reason)
{
    if (reason.empty()) {
        error(message);
    } else {
        error(message + ": " + reason);
    }
}

void ErrorUtils::error(const std::exception &exception)
{
    error(exception.what());
}

std::string ErrorUtils::getSystemErrorString()
{
    static char error[1024];
    strerror_s(error, sizeof(error), errno);
    return error;
}

HWND ErrorUtils::hwnd = nullptr;

void ErrorUtils::setHWnd(HWND _hwnd)
{
    hwnd = _hwnd;
}

}
