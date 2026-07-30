#pragma once

#include <stdint.h>

void FakeStats_SetSecrets(const int32_t *nums, int32_t count);
void FakeStats_SetFound(int32_t num, bool found);
void FakeStats_SetMaxSecretCount(int32_t count);
