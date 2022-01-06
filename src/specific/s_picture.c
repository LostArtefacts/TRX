#include "specific/s_picture.h"

#include "filesystem.h"
#include "log.h"
#include "memory.h"

#include <assert.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>

bool S_Picture_LoadFromFile(PICTURE *target_pic, const char *path)
{
    assert(target_pic);
    assert(!target_pic->data);

    int error_code;
    AVFormatContext *format_ctx = NULL;
    const AVCodec *codec = NULL;
    AVCodecContext *codec_ctx = NULL;
    AVFrame *frame = NULL;
    AVPacket *packet = NULL;
    struct SwsContext *sws_ctx = NULL;

    target_pic->width = 0;
    target_pic->height = 0;
    target_pic->data = NULL;

    error_code = avformat_open_input(&format_ctx, path, NULL, NULL);
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
        error_code = AVERROR(ENOMEM);
        goto fail;
    }

    error_code =
        avcodec_parameters_to_context(codec_ctx, video_stream->codecpar);
    if (error_code) {
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
        error_code = AVERROR(ENOMEM);
        goto fail;
    }

    error_code = avcodec_receive_frame(codec_ctx, frame);
    if (error_code < 0) {
        goto fail;
    }

    target_pic->width = frame->width;
    target_pic->height = frame->height;

    sws_ctx = sws_getContext(
        codec_ctx->width, codec_ctx->height, codec_ctx->pix_fmt,
        target_pic->width, target_pic->height, AV_PIX_FMT_RGB24, SWS_BILINEAR,
        NULL, NULL, NULL);

    if (!sws_ctx) {
        LOG_ERROR("Failed to get SWS context");
        error_code = AVERROR_EXTERNAL;
        goto fail;
    }

    uint8_t *dst_data[4];
    int dst_linesize[4];
    error_code = av_image_alloc(
        dst_data, dst_linesize, target_pic->width, target_pic->height,
        AV_PIX_FMT_RGB24, 1);
    if (error_code < 0) {
        goto fail;
    }

    sws_scale(
        sws_ctx, (const uint8_t *const *)frame->data, frame->linesize, 0,
        frame->height, dst_data, dst_linesize);

    target_pic->data =
        Memory_Alloc(target_pic->height * target_pic->width * sizeof(RGB888));

    av_image_copy_to_buffer(
        (uint8_t *)target_pic->data,
        target_pic->width * target_pic->height * sizeof(RGB888),
        (const uint8_t *const *)dst_data, dst_linesize, AV_PIX_FMT_RGB24,
        target_pic->width, target_pic->height, 1);

    return true;

fail:
    LOG_ERROR(
        "Error while opening picture %s: %s", path, av_err2str(error_code));

    target_pic->width = 0;
    target_pic->height = 0;
    if (target_pic->data) {
        Memory_Free(target_pic->data);
        target_pic->data = NULL;
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

    if (codec_ctx) {
        avcodec_close(codec_ctx);
        av_free(codec_ctx);
        codec_ctx = NULL;
    }

    if (format_ctx) {
        avformat_close_input(&format_ctx);
    }

    return false;
}

