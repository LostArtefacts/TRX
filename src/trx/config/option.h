#pragma once

#include <trx/core/value.h>

typedef struct {
    const char *name;
    TRX_VALUE_TYPE type;
    // A float presented as a percentage: stored 0..1, shown and entered as a
    // 0..100 percentage. Only meaningful for TVT_FLOAT.
    bool percent;
    const void *target;
    const void *default_value;
    const void *param;
} CONFIG_OPTION;
