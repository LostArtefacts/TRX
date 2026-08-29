#include <harness/harness.h>

#include <trx/game/cutseq/pak.h>
#include <trx/core/memory.h>
#include <trx/game/objects.h>
#include <trx/game/paths.h>

#include <zlib.h>

#include <string.h>

// The pak is untrusted input - a header table whose extent the file itself
// declares, then descriptors whose every field is an offset into it. These
// feed the loader whole images, valid and malformed, so the bounds hold
// without a copy of the retail file to test against.

#define M_HEADER_ENTRY_SIZE 8
#define M_DESCRIPTOR_SIZE 104
// A payload is the descriptor followed by the track data its offsets name.
#define M_PAYLOAD_SIZE 256
#define M_ACTOR_OFFSET M_DESCRIPTOR_SIZE
#define M_CAMERA_OFFSET 180
#define M_IMAGE_SIZE 4096

// What the fake FS_Load hands back. The loader owns neither, and reads the
// image only until it has inflated it.
static uint8_t m_Image[M_IMAGE_SIZE];
static uint32_t m_ImageSize = 0;
static bool m_FileExists = true;

static void M_WriteU32(uint8_t *const buf, const uint32_t value)
{
    buf[0] = value & 0xFF;
    buf[1] = (value >> 8) & 0xFF;
    buf[2] = (value >> 16) & 0xFF;
    buf[3] = (value >> 24) & 0xFF;
}

static void M_WriteU16(uint8_t *const buf, const uint16_t value)
{
    buf[0] = value & 0xFF;
    buf[1] = (value >> 8) & 0xFF;
}

// Compresses the image the way the retail file is laid out: the inflated size
// as a bare int32, then the zlib stream.
static void M_Publish(const uint8_t *const data, const uint32_t size)
{
    uLongf packed_size = M_IMAGE_SIZE - 4;
    const int32_t error =
        compress(m_Image + 4, &packed_size, (const Bytef *)data, size);
    CHECK_EQ_INT(error, Z_OK);
    M_WriteU32(m_Image, size);
    m_ImageSize = 4 + (uint32_t)packed_size;
    m_FileExists = true;
}

// A pak holding one cutscene: the header table, then the payload. The loader
// does not parse the track data, so what follows the descriptor is only there
// for its offsets to point at.
static void M_PublishOneCutscene(
    const uint8_t *const payload, const uint32_t payload_size)
{
    uint8_t data[M_IMAGE_SIZE] = {};
    const uint32_t offset = M_HEADER_ENTRY_SIZE;
    M_WriteU32(data, offset);
    M_WriteU32(data + 4, payload_size);
    memcpy(data + offset, payload, payload_size);
    M_Publish(data, offset + payload_size);
}

// A payload for one actor, with every field in range.
static void M_BuildDescriptor(uint8_t *const desc)
{
    memset(desc, 0, M_PAYLOAD_SIZE);
    M_WriteU16(desc, 1); // num_actors
    M_WriteU16(desc + 2, 120); // num_frames
    M_WriteU32(desc + 4, 1024); // origin x
    M_WriteU32(desc + 8, 2048); // origin y
    M_WriteU32(desc + 12, 3072); // origin z
    M_WriteU32(desc + 16, 0xFFFFFFFF); // audio track: none
    M_WriteU32(desc + 20, M_CAMERA_OFFSET);
    M_WriteU32(desc + 24, M_ACTOR_OFFSET);
    M_WriteU16(desc + 28, 0); // actor 0 game slot
    M_WriteU16(desc + 30, 15); // actor 0 node count
}

TEST(a_well_formed_pak_parses_its_descriptor)
{
    uint8_t desc[M_PAYLOAD_SIZE];
    M_BuildDescriptor(desc);
    M_PublishOneCutscene(desc, M_PAYLOAD_SIZE);

    CutSeq_Pak_Unload();
    CHECK(CutSeq_Pak_Load());
    CHECK_EQ_INT(CutSeq_Pak_GetCutsceneCount(), 1);

    CUTSEQ_INFO info = {};
    CHECK(CutSeq_Pak_GetCutscene(0, &info));
    CHECK_EQ_INT(info.num_actors, 1);
    CHECK_EQ_INT(info.num_frames, 120);
    CHECK_EQ_INT(info.origin.x, 1024);
    CHECK_EQ_INT(info.origin.y, 2048);
    CHECK_EQ_INT(info.origin.z, 3072);
    CHECK_EQ_INT(info.audio_track, -1);
    CHECK_EQ_INT((int32_t)info.data_size, M_PAYLOAD_SIZE);
    CHECK_EQ_INT(info.actors[0].node_count, 15);
    CutSeq_Pak_Unload();
}

