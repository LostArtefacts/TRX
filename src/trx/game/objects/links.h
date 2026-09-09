#pragma once

#include <trx/core/result.h>
#include <trx/game/objects/ids.h>

// Define engine-wide links from one object identity to multiple objects.
typedef enum {
#define X_OBJECT_LINK(link_id_, name_) link_id_,
#include <trx/game/objects/links.def>
#undef X_OBJECT_LINK
    OBJ_LINK_NUMBER_OF,
} OBJECT_LINK;

// Link one object to another.
RESULT ObjectLink_Add(OBJECT_ID from, OBJECT_LINK link, OBJECT_ID to);

// Return the link with the given name, or OBJ_LINK_NUMBER_OF if none exists.
OBJECT_LINK ObjectLink_FromName(const char *name);

// Return the first linked object, or NO_OBJECT if no link exists.
OBJECT_ID ObjectLink_Get(OBJECT_ID from, OBJECT_LINK link);

// Return a linked object, or NO_OBJECT if no link exists.
OBJECT_ID ObjectLink_GetInverse(OBJECT_ID to, OBJECT_LINK link);

// Return the number of linked objects and the object at a given position.
int32_t ObjectLink_GetCount(OBJECT_ID from, OBJECT_LINK link);
OBJECT_ID ObjectLink_GetAt(OBJECT_ID from, OBJECT_LINK link, int32_t idx);

// Return the number of pairs in a complete link and the pair at a given
// position.
int32_t ObjectLink_GetPairCount(OBJECT_LINK link);
void ObjectLink_GetPairAt(
    OBJECT_LINK link, int32_t idx, OBJECT_ID *from, OBJECT_ID *to);
