#include <harness/harness.h>

#include <trx/game/cutseq/decoder.h>

#include <string.h>

// The cutseq tracks are bit-packed RLE streams of per-frame deltas, and the
// deltas accumulate over thousands of frames - a word decoded one bit wide or
// one position late is wrong for the rest of the scene, and playing the game
// only shows it as an actor drifting. These build streams by hand and step
// them, so the wire format is pinned rather than inferred.

#define M_MAX_WORDS 8
#define M_MAX_NODES 2
#define M_BUF_SIZE 512

typedef struct {
    int16_t key;
    uint8_t pack_method;
    // What the header declares, which a truncated stream understates.
    uint32_t length;
    int32_t word_count;
    uint32_t words[M_MAX_WORDS];
} M_AXIS;

static void M_WriteU16(uint8_t *const buf, const uint16_t value)
{
    buf[0] = value & 0xFF;
    buf[1] = (value >> 8) & 0xFF;
}

// Words are laid down least significant bit first, running across byte
// boundaries without padding.
static void M_PackTrack(const M_AXIS *const axis, uint8_t *const buf)
{
    for (int32_t i = 0; i < axis->word_count; i++) {
        const uint32_t base = (uint32_t)i * axis->pack_method;
        for (int32_t bit = 0; bit < axis->pack_method; bit++) {
            if (((axis->words[i] >> bit) & 1) != 0) {
                buf[(base + bit) >> 3] |= 1u << ((base + bit) & 7);
            }
        }
    }
}

// Lays out num_nodes 14-byte headers followed by every node's three streams,
// each carrying the four slack bytes the format guarantees. Returns the total
// size, which is what InitNodes must agree on.
static uint32_t M_Build(
    uint8_t *const buf, const M_AXIS *const axes, const int32_t num_nodes)
{
    memset(buf, 0, M_BUF_SIZE);
    uint32_t offset = num_nodes * 14;
    for (int32_t n = 0; n < num_nodes; n++) {
        const M_AXIS *const node = &axes[n * 3];
        uint8_t *const header = &buf[n * 14];
        for (int32_t i = 0; i < 3; i++) {
            M_WriteU16(header + i * 2, (uint16_t)node[i].key);
        }
        M_WriteU16(
            header + 6,
            (uint16_t)((node[0].pack_method << 10) | (node[1].pack_method << 5)
                       | node[2].pack_method));
        for (int32_t i = 0; i < 3; i++) {
            M_WriteU16(header + 8 + i * 2, (uint16_t)node[i].length);
        }
        for (int32_t i = 0; i < 3; i++) {
            M_PackTrack(&node[i], &buf[offset]);
            offset += ((node[i].length * node[i].pack_method) >> 3) + 4;
        }
    }
    return offset;
}

// A track that never moves, for the two axes a test is not looking at.
#define M_IDLE_AXIS                                                            \
    (M_AXIS)                                                                   \
    {                                                                          \
        .key = 0, .pack_method = 8, .length = 0, .word_count = 0,              \
    }

TEST(init_nodes_reports_the_size_it_consumed)
{
    uint8_t buf[M_BUF_SIZE];
    const M_AXIS axes[3] = {
        { .key = 0, .pack_method = 8, .length = 4, .word_count = 0 },
        M_IDLE_AXIS,
        M_IDLE_AXIS,
    };
    const uint32_t size = M_Build(buf, axes, 1);

    CUTSEQ_PACK_NODE node;
    CHECK_EQ_INT(CutSeq_Decoder_InitNodes(buf, size, &node, 1), (int32_t)size);
}

TEST(init_nodes_rejects_a_stream_cut_short)
{
    uint8_t buf[M_BUF_SIZE];
    const M_AXIS axes[3] = {
        { .key = 0, .pack_method = 8, .length = 4, .word_count = 0 },
        M_IDLE_AXIS,
        M_IDLE_AXIS,
    };
    const uint32_t size = M_Build(buf, axes, 1);

    CUTSEQ_PACK_NODE node;
    // One byte short of what the headers claim.
    CHECK_EQ_INT(CutSeq_Decoder_InitNodes(buf, size - 1, &node, 1), -1);
    // Not even room for the headers.
    CHECK_EQ_INT(CutSeq_Decoder_InitNodes(buf, 8, &node, 1), -1);
}

