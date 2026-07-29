#include <trx/game/cutseq/decoder.h>

// Faithful port of the tomb4 deltapak decoder (InitPackNodes / DecodeAnim /
// DecodeTrack / GetTrackWord). The deltas accumulate over thousands of
// frames, so every quirk here (int16 wrap-around, sign extension width,
// masking only child nodes) must be preserved bit-exactly. The guards against
// a zero pack method, a zero run length and a truncated stream are the
// exceptions: they cover inputs the encoder does not produce, where the
// original relies on x86 shift and integer wrap behaviour.

#define M_NODE_HEADER_SIZE 14 // 7 int16 fields

static int16_t M_ReadS16(const uint8_t *const data)
{
    return (int16_t)(data[0] | (data[1] << 8));
}

static uint16_t M_ReadU16(const uint8_t *const data)
{
    return (uint16_t)(data[0] | (data[1] << 8));
}

static int16_t M_GetTrackWord(
    const uint8_t *const packed, const uint32_t word_off,
    const uint8_t pack_method)
{
    if (pack_method == 0) {
        // The original relies on the x86 shift count masking to turn the
        // sign-extension test into 1 << 31, which never matches a zero word.
        return 0;
    }

    const uint32_t bit_off = pack_method * word_off;
    const uint32_t byte_off = bit_off >> 3;
    // Each track is stored with 4 slack bytes, so the 32-bit window never
    // reads past its stream.
    const uint32_t window = packed[byte_off] | (packed[byte_off + 1] << 8)
        | (packed[byte_off + 2] << 16) | ((uint32_t)packed[byte_off + 3] << 24);
    const uint32_t mask = (1u << pack_method) - 1;
    uint32_t word = (window >> (bit_off & 7)) & mask;
    if ((word & (1u << (pack_method - 1))) != 0) {
        word |= ~mask;
    }
    return (int16_t)word;
}

static int16_t M_DecodeTrack(
    const uint8_t *const packed, CUTSEQ_TRACK_STATE *const decode)
{
    // A track holds as many words as its header states. One that runs out
    // before the last frame holds its value here, rather than reading on past
    // the slack bytes its stream ends with.
    if (decode->decode_type != 2 && decode->length == 0) {
        return 0;
    }

    if (decode->decode_type == 0) {
        const int16_t word =
            M_GetTrackWord(packed, decode->off, decode->pack_method);
        if ((word & 0x20) != 0) {
            decode->counter = (word & 0xF) != 0 ? (word & 0xF) : 16;
            decode->decode_type = 1;
            decode->off++;
            decode->length--;
        } else {
            const uint32_t words = (word & 0x10) != 0 ? 3 : 2;
            if (decode->length < words) {
                decode->length = 0;
                return 0;
            }
            decode->decode_type = 2;
            if (words == 3) {
                decode->counter = ((word & 7) << 5)
                    | (M_GetTrackWord(
                           packed, decode->off + 1, decode->pack_method)
                       & 0x1F);
                decode->data = M_GetTrackWord(
                    packed, decode->off + 2, decode->pack_method);
            } else {
                decode->data = M_GetTrackWord(
                    packed, decode->off + 1, decode->pack_method);
                decode->counter = word & 7;
            }
            decode->off += words;
            decode->length -= words;
        }
    }

    if (decode->decode_type == 2) {
        // Both run forms can spell a count of zero, and the original decrements
        // it into a 65535-frame hold. Nothing in the retail pak asks for that,
        // and an encoder emitting a run of no frames at all is not a thing the
        // format means, so a zero count ends the run after one frame instead.
        if (decode->counter != 0) {
            decode->counter--;
        }
        if (decode->counter == 0) {
            decode->decode_type = 0;
        }
        return decode->data;
    }

    // A run header is the last thing a truncated stream can hold, and the run
    // it opens has no words left to read. Ending it here keeps the unsigned
    // length from wrapping, which would let off walk past the stream for the
    // rest of the scene.
    if (decode->length == 0) {
        decode->decode_type = 0;
        return 0;
    }

    const int16_t word =
        M_GetTrackWord(packed, decode->off, decode->pack_method);
    decode->off++;
    decode->length--;
    if (decode->counter != 0) {
        decode->counter--;
    }
    if (decode->counter == 0) {
        decode->decode_type = 0;
    }
    return word;
}

