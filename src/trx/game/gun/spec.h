#pragma once

#include <trx/core/json/util/read_io.h>
#include <trx/game/gun/types.h>

// Read a weapon spec into an existing weapon definition, from the object the
// reader is on. Report failure for a name nothing stands for and for a value
// that is not the kind its key calls for, and leave the weapon unchanged
// where the spec fails. A key the reader does not know is passed over. The
// reader is left where the read stopped.
RESULT Gun_Spec_Read(JSON_READ_IO *io, WEAPON_INFO *weapon);
