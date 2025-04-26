#include "filesystem.h"
#include "game/shell.h"
#include "log.h"
#include "memory.h"
#include "utils.h"

#include <string.h>

static int m_ArgCount = 0;
static const char **m_ArgStrings = nullptr;

void Shell_GetCommandLine(int *arg_count, const char ***args)
{
    *arg_count = m_ArgCount;
    *args = m_ArgStrings;
}

int main(int argc, char *argv[])
{
    char *log_path = File_GetFullPath(PROJECT_NAME ".log");
    Log_Init(log_path);
    Memory_FreePointer(&log_path);

    m_ArgCount = argc;
    m_ArgStrings = (const char **)argv;

    Shell_Setup();
    int32_t exit_code = Shell_Main();
    Shell_Terminate(exit_code);
    return exit_code;
}
