// Initialises the object table empty until a test fills a slot.

#include <trx/game/objects/common.h>

static OBJECT m_Objects[O_NUMBER_OF];

OBJECT *Object_Get(const OBJECT_ID object_id)
{
    return &m_Objects[object_id];
}
