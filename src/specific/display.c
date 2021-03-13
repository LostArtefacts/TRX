#include "3dsystem/3d_gen.h"
#include "game/vars.h"
#include "specific/display.h"
#include "specific/shed.h"
#include <stdlib.h>

void SetupScreenSize()
{
    int32_t width = ((double)GameVidWidth * ScreenSizer);
    int32_t height = ((double)GameVidHeight * ScreenSizer);
    int32_t x = (GameVidWidth - width) / 2;
    int32_t y = (GameVidHeight - height) / 2;
    phd_InitWindow(
        x, y, width, height, 10, DPQ_END, GAME_FOV, GameVidWidth, GameVidHeight,
        ScrPtr);
    DumpX = x;
    DumpY = y;
    DumpWidth = width;
    DumpHeight = height;
    if (!BackScreenSize) {
        BackScreenSize = 640 * 480;
        BackScreen = malloc(BackScreenSize);
        if (!BackScreen) {
            S_ExitSystem("ERROR: Could not allocate enough memory to run (0)");
        }
    }
}

void T1MInjectSpecificDisplay()
{
    INJECT(0x00416470, SetupScreenSize);
}