TEST(the_first_referenced_payload_ends_the_header_table)
{
    // Four entries before the payload starts, so the table holds four slots
    // whether or not every one of them names a cutscene.
    uint8_t data[M_IMAGE_SIZE] = {};
    const uint32_t offset = 4 * M_HEADER_ENTRY_SIZE;
    M_WriteU32(data + 3 * M_HEADER_ENTRY_SIZE, offset);
    M_WriteU32(data + 3 * M_HEADER_ENTRY_SIZE + 4, M_PAYLOAD_SIZE);
    uint8_t desc[M_PAYLOAD_SIZE];
    M_BuildDescriptor(desc);
    memcpy(data + offset, desc, M_PAYLOAD_SIZE);
    M_Publish(data, offset + M_PAYLOAD_SIZE);

    CutSeq_Pak_Unload();
    CHECK(CutSeq_Pak_Load());
    CHECK_EQ_INT(CutSeq_Pak_GetCutsceneCount(), 4);

    CUTSEQ_INFO info = {};
    // The three empty slots are addressable and hold nothing.
    CHECK(!CutSeq_Pak_GetCutscene(0, &info));
    CHECK(CutSeq_Pak_GetCutscene(3, &info));
    CutSeq_Pak_Unload();
}

TEST(a_number_outside_the_table_is_refused)
{
    uint8_t desc[M_PAYLOAD_SIZE];
    M_BuildDescriptor(desc);
    M_PublishOneCutscene(desc, M_PAYLOAD_SIZE);

    CutSeq_Pak_Unload();
    CHECK(CutSeq_Pak_Load());

    CUTSEQ_INFO info = {};
    CHECK(!CutSeq_Pak_GetCutscene(-1, &info));
    CHECK(!CutSeq_Pak_GetCutscene(1, &info));
    CHECK(!CutSeq_Pak_GetCutscene(1 << 20, &info));
    CutSeq_Pak_Unload();
}

TEST(a_payload_reaching_past_the_file_is_refused)
{
    uint8_t data[M_IMAGE_SIZE] = {};
    uint8_t desc[M_PAYLOAD_SIZE];
    M_BuildDescriptor(desc);
    const uint32_t offset = M_HEADER_ENTRY_SIZE;
    M_WriteU32(data, offset);
    // One byte more than the file holds past the offset.
    M_WriteU32(data + 4, M_PAYLOAD_SIZE + 1);
    memcpy(data + offset, desc, M_PAYLOAD_SIZE);
    M_Publish(data, offset + M_PAYLOAD_SIZE);

    CutSeq_Pak_Unload();
    CHECK(CutSeq_Pak_Load());
    CUTSEQ_INFO info = {};
    CHECK(!CutSeq_Pak_GetCutscene(0, &info));
    CutSeq_Pak_Unload();
}

TEST(a_descriptor_naming_offsets_past_its_own_payload_is_refused)
{
    uint8_t desc[M_PAYLOAD_SIZE];

    // A camera track starting at or past the end of the payload.
    M_BuildDescriptor(desc);
    M_WriteU32(desc + 20, M_PAYLOAD_SIZE);
    M_PublishOneCutscene(desc, M_PAYLOAD_SIZE);
    CutSeq_Pak_Unload();
    CHECK(CutSeq_Pak_Load());
    CUTSEQ_INFO info = {};
    CHECK(!CutSeq_Pak_GetCutscene(0, &info));
    CutSeq_Pak_Unload();

    // An actor's track data doing the same.
    M_BuildDescriptor(desc);
    M_WriteU32(desc + 24, M_PAYLOAD_SIZE);
    M_PublishOneCutscene(desc, M_PAYLOAD_SIZE);
    CutSeq_Pak_Unload();
    CHECK(CutSeq_Pak_Load());
    CHECK(!CutSeq_Pak_GetCutscene(0, &info));
    CutSeq_Pak_Unload();

    // A negative offset, which reads back as one far past the payload.
    M_BuildDescriptor(desc);
    M_WriteU32(desc + 24, 0xFFFFFFFF);
    M_PublishOneCutscene(desc, M_PAYLOAD_SIZE);
    CutSeq_Pak_Unload();
    CHECK(CutSeq_Pak_Load());
    CHECK(!CutSeq_Pak_GetCutscene(0, &info));
    CutSeq_Pak_Unload();
}

