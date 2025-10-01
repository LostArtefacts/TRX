#include "game/packer.h"

#include "benchmark.h"
#include "debug.h"
#include "game/output.h"
#include "log.h"
#include "memory.h"
#include "utils.h"

#include <string.h>

typedef enum {
    RC_EQUALS = 0,
    RC_CONTAINS = 1,
    RC_COVERS = 2,
    RC_UNRELATED = 3
} RECTANGLE_COMPARISON;

typedef struct {
    uint16_t x;
    uint16_t y;
    uint16_t w;
    uint16_t h;
} RECTANGLE;

typedef struct {
    uint16_t page;
    uint16_t x;
    uint16_t y;
} TEX_POS;

typedef struct {
    uint16_t index;
    uint16_t page;
    RECTANGLE bounds;
    void (*move)(int32_t index, RECTANGLE old_bounds, TEX_POS new_pos);
} TEX_INFO;

typedef struct {
    int32_t size;
    RECTANGLE bounds;
    TEX_INFO *tex_infos;
} TEX_CONTAINER;

typedef struct {
    int32_t index;
    int32_t free_space;
    uint8_t data[TEXTURE_PAGE_SIZE];
} TEX_PAGE;

static PACKER_DATA *m_Data = nullptr;
static uint8_t m_PaletteLUT[256];
static int32_t m_StartPage = 0;
static int32_t m_EndPage = 0;
static int32_t m_UsedPageCount = 0;
static TEX_PAGE *m_VirtualPages = nullptr;
static int32_t m_QueueSize = 0;
static TEX_CONTAINER *m_Queue = nullptr;

static void M_PreparePaletteLUT(void)
{
    if (m_Data->level.pages_24 == nullptr) {
        return;
    }

    ASSERT(m_Data->source.palette_24 != nullptr);
    ASSERT(m_Data->level.palette_24 != nullptr);

    m_PaletteLUT[0] = 0;
    for (int32_t i = 1; i < 256; i++) {
        const RGB_888 colour = m_Data->source.palette_24[i];
        int32_t best_idx = 0;
        int32_t best_diff = INT32_MAX;
        for (int32_t j = 1; j < 256; j++) {
            const int32_t dr = colour.r - m_Data->level.palette_24[j].r;
            const int32_t dg = colour.g - m_Data->level.palette_24[j].g;
            const int32_t db = colour.b - m_Data->level.palette_24[j].b;
            const int32_t diff = SQUARE(dr) + SQUARE(dg) + SQUARE(db);
            if (diff < best_diff) {
                best_diff = diff;
                best_idx = j;
            }
        }

        m_PaletteLUT[i] = (uint8_t)best_idx;
    }
}

static RECTANGLE M_GetObjectBounds(const OBJECT_TEXTURE *const texture)
{
    int32_t min_u = 0xFF, min_v = 0xFF;
    int32_t max_u = 0, max_v = 0;

    for (int32_t i = 0; i < 4; i++) {
        if (texture->uv[i].u == 0 && texture->uv[i].v == 0) {
            // This is a dummy vertex for a triangle.
            continue;
        }

        const int32_t u = (texture->uv[i].u & 0xFF00) >> 8;
        const int32_t v = (texture->uv[i].v & 0xFF00) >> 8;
        min_u = MIN(min_u, u);
        max_u = MAX(max_u, u);
        min_v = MIN(min_v, v);
        max_v = MAX(max_v, v);
    }

    return (RECTANGLE) {
        .x = min_u,
        .y = min_v,
        .w = max_u - min_u + 1,
        .h = max_v - min_v + 1,
    };
}

static RECTANGLE M_GetSpriteBounds(const SPRITE_TEXTURE *const texture)
{
    return (RECTANGLE) {
        .x = texture->offset & 0xFF,
        .y = (texture->offset & 0xFF00) >> 8,
        .w = (texture->width + 1) / TEXTURE_PAGE_WIDTH,
        .h = (texture->height + 1) / TEXTURE_PAGE_HEIGHT,
    };
}

