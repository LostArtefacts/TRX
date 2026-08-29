// Initialises the object table empty until a test fills a slot.

#include <harness/fake_objects.h>
#include <trx/game/objects/common.h>

static OBJECT m_Objects[FAKE_OBJ_COUNT];

OBJECT *Object_Get(const OBJECT_ID object_id)
{
    return &m_Objects[object_id];
}
