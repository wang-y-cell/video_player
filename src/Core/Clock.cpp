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
    paused_.store(false);
}

void Clock::setAudioClock(double pts_seconds) {
    audio_pts_seconds_.store(pts_seconds);
    last_update_us_.store(nowUs());
}

bool Clock::audioClockSynced() const {
    return last_update_us_.load() != 0;
}

double Clock::getAudioClock() const {
    const double base = audio_pts_seconds_.load();
    const int64_t last = last_update_us_.load();
    //播放器刚启动或者刚刚被重置，或者处于暂停状态
    if (last == 0 || paused_.load()) { //如果视频解码出来一帧,但是音频还没哟解码出来
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

void Clock::pause() {
    if (paused_.load()) return;
    // 暂停时，将当前时间冻结为 base pts
    double current = getAudioClock();
    paused_.store(true);
    setAudioClock(current);
}

void Clock::resume() {
    if (!paused_.load()) return;
    paused_.store(false);
    // 恢复时，重置时间基准为当前物理时间，防止时间跳跃
    last_update_us_.store(nowUs());
}
