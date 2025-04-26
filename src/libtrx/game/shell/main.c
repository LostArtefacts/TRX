#include "filesystem.h"
#include "game/shell.h"
#include "log.h"
#include "memory.h"
#include "utils.h"

#include <string.h>

int main(int argc, char *argv[])
{
    if (!Shell_ParseArgs(argc, (const char **)argv)) {
        return 0;
    }

    char *log_path = File_GetFullPath(PROJECT_NAME ".log");
    Log_Init(log_path);
    Memory_FreePointer(&log_path);

    Shell_Setup();
    int32_t exit_code = Shell_Main();
    Shell_Terminate(exit_code);
    return exit_code;
}
