#include <trx/game/camera/fixed.h>

#include <trx/game/camera.h>
#include <trx/game/game_buf.h>

#include <stdlib.h>

#define M_LOCKED_CAMERA 1

static int32_t m_FixedObjectCount = 0;
static OBJECT_VECTOR *m_FixedObjects = nullptr;
static int32_t m_FlybyCount = 0;
static FLYBY_CAMERA *m_Flybys = nullptr;
static int32_t m_SequenceCount = 0;
static FLYBY_SEQUENCE *m_Sequences = nullptr;

static int32_t M_CompareFlybys(const void *const a, const void *const b)
{
    const FLYBY_CAMERA *const camera_a = (FLYBY_CAMERA *)a;
    const FLYBY_CAMERA *const camera_b = (FLYBY_CAMERA *)b;
    if (camera_a->sequence == camera_b->sequence) {
        return camera_a->index - camera_b->index;
    }
    return camera_a->sequence - camera_b->sequence;
}

void Camera_InitialiseFixedObjects(const int32_t num_objects)
{
    m_FixedObjectCount = num_objects + 1;
    m_FixedObjects =
        GameBuf_Alloc(m_FixedObjectCount * sizeof(OBJECT_VECTOR), GBUF_CAMERAS);
}

int32_t Camera_GetFixedObjectCount(void)
{
    return m_FixedObjectCount - 1;
}

int32_t Camera_GetDynamicFixedObjectIdx(void)
{
    return m_FixedObjectCount - 1;
}

void Camera_UpdateDynamicFixedObject(const XYZ_32 pos, const int16_t room_num)
{
    const int32_t idx = Camera_GetDynamicFixedObjectIdx();
    OBJECT_VECTOR *const camera = Camera_GetFixedObject(idx);
    camera->pos = pos;
    camera->data = room_num;
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

void Camera_InitialiseFlybys(const int32_t num_cameras)
{
    if (num_cameras <= 0) {
        m_FlybyCount = 0;
        m_Flybys = nullptr;
        return;
    }

    m_FlybyCount = num_cameras;
    m_Flybys = GameBuf_Alloc(m_FlybyCount * sizeof(FLYBY_CAMERA), GBUF_CAMERAS);
}

int32_t Camera_GetFlybyCount(void)
{
    return m_FlybyCount;
}

FLYBY_CAMERA *Camera_GetFlybyCamera(const int32_t camera_idx)
{
    if (camera_idx < 0 || camera_idx >= m_FlybyCount) {
        return nullptr;
    }
    return &m_Flybys[camera_idx];
}

void Camera_SetupSequences(void)
{
    m_Sequences = nullptr;
    m_SequenceCount = 0;
    if (m_FlybyCount == 0) {
        return;
    }

    qsort(m_Flybys, m_FlybyCount, sizeof(FLYBY_CAMERA), M_CompareFlybys);

    const FLYBY_CAMERA *const last_camera = &m_Flybys[m_FlybyCount - 1];
    m_SequenceCount = last_camera->sequence + 1;
    m_Sequences =
        GameBuf_Alloc(m_SequenceCount * sizeof(FLYBY_SEQUENCE), GBUF_CAMERAS);

    for (int32_t i = 0; i < m_SequenceCount; i++) {
        m_Sequences[i].camera_idx = NO_CAMERA;
    }

    for (int32_t i = 0; i < m_FlybyCount; i++) {
        const FLYBY_CAMERA *const camera = &m_Flybys[i];
        FLYBY_SEQUENCE *const sequence = &m_Sequences[camera->sequence];
        sequence->num_cameras++;
        sequence->one_shot = camera->flags.one_shot;
        if (sequence->camera_idx == NO_CAMERA) {
            sequence->camera_idx = i;
        }
    }
}

int32_t Camera_GetSequenceCount(void)
{
    return m_SequenceCount;
}

FLYBY_SEQUENCE *Camera_GetSequence(const int32_t sequence_idx)
{
    if (sequence_idx < 0 || sequence_idx >= m_SequenceCount) {
        return nullptr;
    }
    return &m_Sequences[sequence_idx];
}
