#pragma once

#include <string>
#include <windows.h>

namespace glrage {

class ErrorUtils {
public:
    static void warning(const std::string &message);
    static void warning(const std::string &message, const std::string &reason);
    static void warning(
        const std::string &message, const std::exception &exception);
    static void warning(const std::exception &exception);

    static void error(const std::string &message);
    static void error(const std::string &message, const std::string &reason);
    static void error(
        const std::string &message, const std::exception &exception);
    static void error(const std::exception &exception);
    static std::string getSystemErrorString();

    static void setHWnd(HWND hwnd);

private:
    ErrorUtils();

    static HWND hwnd;
};

}
