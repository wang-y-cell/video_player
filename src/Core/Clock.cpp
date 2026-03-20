#include "Clock.hpp"
#include <chrono>

int64_t Clock::nowUs() {
    auto now = std::chrono::steady_clock::now();
    //返回从纪元开始到现在,一共流失了多少时间
    return std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()).count();
}

void Clock::reset() {
    audio_pts_seconds_.store(0.0);
    last_update_us_.store(0);
    speed_.store(1.0);
}

void Clock::setAudioClock(double pts_seconds) {
    audio_pts_seconds_.store(pts_seconds);
    last_update_us_.store(nowUs());
}

double Clock::getAudioClock() const {
    const double base = audio_pts_seconds_.load();
    const int64_t last = last_update_us_.load();
    //播放器刚启动或者刚刚被重置
    if (last == 0) { //如果视频解码出来一帧,但是音频还没哟解码出来
        return base;
    }
    const int64_t delta_us = nowUs() - last;
    //delta_us是微妙,需要转换为秒,所以需要除以1000000.0
    // 注意：SDL_SetAudioStreamFrequencyRatio 改变了音频的实际消耗速度，
    // 所以这段 delta_us 时间内，实际“流逝”的媒体时间也要乘上 speed_
    return base + (static_cast<double>(delta_us) / 1000000.0) * speed_.load();
}

void Clock::setSpeed(double speed) {
    //设置速度需要更新时间
    double current = getAudioClock();
    setAudioClock(current);
    speed_.store(speed);
}

double Clock::getSpeed() const {
    return speed_.load();
}