TEST(an_actor_count_the_descriptor_cannot_hold_is_refused)
{
    uint8_t desc[M_PAYLOAD_SIZE];

    M_BuildDescriptor(desc);
    M_WriteU16(desc, 0);
    M_PublishOneCutscene(desc, M_PAYLOAD_SIZE);
    CutSeq_Pak_Unload();
    CHECK(CutSeq_Pak_Load());
    CUTSEQ_INFO info = {};
    CHECK(!CutSeq_Pak_GetCutscene(0, &info));
    CutSeq_Pak_Unload();

    M_BuildDescriptor(desc);
    M_WriteU16(desc, CUTSEQ_MAX_ACTORS + 1);
    M_PublishOneCutscene(desc, M_PAYLOAD_SIZE);
    CutSeq_Pak_Unload();
    CHECK(CutSeq_Pak_Load());
    CHECK(!CutSeq_Pak_GetCutscene(0, &info));
    CutSeq_Pak_Unload();
}

TEST(a_frame_count_of_zero_is_refused)
{
    uint8_t desc[M_PAYLOAD_SIZE];
    M_BuildDescriptor(desc);
    M_WriteU16(desc + 2, 0);
    M_PublishOneCutscene(desc, M_PAYLOAD_SIZE);

    CutSeq_Pak_Unload();
    CHECK(CutSeq_Pak_Load());
    CUTSEQ_INFO info = {};
    CHECK(!CutSeq_Pak_GetCutscene(0, &info));
    CutSeq_Pak_Unload();
}

TEST(a_payload_too_small_to_be_a_descriptor_is_refused)
{
    uint8_t desc[M_PAYLOAD_SIZE];
    M_BuildDescriptor(desc);
    M_PublishOneCutscene(desc, M_DESCRIPTOR_SIZE - 1);

    CutSeq_Pak_Unload();
    CHECK(CutSeq_Pak_Load());
    CUTSEQ_INFO info = {};
    CHECK(!CutSeq_Pak_GetCutscene(0, &info));
    CutSeq_Pak_Unload();
}

TEST(an_implausible_inflated_size_fails_the_load)
{
    uint8_t desc[M_PAYLOAD_SIZE];
    M_BuildDescriptor(desc);
    M_PublishOneCutscene(desc, M_PAYLOAD_SIZE);

    // Larger than the ceiling the loader allocates against.
    M_WriteU32(m_Image, 1024u * 1024u * 1024u);
    CutSeq_Pak_Unload();
    CHECK(!CutSeq_Pak_Load());
    CHECK(!CutSeq_Pak_IsLoaded());
    CutSeq_Pak_Unload();

    // Smaller than a single descriptor.
    M_PublishOneCutscene(desc, M_PAYLOAD_SIZE);
    M_WriteU32(m_Image, 8);
    CutSeq_Pak_Unload();
    CHECK(!CutSeq_Pak_Load());
    CutSeq_Pak_Unload();
}

TEST(a_stream_that_does_not_inflate_fails_the_load)
{
    uint8_t desc[M_PAYLOAD_SIZE];
    M_BuildDescriptor(desc);
    M_PublishOneCutscene(desc, M_PAYLOAD_SIZE);

    // The declared size no longer matches what the stream holds.
    M_WriteU32(m_Image, M_PAYLOAD_SIZE * 2);
    CutSeq_Pak_Unload();
    CHECK(!CutSeq_Pak_Load());
    CHECK(!CutSeq_Pak_IsLoaded());
    CutSeq_Pak_Unload();
}

TEST(a_missing_file_is_not_a_loaded_pak)
{
    CutSeq_Pak_Unload();
    m_FileExists = false;
    CHECK(!CutSeq_Pak_Load());
    CHECK(!CutSeq_Pak_IsLoaded());
    CHECK_EQ_INT(CutSeq_Pak_GetCutsceneCount(), 0);

    CUTSEQ_INFO info = {};
    CHECK(!CutSeq_Pak_GetCutscene(0, &info));
    CutSeq_Pak_Unload();
    m_FileExists = true;
}

// The loader's view of the outside world: a file that holds what the test
// last published, and object ids it does not act on here.

const char *GamePath_TryResolve(
    const GAME_DYNAMIC_PATH path, const char *const rel)
{
    return m_FileExists ? rel : nullptr;
}

RESULT FS_Load(
    const char *const path, char **const output_data, size_t *const output_size)
{
    if (!m_FileExists) {
        return FAIL("%s: the file could not be opened", path);
    }
    *output_data = Memory_Alloc(m_ImageSize);
    memcpy(*output_data, m_Image, m_ImageSize);
    *output_size = m_ImageSize;
    return OK;
}

OBJECT_ID Object_SlotToID(const int32_t game_id)
{
    return NO_OBJECT;
}
