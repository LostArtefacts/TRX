// Initialises the object table empty until a test fills a slot.

#include <harness/fake_objects.h>
#include <trx/game/anims/types.h>
#include <trx/game/objects/common.h>

static OBJECT m_Objects[FAKE_OBJ_COUNT];
static ANIM_FRAME m_Frames[FAKE_OBJ_COUNT];
static ANIM m_Anims[FAKE_OBJ_COUNT];

OBJECT *Object_Get(const OBJECT_ID object_id)
{
    return &m_Objects[object_id];
}

ANIM *Object_GetAnim(const OBJECT *const object, const int32_t anim_idx)
{
    const int32_t object_id = object - m_Objects;
    if (object_id < 0 || object_id >= FAKE_OBJ_COUNT) {
        return nullptr;
    }
    return &m_Anims[object_id];
}

void FakeObjects_SetMeshBounds(
    const OBJECT_ID object_id, const BOUNDS_16 bounds)
{
    m_Frames[object_id].bounds = bounds;
    m_Anims[object_id].frame_ptr = &m_Frames[object_id];
    m_Objects[object_id].loaded = true;
}
