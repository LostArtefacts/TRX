#include <harness/harness.h>

#include <trx/core/file.h>
#include <trx/game/inject.h>
#include <trx/game/pathing.h>

#define M_BOX_COUNT 2

static BOX_INFO m_Boxes[M_BOX_COUNT];
static void (*m_Handler)(const INJECTION_CONTEXT *, INJECTION_CHUNK) = nullptr;

static void M_Put32(char **const at, const uint32_t value)
{
    for (int32_t i = 0; i < 4; i++) {
        *(*at)++ = (value >> (i * 8)) & 0xFF;
    }
}

static int32_t M_WriteBlock(
    char *const out, const int32_t count, const uint32_t *const indices)
{
    char *at = out;
    M_Put32(&at, IDT_OVERLAP_INDICES);
    M_Put32(&at, count);
    M_Put32(&at, count * sizeof(uint32_t));
    for (int32_t i = 0; i < count; i++) {
        M_Put32(&at, indices[i]);
    }
    return at - out;
}

static TRX_FILE *M_Run(
    const INJECTION_MODE mode, const char *const data, const int32_t size)
{
    TRX_FILE *const file = File_OpenBuffer(data, size);
    const INJECTION injection = { .fp = file };
    const INJECTION_CONTEXT ctx = { .mode = mode };
    m_Handler(
        &ctx,
        (INJECTION_CHUNK) {
            .injection = &injection,
            .type = ICT_PATHING_DATA,
            .version = 1,
            .num_blocks = 1,
            .total_size = size,
        });
    return file;
}

static void M_ResetBoxes(void)
{
    m_Boxes[0].overlap_index = BOX_BLOCKABLE | BOX_BLOCKED | 0x3FFF;
    m_Boxes[1].overlap_index = 0x1234;
}

void Inject_RegisterHandler(
    const INJECTION_CHUNK_TYPE type,
    void (*const handle_func)(const INJECTION_CONTEXT *, INJECTION_CHUNK))
{
    if (type == ICT_PATHING_DATA) {
        m_Handler = handle_func;
    }
}

int32_t Box_GetCount(void)
{
    return M_BOX_COUNT;
}

BOX_INFO *Box_GetBox(const int32_t box_idx)
{
    return box_idx < 0 || box_idx >= M_BOX_COUNT ? nullptr : &m_Boxes[box_idx];
}

TEST(handler_is_registered)
{
    CHECK_NOT_NULL(m_Handler);
}

TEST(overlap_indices_widen_and_keep_the_blocking_flags)
{
    M_ResetBoxes();
    const uint32_t indices[M_BOX_COUNT] = { 0x12345, BOX_OVERLAP_BITS };
    char data[64];
    const int32_t size = M_WriteBlock(data, M_BOX_COUNT, indices);

    TRX_FILE *const file = M_Run(INJECTION_MODE_FULL, data, size);
    CHECK_EQ_INT(File_BytesLeft(file), 0);
    File_Close(file);

    CHECK_EQ_INT(
        m_Boxes[0].overlap_index, BOX_BLOCKABLE | BOX_BLOCKED | 0x12345);
    CHECK_EQ_INT(m_Boxes[1].overlap_index, BOX_OVERLAP_BITS);
}

TEST(overlap_indices_beyond_the_level_boxes_are_read_past)
{
    M_ResetBoxes();
    const uint32_t indices[M_BOX_COUNT + 1] = { 0x4000, 0x4001, 0x4002 };
    char data[64];
    const int32_t size = M_WriteBlock(data, M_BOX_COUNT + 1, indices);

    TRX_FILE *const file = M_Run(INJECTION_MODE_FULL, data, size);
    CHECK_EQ_INT(File_BytesLeft(file), 0);
    File_Close(file);

    CHECK_EQ_INT(
        m_Boxes[0].overlap_index, BOX_BLOCKABLE | BOX_BLOCKED | 0x4000);
    CHECK_EQ_INT(m_Boxes[1].overlap_index, 0x4001);
}

TEST(overlap_indices_are_skipped_when_gathering_stats)
{
    M_ResetBoxes();
    const uint32_t indices[M_BOX_COUNT] = { 0x12345, 0x23456 };
    char data[64];
    const int32_t size = M_WriteBlock(data, M_BOX_COUNT, indices);

    TRX_FILE *const file = M_Run(INJECTION_MODE_STATS, data, size);
    CHECK_EQ_INT(File_BytesLeft(file), 0);
    File_Close(file);

    CHECK_EQ_INT(
        m_Boxes[0].overlap_index, BOX_BLOCKABLE | BOX_BLOCKED | 0x3FFF);
    CHECK_EQ_INT(m_Boxes[1].overlap_index, 0x1234);
}
