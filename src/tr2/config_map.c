#include "config_map.h"

#include "config.h"

// import order guard
#include <libtrx/config/map.h>
// import order guard

#include "global/types.h"

const CONFIG_OPTION g_ConfigOptionMap[] = {
#include "config_map.def"
    // guard
    { 0 },
};
