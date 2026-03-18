#include "VideoDecoder.hpp"
#include <thread>

VideoDecoder::~VideoDecoder() {
    close();
}

bool VideoDecoder::init(AVFormatContext* fmt, int stream_index, AVRational time_base) {
    close();
    time_base_ = time_base;

    if (!fmt || stream_index < 0 || stream_index >= static_cast<int>(fmt->nb_streams)) {
        last_error_ = "invalid video stream";
        return false;
    }

    AVStream* st = fmt->streams[stream_index];
    const AVCodec* codec = avcodec_find_decoder(st->codecpar->codec_id);
    if (!codec) {
        last_error_ = "video decoder not found";
        return false;
    }

    codec_ctx_ = avcodec_alloc_context3(codec);
    if (!codec_ctx_) {
        last_error_ = "avcodec_alloc_context3 failed";
        return false;
    }

    int ret = avcodec_parameters_to_context(codec_ctx_, st->codecpar);
    if (ret < 0) {
        last_error_ = ff::errStr(ret);
        return false;
    }

    ret = avcodec_open2(codec_ctx_, codec, nullptr);
    if (ret < 0) {
        last_error_ = ff::errStr(ret);
        return false;
    }

    width_ = codec_ctx_->width;
    height_ = codec_ctx_->height;

    if (mediator_) {
        mediator_->Notify(this, "VideoReady");
    }
    return true;
}

void VideoDecoder::close() {
    if (sws_ctx_) {
        sws_freeContext(sws_ctx_);
        sws_ctx_ = nullptr;
    }
    if (codec_ctx_) {
        avcodec_free_context(&codec_ctx_);
    }
    time_base_ = AVRational{0, 1};
    width_ = 0;
    height_ = 0;
    last_error_.clear();
}

void VideoDecoder::decodeLoop(std::atomic<bool>& abort_flag, SafeQueue<ff::PacketPtr>& packets, SafeQueue<VideoFramePtr>& frames) {
    ff::FramePtr frame = ff::makeFrame();

    while (!abort_flag.load()) {
        ff::PacketPtr pkt;
        if (!packets.pop(pkt)) {
            break;
        }
        if (!pkt) {
            break;
        }

        int ret = avcodec_send_packet(codec_ctx_, pkt.get());
        if (ret < 0) {
            continue;
        }

        while (!abort_flag.load()) {
            ret = avcodec_receive_frame(codec_ctx_, frame.get());
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                break;
            }
            if (ret < 0) {
                break;
            }

            const double pts_seconds =
                (frame->best_effort_timestamp != AV_NOPTS_VALUE) ? static_cast<double>(frame->best_effort_timestamp) * av_q2d(time_base_) : 0.0;

            if (!sws_ctx_) {
                sws_ctx_ = sws_getContext(codec_ctx_->width, codec_ctx_->height, codec_ctx_->pix_fmt, codec_ctx_->width, codec_ctx_->height,
                                          AV_PIX_FMT_YUV420P, SWS_BILINEAR, nullptr, nullptr, nullptr);
                if (!sws_ctx_) {
                    av_frame_unref(frame.get());
                    continue;
                }
            }

            AVFrame* yuv = av_frame_alloc();
            yuv->format = AV_PIX_FMT_YUV420P;
            yuv->width = codec_ctx_->width;
            yuv->height = codec_ctx_->height;
            if (av_frame_get_buffer(yuv, 32) < 0) {
                av_frame_free(&yuv);
                av_frame_unref(frame.get());
                continue;
            }

            sws_scale(sws_ctx_, frame->data, frame->linesize, 0, codec_ctx_->height, yuv->data, yuv->linesize);

            VideoFramePtr vf = std::make_shared<VideoFrame>();
            vf->frame = ff::FramePtr(yuv, ff::FrameDeleter{});
            vf->pts_seconds = pts_seconds;

            while (!abort_flag.load() && frames.size() > 20) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }

            frames.push(std::move(vf));
            av_frame_unref(frame.get());
        }
    }

    frames.push(VideoFramePtr{});
}

int VideoDecoder::width() const { return width_; }
int VideoDecoder::height() const { return height_; }
const std::string& VideoDecoder::lastError() const { return last_error_; }
