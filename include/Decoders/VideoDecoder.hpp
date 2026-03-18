#pragma once

#include <atomic>
#include <memory>
#include <string>

extern "C" {
#include <libswscale/swscale.h>
}

#include "FFmpegCommon.hpp"
#include "Mediator.hpp"
#include "SafeQueue.hpp"
#include "IDecoder.hpp"

struct VideoFrame {
    ff::FramePtr frame;
    double pts_seconds = 0.0;
};

using VideoFramePtr = std::shared_ptr<VideoFrame>;

class VideoDecoder final : public BaseComponent, public IDecoder {
public:
    VideoDecoder() = default;
    ~VideoDecoder() override;

    bool init(AVFormatContext* fmt, int stream_index, AVRational time_base);
    void close() override;
    void decodeLoop(std::atomic<bool>& abort_flag, SafeQueue<ff::PacketPtr>& packets, SafeQueue<VideoFramePtr>& frames);

    int width() const;
    int height() const;
    const std::string& lastError() const override;

private:
    AVCodecContext* codec_ctx_ = nullptr;
    SwsContext* sws_ctx_ = nullptr;
    AVRational time_base_{0, 1};
    int width_ = 0;
    int height_ = 0;
    std::string last_error_;
};