bool S_Picture_SaveToFile(const PICTURE *pic, const char *path)
{
    assert(pic);
    assert(path);

    bool ret = false;

    int error_code = 0;
    const AVCodec *codec = NULL;
    AVCodecContext *codec_ctx = NULL;
    AVFrame *frame = NULL;
    AVPacket *packet = NULL;
    struct SwsContext *sws_ctx = NULL;
    MYFILE *fp = NULL;

    enum AVPixelFormat source_pix_fmt = AV_PIX_FMT_RGB24;
    enum AVPixelFormat target_pix_fmt;
    enum AVCodecID codec_id;

    if (strstr(path, ".jpg")) {
        target_pix_fmt = AV_PIX_FMT_YUVJ420P;
        codec_id = AV_CODEC_ID_MJPEG;
    } else if (strstr(path, ".png")) {
        target_pix_fmt = AV_PIX_FMT_RGB24;
        codec_id = AV_CODEC_ID_PNG;
    } else {
        LOG_ERROR("Cannot determine picture format based on path '%s'", path);
        goto cleanup;
    }

    fp = File_Open(path, FILE_OPEN_WRITE);
    if (!fp) {
        LOG_ERROR("Cannot create picture file: %s", path);
        goto cleanup;
    }

    codec = avcodec_find_encoder(codec_id);
    if (!codec) {
        error_code = AVERROR_MUXER_NOT_FOUND;
        goto cleanup;
    }

    codec_ctx = avcodec_alloc_context3(codec);
    if (!codec_ctx) {
        error_code = AVERROR(ENOMEM);
        goto cleanup;
    }

    codec_ctx->bit_rate = 400000;
    codec_ctx->width = pic->width;
    codec_ctx->height = pic->height;
    codec_ctx->time_base = (AVRational) { 1, 25 };
    codec_ctx->pix_fmt = target_pix_fmt;

    if (codec_id == AV_CODEC_ID_MJPEG) {
        // 9 JPEG quality
        codec_ctx->flags |= AV_CODEC_FLAG_QSCALE;
        codec_ctx->global_quality = FF_QP2LAMBDA * 9;
    }

    error_code = avcodec_open2(codec_ctx, codec, NULL);
    if (error_code < 0) {
        goto cleanup;
    }

    frame = av_frame_alloc();
    if (!frame) {
        error_code = AVERROR(ENOMEM);
        goto cleanup;
    }
    frame->format = codec_ctx->pix_fmt;
    frame->width = codec_ctx->width;
    frame->height = codec_ctx->height;
    frame->pts = 0;

    error_code = av_image_alloc(
        frame->data, frame->linesize, codec_ctx->width, codec_ctx->height,
        codec_ctx->pix_fmt, 32);
    if (error_code < 0) {
        goto cleanup;
    }

    packet = av_packet_alloc();
    av_new_packet(packet, 0);

    sws_ctx = sws_getContext(
        pic->width, pic->height, source_pix_fmt, frame->width, frame->height,
        target_pix_fmt, SWS_BILINEAR, NULL, NULL, NULL);

    if (!sws_ctx) {
        LOG_ERROR("Failed to get SWS context");
        error_code = AVERROR_EXTERNAL;
        goto cleanup;
    }

    uint8_t *src_planes[4];
    int src_linesize[4];
    av_image_fill_arrays(
        src_planes, src_linesize, (const uint8_t *)pic->data, source_pix_fmt,
        pic->width, pic->height, 1);

    sws_scale(
        sws_ctx, (const uint8_t *const *)src_planes, src_linesize, 0,
        pic->height, frame->data, frame->linesize);

    error_code = avcodec_send_frame(codec_ctx, frame);
    if (error_code < 0) {
        goto cleanup;
    }

    while (error_code >= 0) {
        error_code = avcodec_receive_packet(codec_ctx, packet);
        if (error_code == AVERROR(EAGAIN) || error_code == AVERROR_EOF) {
            error_code = 0;
            break;
        }
        if (error_code < 0) {
            goto cleanup;
        }

        File_Write(packet->data, 1, packet->size, fp);
        av_packet_unref(packet);
    }

cleanup:
    if (error_code) {
        LOG_ERROR(
            "Error while saving picture %s: %s", path, av_err2str(error_code));
    }

    if (fp) {
        File_Close(fp);
        fp = NULL;
    }

    if (sws_ctx) {
        sws_freeContext(sws_ctx);
    }

    if (packet) {
        av_packet_free(&packet);
    }

    if (codec) {
        avcodec_close(codec_ctx);
        av_free(codec_ctx);
        codec_ctx = NULL;
    }

    if (frame) {
        av_freep(&frame->data[0]);
        av_frame_free(&frame);
    }

    return ret;
}

bool S_Picture_ScaleLetterbox(
    PICTURE *target_pic, const PICTURE *source_pic, int target_width,
    int target_height)
{
    assert(source_pic);
    assert(source_pic->data);
    assert(target_pic);
    assert(!target_pic->data);

    int source_width = source_pic->width;
    int source_height = source_pic->height;

    const float source_ratio = source_width / (float)source_height;
    const float target_ratio = target_width / (float)target_height;
    {
        int new_width = source_ratio < target_ratio
            ? target_height * source_ratio
            : target_width;
        int new_height = source_ratio < target_ratio
            ? target_height
            : target_width / source_ratio;
        target_width = new_width;
        target_height = new_height;
    }

    bool ret = false;
    struct SwsContext *sws_ctx = sws_getContext(
        source_width, source_height, AV_PIX_FMT_RGB24, target_width,
        target_height, AV_PIX_FMT_RGB24, SWS_BILINEAR, NULL, NULL, NULL);

    if (!sws_ctx) {
        LOG_ERROR("Failed to get SWS context");
        goto cleanup;
    }

    target_pic->width = target_width;
    target_pic->height = target_height;
    target_pic->data =
        Memory_Alloc(target_height * target_width * sizeof(RGB888));

    uint8_t *src_planes[4];
    uint8_t *dst_planes[4];
    int src_linesize[4];
    int dst_linesize[4];

    av_image_fill_arrays(
        src_planes, src_linesize, (const uint8_t *)source_pic->data,
        AV_PIX_FMT_RGB24, source_width, source_height, 1);

    av_image_fill_arrays(
        dst_planes, dst_linesize, (const uint8_t *)target_pic->data,
        AV_PIX_FMT_RGB24, target_pic->width, target_pic->height, 1);

    sws_scale(
        sws_ctx, (const uint8_t *const *)src_planes, src_linesize, 0,
        source_height, (uint8_t *const *)dst_planes, dst_linesize);

    ret = true;

cleanup:
    if (sws_ctx) {
        sws_freeContext(sws_ctx);
    }

    if (!ret) {
        if (target_pic) {
            Memory_Free(target_pic->data);
            target_pic->width = 0;
            target_pic->height = 0;
            target_pic->data = NULL;
        }
    }

    return ret;
}

