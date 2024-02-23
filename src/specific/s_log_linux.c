#include "specific/s_log.h"

#include "log.h"

#include <backtrace-supported.h>
#include <backtrace.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

static void S_Log_ErrorCallback(void *data, const char *msg, int errnum);
static int S_Log_FullTrace(
    void *data, uintptr_t pc, const char *filename, int lineno,
    const char *function);
static void S_Log_SignalHandler(int sig);

static void S_Log_ErrorCallback(void *data, const char *msg, int errnum)
{
    LOG_ERROR("%s", msg);
}

static int S_Log_FullTrace(
    void *data, uintptr_t pc, const char *filename, int lineno,
    const char *function)
{
    if (filename) {
        LOG_INFO(
            "0x%08X: %s (%s:%d)", pc, function ? function : "???",
            filename ? filename : "???", lineno);
    } else {
        LOG_INFO("0x%08X: %s", pc, function ? function : "???");
    }
    return 0;
}

static void S_Log_SignalHandler(int sig)
{
    LOG_ERROR("== CRASH REPORT ==");
    LOG_INFO("SIGNAL: %d", sig);
    LOG_INFO("STACK TRACE:");
    struct backtrace_state *state = backtrace_create_state(
        NULL, BACKTRACE_SUPPORTS_THREADS, S_Log_ErrorCallback, NULL);
    backtrace_full(state, 0, S_Log_FullTrace, S_Log_ErrorCallback, NULL);
    exit(EXIT_FAILURE);
}

void S_Log_Init(void)
{
    signal(SIGSEGV, S_Log_SignalHandler);
    signal(SIGFPE, S_Log_SignalHandler);
}
