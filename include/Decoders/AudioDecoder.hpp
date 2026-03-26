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
    //控制是否暂停
    void setPaused(bool paused);
    //设置播放速度
    void setSpeed(double speed);
    void flush();
    //关闭解码器,音频播放器,重定向,声道布局
    void close() override;

    //接收音频帧,解码,重采样,播放
    void decodeLoop(std::atomic<bool>& abort_flag, SafeQueue<ff::PacketPtr>& packets);

    const std::string& lastError() const override;

private:
    //指向音频播放器的指针
    std::unique_ptr<IAudioOutput> output_;
    //音频播放器上下文
    AVCodecContext* codec_ctx_ = nullptr;
    //重定向上下文
    SwrContext* swr_ctx_ = nullptr;
    //时间基
    AVRational time_base_{0, 1};

    //输入声道布局
    AVChannelLayout in_ch_layout_{};
    //输出声道布局
    AVChannelLayout out_ch_layout_{};
    //样本格式
    AVSampleFormat out_sample_fmt_ = AV_SAMPLE_FMT_S16;

    //输出码率
    int out_rate_ = 0;
    //输出声道数
    int out_channels_ = 0;
    //表示“输出音频在当前格式下，每秒对应的字节数”。
    //用于把字节数（例如设备队列里的可用字节）换算成秒。
    int bytes_per_second_ = 0;

    //指向时钟的指针
    Clock* clock_ = nullptr;
    std::string last_error_;
};
