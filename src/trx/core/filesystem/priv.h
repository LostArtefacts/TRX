#pragma once

#include <stdio.h>

// fopen that takes a UTF-8 path on every platform, including the one whose
// narrow fopen does not.
FILE *File_PlatformFopen(const char *path, const char *mode);
