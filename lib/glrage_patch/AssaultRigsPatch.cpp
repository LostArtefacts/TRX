#include "AssaultRigsPatch.hpp"

#include <glrage/ContextImpl.hpp>
#include <glrage_util/Config.hpp>

#include <windows.h>

namespace glrage {

GameID AssaultRigsPatch::gameID()
{
    return GameID::AssaultRigs;
}

void AssaultRigsPatch::apply()
{
    // Fix "Insufficient memory" error on systems with more than 4 GB RAM where
    // GlobalMemoryStatus returns -1 (unless compatibility mode is activated,
    // which would break GLRage).
    patchNop(0x434B63, "0F 8E 32 01 00 00");

    // HD/widescreen resolution patch. Replaces 640x480, normally the maximum
    // resolution, with the current desktop resolution.
    if (m_config.getBool("patch.resolution_override", true)) {
        int32_t width = m_config.getInt("patch.resolution_width", -1);
        int32_t height = m_config.getInt("patch.resolution_height", -1);

        ContextImpl& ctx = ContextImpl::instance();

        if (width <= 0) {
            width = ctx.getScreenWidth();
        }

        if (height <= 0) {
            height = ctx.getScreenHeight();
        }

        auto c = Chunk();
        c = c << width;
        c = c << height;
        patch(0x490234, "80 02 00 00 E0 01 00 00", c);

        // Replace "640 BY 480" in the options with "CUSTOM".
        patch(0x486AC0, "36 34 30 20 42 59 20 34 38 30",
            "43 55 53 54 4F 4D 20 20 20 20");
    }

    // Disable re-initialization when losing window focus, which is unnecessary
    // and pretty annoying.
    patchNop(0x004342F8, "0F 86 59 F4 FF FF");
}

} // namespace glrage
