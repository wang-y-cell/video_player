#pragma once

#include <atomic>
#include <cstdint>

class Clock {
public:
    Clock() = default;

    //将时间和倍速重设
    void reset();

    //设置当前pts和当前计算机系统时间
    void setAudioClock(double pts_seconds);

    //获得当前pts值(double)
    double getAudioClock() const;

    /** 是否已有至少一次 setAudioClock（音频线程已开始更新主时钟）。未就绪时 getAudioClock 恒为 0，不可用于与视频 PTS 比较。 */
    bool audioClockSynced() const;

    void setSpeed(double speed);
    double getSpeed() const;

    void pause();
    void resume();

private:
    static int64_t nowUs();

    //记录上一次音频解码器报告的准确时间戳(pts),单位为秒
    std::atomic<double> audio_pts_seconds_{0.0};
    //记录上次调用setAudioClock的时候,计算机系统的绝对时间,修改的时候与audio_pts_seconds_同时进行
    std::atomic<int64_t> last_update_us_{0};

    //暂停状态
    std::atomic<bool> paused_{false};

    //播放倍速
    std::atomic<double> speed_{1.0};
};
