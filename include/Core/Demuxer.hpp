#pragma once

#include <atomic>
#include <string>

#include "FFmpegCommon.hpp"
#include "Mediator.hpp"
#include "SafeQueue.hpp"

class Demuxer final : public BaseComponent {
public:
    Demuxer() = default;
    ~Demuxer() override;

    bool open(const std::string& url);
    void close();
    void readLoop(std::atomic<bool>& abort_flag, SafeQueue<ff::PacketPtr>& audio_packets, SafeQueue<ff::PacketPtr>& video_packets);

    AVFormatContext* context() const;
    int audioIndex() const;
    int videoIndex() const;
    AVRational audioTimeBase() const;
    AVRational videoTimeBase() const;
    const std::string& lastError() const;

private:
    AVFormatContext* fmt_ctx_ = nullptr;
    int audio_index_ = -1;
    int video_index_ = -1;
    AVRational audio_time_base_{0, 1};
    AVRational video_time_base_{0, 1};
    std::string last_error_;
};
