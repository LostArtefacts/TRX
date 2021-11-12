#pragma once

#include "Patch.hpp"

#include <Windows.h>

namespace glrage {

class WipeoutPatch : public Patch
{
public:
    virtual GameID gameID();
    virtual void apply();

private:
    static BOOL WINAPI hookSystemParametersInfoA(
        UINT uiAction, UINT uiParam, PVOID pvParam, UINT fWinIni);
};

} // namespace glrage