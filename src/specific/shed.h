#ifndef T1M_SPECIFIC_SHED_H
#define T1M_SPECIFIC_SHED_H

// a place for odd functions that have no place to go yet

// clang-format off
#define WinVidSpinMessageLoop   ((void          __cdecl(*)())0x00437AD0)
#define ShowFatalError          ((void          __cdecl(*)(const char *message))0x0043D770)
#define S_ExitSystem            ((void          __cdecl(*)(const char *message))0x0041E260)
#define WriteTombAtiSettings    ((void          __cdecl(*)())0x00438B60)
#define sub_4380E0              ((void          __cdecl(*)(int16_t *unk))0x004380E0)
#define InitialiseHardware      ((void          __cdecl(*)())0x00408005)
// clang-format on

#endif
