#ifndef TR1MAIN_SPECIFIC_SHED_H
#define TR1MAIN_SPECIFIC_SHED_H

// a place for odd functions that have no place to go yet

// clang-format off
#define WinVidSpinMessageLoop   ((void          __cdecl(*)())0x00437AD0)
#define ShowFatalError          ((void          __cdecl(*)(const char *message))0x0043D770)
#define S_ExitSystem            ((void          __cdecl(*)(const char *message))0x0041E260)
#define WriteTombAtiSettings    ((void          __cdecl(*)())0x00438B60)
// clang-format on

#endif
