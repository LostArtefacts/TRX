#pragma once

// Returns the executable's directory, or null when the platform cannot report
// it. The caller owns the returned string.
char *Shell_GetBasePath(void);

void Shell_ExitSystem(const char *message);
void Shell_ExitSystemEx(const char *log_message, const char *dialog_message);
void Shell_ExitSystemFmt(const char *fmt, ...);