int32_t CutSeq_Decoder_InitNodes(
    const uint8_t *const packed, const uint32_t data_size,
    CUTSEQ_PACK_NODE *const nodes, const int32_t num_nodes)
{
    uint32_t offset = num_nodes * M_NODE_HEADER_SIZE;
    if (offset > data_size) {
        return -1;
    }

    for (int32_t i = 0; i < num_nodes; i++) {
        const uint8_t *const header = &packed[i * M_NODE_HEADER_SIZE];
        CUTSEQ_PACK_NODE *const node = &nodes[i];
        node->x_key = M_ReadS16(header);
        node->y_key = M_ReadS16(header + 2);
        node->z_key = M_ReadS16(header + 4);
        const uint16_t pack_method = M_ReadU16(header + 6);
        node->decode_x.pack_method = (pack_method >> 10) & 0xF;
        node->decode_y.pack_method = (pack_method >> 5) & 0xF;
        node->decode_z.pack_method = pack_method & 0xF;
        node->x_length = M_ReadU16(header + 8);
        node->y_length = M_ReadU16(header + 10);
        node->z_length = M_ReadU16(header + 12);

        const uint32_t x_size =
            ((node->x_length * node->decode_x.pack_method) >> 3) + 4;
        const uint32_t y_size =
            ((node->y_length * node->decode_y.pack_method) >> 3) + 4;
        const uint32_t z_size =
            ((node->z_length * node->decode_z.pack_method) >> 3) + 4;
        if (offset + x_size + y_size + z_size > data_size) {
            return -1;
        }
        node->x_packed = &packed[offset];
        node->y_packed = &packed[offset + x_size];
        node->z_packed = &packed[offset + x_size + y_size];
        offset += x_size + y_size + z_size;
    }

    return (int32_t)offset;
}

void CutSeq_Decoder_Reset(
    CUTSEQ_PACK_NODE *const nodes, const int32_t num_nodes)
{
    for (int32_t i = 0; i < num_nodes; i++) {
        CUTSEQ_PACK_NODE *const node = &nodes[i];
        node->decode_x.off = 0;
        node->decode_x.counter = 0;
        node->decode_x.data = 0;
        node->decode_x.decode_type = 0;
        node->decode_x.length = node->x_length;
        node->decode_y.off = 0;
        node->decode_y.counter = 0;
        node->decode_y.data = 0;
        node->decode_y.decode_type = 0;
        node->decode_y.length = node->y_length;
        node->decode_z.off = 0;
        node->decode_z.counter = 0;
        node->decode_z.data = 0;
        node->decode_z.decode_type = 0;
        node->decode_z.length = node->z_length;
        node->x_run = node->x_key;
        node->y_run = node->y_key;
        node->z_run = node->z_key;
    }
}

void CutSeq_Decoder_Advance(
    CUTSEQ_PACK_NODE *const nodes, const int32_t num_nodes, const uint16_t mask)
{
    for (int32_t i = 0; i < num_nodes; i++) {
        CUTSEQ_PACK_NODE *const node = &nodes[i];
        node->x_run += M_DecodeTrack(node->x_packed, &node->decode_x);
        node->y_run += M_DecodeTrack(node->y_packed, &node->decode_y);
        node->z_run += M_DecodeTrack(node->z_packed, &node->decode_z);
        // The root node carries a position and accumulates the full int16
        // range; child nodes carry angles and wrap within the mask.
        if (i != 0) {
            node->x_run &= mask;
            node->y_run &= mask;
            node->z_run &= mask;
        }
    }
}

void CutSeq_Decoder_BuildPose(
    const CUTSEQ_PACK_NODE *const nodes, const int32_t num_nodes,
    CUTSEQ_POSE *const pose)
{
    // The x3 scale (and its int16 overflow) matches the original engine.
    pose->offset.x = (int16_t)(3 * nodes[0].x_run);
    pose->offset.y = (int16_t)(3 * nodes[0].y_run);
    pose->offset.z = (int16_t)(3 * nodes[0].z_run);
    int32_t mesh_idx = 0;
    for (int32_t i = 1; i < num_nodes && i <= CUTSEQ_MAX_MESHES; i++) {
        pose->rots[mesh_idx++] = (XYZ_16) {
            .x = (int16_t)((nodes[i].x_run & 0x3FF) << 6),
            .y = (int16_t)((nodes[i].y_run & 0x3FF) << 6),
            .z = (int16_t)((nodes[i].z_run & 0x3FF) << 6),
        };
    }
    // An actor with fewer nodes than the pose holds would otherwise leave the
    // tail as the last cutscene wrote it, and Lara is always posed to the full
    // LM_NUMBER_OF regardless of what her actor declares.
    while (mesh_idx < CUTSEQ_MAX_MESHES) {
        pose->rots[mesh_idx++] = (XYZ_16) {};
    }
}
