#include "game/inject.h"
#include "game/output/const.h"
#include "log.h"
#include "memory.h"

static void M_TextureEdits(const INJECTION *injection, int32_t data_count);

static void M_TextureEdits(
    const INJECTION *const injection, const int32_t data_count)
{
    const LEVEL_INFO *const level_info = Level_GetInfo();
    for (int32_t i = 0; i < data_count; i++) {
        const uint16_t target_page = VFile_ReadU16(injection->fp);
        const uint8_t target_x = VFile_ReadU8(injection->fp);
        const uint8_t target_y = VFile_ReadU8(injection->fp);
        const uint16_t source_width = VFile_ReadU16(injection->fp);
        const uint16_t source_height = VFile_ReadU16(injection->fp);

        const int32_t size = source_width * source_height * sizeof(RGBA_8888);
        RGBA_8888 *source_img = Memory_Alloc(size);
        VFile_Read(injection->fp, source_img, size);

        if (target_page >= level_info->textures.page_count) {
            LOG_WARNING("Texture page %d is beyond level range", target_page);
            continue;
        }

        RGBA_8888 *page =
            &level_info->textures.pages_32[target_page * TEXTURE_PAGE_SIZE];
        for (int32_t y = 0; y < source_height; y++) {
            for (int32_t x = 0; x < source_width; x++) {
                const int32_t target_pixel =
                    (y + target_y) * TEXTURE_PAGE_WIDTH + x + target_x;
                RGBA_8888 *const target_rgb = &page[target_pixel];
                *target_rgb = source_img[y * source_width + x];
            }
        }

        Memory_FreePointer(&source_img);
    }
}

REGISTER_INJECT_EDITOR(IDT_TEXTURE_EDITS, M_TextureEdits)