TEST(a_literal_run_yields_one_delta_per_frame)
{
    uint8_t buf[M_BUF_SIZE];
    // 0x23: bit 5 opens a run, the low nibble counts its three words.
    const M_AXIS axes[3] = {
        {
            .key = 100,
            .pack_method = 8,
            .length = 4,
            .word_count = 4,
            .words = { 0x23, 5, 0xFB, 2 },
        },
        M_IDLE_AXIS,
        M_IDLE_AXIS,
    };
    const uint32_t size = M_Build(buf, axes, 1);

    CUTSEQ_PACK_NODE node;
    CHECK(CutSeq_Decoder_InitNodes(buf, size, &node, 1) > 0);
    CutSeq_Decoder_Reset(&node, 1);
    CHECK_EQ_INT(node.x_run, 100);

    CutSeq_Decoder_Advance(&node, 1, 0xFFFF);
    CHECK_EQ_INT(node.x_run, 105);
    // 0xFB is eight bits wide and negative, not 251.
    CutSeq_Decoder_Advance(&node, 1, 0xFFFF);
    CHECK_EQ_INT(node.x_run, 100);
    CutSeq_Decoder_Advance(&node, 1, 0xFFFF);
    CHECK_EQ_INT(node.x_run, 102);

    // The words are spent, so the track holds rather than reading its slack.
    CutSeq_Decoder_Advance(&node, 1, 0xFFFF);
    CHECK_EQ_INT(node.x_run, 102);
    CutSeq_Decoder_Advance(&node, 1, 0xFFFF);
    CHECK_EQ_INT(node.x_run, 102);
}

TEST(a_repeated_delta_holds_for_its_counter)
{
    uint8_t buf[M_BUF_SIZE];
    // Bits 4 and 5 clear: the low three bits count the frames, and the word
    // after it carries the delta each of them applies.
    const M_AXIS axes[3] = {
        {
            .key = 0,
            .pack_method = 8,
            .length = 2,
            .word_count = 2,
            .words = { 0x04, 0x07 },
        },
        M_IDLE_AXIS,
        M_IDLE_AXIS,
    };
    const uint32_t size = M_Build(buf, axes, 1);

    CUTSEQ_PACK_NODE node;
    CHECK(CutSeq_Decoder_InitNodes(buf, size, &node, 1) > 0);
    CutSeq_Decoder_Reset(&node, 1);

    for (int32_t i = 1; i <= 4; i++) {
        CutSeq_Decoder_Advance(&node, 1, 0xFFFF);
        CHECK_EQ_INT(node.x_run, 7 * i);
    }
    CutSeq_Decoder_Advance(&node, 1, 0xFFFF);
    CHECK_EQ_INT(node.x_run, 28);
}

TEST(the_wide_repeat_form_counts_past_seven_frames)
{
    uint8_t buf[M_BUF_SIZE];
    // Bit 4 set: the counter spans two words, three high bits then five low.
    // 0x11 and 0x08 spell 1 << 5 | 8 == 40.
    const M_AXIS axes[3] = {
        {
            .key = 0,
            .pack_method = 8,
            .length = 3,
            .word_count = 3,
            .words = { 0x11, 0x08, 0x03 },
        },
        M_IDLE_AXIS,
        M_IDLE_AXIS,
    };
    const uint32_t size = M_Build(buf, axes, 1);

    CUTSEQ_PACK_NODE node;
    CHECK(CutSeq_Decoder_InitNodes(buf, size, &node, 1) > 0);
    CutSeq_Decoder_Reset(&node, 1);

    for (int32_t i = 1; i <= 40; i++) {
        CutSeq_Decoder_Advance(&node, 1, 0xFFFF);
        CHECK_EQ_INT(node.x_run, 3 * i);
    }
    CutSeq_Decoder_Advance(&node, 1, 0xFFFF);
    CHECK_EQ_INT(node.x_run, 120);
}

TEST(a_word_is_sign_extended_at_its_own_width)
{
    uint8_t buf[M_BUF_SIZE];
    // Six bits wide, so 0x3F is -1 rather than 63.
    const M_AXIS axes[3] = {
        {
            .key = 0,
            .pack_method = 6,
            .length = 2,
            .word_count = 2,
            .words = { 0x02, 0x3F },
        },
        M_IDLE_AXIS,
        M_IDLE_AXIS,
    };
    const uint32_t size = M_Build(buf, axes, 1);

    CUTSEQ_PACK_NODE node;
    CHECK(CutSeq_Decoder_InitNodes(buf, size, &node, 1) > 0);
    CutSeq_Decoder_Reset(&node, 1);

    CutSeq_Decoder_Advance(&node, 1, 0xFFFF);
    CHECK_EQ_INT(node.x_run, -1);
    CutSeq_Decoder_Advance(&node, 1, 0xFFFF);
    CHECK_EQ_INT(node.x_run, -2);
}

TEST(a_run_header_with_no_words_behind_it_ends_the_track)
{
    uint8_t buf[M_BUF_SIZE];
    // Malformed: the run says three words follow, and the stream ends. The
    // track has to stop rather than walk off the end of its slack bytes for
    // the rest of the scene.
    const M_AXIS axes[3] = {
        {
            .key = 42,
            .pack_method = 8,
            .length = 1,
            .word_count = 1,
            .words = { 0x23 },
        },
        M_IDLE_AXIS,
        M_IDLE_AXIS,
    };
    const uint32_t size = M_Build(buf, axes, 1);

    CUTSEQ_PACK_NODE node;
    CHECK(CutSeq_Decoder_InitNodes(buf, size, &node, 1) > 0);
    CutSeq_Decoder_Reset(&node, 1);

    for (int32_t i = 0; i < 1000; i++) {
        CutSeq_Decoder_Advance(&node, 1, 0xFFFF);
    }
    CHECK_EQ_INT(node.x_run, 42);
}

