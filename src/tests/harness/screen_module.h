#pragma once

// Runs src/tests/lua/<tests>.lua against the shipped modules that draw engine
// screens. Every test of such a module calls this from its main().
int ScreenModule_Run(const char *tests);
