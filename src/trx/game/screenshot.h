#pragma once

typedef enum {
    SCREENSHOT_FORMAT_JPEG,
    SCREENSHOT_FORMAT_PNG,
} SCREENSHOT_FORMAT;

void Screenshot_Make(SCREENSHOT_FORMAT format);
void Screenshot_MakeToPath(const char *path);
