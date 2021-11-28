/*
 * Copyright (c) 2003 Fabrice Bellard
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "specific/s_fmv.h"

#include "config.h"
#include "filesystem.h"
#include "game/gamebuf.h"
#include "game/input.h"
#include "log.h"
#include "memory.h"
#include "specific/s_hwr.h"
#include "specific/s_shell.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_thread.h>
#include <libavcodec/avcodec.h>
#include <libavdevice/avdevice.h>
#include <libavformat/avformat.h>
#include <libavutil/channel_layout.h>
#include <libavutil/fifo.h>
#include <libavutil/imgutils.h>
#include <libavutil/time.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>

#define MAX_QUEUE_SIZE (15 * 1024 * 1024)
#define MIN_FRAMES 25
#define SDL_AUDIO_MIN_BUFFER_SIZE 512
#define SDL_AUDIO_MAX_CALLBACKS_PER_SEC 30
#define AV_SYNC_THRESHOLD_MIN 0.04
#define AV_SYNC_THRESHOLD_MAX 0.1
#define AV_SYNC_FRAMEDUP_THRESHOLD 0.1
#define AV_NOSYNC_THRESHOLD 10.0
#define SAMPLE_CORRECTION_PERCENT_MAX 10
#define AUDIO_DIFF_AVG_NB 20
#define REFRESH_RATE 0.01
#define SAMPLE_ARRAY_SIZE (8 * 65536)
#define VIDEO_PICTURE_QUEUE_SIZE 3
#define SUBPICTURE_QUEUE_SIZE 16
#define SAMPLE_QUEUE_SIZE 9
#define FRAME_QUEUE_SIZE                                                       \
    FFMAX(                                                                     \
        SAMPLE_QUEUE_SIZE,                                                     \
        FFMAX(VIDEO_PICTURE_QUEUE_SIZE, SUBPICTURE_QUEUE_SIZE))
#define FF_QUIT_EVENT (SDL_USEREVENT + 2)

typedef struct MyAVPacketList {
    AVPacket *pkt;
    int serial;
} MyAVPacketList;

typedef struct PacketQueue {
    AVFifoBuffer *pkt_list;
    int nb_packets;
    int size;
    int64_t duration;
    bool abort_request;
    int serial;
    SDL_mutex *mutex;
    SDL_cond *cond;
} PacketQueue;

typedef struct AudioParams {
    int freq;
    int channels;
    int64_t channel_layout;
    enum AVSampleFormat fmt;
    int frame_size;
    int bytes_per_sec;
} AudioParams;

typedef struct Clock {
    double pts;
    double pts_drift;
    double last_updated;
    double speed;
    int serial;
    bool paused;
    int *queue_serial;
} Clock;

typedef struct Frame {
    AVFrame *frame;
    AVSubtitle sub;
    int serial;
    double pts;
    double duration;
    int64_t pos;
    int width;
    int height;
    int format;
    AVRational sar;
    bool uploaded;
    int flip_v;
} Frame;

typedef struct FrameQueue {
    Frame queue[FRAME_QUEUE_SIZE];
    int rindex;
    int windex;
    int size;
    int max_size;
    int keep_last;
    int rindex_shown;
    SDL_mutex *mutex;
    SDL_cond *cond;
    PacketQueue *pktq;
} FrameQueue;

enum {
    AV_SYNC_AUDIO_MASTER,
    AV_SYNC_VIDEO_MASTER,
    AV_SYNC_EXTERNAL_CLOCK,
};

typedef struct Decoder {
    AVPacket *pkt;
    PacketQueue *queue;
    AVCodecContext *avctx;
    int pkt_serial;
    int finished;
    bool packet_pending;
    SDL_cond *empty_queue_cond;
    int64_t start_pts;
    AVRational start_pts_tb;
    int64_t next_pts;
    AVRational next_pts_tb;
    SDL_Thread *decoder_tid;
} Decoder;

typedef struct VideoState {
    SDL_Thread *read_tid;
    const AVInputFormat *iformat;
    bool abort_request;
    bool force_refresh;
    bool paused;
    bool last_paused;
    bool queue_attachments_req;
    int read_pause_return;
    AVFormatContext *ic;

    Clock audclk;
    Clock vidclk;
    Clock extclk;

    FrameQueue pictq;
    FrameQueue subpq;
    FrameQueue sampq;

    Decoder auddec;
    Decoder viddec;
    Decoder subdec;

    int audio_stream;

    int av_sync_type;

    double audio_clock;
    int audio_clock_serial;
    double audio_diff_cum;
    double audio_diff_avg_coef;
    double audio_diff_threshold;
    int audio_diff_avg_count;
    AVStream *audio_st;
    PacketQueue audioq;
    int audio_hw_buf_size;
    uint8_t *audio_buf;
    uint8_t *audio_buf1;
    unsigned int audio_buf_size;
    unsigned int audio_buf1_size;
    int audio_buf_index;
    int audio_write_buf_size;
    int audio_volume;
    struct AudioParams audio_src;
    struct AudioParams audio_tgt;
    struct SwrContext *swr_ctx;
    int frame_drops_early;
    int frame_drops_late;

    SDL_Texture *sub_texture;
    SDL_Texture *vid_texture;

    int subtitle_stream;
    AVStream *subtitle_st;
    PacketQueue subtitleq;

    double frame_timer;
    int video_stream;
    AVStream *video_st;
    PacketQueue videoq;
    double max_frame_duration; // maximum duration of a frame - above this, we
                               // consider the jump a timestamp discontinuity
    struct SwsContext *img_convert_ctx;
    struct SwsContext *sub_convert_ctx;
    bool eof;

    char *filename;
    int width;
    int height;

    SDL_cond *continue_read_thread;
} VideoState;

static int64_t m_AudioCallbackTime;

static SDL_Window *m_Window;
static SDL_Renderer *m_Renderer;
static SDL_RendererInfo m_RendererInfo = { 0 };
static SDL_AudioDeviceID m_AudioDevice;

static const struct TextureFormatEntry {
    enum AVPixelFormat format;
    int texture_fmt;
} sdl_texture_format_map[] = {
    { AV_PIX_FMT_RGB8, SDL_PIXELFORMAT_RGB332 },
    { AV_PIX_FMT_RGB444, SDL_PIXELFORMAT_RGB444 },
    { AV_PIX_FMT_RGB555, SDL_PIXELFORMAT_RGB555 },
    { AV_PIX_FMT_BGR555, SDL_PIXELFORMAT_BGR555 },
    { AV_PIX_FMT_RGB565, SDL_PIXELFORMAT_RGB565 },
    { AV_PIX_FMT_BGR565, SDL_PIXELFORMAT_BGR565 },
    { AV_PIX_FMT_RGB24, SDL_PIXELFORMAT_RGB24 },
    { AV_PIX_FMT_BGR24, SDL_PIXELFORMAT_BGR24 },
    { AV_PIX_FMT_0RGB32, SDL_PIXELFORMAT_RGB888 },
    { AV_PIX_FMT_0BGR32, SDL_PIXELFORMAT_BGR888 },
    { AV_PIX_FMT_NE(RGB0, 0BGR), SDL_PIXELFORMAT_RGBX8888 },
    { AV_PIX_FMT_NE(BGR0, 0RGB), SDL_PIXELFORMAT_BGRX8888 },
    { AV_PIX_FMT_RGB32, SDL_PIXELFORMAT_ARGB8888 },
    { AV_PIX_FMT_RGB32_1, SDL_PIXELFORMAT_RGBA8888 },
    { AV_PIX_FMT_BGR32, SDL_PIXELFORMAT_ABGR8888 },
    { AV_PIX_FMT_BGR32_1, SDL_PIXELFORMAT_BGRA8888 },
    { AV_PIX_FMT_YUV420P, SDL_PIXELFORMAT_IYUV },
    { AV_PIX_FMT_YUYV422, SDL_PIXELFORMAT_YUY2 },
    { AV_PIX_FMT_UYVY422, SDL_PIXELFORMAT_UYVY },
    { AV_PIX_FMT_NONE, SDL_PIXELFORMAT_UNKNOWN },
};

void avcodec_free_context(AVCodecContext **pavctx)
{
    AVCodecContext *avctx = *pavctx;
    if (!avctx) {
        return;
    }

    avcodec_close(avctx);
    av_freep(&avctx->extradata);
    av_freep(&avctx->subtitle_header);
    av_freep(&avctx->intra_matrix);
    av_freep(&avctx->inter_matrix);
    av_freep(&avctx->rc_override);
    av_freep(pavctx);
}

static int S_FMV_PacketQueuePutPrivate(PacketQueue *q, AVPacket *pkt)
{
    MyAVPacketList pkt1;

    if (q->abort_request) {
        return -1;
    }

    if (av_fifo_space(q->pkt_list) < (signed)sizeof(pkt1)) {
        if (av_fifo_grow(q->pkt_list, sizeof(pkt1)) < 0) {
            return -1;
        }
    }

    pkt1.pkt = pkt;
    pkt1.serial = q->serial;

    av_fifo_generic_write(q->pkt_list, &pkt1, sizeof(pkt1), NULL);
    q->nb_packets++;
    q->size += pkt1.pkt->size + sizeof(pkt1);
    q->duration += pkt1.pkt->duration;
    SDL_CondSignal(q->cond);
    return 0;
}

static int S_FMV_PacketQueuePut(PacketQueue *q, AVPacket *pkt)
{
    AVPacket *pkt1;
    int ret;

    pkt1 = av_packet_alloc();
    if (!pkt1) {
        av_packet_unref(pkt);
        return -1;
    }
    av_packet_move_ref(pkt1, pkt);

    SDL_LockMutex(q->mutex);
    ret = S_FMV_PacketQueuePutPrivate(q, pkt1);
    SDL_UnlockMutex(q->mutex);

    if (ret < 0) {
        av_packet_free(&pkt1);
    }

    return ret;
}

static int S_FMV_PacketQueuePutNullPacket(
    PacketQueue *q, AVPacket *pkt, int stream_index)
{
    pkt->stream_index = stream_index;
    return S_FMV_PacketQueuePut(q, pkt);
}

static int S_FMV_PacketQueueInit(PacketQueue *q)
{
    memset(q, 0, sizeof(PacketQueue));
    q->pkt_list = av_fifo_alloc(sizeof(MyAVPacketList));
    if (!q->pkt_list) {
        return AVERROR(ENOMEM);
    }

    q->mutex = SDL_CreateMutex();
    if (!q->mutex) {
        LOG_ERROR("SDL_CreateMutex(): %s", SDL_GetError());
        return AVERROR(ENOMEM);
    }

    q->cond = SDL_CreateCond();
    if (!q->cond) {
        LOG_ERROR("SDL_CreateCond(): %s", SDL_GetError());
        return AVERROR(ENOMEM);
    }

    q->abort_request = true;
    return 0;
}

static void S_FMV_PacketQueueFlush(PacketQueue *q)
{
    MyAVPacketList pkt1;

    SDL_LockMutex(q->mutex);
    while (av_fifo_size(q->pkt_list) >= (signed)sizeof(pkt1)) {
        av_fifo_generic_read(q->pkt_list, &pkt1, sizeof(pkt1), NULL);
        av_packet_free(&pkt1.pkt);
    }
    q->nb_packets = 0;
    q->size = 0;
    q->duration = 0;
    q->serial++;
    SDL_UnlockMutex(q->mutex);
}

static void S_FMV_PacketQueueDestroy(PacketQueue *q)
{
    S_FMV_PacketQueueFlush(q);
    av_fifo_freep(&q->pkt_list);
    SDL_DestroyMutex(q->mutex);
    SDL_DestroyCond(q->cond);
}

static void S_FMV_PacketQueueAbort(PacketQueue *q)
{
    SDL_LockMutex(q->mutex);
    q->abort_request = true;
    SDL_CondSignal(q->cond);
    SDL_UnlockMutex(q->mutex);
}

static void S_FMV_PacketQueueStart(PacketQueue *q)
{
    SDL_LockMutex(q->mutex);
    q->abort_request = false;
    q->serial++;
    SDL_UnlockMutex(q->mutex);
}

static int S_FMV_PacketQueueGet(
    PacketQueue *q, AVPacket *pkt, int block, int *serial)
{
    MyAVPacketList pkt1;
    int ret;

    SDL_LockMutex(q->mutex);

    while (1) {
        if (q->abort_request) {
            ret = -1;
            break;
        }

        if (av_fifo_size(q->pkt_list) >= (signed)sizeof(pkt1)) {
            av_fifo_generic_read(q->pkt_list, &pkt1, sizeof(pkt1), NULL);
            q->nb_packets--;
            q->size -= pkt1.pkt->size + sizeof(pkt1);
            q->duration -= pkt1.pkt->duration;
            av_packet_move_ref(pkt, pkt1.pkt);
            if (serial) {
                *serial = pkt1.serial;
            }
            av_packet_free(&pkt1.pkt);
            ret = 1;
            break;
        } else if (!block) {
            ret = 0;
            break;
        } else {
            SDL_CondWait(q->cond, q->mutex);
        }
    }
    SDL_UnlockMutex(q->mutex);
    return ret;
}

static int S_FMV_DecoderInit(
    Decoder *d, AVCodecContext *avctx, PacketQueue *queue,
    SDL_cond *empty_queue_cond)
{
    memset(d, 0, sizeof(Decoder));
    d->pkt = av_packet_alloc();
    if (!d->pkt) {
        return AVERROR(ENOMEM);
    }
    d->avctx = avctx;
    d->queue = queue;
    d->empty_queue_cond = empty_queue_cond;
    d->start_pts = AV_NOPTS_VALUE;
    d->pkt_serial = -1;
    return 0;
}

static int S_FMV_DecoderDecodeFrame(Decoder *d, AVFrame *frame, AVSubtitle *sub)
{
    int ret = AVERROR(EAGAIN);

    while (1) {
        if (d->queue->serial == d->pkt_serial) {
            do {
                if (d->queue->abort_request) {
                    return -1;
                }

                switch (d->avctx->codec_type) {
                case AVMEDIA_TYPE_VIDEO:
                    ret = avcodec_receive_frame(d->avctx, frame);
                    if (ret >= 0) {
                        frame->pts = frame->best_effort_timestamp;
                    }
                    break;

                case AVMEDIA_TYPE_AUDIO:
                    ret = avcodec_receive_frame(d->avctx, frame);
                    if (ret >= 0) {
                        AVRational tb = (AVRational) { 1, frame->sample_rate };
                        if (frame->pts != AV_NOPTS_VALUE) {
                            frame->pts = av_rescale_q(
                                frame->pts, d->avctx->pkt_timebase, tb);
                        } else if (d->next_pts != AV_NOPTS_VALUE) {
                            frame->pts =
                                av_rescale_q(d->next_pts, d->next_pts_tb, tb);
                        }
                        if (frame->pts != AV_NOPTS_VALUE) {
                            d->next_pts = frame->pts + frame->nb_samples;
                            d->next_pts_tb = tb;
                        }
                    }
                    break;

                default:
                    break;
                }

                if (ret == AVERROR_EOF) {
                    d->finished = d->pkt_serial;
                    avcodec_flush_buffers(d->avctx);
                    return 0;
                }
                if (ret >= 0) {
                    return 1;
                }
            } while (ret != AVERROR(EAGAIN));
        }

        while (1) {
            if (d->queue->nb_packets == 0) {
                SDL_CondSignal(d->empty_queue_cond);
            }
            if (d->packet_pending) {
                d->packet_pending = false;
            } else {
                int old_serial = d->pkt_serial;
                if (S_FMV_PacketQueueGet(d->queue, d->pkt, 1, &d->pkt_serial)
                    < 0) {
                    return -1;
                }
                if (old_serial != d->pkt_serial) {
                    avcodec_flush_buffers(d->avctx);
                    d->finished = 0;
                    d->next_pts = d->start_pts;
                    d->next_pts_tb = d->start_pts_tb;
                }
            }
            if (d->queue->serial == d->pkt_serial) {
                break;
            }
            av_packet_unref(d->pkt);
        }

        if (d->avctx->codec_type == AVMEDIA_TYPE_SUBTITLE) {
            int got_frame = 0;
            ret = avcodec_decode_subtitle2(d->avctx, sub, &got_frame, d->pkt);
            if (ret < 0) {
                ret = AVERROR(EAGAIN);
            } else {
                if (got_frame && !d->pkt->data) {
                    d->packet_pending = true;
                }
                ret = got_frame
                    ? 0
                    : (d->pkt->data ? AVERROR(EAGAIN) : AVERROR_EOF);
            }
            av_packet_unref(d->pkt);
        } else {
            if (avcodec_send_packet(d->avctx, d->pkt) == AVERROR(EAGAIN)) {
                LOG_ERROR("Receive_frame and send_packet both returned EAGAIN, "
                          "which is an API violation.");
                d->packet_pending = true;
            } else {
                av_packet_unref(d->pkt);
            }
        }
    }
}

static void S_FMV_DecoderShutdown(Decoder *d)
{
    av_packet_free(&d->pkt);
    avcodec_free_context(&d->avctx);
}

static void S_FMV_FrameQueueUnrefItem(Frame *vp)
{
    av_frame_unref(vp->frame);
    avsubtitle_free(&vp->sub);
}

static int S_FMV_FrameQueueInit(
    FrameQueue *f, PacketQueue *pktq, int max_size, int keep_last)
{
    memset(f, 0, sizeof(FrameQueue));
    if (!(f->mutex = SDL_CreateMutex())) {
        LOG_ERROR("SDL_CreateMutex(): %s", SDL_GetError());
        return AVERROR(ENOMEM);
    }
    if (!(f->cond = SDL_CreateCond())) {
        LOG_ERROR("SDL_CreateCond(): %s", SDL_GetError());
        return AVERROR(ENOMEM);
    }
    f->pktq = pktq;
    f->max_size = FFMIN(max_size, FRAME_QUEUE_SIZE);
    f->keep_last = !!keep_last;
    for (int i = 0; i < f->max_size; i++) {
        if (!(f->queue[i].frame = av_frame_alloc())) {
            return AVERROR(ENOMEM);
        }
    }
    return 0;
}

static void S_FMV_FrameQueueShutdown(FrameQueue *f)
{
    for (int i = 0; i < f->max_size; i++) {
        Frame *vp = &f->queue[i];
        S_FMV_FrameQueueUnrefItem(vp);
        av_frame_free(&vp->frame);
    }
    SDL_DestroyMutex(f->mutex);
    SDL_DestroyCond(f->cond);
}

static void S_FMV_FrameQueueSignal(FrameQueue *f)
{
    SDL_LockMutex(f->mutex);
    SDL_CondSignal(f->cond);
    SDL_UnlockMutex(f->mutex);
}

static Frame *S_FMV_FrameQueuePeek(FrameQueue *f)
{
    return &f->queue[(f->rindex + f->rindex_shown) % f->max_size];
}

static Frame *S_FMV_FrameQueuePeekNext(FrameQueue *f)
{
    return &f->queue[(f->rindex + f->rindex_shown + 1) % f->max_size];
}

static Frame *S_FMV_FrameQueuePeekLast(FrameQueue *f)
{
    return &f->queue[f->rindex];
}

static Frame *S_FMV_FrameQueuePeekWritable(FrameQueue *f)
{
    SDL_LockMutex(f->mutex);
    while (f->size >= f->max_size && !f->pktq->abort_request) {
        SDL_CondWait(f->cond, f->mutex);
    }
    SDL_UnlockMutex(f->mutex);

    if (f->pktq->abort_request) {
        return NULL;
    }

    return &f->queue[f->windex];
}

static Frame *S_FMV_FrameQueuePeekReadable(FrameQueue *f)
{
    SDL_LockMutex(f->mutex);
    while (f->size - f->rindex_shown <= 0 && !f->pktq->abort_request) {
        SDL_CondWait(f->cond, f->mutex);
    }
    SDL_UnlockMutex(f->mutex);

    if (f->pktq->abort_request) {
        return NULL;
    }

    return &f->queue[(f->rindex + f->rindex_shown) % f->max_size];
}

static void S_FMV_FrameQueuePush(FrameQueue *f)
{
    if (++f->windex == f->max_size) {
        f->windex = 0;
    }
    SDL_LockMutex(f->mutex);
    f->size++;
    SDL_CondSignal(f->cond);
    SDL_UnlockMutex(f->mutex);
}

static void S_FMV_FrameQueueNext(FrameQueue *f)
{
    if (f->keep_last && !f->rindex_shown) {
        f->rindex_shown = 1;
        return;
    }
    S_FMV_FrameQueueUnrefItem(&f->queue[f->rindex]);
    if (++f->rindex == f->max_size) {
        f->rindex = 0;
    }
    SDL_LockMutex(f->mutex);
    f->size--;
    SDL_CondSignal(f->cond);
    SDL_UnlockMutex(f->mutex);
}

static int S_FMV_FrameQueueNBRemaining(FrameQueue *f)
{
    return f->size - f->rindex_shown;
}

static void S_FMV_DecoderAbort(Decoder *d, FrameQueue *fq)
{
    S_FMV_PacketQueueAbort(d->queue);
    S_FMV_FrameQueueSignal(fq);
    SDL_WaitThread(d->decoder_tid, NULL);
    d->decoder_tid = NULL;
    S_FMV_PacketQueueFlush(d->queue);
}

static int S_FMV_ReallocTexture(
    SDL_Texture **texture, Uint32 new_format, int new_width, int new_height,
    SDL_BlendMode blendmode, int init_texture)
{
    Uint32 format;
    int access, w, h;
    if (!*texture || SDL_QueryTexture(*texture, &format, &access, &w, &h) < 0
        || new_width != w || new_height != h || new_format != format) {
        void *pixels;
        int pitch;
        if (*texture) {
            SDL_DestroyTexture(*texture);
        }
        if (!(*texture = SDL_CreateTexture(
                  m_Renderer, new_format, SDL_TEXTUREACCESS_STREAMING,
                  new_width, new_height))) {
            return -1;
        }
        if (SDL_SetTextureBlendMode(*texture, blendmode) < 0) {
            return -1;
        }
        if (init_texture) {
            if (SDL_LockTexture(*texture, NULL, &pixels, &pitch) < 0) {
                return -1;
            }
            memset(pixels, 0, pitch * new_height);
            SDL_UnlockTexture(*texture);
        }
    }
    return 0;
}

static void S_FMV_CalculateDisplayRect(
    SDL_Rect *rect, int scr_xleft, int scr_ytop, int scr_width, int scr_height,
    int pic_width, int pic_height, AVRational pic_sar)
{
    AVRational aspect_ratio = pic_sar;
    int64_t width, height, x, y;

    if (av_cmp_q(aspect_ratio, av_make_q(0, 1)) <= 0) {
        aspect_ratio = av_make_q(1, 1);
    }

    aspect_ratio = av_mul_q(aspect_ratio, av_make_q(pic_width, pic_height));

    height = scr_height;
    width = av_rescale(height, aspect_ratio.num, aspect_ratio.den) & ~1;
    if (width > scr_width) {
        width = scr_width;
        height = av_rescale(width, aspect_ratio.den, aspect_ratio.num) & ~1;
    }
    x = (scr_width - width) / 2;
    y = (scr_height - height) / 2;
    rect->x = scr_xleft + x;
    rect->y = scr_ytop + y;
    rect->w = FFMAX((int)width, 1);
    rect->h = FFMAX((int)height, 1);
}

static int S_FMV_UploadTexture(
    SDL_Texture **tex, AVFrame *frame, struct SwsContext **img_convert_ctx)
{
    int ret = 0;

    SDL_BlendMode sdl_blendmode = SDL_BLENDMODE_NONE;
    if (frame->format == AV_PIX_FMT_RGB32 || frame->format == AV_PIX_FMT_RGB32_1
        || frame->format == AV_PIX_FMT_BGR32
        || frame->format == AV_PIX_FMT_BGR32_1) {
        sdl_blendmode = SDL_BLENDMODE_BLEND;
    }

    Uint32 sdl_pix_fmt = SDL_PIXELFORMAT_UNKNOWN;
    for (int i = 0; i < (signed)FF_ARRAY_ELEMS(sdl_texture_format_map) - 1;
         i++) {
        if (frame->format == sdl_texture_format_map[i].format) {
            sdl_pix_fmt = sdl_texture_format_map[i].texture_fmt;
            break;
        }
    }

    if (S_FMV_ReallocTexture(
            tex,
            sdl_pix_fmt == SDL_PIXELFORMAT_UNKNOWN ? SDL_PIXELFORMAT_ARGB8888
                                                   : sdl_pix_fmt,
            frame->width, frame->height, sdl_blendmode, 0)
        < 0) {
        return -1;
    }

    switch (sdl_pix_fmt) {
    case SDL_PIXELFORMAT_UNKNOWN:
        *img_convert_ctx = sws_getCachedContext(
            *img_convert_ctx, frame->width, frame->height, frame->format,
            frame->width, frame->height, AV_PIX_FMT_BGRA, SWS_BICUBIC, NULL,
            NULL, NULL);
        if (*img_convert_ctx != NULL) {
            uint8_t *pixels[4];
            int pitch[4];
            if (!SDL_LockTexture(*tex, NULL, (void **)pixels, pitch)) {
                sws_scale(
                    *img_convert_ctx, (const uint8_t *const *)frame->data,
                    frame->linesize, 0, frame->height, pixels, pitch);
                SDL_UnlockTexture(*tex);
            }
        } else {
            LOG_ERROR("Cannot initialize the conversion context");
            ret = -1;
        }
        break;

    case SDL_PIXELFORMAT_IYUV:
        if (frame->linesize[0] > 0 && frame->linesize[1] > 0
            && frame->linesize[2] > 0) {
            ret = SDL_UpdateYUVTexture(
                *tex, NULL, frame->data[0], frame->linesize[0], frame->data[1],
                frame->linesize[1], frame->data[2], frame->linesize[2]);
        } else if (
            frame->linesize[0] < 0 && frame->linesize[1] < 0
            && frame->linesize[2] < 0) {
            ret = SDL_UpdateYUVTexture(
                *tex, NULL,
                frame->data[0] + frame->linesize[0] * (frame->height - 1),
                -frame->linesize[0],
                frame->data[1]
                    + frame->linesize[1]
                        * (AV_CEIL_RSHIFT(frame->height, 1) - 1),
                -frame->linesize[1],
                frame->data[2]
                    + frame->linesize[2]
                        * (AV_CEIL_RSHIFT(frame->height, 1) - 1),
                -frame->linesize[2]);
        } else {
            LOG_ERROR(
                "Mixed negative and positive linesizes are not supported.");
            return -1;
        }
        break;

    default:
        if (frame->linesize[0] < 0) {
            ret = SDL_UpdateTexture(
                *tex, NULL,
                frame->data[0] + frame->linesize[0] * (frame->height - 1),
                -frame->linesize[0]);
        } else {
            ret = SDL_UpdateTexture(
                *tex, NULL, frame->data[0], frame->linesize[0]);
        }
        break;
    }

    return ret;
}

static void S_FMV_SetSDLYUVConversionMode(AVFrame *frame)
{
    SDL_YUV_CONVERSION_MODE mode = SDL_YUV_CONVERSION_AUTOMATIC;
    if (frame
        && (frame->format == AV_PIX_FMT_YUV420P
            || frame->format == AV_PIX_FMT_YUYV422
            || frame->format == AV_PIX_FMT_UYVY422)) {
        if (frame->color_range == AVCOL_RANGE_JPEG) {
            mode = SDL_YUV_CONVERSION_JPEG;
        } else if (frame->colorspace == AVCOL_SPC_BT709) {
            mode = SDL_YUV_CONVERSION_BT709;
        } else if (
            frame->colorspace == AVCOL_SPC_BT470BG
            || frame->colorspace == AVCOL_SPC_SMPTE170M) {
            mode = SDL_YUV_CONVERSION_BT601;
        }
    }
    SDL_SetYUVConversionMode(mode);
}

static void S_FMV_VideoImageDisplay(VideoState *is)
{
    Frame *vp;
    Frame *sp = NULL;
    SDL_Rect rect;

    vp = S_FMV_FrameQueuePeekLast(&is->pictq);
    if (is->subtitle_st) {
        if (S_FMV_FrameQueueNBRemaining(&is->subpq) > 0) {
            sp = S_FMV_FrameQueuePeek(&is->subpq);

            if (vp->pts
                >= sp->pts + ((float)sp->sub.start_display_time / 1000)) {
                if (!sp->uploaded) {
                    uint8_t *pixels[4];
                    int pitch[4];
                    if (!sp->width || !sp->height) {
                        sp->width = vp->width;
                        sp->height = vp->height;
                    }
                    if (S_FMV_ReallocTexture(
                            &is->sub_texture, SDL_PIXELFORMAT_ARGB8888,
                            sp->width, sp->height, SDL_BLENDMODE_BLEND, 1)
                        < 0) {
                        return;
                    }

                    for (int i = 0; i < (signed)sp->sub.num_rects; i++) {
                        AVSubtitleRect *sub_rect = sp->sub.rects[i];

                        sub_rect->x = av_clip(sub_rect->x, 0, sp->width);
                        sub_rect->y = av_clip(sub_rect->y, 0, sp->height);
                        sub_rect->w =
                            av_clip(sub_rect->w, 0, sp->width - sub_rect->x);
                        sub_rect->h =
                            av_clip(sub_rect->h, 0, sp->height - sub_rect->y);

                        is->sub_convert_ctx = sws_getCachedContext(
                            is->sub_convert_ctx, sub_rect->w, sub_rect->h,
                            AV_PIX_FMT_PAL8, sub_rect->w, sub_rect->h,
                            AV_PIX_FMT_BGRA, 0, NULL, NULL, NULL);
                        if (!is->sub_convert_ctx) {
                            LOG_ERROR(
                                "Cannot initialize the conversion context");
                            return;
                        }
                        if (!SDL_LockTexture(
                                is->sub_texture, (SDL_Rect *)sub_rect,
                                (void **)pixels, pitch)) {
                            sws_scale(
                                is->sub_convert_ctx,
                                (const uint8_t *const *)sub_rect->data,
                                sub_rect->linesize, 0, sub_rect->h, pixels,
                                pitch);
                            SDL_UnlockTexture(is->sub_texture);
                        }
                    }
                    sp->uploaded = true;
                }
            } else {
                sp = NULL;
            }
        }
    }

    S_FMV_CalculateDisplayRect(
        &rect, 0, 0, is->width, is->height, vp->width, vp->height, vp->sar);

    if (!vp->uploaded) {
        if (S_FMV_UploadTexture(
                &is->vid_texture, vp->frame, &is->img_convert_ctx)
            < 0) {
            return;
        }
        vp->uploaded = true;
        vp->flip_v = vp->frame->linesize[0] < 0;
    }

    S_FMV_SetSDLYUVConversionMode(vp->frame);
    SDL_RenderCopyEx(
        m_Renderer, is->vid_texture, NULL, &rect, 0, NULL,
        vp->flip_v ? SDL_FLIP_VERTICAL : 0);
    S_FMV_SetSDLYUVConversionMode(NULL);
    if (sp) {
        SDL_RenderCopy(m_Renderer, is->sub_texture, NULL, &rect);
    }
}

static void S_FMV_StreamComponentClose(VideoState *is, int stream_index)
{
    AVFormatContext *ic = is->ic;
    AVCodecParameters *codecpar;

    if (stream_index < 0 || stream_index >= (signed)ic->nb_streams) {
        return;
    }
    codecpar = ic->streams[stream_index]->codecpar;

    switch (codecpar->codec_type) {
    case AVMEDIA_TYPE_AUDIO:
        S_FMV_DecoderAbort(&is->auddec, &is->sampq);
        SDL_CloseAudioDevice(m_AudioDevice);
        S_FMV_DecoderShutdown(&is->auddec);
        swr_free(&is->swr_ctx);
        av_freep(&is->audio_buf1);
        is->audio_buf1_size = 0;
        is->audio_buf = NULL;

        break;
    case AVMEDIA_TYPE_VIDEO:
        S_FMV_DecoderAbort(&is->viddec, &is->pictq);
        S_FMV_DecoderShutdown(&is->viddec);
        break;
    case AVMEDIA_TYPE_SUBTITLE:
        S_FMV_DecoderAbort(&is->subdec, &is->subpq);
        S_FMV_DecoderShutdown(&is->subdec);
        break;
    default:
        break;
    }

    ic->streams[stream_index]->discard = AVDISCARD_ALL;
    switch (codecpar->codec_type) {
    case AVMEDIA_TYPE_AUDIO:
        is->audio_st = NULL;
        is->audio_stream = -1;
        break;
    case AVMEDIA_TYPE_VIDEO:
        is->video_st = NULL;
        is->video_stream = -1;
        break;
    case AVMEDIA_TYPE_SUBTITLE:
        is->subtitle_st = NULL;
        is->subtitle_stream = -1;
        break;
    default:
        break;
    }
}

static void S_FMV_StreamClose(VideoState *is)
{
    SDL_WaitThread(is->read_tid, NULL);

    if (is->audio_stream >= 0) {
        S_FMV_StreamComponentClose(is, is->audio_stream);
    }
    if (is->video_stream >= 0) {
        S_FMV_StreamComponentClose(is, is->video_stream);
    }
    if (is->subtitle_stream >= 0) {
        S_FMV_StreamComponentClose(is, is->subtitle_stream);
    }

    avformat_close_input(&is->ic);

    S_FMV_PacketQueueDestroy(&is->videoq);
    S_FMV_PacketQueueDestroy(&is->audioq);
    S_FMV_PacketQueueDestroy(&is->subtitleq);

    S_FMV_FrameQueueShutdown(&is->pictq);
    S_FMV_FrameQueueShutdown(&is->sampq);
    S_FMV_FrameQueueShutdown(&is->subpq);
    SDL_DestroyCond(is->continue_read_thread);
    sws_freeContext(is->img_convert_ctx);
    sws_freeContext(is->sub_convert_ctx);
    av_free(is->filename);
    if (is->vid_texture) {
        SDL_DestroyTexture(is->vid_texture);
    }
    if (is->sub_texture) {
        SDL_DestroyTexture(is->sub_texture);
    }
    av_free(is);
}

static void S_FMV_VideoDisplay(VideoState *is)
{
    SDL_SetRenderDrawColor(m_Renderer, 0, 0, 0, 255);
    SDL_RenderClear(m_Renderer);
    if (is->video_st) {
        S_FMV_VideoImageDisplay(is);
    }
    SDL_RenderPresent(m_Renderer);
}

static double S_FMV_GetClock(Clock *c)
{
    if (*c->queue_serial != c->serial) {
        return NAN;
    }
    if (c->paused) {
        return c->pts;
    } else {
        double time = av_gettime_relative() / 1000000.0;
        return c->pts_drift + time
            - (time - c->last_updated) * (1.0 - c->speed);
    }
}

static void S_FMV_SetClockAt(Clock *c, double pts, int serial, double time)
{
    c->pts = pts;
    c->last_updated = time;
    c->pts_drift = c->pts - time;
    c->serial = serial;
}

static void S_FMV_SetClock(Clock *c, double pts, int serial)
{
    double time = av_gettime_relative() / 1000000.0;
    S_FMV_SetClockAt(c, pts, serial, time);
}

static void S_FMV_InitClock(Clock *c, int *queue_serial)
{
    c->speed = 1.0;
    c->paused = false;
    c->queue_serial = queue_serial;
    S_FMV_SetClock(c, NAN, -1);
}

static void S_FMV_SyncClockToSlave(Clock *c, Clock *slave)
{
    double clock = S_FMV_GetClock(c);
    double slave_clock = S_FMV_GetClock(slave);
    if (!isnan(slave_clock)
        && (isnan(clock) || fabs(clock - slave_clock) > AV_NOSYNC_THRESHOLD)) {
        S_FMV_SetClock(c, slave_clock, slave->serial);
    }
}

static int S_FMV_GetMasterSyncType(VideoState *is)
{
    if (is->av_sync_type == AV_SYNC_VIDEO_MASTER) {
        if (is->video_st) {
            return AV_SYNC_VIDEO_MASTER;
        } else {
            return AV_SYNC_AUDIO_MASTER;
        }
    } else if (is->av_sync_type == AV_SYNC_AUDIO_MASTER) {
        if (is->audio_st) {
            return AV_SYNC_AUDIO_MASTER;
        } else {
            return AV_SYNC_EXTERNAL_CLOCK;
        }
    } else {
        return AV_SYNC_EXTERNAL_CLOCK;
    }
}

static double S_FMV_GetMasterClock(VideoState *is)
{
    switch (S_FMV_GetMasterSyncType(is)) {
    case AV_SYNC_VIDEO_MASTER:
        return S_FMV_GetClock(&is->vidclk);

    case AV_SYNC_AUDIO_MASTER:
        return S_FMV_GetClock(&is->audclk);

    default:
        return S_FMV_GetClock(&is->extclk);
    }
}

static double S_FMV_ComputeTargetDelay(double delay, VideoState *is)
{
    double sync_threshold, diff = 0;

    if (S_FMV_GetMasterSyncType(is) != AV_SYNC_VIDEO_MASTER) {
        diff = S_FMV_GetClock(&is->vidclk) - S_FMV_GetMasterClock(is);

        sync_threshold =
            FFMAX(AV_SYNC_THRESHOLD_MIN, FFMIN(AV_SYNC_THRESHOLD_MAX, delay));
        if (!isnan(diff) && fabs(diff) < is->max_frame_duration) {
            if (diff <= -sync_threshold) {
                delay = FFMAX(0, delay + diff);
            } else if (
                diff >= sync_threshold && delay > AV_SYNC_FRAMEDUP_THRESHOLD) {
                delay = delay + diff;
            } else if (diff >= sync_threshold) {
                delay = 2 * delay;
            }
        }
    }

    return delay;
}

static double S_FMV_VPDuration(VideoState *is, Frame *vp, Frame *nextvp)
{
    if (vp->serial == nextvp->serial) {
        double duration = nextvp->pts - vp->pts;
        if (isnan(duration) || duration <= 0
            || duration > is->max_frame_duration) {
            return vp->duration;
        } else {
            return duration;
        }
    } else {
        return 0.0;
    }
}

static void S_FMV_UpdateVideoPTS(
    VideoState *is, double pts, int64_t pos, int serial)
{
    S_FMV_SetClock(&is->vidclk, pts, serial);
    S_FMV_SyncClockToSlave(&is->extclk, &is->vidclk);
}

static void S_FMV_VideoRefresh(void *opaque, double *remaining_time)
{
    VideoState *is = opaque;
    double time;

    Frame *sp, *sp2;

    if (is->video_st) {
    retry:
        if (S_FMV_FrameQueueNBRemaining(&is->pictq) != 0) {
            double last_duration, duration, delay;
            Frame *vp, *lastvp;

            lastvp = S_FMV_FrameQueuePeekLast(&is->pictq);
            vp = S_FMV_FrameQueuePeek(&is->pictq);

            if (vp->serial != is->videoq.serial) {
                S_FMV_FrameQueueNext(&is->pictq);
                goto retry;
            }

            if (lastvp->serial != vp->serial) {
                is->frame_timer = av_gettime_relative() / 1000000.0;
            }

            if (is->paused) {
                goto display;
            }

            last_duration = S_FMV_VPDuration(is, lastvp, vp);
            delay = S_FMV_ComputeTargetDelay(last_duration, is);

            time = av_gettime_relative() / 1000000.0;
            if (time < is->frame_timer + delay) {
                *remaining_time =
                    FFMIN(is->frame_timer + delay - time, *remaining_time);
                goto display;
            }

            is->frame_timer += delay;
            if (delay > 0 && time - is->frame_timer > AV_SYNC_THRESHOLD_MAX) {
                is->frame_timer = time;
            }

            SDL_LockMutex(is->pictq.mutex);
            if (!isnan(vp->pts)) {
                S_FMV_UpdateVideoPTS(is, vp->pts, vp->pos, vp->serial);
            }
            SDL_UnlockMutex(is->pictq.mutex);

            if (S_FMV_FrameQueueNBRemaining(&is->pictq) > 1) {
                Frame *nextvp = S_FMV_FrameQueuePeekNext(&is->pictq);
                duration = S_FMV_VPDuration(is, vp, nextvp);
                if (S_FMV_GetMasterSyncType(is) != AV_SYNC_VIDEO_MASTER
                    && time > is->frame_timer + duration) {
                    is->frame_drops_late++;
                    S_FMV_FrameQueueNext(&is->pictq);
                    goto retry;
                }
            }

            if (is->subtitle_st) {
                while (S_FMV_FrameQueueNBRemaining(&is->subpq) > 0) {
                    sp = S_FMV_FrameQueuePeek(&is->subpq);

                    if (S_FMV_FrameQueueNBRemaining(&is->subpq) > 1) {
                        sp2 = S_FMV_FrameQueuePeekNext(&is->subpq);
                    } else {
                        sp2 = NULL;
                    }

                    if (sp->serial != is->subtitleq.serial
                        || (is->vidclk.pts
                            > (sp->pts
                               + ((float)sp->sub.end_display_time / 1000)))
                        || (sp2
                            && is->vidclk.pts
                                > (sp2->pts
                                   + ((float)sp2->sub.start_display_time
                                      / 1000)))) {
                        if (sp->uploaded) {
                            for (int i = 0; i < (signed)sp->sub.num_rects;
                                 i++) {
                                AVSubtitleRect *sub_rect = sp->sub.rects[i];
                                uint8_t *pixels;
                                int pitch;

                                if (!SDL_LockTexture(
                                        is->sub_texture, (SDL_Rect *)sub_rect,
                                        (void **)&pixels, &pitch)) {
                                    for (int j = 0; j < sub_rect->h;
                                         j++, pixels += pitch) {
                                        memset(pixels, 0, sub_rect->w << 2);
                                    }
                                    SDL_UnlockTexture(is->sub_texture);
                                }
                            }
                        }
                        S_FMV_FrameQueueNext(&is->subpq);
                    } else {
                        break;
                    }
                }
            }

            S_FMV_FrameQueueNext(&is->pictq);
            is->force_refresh = true;
        }

    display:
        if (is->force_refresh && is->pictq.rindex_shown) {
            S_FMV_VideoDisplay(is);
        }
    }
    is->force_refresh = false;
}

static int S_FMV_QueuePicture(
    VideoState *is, AVFrame *src_frame, double pts, double duration,
    int64_t pos, int serial)
{
    Frame *vp;

    if (!(vp = S_FMV_FrameQueuePeekWritable(&is->pictq))) {
        return -1;
    }

    vp->sar = src_frame->sample_aspect_ratio;
    vp->uploaded = false;

    vp->width = src_frame->width;
    vp->height = src_frame->height;
    vp->format = src_frame->format;

    vp->pts = pts;
    vp->duration = duration;
    vp->pos = pos;
    vp->serial = serial;

    av_frame_move_ref(vp->frame, src_frame);
    S_FMV_FrameQueuePush(&is->pictq);
    return 0;
}

static int S_FMV_GetVideoFrame(VideoState *is, AVFrame *frame)
{
    int got_picture;

    if ((got_picture = S_FMV_DecoderDecodeFrame(&is->viddec, frame, NULL))
        < 0) {
        return -1;
    }

    if (got_picture) {
        double dpts = NAN;

        if (frame->pts != AV_NOPTS_VALUE) {
            dpts = av_q2d(is->video_st->time_base) * frame->pts;
        }

        frame->sample_aspect_ratio =
            av_guess_sample_aspect_ratio(is->ic, is->video_st, frame);

        if (S_FMV_GetMasterSyncType(is) != AV_SYNC_VIDEO_MASTER) {
            if (frame->pts != AV_NOPTS_VALUE) {
                double diff = dpts - S_FMV_GetMasterClock(is);
                if (!isnan(diff) && fabs(diff) < AV_NOSYNC_THRESHOLD && diff < 0
                    && is->viddec.pkt_serial == is->vidclk.serial
                    && is->videoq.nb_packets) {
                    is->frame_drops_early++;
                    av_frame_unref(frame);
                    got_picture = 0;
                }
            }
        }
    }

    return got_picture;
}

static int S_FMV_AudioThread(void *arg)
{
    VideoState *is = arg;
    AVFrame *frame = av_frame_alloc();
    Frame *af;
    int got_frame = 0;
    AVRational tb;
    int ret = 0;

    if (!frame) {
        return AVERROR(ENOMEM);
    }

    do {
        if ((got_frame = S_FMV_DecoderDecodeFrame(&is->auddec, frame, NULL))
            < 0) {
            goto the_end;
        }

        if (got_frame) {
            tb = (AVRational) { 1, frame->sample_rate };

            if (!(af = S_FMV_FrameQueuePeekWritable(&is->sampq))) {
                goto the_end;
            }

            af->pts =
                (frame->pts == AV_NOPTS_VALUE) ? NAN : frame->pts * av_q2d(tb);
            af->pos = frame->pkt_pos;
            af->serial = is->auddec.pkt_serial;
            af->duration =
                av_q2d((AVRational) { frame->nb_samples, frame->sample_rate });

            av_frame_move_ref(af->frame, frame);
            S_FMV_FrameQueuePush(&is->sampq);
        }
    } while (ret >= 0 || ret == AVERROR(EAGAIN) || ret == AVERROR_EOF);
the_end:
    av_frame_free(&frame);
    return ret;
}

static int S_FMV_DecoderStart(
    Decoder *d, int (*fn)(void *), const char *thread_name, void *arg)
{
    S_FMV_PacketQueueStart(d->queue);
    d->decoder_tid = SDL_CreateThread(fn, thread_name, arg);
    if (!d->decoder_tid) {
        LOG_ERROR("SDL_CreateThread(): %s", SDL_GetError());
        return AVERROR(ENOMEM);
    }
    return 0;
}

static int S_FMV_VideoThread(void *arg)
{
    VideoState *is = arg;
    AVFrame *frame = av_frame_alloc();
    double pts;
    double duration;
    int ret;
    AVRational tb = is->video_st->time_base;
    AVRational frame_rate = av_guess_frame_rate(is->ic, is->video_st, NULL);

    if (!frame) {
        return AVERROR(ENOMEM);
    }

    while (1) {
        ret = S_FMV_GetVideoFrame(is, frame);
        if (ret < 0) {
            goto the_end;
        }
        if (!ret) {
            continue;
        }

        duration =
            (frame_rate.num && frame_rate.den
                 ? av_q2d((AVRational) { frame_rate.den, frame_rate.num })
                 : 0);
        pts = (frame->pts == AV_NOPTS_VALUE) ? NAN : frame->pts * av_q2d(tb);
        ret = S_FMV_QueuePicture(
            is, frame, pts, duration, frame->pkt_pos, is->viddec.pkt_serial);
        av_frame_unref(frame);

        if (ret < 0) {
            goto the_end;
        }
    }
the_end:
    av_frame_free(&frame);
    return 0;
}

static int S_FMV_SubtitleThread(void *arg)
{
    VideoState *is = arg;
    Frame *sp;
    int got_subtitle;
    double pts;

    while (1) {
        if (!(sp = S_FMV_FrameQueuePeekWritable(&is->subpq))) {
            return 0;
        }

        if ((got_subtitle =
                 S_FMV_DecoderDecodeFrame(&is->subdec, NULL, &sp->sub))
            < 0) {
            break;
        }

        pts = 0;

        if (got_subtitle && sp->sub.format == 0) {
            if (sp->sub.pts != AV_NOPTS_VALUE) {
                pts = sp->sub.pts / (double)AV_TIME_BASE;
            }
            sp->pts = pts;
            sp->serial = is->subdec.pkt_serial;
            sp->width = is->subdec.avctx->width;
            sp->height = is->subdec.avctx->height;
            sp->uploaded = false;

            S_FMV_FrameQueuePush(&is->subpq);
        } else if (got_subtitle) {
            avsubtitle_free(&sp->sub);
        }
    }

    return 0;
}

static int S_FMV_SynchronizeAudio(VideoState *is, int nb_samples)
{
    int wanted_nb_samples = nb_samples;

    if (S_FMV_GetMasterSyncType(is) != AV_SYNC_AUDIO_MASTER) {
        double diff, avg_diff;
        int min_nb_samples, max_nb_samples;

        diff = S_FMV_GetClock(&is->audclk) - S_FMV_GetMasterClock(is);

        if (!isnan(diff) && fabs(diff) < AV_NOSYNC_THRESHOLD) {
            is->audio_diff_cum =
                diff + is->audio_diff_avg_coef * is->audio_diff_cum;
            if (is->audio_diff_avg_count < AUDIO_DIFF_AVG_NB) {
                is->audio_diff_avg_count++;
            } else {
                avg_diff = is->audio_diff_cum * (1.0 - is->audio_diff_avg_coef);

                if (fabs(avg_diff) >= is->audio_diff_threshold) {
                    wanted_nb_samples =
                        nb_samples + (int)(diff * is->audio_src.freq);
                    min_nb_samples =
                        ((nb_samples * (100 - SAMPLE_CORRECTION_PERCENT_MAX)
                          / 100));
                    max_nb_samples =
                        ((nb_samples * (100 + SAMPLE_CORRECTION_PERCENT_MAX)
                          / 100));
                    wanted_nb_samples = av_clip(
                        wanted_nb_samples, min_nb_samples, max_nb_samples);
                }
            }
        } else {
            is->audio_diff_avg_count = 0;
            is->audio_diff_cum = 0;
        }
    }

    return wanted_nb_samples;
}

static int S_FMV_AudioDecodeFrame(VideoState *is)
{
    int data_size, resampled_data_size;
    av_unused double audio_clock0;
    int wanted_nb_samples;
    Frame *af;

    if (is->paused) {
        return -1;
    }

    do {
        if (!(af = S_FMV_FrameQueuePeekReadable(&is->sampq))) {
            return -1;
        }
        S_FMV_FrameQueueNext(&is->sampq);
    } while (af->serial != is->audioq.serial);

    data_size = av_samples_get_buffer_size(
        NULL, af->frame->channels, af->frame->nb_samples, af->frame->format, 1);

    int64_t dec_channel_layout =
        (af->frame->channel_layout
         && af->frame->channels
             == av_get_channel_layout_nb_channels(af->frame->channel_layout))
        ? (signed)af->frame->channel_layout
        : av_get_default_channel_layout(af->frame->channels);
    wanted_nb_samples = S_FMV_SynchronizeAudio(is, af->frame->nb_samples);

    if (af->frame->format != is->audio_src.fmt
        || dec_channel_layout != is->audio_src.channel_layout
        || af->frame->sample_rate != is->audio_src.freq
        || (wanted_nb_samples != af->frame->nb_samples && !is->swr_ctx)) {
        swr_free(&is->swr_ctx);
        is->swr_ctx = swr_alloc_set_opts(
            NULL, is->audio_tgt.channel_layout, is->audio_tgt.fmt,
            is->audio_tgt.freq, dec_channel_layout, af->frame->format,
            af->frame->sample_rate, 0, NULL);
        if (!is->swr_ctx || swr_init(is->swr_ctx) < 0) {
            LOG_ERROR(
                "Cannot create sample rate converter for conversion of %d Hz "
                "%s %d channels to %d Hz %s %d channels!",
                af->frame->sample_rate,
                av_get_sample_fmt_name(af->frame->format), af->frame->channels,
                is->audio_tgt.freq, av_get_sample_fmt_name(is->audio_tgt.fmt),
                is->audio_tgt.channels);
            swr_free(&is->swr_ctx);
            return -1;
        }
        is->audio_src.channel_layout = dec_channel_layout;
        is->audio_src.channels = af->frame->channels;
        is->audio_src.freq = af->frame->sample_rate;
        is->audio_src.fmt = af->frame->format;
    }

    if (is->swr_ctx) {
        const uint8_t **in = (const uint8_t **)af->frame->extended_data;
        uint8_t **out = &is->audio_buf1;
        int out_count = (int64_t)wanted_nb_samples * is->audio_tgt.freq
                / af->frame->sample_rate
            + 256;
        int out_size = av_samples_get_buffer_size(
            NULL, is->audio_tgt.channels, out_count, is->audio_tgt.fmt, 0);
        int len2;
        if (out_size < 0) {
            LOG_ERROR("av_samples_get_buffer_size() failed");
            return -1;
        }
        if (wanted_nb_samples != af->frame->nb_samples) {
            if (swr_set_compensation(
                    is->swr_ctx,
                    (wanted_nb_samples - af->frame->nb_samples)
                        * is->audio_tgt.freq / af->frame->sample_rate,
                    wanted_nb_samples * is->audio_tgt.freq
                        / af->frame->sample_rate)
                < 0) {
                LOG_ERROR("swr_set_compensation() failed");
                return -1;
            }
        }
        av_fast_malloc(&is->audio_buf1, &is->audio_buf1_size, out_size);
        if (!is->audio_buf1) {
            return AVERROR(ENOMEM);
        }
        len2 =
            swr_convert(is->swr_ctx, out, out_count, in, af->frame->nb_samples);
        if (len2 < 0) {
            LOG_ERROR("swr_convert() failed");
            return -1;
        }
        if (len2 == out_count) {
            if (swr_init(is->swr_ctx) < 0) {
                swr_free(&is->swr_ctx);
            }
        }
        is->audio_buf = is->audio_buf1;
        resampled_data_size = len2 * is->audio_tgt.channels
            * av_get_bytes_per_sample(is->audio_tgt.fmt);
    } else {
        is->audio_buf = af->frame->data[0];
        resampled_data_size = data_size;
    }

    audio_clock0 = is->audio_clock;
    if (!isnan(af->pts)) {
        is->audio_clock =
            af->pts + (double)af->frame->nb_samples / af->frame->sample_rate;
    } else {
        is->audio_clock = NAN;
    }
    is->audio_clock_serial = af->serial;
    return resampled_data_size;
}

static void S_FMV_SDLAudioCallback(void *opaque, Uint8 *stream, int len)
{
    VideoState *is = opaque;
    int audio_size, len1;

    m_AudioCallbackTime = av_gettime_relative();

    while (len > 0) {
        if (is->audio_buf_index >= (signed)is->audio_buf_size) {
            audio_size = S_FMV_AudioDecodeFrame(is);
            if (audio_size < 0) {
                is->audio_buf = NULL;
                is->audio_buf_size = SDL_AUDIO_MIN_BUFFER_SIZE
                    / is->audio_tgt.frame_size * is->audio_tgt.frame_size;
            } else {
                is->audio_buf_size = audio_size;
            }
            is->audio_buf_index = 0;
        }
        len1 = is->audio_buf_size - is->audio_buf_index;
        if (len1 > len) {
            len1 = len;
        }
        if (is->audio_buf && is->audio_volume == SDL_MIX_MAXVOLUME) {
            memcpy(
                stream, (uint8_t *)is->audio_buf + is->audio_buf_index, len1);
        } else {
            memset(stream, 0, len1);
            if (is->audio_buf) {
                SDL_MixAudioFormat(
                    stream, (uint8_t *)is->audio_buf + is->audio_buf_index,
                    AUDIO_S16SYS, len1, is->audio_volume);
            }
        }
        len -= len1;
        stream += len1;
        is->audio_buf_index += len1;
    }
    is->audio_write_buf_size = is->audio_buf_size - is->audio_buf_index;
    if (!isnan(is->audio_clock)) {
        S_FMV_SetClockAt(
            &is->audclk,
            is->audio_clock
                - (double)(2 * is->audio_hw_buf_size + is->audio_write_buf_size)
                    / is->audio_tgt.bytes_per_sec,
            is->audio_clock_serial, m_AudioCallbackTime / 1000000.0);
        S_FMV_SyncClockToSlave(&is->extclk, &is->audclk);
    }
}

static int S_FMV_AudioOpen(
    void *opaque, int64_t wanted_channel_layout, int wanted_nb_channels,
    int wanted_sample_rate, struct AudioParams *audio_hw_params)
{
    SDL_AudioSpec wanted_spec, spec;
    const char *env;
    static const int next_nb_channels[] = { 0, 0, 1, 6, 2, 6, 4, 6 };
    static const int next_sample_rates[] = { 0, 44100, 48000, 96000, 192000 };
    int next_sample_rate_idx = FF_ARRAY_ELEMS(next_sample_rates) - 1;

    env = SDL_getenv("SDL_AUDIO_CHANNELS");
    if (env) {
        wanted_nb_channels = atoi(env);
        wanted_channel_layout =
            av_get_default_channel_layout(wanted_nb_channels);
    }
    if (!wanted_channel_layout
        || wanted_nb_channels
            != av_get_channel_layout_nb_channels(wanted_channel_layout)) {
        wanted_channel_layout =
            av_get_default_channel_layout(wanted_nb_channels);
        wanted_channel_layout &= ~AV_CH_LAYOUT_STEREO_DOWNMIX;
    }
    wanted_nb_channels =
        av_get_channel_layout_nb_channels(wanted_channel_layout);
    wanted_spec.channels = wanted_nb_channels;
    wanted_spec.freq = wanted_sample_rate;
    if (wanted_spec.freq <= 0 || wanted_spec.channels <= 0) {
        LOG_ERROR("Invalid sample rate or channel count!");
        return -1;
    }
    while (next_sample_rate_idx
           && next_sample_rates[next_sample_rate_idx] >= wanted_spec.freq)
        next_sample_rate_idx--;
    wanted_spec.format = AUDIO_S16SYS;
    wanted_spec.silence = 0;
    wanted_spec.samples = FFMAX(
        SDL_AUDIO_MIN_BUFFER_SIZE,
        2 << av_log2(wanted_spec.freq / SDL_AUDIO_MAX_CALLBACKS_PER_SEC));
    wanted_spec.callback = S_FMV_SDLAudioCallback;
    wanted_spec.userdata = opaque;
    while (
        !(m_AudioDevice = SDL_OpenAudioDevice(
              NULL, 0, &wanted_spec, &spec,
              SDL_AUDIO_ALLOW_FREQUENCY_CHANGE
                  | SDL_AUDIO_ALLOW_CHANNELS_CHANGE))) {
        wanted_spec.channels = next_nb_channels[FFMIN(7, wanted_spec.channels)];
        if (!wanted_spec.channels) {
            wanted_spec.freq = next_sample_rates[next_sample_rate_idx--];
            wanted_spec.channels = wanted_nb_channels;
            if (!wanted_spec.freq) {
                LOG_ERROR("No more combinations to try, audio open failed");
                return -1;
            }
        }
        wanted_channel_layout =
            av_get_default_channel_layout(wanted_spec.channels);
    }
    if (spec.format != AUDIO_S16SYS) {
        LOG_ERROR("SDL advised audio format %d is not supported!", spec.format);
        return -1;
    }
    if (spec.channels != wanted_spec.channels) {
        wanted_channel_layout = av_get_default_channel_layout(spec.channels);
        if (!wanted_channel_layout) {
            LOG_ERROR(
                "SDL advised channel count %d is not supported!",
                spec.channels);
            return -1;
        }
    }

    audio_hw_params->fmt = AV_SAMPLE_FMT_S16;
    audio_hw_params->freq = spec.freq;
    audio_hw_params->channel_layout = wanted_channel_layout;
    audio_hw_params->channels = spec.channels;
    audio_hw_params->frame_size = av_samples_get_buffer_size(
        NULL, audio_hw_params->channels, 1, audio_hw_params->fmt, 1);
    audio_hw_params->bytes_per_sec = av_samples_get_buffer_size(
        NULL, audio_hw_params->channels, audio_hw_params->freq,
        audio_hw_params->fmt, 1);
    if (audio_hw_params->bytes_per_sec <= 0
        || audio_hw_params->frame_size <= 0) {
        LOG_ERROR("av_samples_get_buffer_size failed");
        return -1;
    }
    return spec.size;
}

static int S_FMV_StreamComponentOpen(VideoState *is, int stream_index)
{
    AVFormatContext *ic = is->ic;
    AVCodecContext *avctx = NULL;
    const AVCodec *codec = NULL;
    const char *forced_codec_name = NULL;
    AVDictionary *opts = NULL;
    const AVDictionaryEntry *t = NULL;
    int sample_rate;
    int nb_channels;
    int64_t channel_layout;
    int ret = 0;

    if (stream_index < 0 || stream_index >= (signed)ic->nb_streams) {
        return -1;
    }

    avctx = avcodec_alloc_context3(NULL);
    if (!avctx) {
        return AVERROR(ENOMEM);
    }

    ret = avcodec_parameters_to_context(
        avctx, ic->streams[stream_index]->codecpar);
    if (ret < 0) {
        goto fail;
    }
    avctx->pkt_timebase = ic->streams[stream_index]->time_base;

    codec = avcodec_find_decoder(avctx->codec_id);

    if (!codec) {
        LOG_ERROR(
            "No decoder could be found for codec %s",
            avcodec_get_name(avctx->codec_id));
        ret = AVERROR(EINVAL);
        goto fail;
    }

    avctx->codec_id = codec->id;
    avctx->lowres = 0;

    if ((ret = avcodec_open2(avctx, codec, NULL)) < 0) {
        goto fail;
    }

    is->eof = false;
    ic->streams[stream_index]->discard = AVDISCARD_DEFAULT;
    switch (avctx->codec_type) {
    case AVMEDIA_TYPE_AUDIO:
        sample_rate = avctx->sample_rate;
        nb_channels = avctx->channels;
        channel_layout = avctx->channel_layout;

        if ((ret = S_FMV_AudioOpen(
                 is, channel_layout, nb_channels, sample_rate, &is->audio_tgt))
            < 0) {
            goto fail;
        }
        is->audio_hw_buf_size = ret;
        is->audio_src = is->audio_tgt;
        is->audio_buf_size = 0;
        is->audio_buf_index = 0;

        is->audio_diff_avg_coef = exp(log(0.01) / AUDIO_DIFF_AVG_NB);
        is->audio_diff_avg_count = 0;
        is->audio_diff_threshold =
            (double)(is->audio_hw_buf_size) / is->audio_tgt.bytes_per_sec;

        is->audio_stream = stream_index;
        is->audio_st = ic->streams[stream_index];

        if ((ret = S_FMV_DecoderInit(
                 &is->auddec, avctx, &is->audioq, is->continue_read_thread))
            < 0) {
            goto fail;
        }
        if ((is->ic->iformat->flags
             & (AVFMT_NOBINSEARCH | AVFMT_NOGENSEARCH | AVFMT_NO_BYTE_SEEK))
            && !is->ic->iformat->read_seek) {
            is->auddec.start_pts = is->audio_st->start_time;
            is->auddec.start_pts_tb = is->audio_st->time_base;
        }
        if ((ret = S_FMV_DecoderStart(
                 &is->auddec, S_FMV_AudioThread, "audio_decoder", is))
            < 0) {
            goto out;
        }
        SDL_PauseAudioDevice(m_AudioDevice, 0);
        break;

    case AVMEDIA_TYPE_VIDEO:
        is->video_stream = stream_index;
        is->video_st = ic->streams[stream_index];

        if ((ret = S_FMV_DecoderInit(
                 &is->viddec, avctx, &is->videoq, is->continue_read_thread))
            < 0) {
            goto fail;
        }
        if ((ret = S_FMV_DecoderStart(
                 &is->viddec, S_FMV_VideoThread, "video_decoder", is))
            < 0) {
            goto out;
        }
        is->queue_attachments_req = true;
        break;

    case AVMEDIA_TYPE_SUBTITLE:
        is->subtitle_stream = stream_index;
        is->subtitle_st = ic->streams[stream_index];

        if ((ret = S_FMV_DecoderInit(
                 &is->subdec, avctx, &is->subtitleq, is->continue_read_thread))
            < 0) {
            goto fail;
        }
        if ((ret = S_FMV_DecoderStart(
                 &is->subdec, S_FMV_SubtitleThread, "subtitle_decoder", is))
            < 0) {
            goto out;
        }
        break;

    default:
        break;
    }
    goto out;

fail:
    avcodec_free_context(&avctx);

out:
    return ret;
}

static int S_FMV_DecodeInterruptCB(void *ctx)
{
    VideoState *is = ctx;
    return is->abort_request;
}

static int S_FMV_StreamHasEnoughPackets(
    AVStream *st, int stream_id, PacketQueue *queue)
{
    return stream_id < 0 || queue->abort_request
        || (st->disposition & AV_DISPOSITION_ATTACHED_PIC)
        || (queue->nb_packets > MIN_FRAMES
            && (!queue->duration
                || av_q2d(st->time_base) * queue->duration > 1.0));
}

static int S_FMV_ReadThread(void *arg)
{
    VideoState *is = arg;
    AVFormatContext *ic = NULL;
    int err;
    int ret;
    int st_index[AVMEDIA_TYPE_NB];
    AVPacket *pkt = NULL;
    SDL_mutex *wait_mutex = SDL_CreateMutex();
    int64_t pkt_ts;

    if (!wait_mutex) {
        LOG_ERROR("SDL_CreateMutex(): %s", SDL_GetError());
        ret = AVERROR(ENOMEM);
        goto fail;
    }

    memset(st_index, -1, sizeof(st_index));
    is->eof = false;

    pkt = av_packet_alloc();
    if (!pkt) {
        LOG_ERROR("Could not allocate packet.");
        ret = AVERROR(ENOMEM);
        goto fail;
    }
    ic = avformat_alloc_context();
    if (!ic) {
        LOG_ERROR("Could not allocate context.");
        ret = AVERROR(ENOMEM);
        goto fail;
    }
    ic->interrupt_callback.callback = S_FMV_DecodeInterruptCB;
    ic->interrupt_callback.opaque = is;
    err = avformat_open_input(&ic, is->filename, is->iformat, NULL);
    if (err < 0) {
        LOG_ERROR("Error while opening file %s: %s", av_err2str(err));
        ret = -1;
        goto fail;
    }

    is->ic = ic;

    av_format_inject_global_side_data(ic);

    if (ic->pb) {
        ic->pb->eof_reached = 0;
    }

    is->max_frame_duration =
        (ic->iformat->flags & AVFMT_TS_DISCONT) ? 10.0 : 3600.0;

    for (int i = 0; i < (signed)ic->nb_streams; i++) {
        AVStream *st = ic->streams[i];
        enum AVMediaType type = st->codecpar->codec_type;
        st->discard = AVDISCARD_ALL;
    }

    st_index[AVMEDIA_TYPE_VIDEO] = av_find_best_stream(
        ic, AVMEDIA_TYPE_VIDEO, st_index[AVMEDIA_TYPE_VIDEO], -1, NULL, 0);

    st_index[AVMEDIA_TYPE_AUDIO] = av_find_best_stream(
        ic, AVMEDIA_TYPE_AUDIO, st_index[AVMEDIA_TYPE_AUDIO],
        st_index[AVMEDIA_TYPE_VIDEO], NULL, 0);

    st_index[AVMEDIA_TYPE_SUBTITLE] = av_find_best_stream(
        ic, AVMEDIA_TYPE_SUBTITLE, st_index[AVMEDIA_TYPE_SUBTITLE],
        (st_index[AVMEDIA_TYPE_AUDIO] >= 0 ? st_index[AVMEDIA_TYPE_AUDIO]
                                           : st_index[AVMEDIA_TYPE_VIDEO]),
        NULL, 0);

    if (st_index[AVMEDIA_TYPE_VIDEO] >= 0) {
        AVStream *st = ic->streams[st_index[AVMEDIA_TYPE_VIDEO]];
        AVCodecParameters *codecpar = st->codecpar;
        AVRational sar = av_guess_sample_aspect_ratio(ic, st, NULL);
    }

    if (st_index[AVMEDIA_TYPE_AUDIO] >= 0) {
        S_FMV_StreamComponentOpen(is, st_index[AVMEDIA_TYPE_AUDIO]);
    }

    ret = -1;
    if (st_index[AVMEDIA_TYPE_VIDEO] >= 0) {
        ret = S_FMV_StreamComponentOpen(is, st_index[AVMEDIA_TYPE_VIDEO]);
    }

    if (st_index[AVMEDIA_TYPE_SUBTITLE] >= 0) {
        S_FMV_StreamComponentOpen(is, st_index[AVMEDIA_TYPE_SUBTITLE]);
    }

    if (is->video_stream < 0 && is->audio_stream < 0) {
        LOG_ERROR(
            "Failed to open file '%s' or configure filtergraph", is->filename);
        ret = -1;
        goto fail;
    }

    while (1) {
        if (is->abort_request) {
            break;
        }
        if (is->paused != is->last_paused) {
            is->last_paused = is->paused;
            if (is->paused) {
                is->read_pause_return = av_read_pause(ic);
            } else {
                av_read_play(ic);
            }
        }
        if (is->queue_attachments_req) {
            if (is->video_st
                && is->video_st->disposition & AV_DISPOSITION_ATTACHED_PIC) {
                if ((ret = av_packet_ref(pkt, &is->video_st->attached_pic))
                    < 0) {
                    goto fail;
                }
                S_FMV_PacketQueuePut(&is->videoq, pkt);
                S_FMV_PacketQueuePutNullPacket(
                    &is->videoq, pkt, is->video_stream);
            }
            is->queue_attachments_req = false;
        }

        if (is->audioq.size + is->videoq.size + is->subtitleq.size
                > MAX_QUEUE_SIZE
            || (S_FMV_StreamHasEnoughPackets(
                    is->audio_st, is->audio_stream, &is->audioq)
                && S_FMV_StreamHasEnoughPackets(
                    is->video_st, is->video_stream, &is->videoq)
                && S_FMV_StreamHasEnoughPackets(
                    is->subtitle_st, is->subtitle_stream, &is->subtitleq))) {
            SDL_LockMutex(wait_mutex);
            SDL_CondWaitTimeout(is->continue_read_thread, wait_mutex, 10);
            SDL_UnlockMutex(wait_mutex);
            continue;
        }
        if (!is->paused
            && (!is->audio_st
                || (is->auddec.finished == is->audioq.serial
                    && S_FMV_FrameQueueNBRemaining(&is->sampq) == 0))
            && (!is->video_st
                || (is->viddec.finished == is->videoq.serial
                    && S_FMV_FrameQueueNBRemaining(&is->pictq) == 0))) {
            ret = AVERROR_EOF;
            goto fail;
        }
        ret = av_read_frame(ic, pkt);
        if (ret < 0) {
            if ((ret == AVERROR_EOF || avio_feof(ic->pb)) && !is->eof) {
                if (is->video_stream >= 0) {
                    S_FMV_PacketQueuePutNullPacket(
                        &is->videoq, pkt, is->video_stream);
                }
                if (is->audio_stream >= 0) {
                    S_FMV_PacketQueuePutNullPacket(
                        &is->audioq, pkt, is->audio_stream);
                }
                if (is->subtitle_stream >= 0) {
                    S_FMV_PacketQueuePutNullPacket(
                        &is->subtitleq, pkt, is->subtitle_stream);
                }
                is->eof = true;
            }
            if (ic->pb && ic->pb->error) {
                goto fail;
            }
            SDL_LockMutex(wait_mutex);
            SDL_CondWaitTimeout(is->continue_read_thread, wait_mutex, 10);
            SDL_UnlockMutex(wait_mutex);
            continue;
        } else {
            is->eof = false;
        }
        pkt_ts = pkt->pts == AV_NOPTS_VALUE ? pkt->dts : pkt->pts;
        if (pkt->stream_index == is->audio_stream) {
            S_FMV_PacketQueuePut(&is->audioq, pkt);
        } else if (
            pkt->stream_index == is->video_stream
            && !(is->video_st->disposition & AV_DISPOSITION_ATTACHED_PIC)) {
            S_FMV_PacketQueuePut(&is->videoq, pkt);
        } else if (pkt->stream_index == is->subtitle_stream) {
            S_FMV_PacketQueuePut(&is->subtitleq, pkt);
        } else {
            av_packet_unref(pkt);
        }
    }

    ret = 0;
fail:
    if (ic && !is->ic) {
        avformat_close_input(&ic);
    }

    av_packet_free(&pkt);
    if (ret != 0) {
        SDL_Event event;

        event.type = FF_QUIT_EVENT;
        event.user.data1 = is;
        SDL_PushEvent(&event);
    }
    SDL_DestroyMutex(wait_mutex);
    return 0;
}

static VideoState *S_FMV_StreamOpen(
    const char *filename, const AVInputFormat *iformat)
{
    VideoState *is;

    is = av_mallocz(sizeof(VideoState));
    if (!is) {
        return NULL;
    }
    is->video_stream = -1;
    is->audio_stream = -1;
    is->subtitle_stream = -1;
    is->filename = av_strdup(filename);
    if (!is->filename) {
        goto fail;
    }
    is->iformat = iformat;

    SDL_GetWindowSize(m_Window, &is->width, &is->height);

    if (S_FMV_FrameQueueInit(
            &is->pictq, &is->videoq, VIDEO_PICTURE_QUEUE_SIZE, 1)
        < 0) {
        goto fail;
    }
    if (S_FMV_FrameQueueInit(
            &is->subpq, &is->subtitleq, SUBPICTURE_QUEUE_SIZE, 0)
        < 0) {
        goto fail;
    }
    if (S_FMV_FrameQueueInit(&is->sampq, &is->audioq, SAMPLE_QUEUE_SIZE, 1)
        < 0) {
        goto fail;
    }

    if (S_FMV_PacketQueueInit(&is->videoq) < 0
        || S_FMV_PacketQueueInit(&is->audioq) < 0
        || S_FMV_PacketQueueInit(&is->subtitleq) < 0)
        goto fail;

    if (!(is->continue_read_thread = SDL_CreateCond())) {
        LOG_ERROR("SDL_CreateCond(): %s", SDL_GetError());
        goto fail;
    }

    S_FMV_InitClock(&is->vidclk, &is->videoq.serial);
    S_FMV_InitClock(&is->audclk, &is->audioq.serial);
    S_FMV_InitClock(&is->extclk, &is->extclk.serial);
    is->audio_clock_serial = -1;
    is->audio_volume = SDL_MIX_MAXVOLUME;
    is->av_sync_type = AV_SYNC_AUDIO_MASTER;
    is->read_tid = SDL_CreateThread(S_FMV_ReadThread, "read_thread", is);
    if (!is->read_tid) {
        LOG_ERROR("SDL_CreateThread(): %s", SDL_GetError());
    fail:
        S_FMV_StreamClose(is);
        return NULL;
    }
    return is;
}

static void S_FMV_RefreshLoopWaitEvent(VideoState *is, SDL_Event *event)
{
    double remaining_time = 0.0;
    SDL_PumpEvents();

    bool keypress = false;
    while (!is->abort_request
           && !SDL_PeepEvents(
               event, 1, SDL_GETEVENT, SDL_FIRSTEVENT, SDL_LASTEVENT)) {
        Input_Update();
        if (g_Input.deselect) {
            keypress = true;
        } else if (keypress && !g_Input.deselect) {
            is->abort_request = 1;
        }

        if (remaining_time > 0.0) {
            av_usleep((int64_t)(remaining_time * 1000000.0));
        }
        remaining_time = REFRESH_RATE;
        if (!is->paused || is->force_refresh) {
            S_FMV_VideoRefresh(is, &remaining_time);
        }
        SDL_PumpEvents();
    }
}

static void S_FMV_EventLoop(VideoState *is)
{
    SDL_Event event;

    while (!is->abort_request) {
        S_FMV_RefreshLoopWaitEvent(is, &event);

        switch (event.type) {
        case SDL_QUIT:
            is->abort_request = true;
            S_Shell_TerminateGame(0);
            break;

        case SDL_KEYDOWN:
            if (event.key.keysym.sym == SDLK_ESCAPE) {
                is->abort_request = true;
                break;
            }

            const Uint8 *keyboard_state = SDL_GetKeyboardState(NULL);
            if (keyboard_state[SDL_SCANCODE_LALT]
                && keyboard_state[SDL_SCANCODE_RETURN]) {
                S_Shell_ToggleFullscreen();
            }
            break;

        case SDL_WINDOWEVENT:
            switch (event.window.event) {
            case SDL_WINDOWEVENT_FOCUS_GAINED:
                is->audio_volume = SDL_MIX_MAXVOLUME;
                break;

            case SDL_WINDOWEVENT_FOCUS_LOST:
                is->audio_volume = 0;
                break;

            case SDL_WINDOWEVENT_SIZE_CHANGED:
                is->width = event.window.data1;
                is->height = event.window.data2;
                is->force_refresh = true;
                break;

            case SDL_WINDOWEVENT_EXPOSED:
                is->force_refresh = true;
                break;
            }
            break;

        case FF_QUIT_EVENT:
            is->abort_request = true;
            break;

        default:
            break;
        }
    }
}

bool S_FMV_Init()
{
    return true;
}

bool S_FMV_Play(const char *file_path)
{
    bool ret = false;
    if (g_Config.disable_fmv) {
        return true;
    }

    char *full_path = NULL;
    File_GetFullPath(file_path, &full_path);

    // exit early in case the file does not exist
    {
        MYFILE *fp = File_Open(full_path, FILE_OPEN_READ);
        if (fp) {
            File_Close(fp);
        } else {
            LOG_ERROR("FMV does not exist: %s", full_path);
            return false;
        }
    }

    GameBuf_Shutdown();
    HWR_Shutdown();

    int flags = SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER;
    if (SDL_Init(flags)) {
        LOG_ERROR("Could not initialize SDL - %s", SDL_GetError());
        goto cleanup;
    }

    SDL_EventState(SDL_SYSWMEVENT, SDL_IGNORE);
    SDL_EventState(SDL_USEREVENT, SDL_IGNORE);

    m_Window = (SDL_Window *)S_Shell_GetWindowHandle();
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
    if (m_Window) {
        m_Renderer = SDL_CreateRenderer(
            m_Window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
        if (!m_Renderer) {
            LOG_INFO(
                "Failed to initialize a hardware accelerated renderer: %s",
                SDL_GetError());
            m_Renderer = SDL_CreateRenderer(m_Window, -1, 0);
        }
        if (m_Renderer) {
            SDL_GetRendererInfo(m_Renderer, &m_RendererInfo);
        }
    }
    if (!m_Window || !m_Renderer || !m_RendererInfo.num_texture_formats) {
        LOG_ERROR("Failed to create window or renderer: %s", SDL_GetError());
        goto cleanup;
    }

    VideoState *is = S_FMV_StreamOpen(full_path, NULL);
    if (!is) {
        LOG_ERROR("Failed to initialize VideoState!");
        goto cleanup;
    }

    S_FMV_EventLoop(is);

    ret = true;

cleanup:
    if (is) {
        S_FMV_StreamClose(is);
    }

    if (m_Renderer) {
        SDL_DestroyRenderer(m_Renderer);
    }

    if (full_path) {
        Memory_Free(full_path);
    }

    HWR_Init();
    GameBuf_Init();
    return ret;
}
