#include <harness/harness.h>

#include <trx/core/file.h>
#include <trx/core/memory.h>

#include <stdio.h>
#include <string.h>

#define M_TEMP_PATH "file_handle_test.bin"

static const char m_Bytes[] = {
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08
};

static TRX_FILE *M_Open(const char *const path, const FILE_OPEN_MODE mode)
{
    TRX_FILE *file = nullptr;
    CHECK(IGNORE(File_OpenPath(path, mode, &file)));
    return file;
}

static TRX_FILE *M_Buffer(void)
{
    return File_OpenBuffer(m_Bytes, sizeof(m_Bytes));
}

static TRX_FILE *M_Disk(void)
{
    FILE *const fp = fopen(M_TEMP_PATH, "wb");
    CHECK_NOT_NULL(fp);
    fwrite(m_Bytes, 1, sizeof(m_Bytes), fp);
    fclose(fp);
    return M_Open(M_TEMP_PATH, FILE_OPEN_READ);
}

static void M_DropDisk(void)
{
    remove(M_TEMP_PATH);
}

TEST(a_buffer_reads_back_what_it_was_given)
{
    TRX_FILE *const file = M_Buffer();
    CHECK_EQ_INT((int32_t)File_Size(file), (int32_t)sizeof(m_Bytes));
    CHECK_EQ_INT(File_ReadU8(file), 0x01);
    CHECK_EQ_INT(File_ReadU16(file), 0x0302);
    CHECK_EQ_INT((int32_t)File_Pos(file), 3);
    CHECK_EQ_INT((int32_t)File_BytesLeft(file), 5);
    File_Close(file);
}

TEST(a_file_on_disk_reads_the_same_as_a_buffer)
{
    TRX_FILE *const file = M_Disk();
    CHECK_NOT_NULL(file);
    CHECK_EQ_INT((int32_t)File_Size(file), (int32_t)sizeof(m_Bytes));
    CHECK_EQ_INT(File_ReadU8(file), 0x01);
    CHECK_EQ_INT(File_ReadU16(file), 0x0302);
    CHECK_EQ_INT((int32_t)File_Pos(file), 3);
    File_Close(file);
    M_DropDisk();
}

TEST(a_seek_moves_where_the_next_read_starts)
{
    TRX_FILE *const file = M_Buffer();
    File_Seek(file, 4, FILE_SEEK_SET);
    CHECK_EQ_INT(File_ReadU8(file), 0x05);
    File_Seek(file, 1, FILE_SEEK_CUR);
    CHECK_EQ_INT(File_ReadU8(file), 0x07);
    File_Seek(file, 0, FILE_SEEK_END);
    CHECK_EQ_INT((int32_t)File_BytesLeft(file), 0);
    File_Close(file);
}

TEST(a_seek_past_the_end_is_refused)
{
    TRX_FILE *const file = M_Buffer();
    CHECK(!File_TrySeek(file, sizeof(m_Bytes) + 1, FILE_SEEK_SET));
    CHECK(File_TrySeek(file, sizeof(m_Bytes), FILE_SEEK_SET));
    File_Close(file);
}

TEST(a_skip_back_before_the_start_is_refused)
{
    TRX_FILE *const file = M_Buffer();
    File_Seek(file, 2, FILE_SEEK_SET);
    CHECK(!File_TrySkip(file, -3));
    CHECK(File_TrySkip(file, -2));
    CHECK_EQ_INT((int32_t)File_Pos(file), 0);
    File_Close(file);
}

TEST(a_soft_reader_is_told_it_read_past_the_end)
{
    TRX_FILE *const file = M_Buffer();
    File_SetSoftFailure(file, true);
    File_Seek(file, 6, FILE_SEEK_SET);

    CHECK(!File_HasFailed(file));
    CHECK_EQ_INT(File_ReadS32(file), 0);
    CHECK(File_HasFailed(file));

    File_ClearFailure(file);
    CHECK(!File_HasFailed(file));
    File_Close(file);
}

TEST(a_soft_reader_that_has_failed_stays_where_it_was)
{
    TRX_FILE *const file = M_Buffer();
    File_SetSoftFailure(file, true);
    File_Seek(file, 6, FILE_SEEK_SET);
    CHECK_EQ_INT(File_ReadS32(file), 0);

    const size_t pos = File_Pos(file);
    CHECK_EQ_INT(File_ReadU8(file), 0);
    CHECK_EQ_INT((int32_t)File_Pos(file), (int32_t)pos);
    File_Close(file);
}

TEST(a_count_that_cannot_fit_is_refused)
{
    const char bytes[] = { 0x00, 0x10, 0x00, 0x00, 0x01, 0x02 };
    TRX_FILE *const file = File_OpenBuffer(bytes, sizeof(bytes));
    File_SetSoftFailure(file, true);

    CHECK_EQ_INT(File_ReadCountS32(file), 0);
    CHECK(File_HasFailed(file));
    File_Close(file);
}

TEST(a_read_of_nothing_leaves_the_buffer_alone)
{
    // A count the reader refuses reads no records, and the buffer that would
    // hold them is a null pointer.
    TRX_FILE *const file = M_Buffer();
    File_ReadData(file, nullptr, 0);
    CHECK(!File_HasFailed(file));
    CHECK_EQ_INT((int32_t)File_Pos(file), 0);
    File_Close(file);
}

