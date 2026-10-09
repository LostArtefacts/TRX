#pragma once

// Runs src/tests/lua/<tests>.lua against the shipped modules that draw engine
// screens. Every test of such a module calls this from its main().
int ScreenModule_Run(const char *tests);

// Runs the tests as ScreenModule_Run does, with more API modules loaded for a
// screen that reads them. The list is NULL-terminated.
int ScreenModule_RunWith(const char *tests, const char *const *extra_deps);
