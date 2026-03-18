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

    std::atomic<bool> abort_{false};
    std::atomic<bool> running_{false};
    std::atomic<bool> paused_{false};
    bool prepared_ = false;
    std::string last_error_;
};
