#pragma once

#include <atomic>
#include <string>
#include <memory>

extern "C" {
#include <libavutil/channel_layout.h>
#include <libswresample/swresample.h>
}

#include "Clock.hpp"
#include "FFmpegCommon.hpp"
#include "Mediator.hpp"
#include "SafeQueue.hpp"
#include "IDecoder.hpp"
#include "IAudioOutput.hpp"

class AudioDecoder final : public BaseComponent, public IDecoder {
public:
    AudioDecoder() = default;
    ~AudioDecoder() override;

    //音频输出接口(SDL)
    void setOutput(std::unique_ptr<IAudioOutput> output);
    
    //初始化音频解码器,初始化声道数,采样率,位深,初始化output
    bool init(AVFormatContext* fmt, int stream_index, AVRational time_base, Clock* clock);
    void setPaused(bool paused);
    void setSpeed(double speed);
    void close() override;
    void decodeLoop(std::atomic<bool>& abort_flag, SafeQueue<ff::PacketPtr>& packets);

    const std::string& lastError() const override;

private:
    std::unique_ptr<IAudioOutput> output_;
    AVCodecContext* codec_ctx_ = nullptr;
    SwrContext* swr_ctx_ = nullptr;
    AVRational time_base_{0, 1};

    AVChannelLayout in_ch_layout_{};
    AVChannelLayout out_ch_layout_{};
    AVSampleFormat out_sample_fmt_ = AV_SAMPLE_FMT_S16;
    int out_rate_ = 0;
    int out_channels_ = 0;
    int bytes_per_second_ = 0;

    Clock* clock_ = nullptr;
    std::string last_error_;
};