TEST(child_nodes_wrap_within_the_mask_and_the_root_does_not)
{
    uint8_t buf[M_BUF_SIZE];
    const M_AXIS run_600[3] = {
        {
            .key = 0,
            .pack_method = 12,
            .length = 2,
            .word_count = 2,
            .words = { 0x02, 600 },
        },
        M_IDLE_AXIS,
        M_IDLE_AXIS,
    };
    M_AXIS axes[M_MAX_NODES * 3];
    memcpy(&axes[0], run_600, sizeof(run_600));
    memcpy(&axes[3], run_600, sizeof(run_600));
    const uint32_t size = M_Build(buf, axes, 2);

    CUTSEQ_PACK_NODE nodes[M_MAX_NODES];
    CHECK(CutSeq_Decoder_InitNodes(buf, size, nodes, 2) > 0);
    CutSeq_Decoder_Reset(nodes, 2);

    CutSeq_Decoder_Advance(nodes, 2, 1023);
    CHECK_EQ_INT(nodes[0].x_run, 600);
    CHECK_EQ_INT(nodes[1].x_run, 600);

    // The root carries a position and keeps climbing; the child carries an
    // angle and comes back round.
    CutSeq_Decoder_Advance(nodes, 2, 1023);
    CHECK_EQ_INT(nodes[0].x_run, 1200);
    CHECK_EQ_INT(nodes[1].x_run, 1200 & 1023);
}

TEST(build_pose_scales_the_root_and_widens_child_angles)
{
    uint8_t buf[M_BUF_SIZE];
    const M_AXIS axes[M_MAX_NODES * 3] = {
        { .key = 10, .pack_method = 8, .length = 0, .word_count = 0 },
        { .key = 20, .pack_method = 8, .length = 0, .word_count = 0 },
        { .key = 30, .pack_method = 8, .length = 0, .word_count = 0 },
        { .key = 1, .pack_method = 8, .length = 0, .word_count = 0 },
        { .key = 2, .pack_method = 8, .length = 0, .word_count = 0 },
        { .key = 3, .pack_method = 8, .length = 0, .word_count = 0 },
    };
    const uint32_t size = M_Build(buf, axes, 2);

    CUTSEQ_PACK_NODE nodes[M_MAX_NODES];
    CHECK(CutSeq_Decoder_InitNodes(buf, size, nodes, 2) > 0);
    CutSeq_Decoder_Reset(nodes, 2);

    CUTSEQ_POSE pose = {};
    CutSeq_Decoder_BuildPose(nodes, 2, &pose);
    CHECK_EQ_INT(pose.offset.x, 30);
    CHECK_EQ_INT(pose.offset.y, 60);
    CHECK_EQ_INT(pose.offset.z, 90);
    // Ten-bit angles widened to the engine's sixteen.
    CHECK_EQ_INT(pose.rots[0].x, 1 << 6);
    CHECK_EQ_INT(pose.rots[0].y, 2 << 6);
    CHECK_EQ_INT(pose.rots[0].z, 3 << 6);
}

TEST(build_pose_clears_the_meshes_the_actor_does_not_carry)
{
    uint8_t buf[M_BUF_SIZE];
    const M_AXIS axes[M_MAX_NODES * 3] = {
        M_IDLE_AXIS,
        M_IDLE_AXIS,
        M_IDLE_AXIS,
        { .key = 7, .pack_method = 8, .length = 0, .word_count = 0 },
        { .key = 7, .pack_method = 8, .length = 0, .word_count = 0 },
        { .key = 7, .pack_method = 8, .length = 0, .word_count = 0 },
    };
    const uint32_t size = M_Build(buf, axes, 2);

    CUTSEQ_PACK_NODE nodes[M_MAX_NODES];
    CHECK(CutSeq_Decoder_InitNodes(buf, size, nodes, 2) > 0);
    CutSeq_Decoder_Reset(nodes, 2);

    // Lara is posed to the full LM_NUMBER_OF whatever her actor declares, so
    // a pose reused across cutscenes may not keep the last one's tail.
    CUTSEQ_POSE pose;
    for (int32_t i = 0; i < CUTSEQ_MAX_MESHES; i++) {
        pose.rots[i] = (XYZ_16) { .x = 999, .y = 999, .z = 999 };
    }

    CutSeq_Decoder_BuildPose(nodes, 2, &pose);
    CHECK_EQ_INT(pose.rots[0].x, 7 << 6);
    for (int32_t i = 1; i < CUTSEQ_MAX_MESHES; i++) {
        CHECK_EQ_INT(pose.rots[i].x, 0);
        CHECK_EQ_INT(pose.rots[i].y, 0);
        CHECK_EQ_INT(pose.rots[i].z, 0);
    }
}
