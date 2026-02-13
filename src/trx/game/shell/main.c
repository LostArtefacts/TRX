#include <trx/filesystem.h>
#include <trx/game/shell.h>
#include <trx/log.h>
#include <trx/memory.h>
#include <trx/utils.h>
#include <trx/version.h>

#include <string.h>

int main(int argc, char *argv[])
{
    VECTOR *raw_args = Vector_Create(sizeof(const char *));
    for (int32_t i = 1; i < argc; i++) {
        Vector_Add(raw_args, &argv[i]);
    }

    Shell_ScanAvailableMods();
    SHELL_ARGS *args = Shell_ParseArgs(raw_args);
    if (args == nullptr) {
        Vector_Free(raw_args);
        return 0;
    }

    char *log_path = File_GetFullPath("TRX.log");
    Log_Init(log_path, args->quiet ? LOG_LEVEL_WARNING : LOG_LEVEL_MAX);
    Memory_FreePointer(&log_path);

    LOG_INFO("Starting %s", g_TRXVersion);
    int32_t exit_code = Shell_Main(args);
    Memory_FreePointer(&args);
    Vector_Free(raw_args);
    Shell_Terminate(exit_code);
    return exit_code;
}
