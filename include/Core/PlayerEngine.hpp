#pragma once

#include <SDL3/SDL.h>

#include <atomic>
#include <string>

#include "Clock.hpp"
#include "Demuxer.hpp"
#include "Mediator.hpp"
#include "SafeQueue.hpp"
#include "AudioDecoder.hpp"
#include "VideoDecoder.hpp"
#include "IVideoOutput.hpp"
#include "ThreadPool.hpp"

class PlayerEngine final : public Mediator {
public:

    //初始化,初始化线程池,输出窗口,初始化解封装器,音视频解码器的中介设置为此类
    explicit PlayerEngine(thread_pool& pool);
    PlayerEngine(thread_pool& pool, std::unique_ptr<IVideoOutput> output);
    ~PlayerEngine() override;

    //重置时钟,解封装文件,初始化视频编码器和音频编码器
    bool prepare(const std::string& url);
    bool play();
    void pause();
    void resume();
    void togglePause();
    void setSpeed(double speed);
    double getSpeed() const;
    bool seekTo(double target_seconds);
    bool seekBy(double delta_seconds);
    void run();
    void stop();

    bool isRunning() const;
    bool isPaused() const;
    const std::string& lastError() const;

    //什么都没有
    void Notify(BaseComponent* sender, const std::string& event) override;

private:
    thread_pool& pool_;

    Demuxer demuxer_;
    AudioDecoder audio_;
    VideoDecoder video_;

    SafeQueue<ff::PacketPtr> audio_packets_;
    SafeQueue<ff::PacketPtr> video_packets_;
    SafeQueue<VideoFramePtr> video_frames_;

    std::unique_ptr<IVideoOutput> video_output_;
    Clock clock_;

    //用来终止所有线程的标指
    std::atomic<bool> abort_{false};
    //判断是否还在运行
    std::atomic<bool> running_{false};
    //设置暂停
    std::atomic<bool> paused_{false};
    //是否准备好
    bool prepared_ = false;
    //上一个错误信息
    std::string last_error_;
};
