#pragma once

#include <Windows.h>
#include <dsound.h>
#include <mmeapi.h>

#include <cstdint>
#include <map>
#include <vector>

namespace glrage {

class TombRaiderHooks
{
public:
    static void init(bool ub);
    static int32_t soundInit();
    static void soundBufferSetVolume(
        LPDIRECTSOUNDBUFFER buffer, int32_t volume);
    static void soundBufferSetPan(LPDIRECTSOUNDBUFFER buffer, int32_t pan);
    static LPDIRECTSOUNDBUFFER soundPlayOneShot(
        int32_t soundID, int32_t volume, int16_t pitch, uint16_t pan);
    static LPDIRECTSOUNDBUFFER soundPlayLoop(int32_t soundID, int32_t volume,
        int16_t pitch, uint16_t pan, int32_t a5, int32_t a6, int32_t a7);
    static void soundStopAll();
    static void soundUpdateVolume();
    static void soundSetVolume(uint8_t volume);
    static BOOL musicPlayRemap(int16_t trackID);
    static BOOL musicPlayLoop();
    static BOOL musicPlay(int16_t trackID);
    static BOOL musicStop();
    static BOOL musicSetVolume(int16_t volume);
    static LRESULT keyboardProc(int32_t nCode, WPARAM wParam, LPARAM lParam);
    static int32_t getPressedKey();
    static BOOL renderHealthBar(int32_t health);
    static BOOL renderAirBar(int32_t air);
    static BOOL renderCollectedItem(int32_t x, int32_t y, int32_t scale,
        int16_t itemID, int16_t brightness);
    static void* createFPSText(
        int16_t x, int16_t y, int16_t a3, const char* text);
    static int16_t setFOV(int16_t fov);
    static BOOL playFMV(int32_t fmvIndex, int32_t unknown);
    static void playAviFile(const char *path);

    // other vars
    static bool m_musicAlwaysLoop;

private:
    static LPDIRECTSOUNDBUFFER soundPlaySample(int32_t soundID, int32_t volume,
        int16_t pitch, uint16_t pan, bool loop);
    static int32_t convertPanToDecibel(uint16_t pan);
    static int32_t convertVolumeToDecibel(int32_t volume);
    static void renderBar(int32_t value, bool air);
    static int32_t getOverlayScale();
    static int32_t getOverlayScale(int32_t base);
};

} // namespace glrage