bool S_Picture_ScaleCrop(
    PICTURE *target_pic, const PICTURE *source_pic, int target_width,
    int target_height)
{
    assert(source_pic);
    assert(source_pic->data);
    assert(target_pic);
    assert(!target_pic->data);

    int source_width = source_pic->width;
    int source_height = source_pic->height;

    const float source_ratio = source_width / (float)source_height;
    const float target_ratio = target_width / (float)target_height;

    int crop_width = source_ratio < target_ratio ? source_width
                                                 : source_height * target_ratio;
    int crop_height = source_ratio < target_ratio ? source_width / target_ratio
                                                  : source_height;

    bool ret = false;
    struct SwsContext *sws_ctx = sws_getContext(
        crop_width, crop_height, AV_PIX_FMT_RGB24, target_width, target_height,
        AV_PIX_FMT_RGB24, SWS_BILINEAR, NULL, NULL, NULL);

    if (!sws_ctx) {
        LOG_ERROR("Failed to get SWS context");
        goto cleanup;
    }

    target_pic->width = target_width;
    target_pic->height = target_height;
    target_pic->data =
        Memory_Alloc(target_height * target_width * sizeof(RGB888));

    uint8_t *src_planes[4];
    uint8_t *dst_planes[4];
    int src_linesize[4];
    int dst_linesize[4];

    av_image_fill_arrays(
        src_planes, src_linesize, (const uint8_t *)source_pic->data,
        AV_PIX_FMT_RGB24, source_width, source_height, 1);

    src_planes[0] += ((source_height - crop_height) / 2) * src_linesize[0];
    src_planes[0] += ((source_width - crop_width) / 2) * sizeof(RGB888);

    av_image_fill_arrays(
        dst_planes, dst_linesize, (const uint8_t *)target_pic->data,
        AV_PIX_FMT_RGB24, target_pic->width, target_pic->height, 1);

    sws_scale(
        sws_ctx, (const uint8_t *const *)src_planes, src_linesize, 0,
        crop_height, (uint8_t *const *)dst_planes, dst_linesize);

    ret = true;

cleanup:
    if (sws_ctx) {
        sws_freeContext(sws_ctx);
    }

    if (!ret) {
        if (target_pic) {
            Memory_Free(target_pic->data);
            target_pic->width = 0;
            target_pic->height = 0;
            target_pic->data = NULL;
        }
    }

    return ret;
}

bool S_Picture_ScaleStretch(
    PICTURE *target_pic, const PICTURE *source_pic, int target_width,
    int target_height)
{
    assert(source_pic);
    assert(source_pic->data);
    assert(target_pic);
    assert(!target_pic->data);

    int source_width = source_pic->width;
    int source_height = source_pic->height;
    bool ret = false;
    struct SwsContext *sws_ctx = sws_getContext(
        source_width, source_height, AV_PIX_FMT_RGB24, target_width,
        target_height, AV_PIX_FMT_RGB24, SWS_BILINEAR, NULL, NULL, NULL);

    if (!sws_ctx) {
        LOG_ERROR("Failed to get SWS context");
        goto cleanup;
    }

    target_pic->width = target_width;
    target_pic->height = target_height;
    target_pic->data =
        Memory_Alloc(target_height * target_width * sizeof(RGB888));

    uint8_t *src_planes[4];
    uint8_t *dst_planes[4];
    int src_linesize[4];
    int dst_linesize[4];

    av_image_fill_arrays(
        src_planes, src_linesize, (const uint8_t *)source_pic->data,
        AV_PIX_FMT_RGB24, source_width, source_height, 1);

    av_image_fill_arrays(
        dst_planes, dst_linesize, (const uint8_t *)target_pic->data,
        AV_PIX_FMT_RGB24, target_pic->width, target_pic->height, 1);

    sws_scale(
        sws_ctx, (const uint8_t *const *)src_planes, src_linesize, 0,
        source_height, (uint8_t *const *)dst_planes, dst_linesize);

    ret = true;

cleanup:
    if (sws_ctx) {
        sws_freeContext(sws_ctx);
    }

    if (!ret) {
        if (target_pic) {
            target_pic->width = 0;
            target_pic->height = 0;
        }
        Memory_FreePointer(&target_pic->data);
    }

    return ret;
}
