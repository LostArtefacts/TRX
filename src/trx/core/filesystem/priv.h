#pragma once

#include <stdio.h>

// Open a file with fopen, taking a UTF-8 path on every platform. Windows
// needs a wide path, so this converts it first.
FILE *File_PlatformFopen(const char *path, const char *mode);
