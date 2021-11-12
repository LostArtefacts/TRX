#pragma once

#include "Patch.hpp"

namespace glrage {

class AssaultRigsPatch : public Patch
{
public:
    virtual GameID gameID();
    virtual void apply();
};

} // namespace glrage
