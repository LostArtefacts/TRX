#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <windows.h>

// The path to the legitimate host process
const char *m_HostProcessPath = "Tomb2.exe";

static bool M_FileExists(const char *path)
{
    DWORD fileAttributes = GetFileAttributes(path);
    if (fileAttributes != INVALID_FILE_ATTRIBUTES
        && !(fileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
        return true;
    }
    return false;
}

static bool M_InjectDLL(HANDLE process_handle, const char *dll_path)
{
    bool success = false;
    LPVOID load_library_addr =
        (LPVOID)GetProcAddress(GetModuleHandle("kernel32.dll"), "LoadLibraryA");

    fprintf(stderr, "Injecting %s\n", dll_path);

    if (!M_FileExists(dll_path)) {
        fprintf(stderr, "DLL does not exist.\n");
        goto finish;
    }

    LPVOID dll_path_adr = VirtualAllocEx(
        process_handle, NULL, strlen(dll_path) + 1, MEM_RESERVE | MEM_COMMIT,
        PAGE_READWRITE);
    if (!dll_path_adr) {
        fprintf(stderr, "Failed to allocate remote memory.\n");
        goto finish;
    }

    if (!WriteProcessMemory(
            process_handle, dll_path_adr, dll_path, strlen(dll_path) + 1,
            NULL)) {
        fprintf(stderr, "Failed to write remote memory.\n");
        goto finish;
    }

    HANDLE remote_thread_handle = CreateRemoteThread(
        process_handle, NULL, 0, (LPTHREAD_START_ROUTINE)load_library_addr,
        dll_path_adr, 0, NULL);
    if (remote_thread_handle == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "Failed to create remote thread.\n");
        goto finish;
    }
    WaitForSingleObject(remote_thread_handle, INFINITE);

    VirtualFreeEx(
        process_handle, dll_path_adr, strlen(dll_path) + 1, MEM_RELEASE);
    CloseHandle(remote_thread_handle);

    success = true;
finish:
    return success;
}

const char *GetDLLPath(void)
{
    static char dll_path[MAX_PATH];
    GetModuleFileNameA(NULL, dll_path, MAX_PATH);
    char *suffix = strstr(dll_path, ".exe");
    if (suffix != NULL) {
        strcpy(suffix, ".dll");
    }
    return dll_path;
}

char *GetHostProcessArguments(
    const char *host_process_path, const int32_t argc, const char *const argv[])
{
    size_t length = 1; // null terminator
    for (int32_t i = 0; i < argc; i++) {
        if (i > 0) {
            length++;
        }

        const char *arg = i == 0 ? host_process_path : argv[i];
        if (strchr(arg, ' ')) {
            length += 1;
            length += strlen(arg);
            length += 1;
        } else {
            length += strlen(arg);
        }
    }

    char *cmdline = malloc(length);
    cmdline[0] = '\0';

    for (int32_t i = 0; i < argc; i++) {
        if (i > 0) {
            strcat(cmdline, " ");
        }

        const char *arg = i == 0 ? host_process_path : argv[i];
        if (strchr(arg, ' ')) {
            strcat(cmdline, "\"");
            strcat(cmdline, arg);
            strcat(cmdline, "\"");
        } else {
            strcat(cmdline, arg);
        }
    }

    return cmdline;
}

int32_t main(const int32_t argc, const char *const argv[])
{
    bool success = false;
    const char *dll_path = GetDLLPath();
    char *cmdline = GetHostProcessArguments(m_HostProcessPath, argc, argv);

    STARTUPINFO si;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);

    PROCESS_INFORMATION pi;
    ZeroMemory(&pi, sizeof(pi));

    if (!CreateProcess(
            m_HostProcessPath, cmdline, NULL, NULL, FALSE, CREATE_SUSPENDED,
            NULL, NULL, &si, &pi)) {
        fprintf(stderr, "Failed to create the process.\n");
        goto finish;
    }

    if (!M_InjectDLL(pi.hProcess, dll_path)) {
        fprintf(stderr, "Failed to inject the DLL.\n");
        goto finish;
    }

    if (ResumeThread(pi.hThread) == (DWORD)-1) {
        fprintf(stderr, "Failed to resume the execution of the process.\n");
        goto finish;
    }

    success = true;

finish:
    if (cmdline) {
        free(cmdline);
        cmdline = NULL;
    }
    if (pi.hThread) {
        CloseHandle(pi.hThread);
    }
    if (pi.hProcess) {
        CloseHandle(pi.hProcess);
    }
    return success ? 0 : 1;
}