static void M_FillVirtualData(TEX_PAGE *const page, const RECTANGLE bounds)
{
    const int32_t y_end = bounds.y + bounds.h;
    const int32_t x_end = bounds.x + bounds.w;
    for (int32_t y = bounds.y; y < y_end; y++) {
        for (int32_t x = bounds.x; x < x_end; x++) {
            page->data[y * TEXTURE_PAGE_WIDTH + x] = 1;
        }
    }

    page->free_space -= bounds.w * bounds.h;
}

static void M_MoveObject(
    const int32_t index, const RECTANGLE old_bounds, const TEX_POS new_pos)
{
    OBJECT_TEXTURE *const texture = Output_GetObjectTexture(index);
    texture->tex_page = new_pos.page;

    int32_t x_diff = (new_pos.x - old_bounds.x) << 8;
    int32_t y_diff = (new_pos.y - old_bounds.y) << 8;
    uint16_t u, v;
    for (int32_t i = 0; i < 4; i++) {
        u = texture->uv[i].u;
        v = texture->uv[i].v;
        if (u == 0 && v == 0) {
            // This is a dummy vertex for a triangle.
            continue;
        }
        texture->uv[i].u = (x_diff + (u & 0xFF00)) | (u & 0xFF);
        texture->uv[i].v = (y_diff + (v & 0xFF00)) | (v & 0xFF);
    }
}

static void M_MoveSprite(
    const int32_t index, const RECTANGLE old_bounds, const TEX_POS new_pos)
{
    SPRITE_TEXTURE *const texture = Output_GetSpriteTexture(index);
    texture->tex_page = new_pos.page;
    texture->offset = (new_pos.y << 8) | new_pos.x;
}

static RECTANGLE_COMPARISON M_Compare(const RECTANGLE r1, const RECTANGLE r2)
{
    if (r1.x == r2.x && r1.w == r2.w && r1.y == r2.y && r1.h == r2.h) {
        return RC_EQUALS;
    }

    if (r1.x <= r2.x && r2.x + r2.w <= r1.x + r1.w && r1.y <= r2.y
        && r2.y + r2.h <= r1.y + r1.h) {
        return RC_CONTAINS;
    }

    if (r2.x <= r1.x && r1.x + r1.w <= r2.x + r2.w && r2.y <= r1.y
        && r1.y + r1.h <= r2.y + r2.h) {
        return RC_COVERS;
    }

    return RC_UNRELATED;
}

static bool M_EnqueueTexInfo(TEX_INFO *const info)
{
    // This may be a child of another, so try to find its
    // parent first and add it there.
    if (m_Queue != nullptr) {
        for (int32_t i = 0; i < m_QueueSize; i++) {
            TEX_CONTAINER *const container = &m_Queue[i];
            if (container->tex_infos->page != info->page) {
                continue;
            }

            const RECTANGLE_COMPARISON comparison =
                M_Compare(container->bounds, info->bounds);
            if (comparison == RC_UNRELATED) {
                continue;
            }

            container->tex_infos = Memory_Realloc(
                container->tex_infos, sizeof(TEX_INFO) * (container->size + 1));
            container->tex_infos[container->size++] = *info;

            if (comparison == RC_COVERS) {
                // This is now the largest item in this container.
                container->bounds = info->bounds;
            }

            // Mark the item as no longer used and eligible for freeing.
            return false;
        }
    }

    // This doesn't have a parent, so make a new container.
    m_Queue =
        Memory_Realloc(m_Queue, sizeof(TEX_CONTAINER) * (m_QueueSize + 1));
    TEX_CONTAINER *const new_container = &m_Queue[m_QueueSize++];
    new_container->size = 1;
    new_container->bounds = info->bounds;
    new_container->tex_infos = info;
    return true;
}

