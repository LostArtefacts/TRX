#pragma once

#include <stdbool.h>
#include <stdint.h>

bool String_Equivalent(const char *a, const char *b);
const char *String_CaseSubstring(const char *subject, const char *pattern);
int32_t String_Match(const char *subject, const char *pattern);
