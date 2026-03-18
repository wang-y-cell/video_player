#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>

class Clock {
public:
    Clock() = default;

    void reset();
    void setAudioClock(double pts_seconds);
    double getAudioClock() const;

private:
    static int64_t nowUs();

    std::atomic<double> audio_pts_seconds_{0.0};
    std::atomic<int64_t> last_update_us_{0};
};
