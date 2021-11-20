#include "specific/s_picture.h"

#include "filesystem.h"
#include "log.h"
#include "memory.h"

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>

bool S_Picture_LoadFromFile(PICTURE *picture, const char *file_path)
{
    int32_t error_code;
    char *full_path = NULL;
    AVFormatContext *format_ctx = NULL;
    const AVCodec *codec = NULL;
    AVCodecContext *codec_ctx = NULL;
    AVFrame *frame = NULL;
    struct SwsContext *sws_ctx = NULL;
    AVPacket *packet = NULL;

    picture->width = 0;
    picture->height = 0;
    picture->data = NULL;

    File_GetFullPath(file_path, &full_path);

    error_code = avformat_open_input(&format_ctx, full_path, NULL, NULL);
    if (error_code != 0) {
        goto fail;
    }

    error_code = avformat_find_stream_info(format_ctx, NULL);
    if (error_code < 0) {
        goto fail;
    }

    AVStream *video_stream = NULL;
    for (unsigned int i = 0; i < format_ctx->nb_streams; i++) {
        AVStream *current_stream = format_ctx->streams[i];
        if (current_stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_stream = current_stream;
            break;
        }
    }
    if (!video_stream) {
        error_code = AVERROR_STREAM_NOT_FOUND;
        goto fail;
    }

    codec = avcodec_find_decoder(video_stream->codecpar->codec_id);
    if (!codec) {
        error_code = AVERROR_DEMUXER_NOT_FOUND;
        goto fail;
    }

    codec_ctx = avcodec_alloc_context3(codec);
    if (!codec_ctx) {
        error_code = AVERROR_UNKNOWN;
        goto fail;
    }

    error_code = avcodec_open2(codec_ctx, codec, NULL);
    if (error_code < 0) {
        goto fail;
    }

    packet = av_packet_alloc();
    av_new_packet(packet, 0);
    error_code = av_read_frame(format_ctx, packet);
    if (error_code < 0) {
        goto fail;
    }

    error_code = avcodec_send_packet(codec_ctx, packet);
    if (error_code < 0) {
        goto fail;
    }

    frame = av_frame_alloc();
    if (!frame) {
        error_code = AVERROR_UNKNOWN;
        goto fail;
    }

    error_code = avcodec_receive_frame(codec_ctx, frame);
    if (error_code < 0) {
        goto fail;
    }

    sws_ctx = sws_getContext(
        codec_ctx->width, codec_ctx->height, codec_ctx->pix_fmt,
        codec_ctx->width, codec_ctx->height, AV_PIX_FMT_RGB24, SWS_BILINEAR,
        NULL, NULL, NULL);

    if (!sws_ctx) {
        LOG_ERROR("Failed to get SWS context");
        error_code = AVERROR_EXTERNAL;
        goto fail;
    }

    uint8_t *dst_data[4];
    int dst_linesize[4];
    error_code = av_image_alloc(
        dst_data, dst_linesize, frame->width, frame->height, AV_PIX_FMT_RGB24,
        1);
    if (error_code < 0) {
        goto fail;
    }

    sws_scale(
        sws_ctx, (const uint8_t *const *)frame->data, frame->linesize, 0,
        frame->height, dst_data, dst_linesize);

    picture->width = frame->width;
    picture->height = frame->height;
    picture->data =
        Memory_Alloc(picture->height * picture->width * sizeof(RGB888));

    av_image_copy_to_buffer(
        (uint8_t *)picture->data,
        picture->width * picture->height * sizeof(RGB888),
        (const uint8_t *const *)dst_data, dst_linesize, AV_PIX_FMT_RGB24,
        frame->width, frame->height, 1);

    // TODO: remove this
    RGB888 *dst = picture->data;
    for (int x = 0; x < picture->width; x++) {
        for (int y = 0; y < picture->height; y++) {
            dst->r = (dst->r >> 2) & 0x3F;
            dst->g = (dst->g >> 2) & 0x3F;
            dst->b = (dst->b >> 2) & 0x3F;
            dst++;
        }
    }

    return true;

fail:
    LOG_ERROR(
        "Error while opening picture %s: %s", full_path,
        av_err2str(error_code));

    if (picture->data) {
        Memory_Free(picture->data);
        picture->data = NULL;
    }

    if (sws_ctx) {
        sws_freeContext(sws_ctx);
    }

    if (packet) {
        av_packet_free(&packet);
    }

    if (frame) {
        av_frame_free(&frame);
    }

    if (format_ctx) {
        avformat_close_input(&format_ctx);
    }

    if (full_path) {
        Memory_Free(full_path);
    }

    return false;
}
