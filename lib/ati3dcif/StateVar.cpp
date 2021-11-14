#include "StateVar.hpp"

#include <cstring>
#include <stdexcept>

namespace glrage {
namespace cif {

void StateVar::set(C3D_PRSDATA pRStateData)
{
    Value valueTmp{0};
    memcpy(&valueTmp.raw[0], pRStateData, m_size);
    set(valueTmp);
}

void StateVar::set(const Value& value)
{
    if (m_value.raw != value.raw) {
        m_value = value;
        notify();
    }
}

const StateVar::Value& StateVar::get()
{
    return m_value;
}

void StateVar::reset()
{
    m_value = m_valueDefault;
    notify();
}

void StateVar::registerObserver(const Observer& observer)
{
    m_observers.push_back(observer);
}

void StateVar::notify()
{
    for (auto& observer : m_observers) {
        observer(m_value);
    }
}

} // namespace cif
} // namespace glrage
