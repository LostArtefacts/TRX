#include "ErrorUtils.hpp"
#include "StringUtils.hpp"

namespace glrage {

void ErrorUtils::warning(const std::string& message)
{
    MessageBox(hwnd, message.c_str(), "Warning", MB_ICONWARNING | MB_OK);
}

void ErrorUtils::warning(
    const std::string& message, const std::exception& exception)
{
    warning(message, exception.what());
}

void ErrorUtils::warning(const std::string& message, const std::string& reason)
{
    if (reason.empty()) {
        warning(message);
    } else {
        warning(message + ": " + reason);
    }
}

void ErrorUtils::warning(const std::exception& exception)
{
    warning(exception.what());
}

void ErrorUtils::error(const std::string& message)
{
    MessageBox(hwnd, message.c_str(), "Error", MB_ICONERROR | MB_OK);
    ExitProcess(1);
}

void ErrorUtils::error(
    const std::string& message, const std::exception& exception)
{
    error(message, exception.what());
}

void ErrorUtils::error(const std::string& message, const std::string& reason)
{
    if (reason.empty()) {
        error(message);
    } else {
        error(message + ": " + reason);
    }
}

void ErrorUtils::error(const std::exception& exception)
{
    error(exception.what());
}

std::string ErrorUtils::getSystemErrorString()
{
    static char error[1024];
    strerror_s(error, sizeof(error), errno);
    return error;
}

// https://stackoverflow.com/a/17387176: Create a string from the last error
// code
std::string ErrorUtils::getWindowsErrorString()
{
    DWORD error = GetLastError();
    if (!error) {
        return "Unknown error";
    }

    LPVOID lpMsgBuf;
    DWORD bufLen = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                                     FORMAT_MESSAGE_FROM_SYSTEM |
                                     FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        reinterpret_cast<LPTSTR>(&lpMsgBuf), 0, nullptr);

    if (!bufLen) {
        return "Unknown error";
    }

    LPCSTR lpMsgStr = static_cast<LPCSTR>(lpMsgBuf);
    std::string result(lpMsgStr, lpMsgStr + bufLen);
    LocalFree(lpMsgBuf);
    return result;
}

HWND ErrorUtils::hwnd = nullptr;

void ErrorUtils::setHWnd(HWND _hwnd)
{
    hwnd = _hwnd;
}

} // namespace glrage
