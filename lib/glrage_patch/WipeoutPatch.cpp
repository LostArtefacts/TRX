#include "WipeoutPatch.hpp"

#include <glrage_util/Config.hpp>
#include <glrage_util/Logger.hpp>

namespace glrage {

static const int64_t build_301096 = 840143410;
static const int64_t build_220796 = 837628019;
static const int64_t build_070796 = 836749807;

GameID WipeoutPatch::gameID()
{
    return GameID::Wipeout;
}

void WipeoutPatch::apply()
{
    // There are currently three known builds, which require different patches
    int64_t buildDate = static_cast<int64_t>(m_ctx.fileInfo.dwFileDateMS) << 32;
    buildDate |= m_ctx.fileInfo.dwFileDateLS;

    // Wipeout 95 apparently has a bug in the keyboard handling, which sometimes
    // produces random crashes when a key is pressed for a prolonged period of
    // time.
    // The devs decided it would be a good idea to tamper with the system
    // settings to reduce the key repeat rate to a minimum in order to "fix"
    // this bug. The screensaver is also disabled for some unrelated reason.
    // Unfortunately, these custom settings are not restored correctly when the
    // game crashes or otherwise exits unexpectedly, much to the annoyance of
    // the user.
    // This patch replaces the SystemParametersInfo function with a stub to
    // disable that behavior.
    if (buildDate == build_301096) {
        patch(0x7E0290, &hookSystemParametersInfoA);
    } else {
        patch(0x7D0290, &hookSystemParametersInfoA);
    }

    // Disable bugged alt+tab check that causes a segfault after playing the
    // into video on Windows 10 and possibly older versions as well.
    // GLRage doesn't really need it anyway and the only side effect is that the
    // game continues to run in the background when tabbed out.
    switch (buildDate) {
        case build_070796:
            patchNop(0x458825, "89 2D 7C E3 69 00");
            break;
        case build_220796:
            patchNop(0x458785, "89 0D 7C E3 69 00");
            break;
        case build_301096:
            patchNop(0x46AFAD, "C7 05 8B DE 6A 00 00 00 00 00");
            break;
    }

    // Disable unskippable title screen, saves a few seconds of wait time.
    if (m_config.getBool("patch.disable_title_screen", false)) {
        switch (buildDate) {
            case build_070796:
                patchNop(0x459087, "E8 F4 34 00 00");
                break;
            case build_220796:
                patchNop(0x458FF3, "E8 F4 34 00 00");
                break;
            case build_301096:
                patchNop(0x46B885, "E8 B8 40 00 00");
                break;
        }
    }

    // Disable introductory video.
    if (m_config.getBool("patch.disable_introductory_video", false)) {
        switch (buildDate) {
            case 1:
                patchNop(0x45902E, "E8 C5 39 00 00");
                break;
            case build_220796:
                patchNop(0x458F9A, "E8 C5 39 00 00");
                break;
            case build_301096:
                patchNop(0x46B808, "E8 33 46 00 00");
                break;
        }
    }
}

BOOL WINAPI WipeoutPatch::hookSystemParametersInfoA(
    UINT uiAction, UINT uiParam, PVOID pvParam, UINT fWinIni)
{
    // do nothing
    return TRUE;
}

} // namespace glrage
