#pragma once

#include "Patch.hpp"

namespace glrage {

class TombRaiderPatch : public Patch
{
public:
    TombRaiderPatch(bool ub);
    virtual GameID gameID();
    virtual void apply();

private:
    void applyCrashPatches();
    void applyGraphicPatches();
    void applySoundPatches();
    void applyFMVPatches();
    void applyKeyboardPatches();
    void applyLogicPatches();
    void applyLocalePatches();

    bool m_ub;
};

} // namespace glrage
