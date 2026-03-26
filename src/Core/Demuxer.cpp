#include "Demuxer.hpp"
#include <chrono>
#include <thread>

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
    std::lock_guard<std::mutex> lock(io_mutex_);
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
    uint64_t observed_seek_generation = seek_generation_.load();
    bool eof_state = false;
    while (!abort_flag.load()) {
        //生成一个pkt,用来装载压缩数据
        ff::PacketPtr pkt = ff::makePacket();
        int ret = 0;
        {
            std::lock_guard<std::mutex> lock(io_mutex_);
            ret = av_read_frame(fmt_ctx_, pkt.get());
        }
        if (ret < 0) {
            const uint64_t current_seek_generation = seek_generation_.load();
            if (current_seek_generation != observed_seek_generation) {
                observed_seek_generation = current_seek_generation;
                eof_state = false;
                continue;
            }
            if (ret == AVERROR(EAGAIN)) {
                continue;
            }
            eof_state = true;
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        if (eof_state) {
            eof_state = false;
        }

        if (pkt->stream_index == audio_index_) {
            audio_packets.push(std::move(pkt));
        } else if (pkt->stream_index == video_index_) {
            video_packets.push(std::move(pkt));
        }
    }
}

bool Demuxer::seekSeconds(double target_seconds) {
    std::lock_guard<std::mutex> lock(io_mutex_);
    if (!fmt_ctx_) {
        last_error_ = "format context is null";
        return false;
    }
    if (target_seconds < 0.0) {
        target_seconds = 0.0;
    }

    const int64_t seek_target = static_cast<int64_t>(target_seconds * static_cast<double>(AV_TIME_BASE));
    int ret = av_seek_frame(fmt_ctx_, -1, seek_target, AVSEEK_FLAG_BACKWARD);
    if (ret < 0) {
        ret = av_seek_frame(fmt_ctx_, -1, seek_target, 0);
    }
    if (ret < 0) {
        last_error_ = ff::errStr(ret);
        return false;
    }

    avformat_flush(fmt_ctx_);
    seek_generation_.fetch_add(1);
    return true;
}

//返回媒体总时间长度
double Demuxer::durationSeconds() const {
    std::lock_guard<std::mutex> lock(io_mutex_);
    if (!fmt_ctx_ || fmt_ctx_->duration == AV_NOPTS_VALUE) {
        return -1.0;
    }
    return static_cast<double>(fmt_ctx_->duration) / static_cast<double>(AV_TIME_BASE);
}

AVFormatContext* Demuxer::context() const { return fmt_ctx_; }
int Demuxer::audioIndex() const { return audio_index_; }
int Demuxer::videoIndex() const { return video_index_; }
AVRational Demuxer::audioTimeBase() const { return audio_time_base_; }
AVRational Demuxer::videoTimeBase() const { return video_time_base_; }
const std::string& Demuxer::lastError() const { return last_error_; }
