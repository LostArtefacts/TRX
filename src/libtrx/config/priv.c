#if TR_VERSION == 1
    #include "priv_tr1.c"
#elif TR_VERSION == 2
    #include "priv_tr2.c"
#endif

void Config_SanitizeCommon(void)
{
    CLAMP(g_Config.audio.music_volume, 0.0f, 1.0f);
    CLAMP(g_Config.audio.sound_volume, 0.0f, 1.0f);
    CLAMP(g_Config.audio.inventory_music_volume, 0.0f, 1.0f);
    CLAMP(g_Config.audio.inventory_ambient_volume, 0.0f, 1.0f);
    CLAMP(g_Config.audio.underwater_music_volume, 0.0f, 1.0f);
    CLAMP(g_Config.audio.underwater_ambient_volume, 0.0f, 1.0f);
}