TEST(a_count_that_fits_is_given_back)
{
    const char bytes[] = { 0x02, 0x00, 0x00, 0x00, 0x01, 0x02 };
    TRX_FILE *const file = File_OpenBuffer(bytes, sizeof(bytes));
    File_SetSoftFailure(file, true);

    CHECK_EQ_INT(File_ReadCountS32(file), 2);
    CHECK(!File_HasFailed(file));
    File_Close(file);
}

TEST(a_view_reads_only_the_part_it_was_given)
{
    TRX_FILE *const file = M_Buffer();
    TRX_FILE *const view = File_OpenView(file, 2, 3);

    CHECK_EQ_INT((int32_t)File_Size(view), 3);
    CHECK_EQ_INT(File_ReadU8(view), 0x03);
    CHECK_EQ_INT(File_ReadU8(view), 0x04);
    CHECK_EQ_INT(File_ReadU8(view), 0x05);
    CHECK_EQ_INT((int32_t)File_BytesLeft(view), 0);

    File_SetSoftFailure(view, true);
    CHECK_EQ_INT(File_ReadU8(view), 0);
    CHECK(File_HasFailed(view));

    File_Close(view);
    File_Close(file);
}

TEST(a_view_does_not_move_the_handle_it_came_from)
{
    TRX_FILE *const file = M_Buffer();
    File_Seek(file, 2, FILE_SEEK_SET);
    TRX_FILE *const view = File_OpenView(file, 2, 3);
    File_ReadU8(view);

    CHECK_EQ_INT((int32_t)File_Pos(file), 2);
    File_Close(view);
    File_Close(file);
}

TEST(peeked_bytes_start_at_the_cursor)
{
    TRX_FILE *const file = M_Buffer();
    File_Seek(file, 5, FILE_SEEK_SET);

    size_t size = 0;
    const char *const bytes = File_PeekBytes(file, &size);
    CHECK_NOT_NULL(bytes);
    CHECK_EQ_INT((int32_t)size, 3);
    CHECK_EQ_INT(bytes[0], 0x06);
    File_Close(file);
}

TEST(a_file_on_disk_cannot_peek)
{
    TRX_FILE *const file = M_Disk();
    size_t size = 1;
    CHECK_NULL(File_PeekBytes(file, &size));
    CHECK_EQ_INT((int32_t)size, 0);
    File_Close(file);
    M_DropDisk();
}

TEST(what_was_written_reads_back)
{
    TRX_FILE *const file = M_Open(M_TEMP_PATH, FILE_OPEN_WRITE);
    CHECK_NOT_NULL(file);
    File_WriteU8(file, 0x11);
    File_WriteU32(file, 0x44332211);
    CHECK_EQ_INT((int32_t)File_Size(file), 5);
    File_Close(file);

    TRX_FILE *const read = M_Open(M_TEMP_PATH, FILE_OPEN_READ);
    CHECK_NOT_NULL(read);
    CHECK_EQ_INT(File_ReadU8(read), 0x11);
    CHECK_EQ_INT((int32_t)File_ReadU32(read), 0x44332211);
    File_Close(read);
    M_DropDisk();
}

TEST(a_write_at_a_seeked_spot_leaves_the_rest_alone)
{
    TRX_FILE *const file = M_Open(M_TEMP_PATH, FILE_OPEN_WRITE);
    File_WriteData(file, m_Bytes, sizeof(m_Bytes));
    File_Close(file);

    TRX_FILE *const patch = M_Open(M_TEMP_PATH, FILE_OPEN_READ_WRITE);
    CHECK_NOT_NULL(patch);
    File_Seek(patch, 3, FILE_SEEK_SET);
    File_WriteU8(patch, 0xFF);
    File_Close(patch);

    TRX_FILE *const read = M_Open(M_TEMP_PATH, FILE_OPEN_READ);
    CHECK_EQ_INT((int32_t)File_Size(read), (int32_t)sizeof(m_Bytes));
    File_Seek(read, 2, FILE_SEEK_SET);
    CHECK_EQ_INT(File_ReadU8(read), 0x03);
    CHECK_EQ_INT(File_ReadU8(read), 0xFF);
    CHECK_EQ_INT(File_ReadU8(read), 0x05);
    File_Close(read);
    M_DropDisk();
}

TEST(a_file_cut_short_loses_its_tail)
{
    TRX_FILE *const file = M_Open(M_TEMP_PATH, FILE_OPEN_WRITE);
    File_WriteData(file, m_Bytes, sizeof(m_Bytes));
    CHECK(IGNORE(File_SetSize(file, 3)));
    CHECK_EQ_INT((int32_t)File_Size(file), 3);
    File_Close(file);

    TRX_FILE *const read = M_Open(M_TEMP_PATH, FILE_OPEN_READ);
    CHECK_EQ_INT((int32_t)File_Size(read), 3);
    CHECK_EQ_INT(File_ReadU8(read), 0x01);
    File_Close(read);
    M_DropDisk();
}

TEST(a_buffer_cannot_be_resized)
{
    TRX_FILE *const file = M_Buffer();
    CHECK(!IGNORE(File_SetSize(file, 3)));
    CHECK_EQ_INT((int32_t)File_Size(file), (int32_t)sizeof(m_Bytes));
    File_Close(file);
}
