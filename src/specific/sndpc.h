#ifndef T1M_SPECIFIC_SNDPC_H
#define T1M_SPECIFIC_SNDPC_H

// clang-format off
#define S_CDLoop                ((void         (*)())0x004380B0)
#define S_CDStop                ((void         (*)())0x00438E40)
#define S_CDVolume              ((void         (*)(int16_t volume))0x00437F30)
#define SoundStart              ((void         (*)())0x0041CDA0)
#define SoundInit               ((int32_t      (*)())0x00437E00)
// clang-format on

#endif