static void M_PrepareObject(const int32_t object_index)
{
    const OBJECT_TEXTURE *const object_texture =
        Output_GetObjectTexture(object_index);
    if (object_texture->tex_page == m_StartPage) {
        const RECTANGLE bounds = M_GetObjectBounds(object_texture);
        M_FillVirtualData(m_VirtualPages, bounds);
    } else if (object_texture->tex_page > m_StartPage) {
        TEX_INFO *info = Memory_Alloc(sizeof(TEX_INFO));
        info->index = object_index;
        info->page = object_texture->tex_page;
        info->bounds = M_GetObjectBounds(object_texture);
        info->move = M_MoveObject;
        if (!M_EnqueueTexInfo(info)) {
            Memory_FreePointer(&info);
        }
    }
}

static void M_PrepareSprite(const int32_t sprite_index)
{
    const SPRITE_TEXTURE *const sprite_texture =
        Output_GetSpriteTexture(sprite_index);
    if (sprite_texture->tex_page == m_StartPage) {
        const RECTANGLE bounds = M_GetSpriteBounds(sprite_texture);
        M_FillVirtualData(m_VirtualPages, bounds);
    } else if (sprite_texture->tex_page > m_StartPage) {
        TEX_INFO *info = Memory_Alloc(sizeof(TEX_INFO));
        info->index = sprite_index;
        info->page = sprite_texture->tex_page;
        info->bounds = M_GetSpriteBounds(sprite_texture);
        info->move = M_MoveSprite;
        if (!M_EnqueueTexInfo(info)) {
            Memory_FreePointer(&info);
        }
    }
}

static bool M_PackContainerAt(
    const TEX_CONTAINER *const container, TEX_PAGE *const page,
    const int32_t x_pos, const int32_t y_pos)
{
    const int32_t y_end = y_pos + container->bounds.h;
    const int32_t x_end = x_pos + container->bounds.w;

    for (int32_t y = y_pos; y < y_end; y++) {
        for (int32_t x = x_pos; x < x_end; x++) {
            if (page->data[y * TEXTURE_PAGE_WIDTH + x] != 0) {
                return false;
            }
        }
    }

    // There is adequate space at this position. Copy the pixel data from the
    // source texture page into the one identified, and fill the placeholder
    // data to avoid anything else taking this position.
    const int32_t source_page_index =
        container->tex_infos->page - m_Data->level.page_count;
    const RGBA_8888 *const source_page_32 =
        &m_Data->source.pages_32[source_page_index * TEXTURE_PAGE_SIZE];
    RGBA_8888 *const level_page_32 =
        &m_Data->level.pages_32[page->index * TEXTURE_PAGE_SIZE];

    const uint8_t *source_page_24 = nullptr;
    uint8_t *level_page_24 = nullptr;
    if (m_Data->level.pages_24 != nullptr) {
        source_page_24 =
            &m_Data->source.pages_24[source_page_index * TEXTURE_PAGE_SIZE];
        level_page_24 =
            &m_Data->level.pages_24[page->index * TEXTURE_PAGE_SIZE];
    }

    int32_t old_pixel, new_pixel;
    for (int32_t y = 0; y < container->bounds.h; y++) {
        for (int32_t x = 0; x < container->bounds.w; x++) {
            old_pixel = (container->bounds.y + y) * TEXTURE_PAGE_WIDTH
                + container->bounds.x + x;
            new_pixel = (y_pos + y) * TEXTURE_PAGE_WIDTH + x_pos + x;
            page->data[new_pixel] = 1;
            level_page_32[new_pixel] = source_page_32[old_pixel];
            if (level_page_24 != nullptr) {
                level_page_24[new_pixel] =
                    m_PaletteLUT[source_page_24[old_pixel]];
            }
        }
    }

    // Move each of the child tex_info coordinates accordingly.
    const TEX_POS new_pos = {
        .page = page->index,
        .x = x_pos,
        .y = y_pos,
    };
    for (int32_t i = 0; i < container->size; i++) {
        const TEX_INFO *const texture = &container->tex_infos[i];
        texture->move(texture->index, texture->bounds, new_pos);
    }

    return true;
}

