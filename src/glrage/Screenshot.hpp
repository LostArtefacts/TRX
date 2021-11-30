#pragma once

#include <cstdint>

namespace glrage {

class Screenshot {
public:
    void schedule(bool schedule);
    void captureScheduled();
    void capture();

private:
    uint32_t m_index = 0;
    bool m_schedule = false;
};

}
