#include "game/anim.h"

#include <stddef.h>

bool Anim_TestAbsFrameEqual(int16_t abs_frame, int16_t frame)
{
    return abs_frame == frame;
}

bool Anim_TestAbsFrameRange(int16_t abs_frame, int16_t start, int16_t end)
{
    return abs_frame >= start && abs_frame <= end;
}
