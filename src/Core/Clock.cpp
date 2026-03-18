#include "Clock.hpp"

void Clock::reset() {
    audio_pts_seconds_.store(0.0);
    last_update_us_.store(0);
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
    return base + static_cast<double>(delta_us) / 1000000.0;
}

int64_t Clock::nowUs() {
    using namespace std::chrono;
    return duration_cast<microseconds>(steady_clock::now().time_since_epoch()).count();
}
