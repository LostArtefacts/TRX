#pragma once

typedef enum {
    COT_BOOL = 0,
    COT_INT32 = 1,
    COT_FLOAT = 2,
    COT_DOUBLE = 3,
    COT_ENUM = 4,
} CONFIG_OPTION_TYPE;

typedef struct {
    const char *name;
    CONFIG_OPTION_TYPE type;
    const void *target;
    const void *default_value;
    const void *param;
} CONFIG_OPTION;
