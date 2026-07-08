#pragma once

#include <stddef.h>

char *CSV_Trim(char *value);
const char *CSV_SkipWhitespace(const char *value);
void CSV_ParseField(const char **cursor, char *out, size_t out_size);
