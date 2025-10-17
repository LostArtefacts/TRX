#include "game/option/compass.h"

#include "game/lara.h"

static int16_t m_CompassNeedle = 0;
static int16_t m_CompassSpeed = 0;

void Option_Compass_UpdateNeedle(const INVENTORY_ITEM *const inv_item)
{
    const ITEM *const lara_item = Lara_GetItem();
    if (lara_item == nullptr) {
        return;
    }
    int16_t delta = -inv_item->y_rot - lara_item->rot.y - m_CompassNeedle;
    m_CompassSpeed = m_CompassSpeed * 19 / 20 + delta / 50;
    m_CompassNeedle += m_CompassSpeed;
}

int16_t Option_Compass_GetNeedleAngle(void)
{
    return m_CompassNeedle;
}
