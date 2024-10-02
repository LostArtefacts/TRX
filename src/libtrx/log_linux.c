#include "log.h"

#include <backtrace-supported.h>
#include <backtrace.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

static void M_ErrorCallback(void *data, const char *msg, int errnum);
static int M_StackTrace(
    void *data, uintptr_t pc, const char *filename, int lineno,
    const char *func_name);
static void M_SignalHandler(int sig);

static void M_ErrorCallback(void *data, const char *msg, int errnum)
{
    LOG_ERROR("%s", msg);
}

static int M_StackTrace(
    void *data, uintptr_t pc, const char *filename, int lineno,
    const char *func_name)
{
    if (filename) {
        LOG_INFO(
            "0x%08X: %s (%s:%d)", pc, func_name ? func_name : "???",
            filename ? filename : "???", lineno);
    } else {
        LOG_INFO("0x%08X: %s", pc, func_name ? func_name : "???");
    }
    return 0;
}

static void M_SignalHandler(int sig)
{
    LOG_ERROR("== CRASH REPORT ==");
    LOG_INFO("SIGNAL: %d", sig);
    LOG_INFO("STACK TRACE:");
    struct backtrace_state *state = backtrace_create_state(
        NULL, BACKTRACE_SUPPORTS_THREADS, M_ErrorCallback, NULL);
    backtrace_full(state, 0, M_StackTrace, M_ErrorCallback, NULL);
    exit(EXIT_FAILURE);
}

void Log_Init_Extra(const char *path)
{
    signal(SIGSEGV, M_SignalHandler);
    signal(SIGFPE, M_SignalHandler);
}

void Log_Shutdown_Extra(void)
{
}
