#ifndef T1M_SPECIFIC_SMAIN_H
#define T1M_SPECIFIC_SMAIN_H

void TerminateGame(int exit_code);
void TerminateGameWithMsg(const char *fmt, ...);
void ShowFatalError(const char *message);

void T1MInjectSpecificSMain();

#endif
