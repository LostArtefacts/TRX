#include <trx/game/game_buf.h>
#include <trx/game/types.h>

static int32_t m_FixedObjectCount = 0;
static OBJECT_VECTOR *m_FixedObjects = nullptr;

void Camera_InitialiseFixedObjects(const int32_t num_objects)
{
    m_FixedObjectCount = num_objects;
    m_FixedObjects = num_objects == 0
        ? nullptr
        : GameBuf_Alloc(num_objects * sizeof(OBJECT_VECTOR), GBUF_CAMERAS);
}

int32_t Camera_GetFixedObjectCount(void)
{
    return m_FixedObjectCount;
}

OBJECT_VECTOR *Camera_GetFixedObject(const int32_t object_idx)
{
    if (m_FixedObjects == nullptr) {
        return nullptr;
    }
    return &m_FixedObjects[object_idx];
}
