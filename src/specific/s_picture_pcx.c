#include "specific/s_picture.h"

#include "filesystem.h"
#include "log.h"
#include "memory.h"

static bool DecompPCX(const char *pcx, size_t pcx_size, PICTURE *picture);

static bool DecompPCX(const char *pcx, size_t pcx_size, PICTURE *picture)
{
    PCX_HEADER *header = (PCX_HEADER *)pcx;
    picture->width = header->x_max - header->x_min + 1;
    picture->height = header->y_max - header->y_min + 1;

    if (header->manufacturer != 10 || header->version < 5 || header->bpp != 8
        || header->rle != 1 || header->planes != 1 || !picture->width
        || !picture->height) {
        return false;
    }

    picture->data =
        Memory_Alloc(picture->width * picture->height * sizeof(RGB888));

    RGB888 pal[256];
    {
        const uint8_t *src = (uint8_t *)pcx + pcx_size - sizeof(RGB888) * 256;
        for (int i = 0; i < 256; i++) {
            pal[i].r = ((*src++) >> 2) & 0x3F;
            pal[i].g = ((*src++) >> 2) & 0x3F;
            pal[i].b = ((*src++) >> 2) & 0x3F;
        }
    }

    {
        const int32_t stride = picture->width + picture->width % 2;
        const uint8_t *src = (uint8_t *)pcx + sizeof(PCX_HEADER);
        RGB888 *dst = picture->data;
        int32_t x = 0;
        int32_t y = 0;

        while (y < picture->height) {
            if ((*src & 0xC0) == 0xC0) {
                uint8_t n = (*src++) & 0x3F;
                uint8_t c = *src++;
                if (n > 0) {
                    if (x < picture->width) {
                        CLAMPG(n, picture->width - x);
                        for (int i = 0; i < n; i++) {
                            *dst++ = pal[c];
                        }
                    }
                    x += n;
                }
            } else {
                *dst++ = pal[*src++];
                x++;
            }
            if (x >= stride) {
                x = 0;
                y++;
            }
        }
    }

    return true;
}

bool S_Picture_LoadFromFile(PICTURE *picture, const char *file_path)
{
    char *file_data = NULL;
    size_t file_size = 0;
    if (!File_Load(file_path, &file_data, &file_size)) {
        LOG_ERROR("Error while opening PCX file %s", file_path);
        return false;
    }

    if (!DecompPCX(file_data, file_size, picture)) {
        LOG_ERROR("Error while decompressing PCX data for file %s", file_path);
        return false;
    }

    Memory_Free(file_data);

    return true;
}
