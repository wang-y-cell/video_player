#include "Demuxer.hpp"

Demuxer::~Demuxer() {
    close();
}

bool Demuxer::open(const std::string& url) {
    close();    //将之前打开的上下文关闭,并初始化
    avformat_network_init();

    const int ret = avformat_open_input(&fmt_ctx_, url.c_str(), nullptr, nullptr);
    if (ret < 0) {
        last_error_ = ff::errStr(ret);
        return false;
    }

    const int info_ret = avformat_find_stream_info(fmt_ctx_, nullptr);
    if (info_ret < 0) {
        last_error_ = ff::errStr(info_ret);
        return false;
    }

    audio_index_ = av_find_best_stream(fmt_ctx_, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
    video_index_ = av_find_best_stream(fmt_ctx_, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);

    if (audio_index_ >= 0) {
        audio_time_base_ = fmt_ctx_->streams[audio_index_]->time_base;
    }
    if (video_index_ >= 0) {
        video_time_base_ = fmt_ctx_->streams[video_index_]->time_base;
    }

    if (mediator_) {
        mediator_->Notify(this, "StreamReady");
    }
    return true;
}

void Demuxer::close() {
    if (fmt_ctx_) {
        avformat_close_input(&fmt_ctx_);
        fmt_ctx_ = nullptr;
    }
    audio_index_ = -1;
    video_index_ = -1;
    audio_time_base_ = AVRational{0, 1};
    video_time_base_ = AVRational{0, 1};
    last_error_.clear();
}

void Demuxer::readLoop(std::atomic<bool>& abort_flag, SafeQueue<ff::PacketPtr>& audio_packets, SafeQueue<ff::PacketPtr>& video_packets) {
    while (!abort_flag.load()) {
        ff::PacketPtr pkt = ff::makePacket();
        const int ret = av_read_frame(fmt_ctx_, pkt.get());
        if (ret < 0) {
            break;
        }
        if (pkt->stream_index == audio_index_) {
            audio_packets.push(std::move(pkt));
        } else if (pkt->stream_index == video_index_) {
            video_packets.push(std::move(pkt));
        }
    }
    audio_packets.push(ff::PacketPtr{});
    video_packets.push(ff::PacketPtr{});
}

AVFormatContext* Demuxer::context() const { return fmt_ctx_; }
int Demuxer::audioIndex() const { return audio_index_; }
int Demuxer::videoIndex() const { return video_index_; }
AVRational Demuxer::audioTimeBase() const { return audio_time_base_; }
AVRational Demuxer::videoTimeBase() const { return video_time_base_; }
const std::string& Demuxer::lastError() const { return last_error_; }
