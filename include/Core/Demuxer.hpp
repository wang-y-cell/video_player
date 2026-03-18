#pragma once

/**
解封装器: 此类用来将接收到的文件的格式解开,并将对应
的视频流和音频流的packet放到对应的队列中

获得的数据:

格式上下文
音频流索引
视频流索引
音频时间基
视频时间基

*/
#include <atomic>
#include <string>

#include "FFmpegCommon.hpp"
#include "Mediator.hpp"
#include "SafeQueue.hpp"

class Demuxer final : public BaseComponent {
public:
    Demuxer() = default;
    ~Demuxer() override;

    //打开文件,找到对应的流下标,和时间基
    bool open(const std::string& url);

    //关闭文件上下文,重设对应流下标和时间基
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
    //时间基,对应分子和分母
    AVRational audio_time_base_{0, 1};
    AVRational video_time_base_{0, 1};
    std::string last_error_; //包含上次错误的信息,在中介中如果出现错误,调用这个函数就是出错的信息
};
