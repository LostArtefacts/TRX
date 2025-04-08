#pragma once

typedef enum {
    COT_BOOL,
    COT_INT32,
    COT_FLOAT,
    COT_DOUBLE,
    COT_ENUM,
    COT_RGB888,
} CONFIG_OPTION_TYPE;

typedef struct {
    const char *name;
    CONFIG_OPTION_TYPE type;
    const void *target;
    const void *default_value;
    const void *param;
} CONFIG_OPTION;
