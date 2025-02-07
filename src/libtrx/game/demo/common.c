#include "game/game_buf.h"

#include <stdint.h>

static uint32_t *m_Data = nullptr;

void Demo_InitialiseData(const uint16_t data_length)
{
    if (data_length == 0) {
        m_Data = nullptr;
    } else {
        m_Data = GameBuf_Alloc(
            (data_length + 1) * sizeof(uint32_t), GBUF_DEMO_BUFFER);
        m_Data[data_length] = -1;
    }
}

uint32_t *Demo_GetData(void)
{
    return m_Data;
}
