#include "Clock.hpp"
#include <chrono>

int64_t Clock::nowUs() {
    auto now = std::chrono::steady_clock::now();
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
    if (last == 0) {
        return base;
    }
    const int64_t delta_us = nowUs() - last;
    return base + (static_cast<double>(delta_us) / 1000000.0) * speed_.load();
}

void Clock::setSpeed(double speed) {
    double current = getAudioClock();
    setAudioClock(current);
    speed_.store(speed);
}

double Clock::getSpeed() const {
    return speed_.load();
}
