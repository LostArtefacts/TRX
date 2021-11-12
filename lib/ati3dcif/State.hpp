#pragma once

#include "StateVar.hpp"

#include <array>

namespace glrage {
namespace cif {

class State
{
public:
    State();
    void set(C3D_ERSID id, C3D_PRSDATA data);
    const StateVar::Value& get(C3D_ERSID id);
    void set(C3D_ERSID id, const StateVar::Value& value);
    void reset();
    void registerObserver(const StateVar::Observer& observer);
    void registerObserver(const StateVar::Observer& observer, C3D_ERSID id);

private:
    std::array<StateVar, C3D_ERS_NUM> m_vars;
};

} // namespace cif
} // namespace glrage
