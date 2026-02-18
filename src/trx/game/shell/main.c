#include <trx/core/filesystem.h>
#include <trx/core/log.h>
#include <trx/core/memory.h>
#include <trx/core/strings.h>
#include <trx/core/utils.h>
#include <trx/game/shell.h>
#include <trx/version.h>

#include <string.h>

int main(int argc, char *argv[])
{
    VECTOR *raw_args = Vector_Create(sizeof(const char *));
    for (int32_t i = 1; i < argc; i++) {
        char *const copied_arg = Memory_DupStr(argv[i]);
        Vector_Add(raw_args, &copied_arg);
    }

    TRXPath_Init(nullptr);
    Shell_ScanAvailableMods();
    SHELL_ARGS *args = Shell_ParseArgs(raw_args);
    if (args == nullptr) {
        return 0;
    }

    TRXPath_Init(args);

    char *log_path = String_Format("%s/TRX.log", TRXPath_Get(TRX_PATH_TRX_DIR));
    Log_Init(log_path, args->quiet ? LOG_LEVEL_WARNING : LOG_LEVEL_MAX);
    Memory_FreePointer(&log_path);

    LOG_INFO("Starting %s", g_TRXVersion);
    const int32_t exit_code = Shell_Main(args);

    Shell_Terminate(exit_code);
    return exit_code;
}