static void M_AllocateNewPage(void)
{
    const int32_t used_count = m_UsedPageCount;
    m_UsedPageCount++;

    m_VirtualPages =
        Memory_Realloc(m_VirtualPages, sizeof(TEX_PAGE) * (used_count + 1));
    TEX_PAGE *const page = &m_VirtualPages[used_count];
    page->index = m_StartPage + used_count;
    page->free_space = TEXTURE_PAGE_SIZE;
    memset(page->data, 0, TEXTURE_PAGE_SIZE * sizeof(uint8_t));

    if (used_count == 0) {
        return;
    }

    const int32_t new_count = m_Data->level.page_count + used_count;

    {
        m_Data->level.pages_32 = Memory_Realloc(
            m_Data->level.pages_32,
            TEXTURE_PAGE_SIZE * new_count * sizeof(RGBA_8888));
        RGBA_8888 *const level_page =
            &m_Data->level.pages_32[(new_count - 1) * TEXTURE_PAGE_SIZE];
        memset(level_page, 0, TEXTURE_PAGE_SIZE * sizeof(RGBA_8888));
    }

    if (m_Data->level.pages_24 != nullptr) {
        m_Data->level.pages_24 = Memory_Realloc(
            m_Data->level.pages_24,
            TEXTURE_PAGE_SIZE * new_count * sizeof(uint8_t));
        uint8_t *const level_page =
            &m_Data->level.pages_24[(new_count - 1) * TEXTURE_PAGE_SIZE];
        memset(level_page, 0, TEXTURE_PAGE_SIZE * sizeof(uint8_t));
    }
}

static bool M_PackContainer(const TEX_CONTAINER *const container)
{
    const int32_t size = container->bounds.w * container->bounds.h;
    if (size > TEXTURE_PAGE_SIZE) {
        LOG_ERROR("Container is too large to pack");
        return false;
    }

    const int32_t y_end = TEXTURE_PAGE_HEIGHT - container->bounds.h;
    const int32_t x_end = TEXTURE_PAGE_WIDTH - container->bounds.w;

    for (int32_t i = 0; i < m_EndPage; i++) {
        if (i == m_UsedPageCount) {
            M_AllocateNewPage();
        }

        TEX_PAGE *const page = &m_VirtualPages[i];
        if (page->free_space < size) {
            continue;
        }

        for (int32_t y = 0; y <= y_end; y++) {
            for (int32_t x = 0; x <= x_end; x++) {
                if (M_PackContainerAt(container, page, x, y)) {
                    return true;
                }
            }
        }
    }

    LOG_ERROR("Texture page limit reached");
    return false;
}

static void M_Cleanup(void)
{
    for (int32_t i = 0; i < m_QueueSize; i++) {
        TEX_CONTAINER *container = &m_Queue[i];
        Memory_FreePointer(&container->tex_infos);
    }

    Memory_FreePointer(&m_VirtualPages);
    Memory_FreePointer(&m_Queue);
}

bool Packer_Pack(PACKER_DATA *const data)
{
    BENCHMARK benchmark = Benchmark_Start();
    m_Data = data;
    M_PreparePaletteLUT();

    m_StartPage = m_Data->level.page_count - 1;
    m_EndPage = MAX_TEXTURE_PAGES - m_StartPage;
    m_UsedPageCount = 0;
    m_QueueSize = 0;

    M_AllocateNewPage();

    for (int32_t i = 0; i < data->object_count; i++) {
        M_PrepareObject(i);
    }
    for (int32_t i = 0; i < data->sprite_count; i++) {
        M_PrepareSprite(i);
    }

    bool result = true;
    for (int32_t i = 0; i < m_QueueSize; i++) {
        const TEX_CONTAINER *const container = &m_Queue[i];
        if (!M_PackContainer(container)) {
            LOG_ERROR("Failed to pack container %d of %d", i, m_QueueSize);
            result = false;
            break;
        }
    }

    M_Cleanup();
    Benchmark_End(&benchmark, nullptr);
    return result;
}

int32_t Packer_GetAddedPageCount(void)
{
    return m_UsedPageCount - 1;
}
