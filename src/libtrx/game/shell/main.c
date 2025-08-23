#include "filesystem.h"
#include "game/shell.h"
#include "log.h"
#include "memory.h"
#include "utils.h"
#include "version.h"

#include <string.h>

int main(int argc, char *argv[])
{
    VECTOR *raw_args = Vector_Create(sizeof(const char *));
    for (int32_t i = 1; i < argc; i++) {
        Vector_Add(raw_args, &argv[i]);
    }
    SHELL_ARGS *const args = Shell_ParseArgs(raw_args);
    if (args == nullptr) {
        Vector_Free(raw_args);
        return 0;
    }

    char *log_path = File_GetFullPath(PROJECT_NAME ".log");
    Log_Init(log_path, args->quiet ? LOG_LEVEL_ERROR : LOG_LEVEL_MAX);
    Memory_FreePointer(&log_path);

    LOG_INFO("Starting %s", g_TRXVersion);
    int32_t exit_code = Shell_Main(args);
    Vector_Free(raw_args);
    Shell_Terminate(exit_code);
    return exit_code;
}
