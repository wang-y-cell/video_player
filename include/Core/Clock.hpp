#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>

class Clock {
public:
    Clock() = default;

    void reset() {
        audio_pts_seconds_.store(0.0);
        last_update_us_.store(0);
    }

    void setAudioClock(double pts_seconds) {
        audio_pts_seconds_.store(pts_seconds);
        last_update_us_.store(nowUs());
    }

    double getAudioClock() const {
        const double base = audio_pts_seconds_.load();
        const int64_t last = last_update_us_.load();
        if (last == 0) {
            return base;
        }
        const int64_t delta_us = nowUs() - last;
        return base + static_cast<double>(delta_us) / 1000000.0;
    }

private:
    static int64_t nowUs() {
        using namespace std::chrono;
        return duration_cast<microseconds>(steady_clock::now().time_since_epoch()).count();
    }

    std::atomic<double> audio_pts_seconds_{0.0};
    std::atomic<int64_t> last_update_us_{0};
};
