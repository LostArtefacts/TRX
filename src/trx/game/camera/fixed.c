#include <trx/game/camera/fixed.h>

#include <trx/game/camera/const.h>
#include <trx/game/game_buf.h>

#define M_LOCKED_CAMERA 1

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

bool Camera_IsLocked(const int32_t camera_num)
{
    if (camera_num == NO_CAMERA) {
        return false;
    }

    const OBJECT_VECTOR *const fixed_camera = Camera_GetFixedObject(camera_num);
    return (fixed_camera->flags & M_LOCKED_CAMERA) != 0;
}
