#pragma once

#include "ati3dcif.hpp"

#include <array>
#include <vector>
#include <functional>

namespace glrage {
namespace cif {

class StateVar
{
public:
    union Value
    {
        C3D_BOOL boolean;
        C3D_COLOR color;
        C3D_EACMP eacmp;
        C3D_EADST eadst;
        C3D_EASEL easel;
        C3D_EASRC easrc;
        C3D_EPIXFMT epixfmt;
        C3D_EPRIM eprim;
        C3D_ESHADE eshade;
        C3D_ETEXCOMPFCN etexcompfcn;
        C3D_ETEXFILTER etexfilter;
        C3D_ETEXOP etexop;
        C3D_ETLIGHT etlight;
        C3D_ETPERSPCOR etperspcor;
        C3D_EVERTEX evertex;
        C3D_EZCMP ezcmp;
        C3D_EZMODE ezmode;
        C3D_HTX htx;
        C3D_PVOID pvoid;
        C3D_RECT rect;
        C3D_UINT32 uint32;
        std::array<uint8_t, sizeof(C3D_RECT)> raw;
    };

    typedef std::function<void(Value&)> Observer;

    template <typename T>
    StateVar(const T& value)
    {
        m_size = sizeof(T);

        if (m_size > sizeof(C3D_RECT)) {
            throw std::logic_error("Value too large");
        }

        const void* ptr = reinterpret_cast<const void*>(&value);
        memcpy(&m_value.raw[0], ptr, m_size);

        m_valueDefault = m_value;
    }

    void set(C3D_PRSDATA pRStateData);
    void set(const Value& value);
    const Value& get();
    void reset();
    void registerObserver(const Observer& observer);

private:
    void notify();

    size_t m_size;
    Value m_value{0};
    Value m_valueDefault{0};
    std::vector<Observer> m_observers;
};

} // namespace cif
} // namespace glrage