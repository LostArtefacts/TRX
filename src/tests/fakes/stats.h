#pragma once

#include <stdint.h>

void FakeStats_SetSecrets(const int32_t *nums, int32_t count);
void FakeStats_SetFound(int32_t num, bool found);
void FakeStats_SetMaxSecretCount(int32_t count);

// The category setters name the level rather than acting on the one being
// played, which is what a test reading another level's counters needs.
void FakeStats_SetCount(int32_t level_num, int32_t id, int32_t count);
void FakeStats_SetMax(int32_t level_num, int32_t id, int32_t max);
void FakeStats_SetUnobtainable(int32_t level_num, int32_t id, int32_t count);
void FakeStats_SetKillSplit(int32_t level_num, int32_t allies, int32_t enemies);
void FakeStats_SetAlliesHurt(int32_t level_num, bool hurt);
