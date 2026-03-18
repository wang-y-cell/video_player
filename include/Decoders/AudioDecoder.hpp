#pragma once

#include <atomic>
#include <string>

#include <SDL3/SDL.h>

extern "C" {
#include <libavutil/channel_layout.h>
#include <libswresample/swresample.h>
}

#include "Clock.hpp"
#include "FFmpegCommon.hpp"
#include "Mediator.hpp"
#include "SafeQueue.hpp"
#include "IDecoder.hpp"

class AudioDecoder final : public BaseComponent, public IDecoder {
public:
    AudioDecoder() = default;
    ~AudioDecoder() override;

    bool init(AVFormatContext* fmt, int stream_index, AVRational time_base, Clock* clock);
    void setPaused(bool paused);
    void close() override;
    void decodeLoop(std::atomic<bool>& abort_flag, SafeQueue<ff::PacketPtr>& packets);

    const std::string& lastError() const override;

private:
    AVCodecContext* codec_ctx_ = nullptr;
    SwrContext* swr_ctx_ = nullptr;
    AVRational time_base_{0, 1};

    AVChannelLayout in_ch_layout_{};
    AVChannelLayout out_ch_layout_{};
    AVSampleFormat out_sample_fmt_ = AV_SAMPLE_FMT_S16;
    int out_rate_ = 0;
    int out_channels_ = 0;
    int bytes_per_second_ = 0;

    SDL_AudioStream* sdl_stream_ = nullptr;
    Clock* clock_ = nullptr;
    std::string last_error_;
};
