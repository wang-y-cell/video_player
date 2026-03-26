#include "PlayerEngine.hpp"
#include "SDL3Output.hpp"
#include "SDLAudioOutput.hpp"
#include <algorithm>
#include <cstdint>
#include <iostream>

namespace {
void sleepSeconds(double seconds) {
    if (seconds <= 0.0) return;
    //获得SDL初始化以来所经过的纳秒时间
    const uint64_t start = SDL_GetTicksNS();
    const uint64_t target = start + static_cast<uint64_t>(seconds * 1000000000.0);
    while (true) {
        //开始记录当前时间,用来比较target时间
        const uint64_t now = SDL_GetTicksNS();
        //如果现在时间大于或者等于target时间了,说明等待的时间到了
        if (now >= target) break;
        //计算出我们还需要多少时间来等待
        const uint64_t remaining_ns = target - now;
        //如果等待的时间大于2ms,就等待2ms,避免等待时间过长
        if (remaining_ns > 2000000ULL) {
            //等待多余的时间,剩下的1ms的时候用于忙等待
            const uint32_t ms = static_cast<uint32_t>((remaining_ns - 1000000ULL) / 1000000ULL);
            SDL_Delay(ms);
        } else {
            //不同于空转,每次主动让出时间片给其他线程,避免占用太多CPU资源,但又可以及时响应其他线程的请求
            SDL_Delay(0);
        }
    }
}
} // namespace

PlayerEngine::PlayerEngine(thread_pool& pool) : pool_(pool), video_output_(new SDL3Output()) {
    demuxer_.set_mediator(this);
    audio_.set_mediator(this);
    video_.set_mediator(this);
    audio_.setOutput(std::make_unique<SDLAudioOutput>());
}

PlayerEngine::PlayerEngine(thread_pool& pool, std::unique_ptr<IVideoOutput> output) : pool_(pool), video_output_(std::move(output)) {
    demuxer_.set_mediator(this);
    audio_.set_mediator(this);
    video_.set_mediator(this);
    audio_.setOutput(std::make_unique<SDLAudioOutput>());
}

PlayerEngine::~PlayerEngine() {
    stop();
}

bool PlayerEngine::prepare(const std::string& url) {
    //初始化相应参数
    prepared_ = false;
    last_error_.clear();
    clock_.reset();

    //打开解封装器
    if (!demuxer_.open(url)) {
        last_error_ = demuxer_.lastError();
        return false;
    }

    //判断有没有音频,如果有音频,就初始化音频解码器
    bool has_audio = false;
    if (demuxer_.audioIndex() >= 0) {
        if (!audio_.init(demuxer_.context(), demuxer_.audioIndex(), demuxer_.audioTimeBase(), &clock_)) {
            last_error_ = "Audio init failed: " + audio_.lastError();
            return false;
        }
        has_audio = true;
    }

    //判断有没有视频,如果有视频,就初始化视频解码器
    bool has_video = false;
    if (demuxer_.videoIndex() >= 0) {
        if (!video_.init(demuxer_.context(), demuxer_.videoIndex(), demuxer_.videoTimeBase())) {
            last_error_ = "Video init failed: " + video_.lastError();
            return false;
        }
        has_video = true;
    }

    //这里我们只要两个都成功,只要有一个没有成功就算失败
    if (!has_audio && !has_video) {
        last_error_ = "No audio or video stream found";
        return false;
    }

    prepared_ = true;
    return true;
}

bool PlayerEngine::play() {
    if (!prepared_) {
        last_error_ = "not prepared";
        return false;
    }

    abort_.store(false);
    running_.store(true);
    paused_.store(false);

    audio_packets_.reset();
    video_packets_.reset();
    video_frames_.reset();

    //解封装线程,用来不断读取格式上下文,获取的pkt,并根据类型放进相应的队列中
    pool_.add_task([this] { demuxer_.readLoop(abort_, audio_packets_, video_packets_); });
    pool_.add_task([this] { audio_.decodeLoop(abort_, audio_packets_); });
    pool_.add_task([this] { video_.decodeLoop(abort_, video_packets_, video_frames_); });
    return true;
}

void PlayerEngine::pause() {
    if (!running_.load()) {
        return;
    }
    paused_.store(true);
    audio_.setPaused(true);
    clock_.pause();
}

void PlayerEngine::resume() {
    if (!running_.load()) {
        return;
    }
    paused_.store(false);
    audio_.setPaused(false);
    clock_.resume();
}

void PlayerEngine::togglePause() {
    if (paused_.load()) {
        resume();
    } else {
        pause();
    }
}

void PlayerEngine::setSpeed(double speed) {
    if (speed <= 0.0) return;
    clock_.setSpeed(speed);
    audio_.setSpeed(speed);
}

double PlayerEngine::getSpeed() const {
    return clock_.getSpeed();
}

bool PlayerEngine::seekTo(double target_seconds) {
    if (!running_.load()) {
        return false;
    }

    const double duration = demuxer_.durationSeconds();
    double clamped_target = std::max(0.0, target_seconds);
    if (duration > 0.0) {
        clamped_target = std::min(clamped_target, duration);
    }
    std::cout << "\nseek target: " << clamped_target << "s";

    const bool was_paused = paused_.load();
    pause();

    audio_packets_.reset();
    video_packets_.reset();
    video_frames_.reset();

    if (!demuxer_.seekSeconds(clamped_target)) {
        last_error_ = demuxer_.lastError();
        std::cout << "\nseek failed: " << last_error_;
        if (!was_paused) {
            resume();
        }
        return false;
    }

    audio_.flush();
    video_.flush();
    clock_.setAudioClockUnsynced(clamped_target);

    if (!was_paused) {
        resume();
    }
    std::cout << "\nseek done";

    return true;
}

bool PlayerEngine::seekBy(double delta_seconds) {
    const double current = std::max(0.0, clock_.getAudioClock());
    return seekTo(current + delta_seconds);
}

void PlayerEngine::run() {
    if (!running_.load()) {
        return;
    }

    VideoFramePtr vf;
    bool video_inited = false;

    while (running_.load()) {
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_EVENT_QUIT) {
                stop();
            } else if (ev.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
                if (ev.button.button == SDL_BUTTON_LEFT) {
                    togglePause();
                }
            } else if (ev.type == SDL_EVENT_KEY_DOWN) {
                if (ev.key.key == SDLK_ESCAPE) {
                    stop();
                } else if (ev.key.key == SDLK_SPACE) {
                    togglePause();
                } else if (ev.key.key == SDLK_UP) {
                    double  speed = 0;
                    setSpeed(speed = std::min(2.0,getSpeed() + 0.5));
                    std::cout << "\r" << std::string(50, ' ') << "\r";
                    std::cout.flush();
                    std::cout << "current speed: " << speed;
                } else if (ev.key.key == SDLK_DOWN) {
                    double  speed = 0;
                    std::cout << "\r" << std::string(50, ' ') << "\r";
                    std::cout.flush();
                    setSpeed(speed = std::max(0.50, getSpeed() - 0.50));
                    std::cout << "current speed: " << speed;
                } else if (ev.key.key == SDLK_R) {
                    setSpeed(1.0);
                    std::cout << "\r" << std::string(50, ' ') << "\r";
                    std::cout.flush();
                    std::cout << "current speed: " << 1.0;
                } else if (ev.key.key == SDLK_RIGHT) {
                    seekBy(10.0);
                } else if (ev.key.key == SDLK_LEFT) {
                    seekBy(-10.0);
                }
            }
        }

        //如果是暂停,就停止10ms,等待用户操作,避免反复检查
        if (paused_.load()) {
            SDL_Delay(10);
            continue;
        }

        if (!video_frames_.popFor(vf, 10)) {
            if (abort_.load()) break;
            continue;
        }
        
        //如果读到最后传入的空指针,就说明视频解码完成了
        if (!vf) {
            break; // End of stream
        }

        //如果没有初始化显示频幕,就初始化
        if (!video_inited) {
            if (!video_output_->ensureInit(vf->frame->width, vf->frame->height)) {
                last_error_ = video_output_->lastError();
                stop();
                break;
            }
            video_inited = true;
        }

        // 视频 PTS 明显落后于音频时丢帧，直到追上或队列暂时为空（解码慢时靠丢帧跟上）
        // 音频尚未首次 setAudioClock 时，getAudioClock 恒为 0，不可用于与 PTS 比较（否则会误判落后/丢帧）
        constexpr double kVideoLateDropSeconds = 0.08;
        {
            bool need_more_frames = false;
            while (vf) {
                const double delay_vs_audio =
                    clock_.audioClockSynced() ? (vf->pts_seconds - clock_.getAudioClock()) : 0.0;
                if (delay_vs_audio >= -kVideoLateDropSeconds) {
                    break;
                }
                vf.reset();
                if (!video_frames_.popFor(vf, 0)) {
                    need_more_frames = true;
                    break;
                }
                if (!vf) {
                    break;
                }
            }
            if (!vf) {
                if (need_more_frames) {
                    continue;
                }
                break;
            }
        }

        // Sync logic
        const double pts = vf->pts_seconds;
        double delay = clock_.audioClockSynced() ? (pts - clock_.getAudioClock()) : 0.0;

        // Handle sync：仅当音频主时钟已开始更新时才根据 delay 等待。
        // 否则 getAudioClock 为 0，delay 恒为正，会每帧 SDL_Delay，表现为起播「慢动作」且随后与真实音频错位。
        //当视频超过音频2ms的时候,认为明显超前,需要等待
        if (delay > 0.002) {
             double speed = getSpeed();
             //获得当前播放倍速,并将等待时间按照倍速缩短
             if (speed > 0) {
                 delay /= speed;
             }

             if (delay > 0.1) {
                 //如果delay的时间超过100ms,就只等待100ms,避免等待时间过长
                 delay = 0.1;
             }

             sleepSeconds(delay);
        }

        video_output_->render(vf->frame.get());
    }
    
    stop();
}

void PlayerEngine::stop() {
    running_.store(false);
    abort_.store(true);
    paused_.store(false);
    
    audio_packets_.abort();
    video_packets_.abort();
    video_frames_.abort();

    // pool_.wait_for_tasks(); // ThreadPool does not support waiting for tasks
    
    video_output_->destroy();
    audio_.close();
    demuxer_.close();
}

bool PlayerEngine::isRunning() const {
    return running_.load();
}

bool PlayerEngine::isPaused() const {
    return paused_.load();
}

const std::string& PlayerEngine::lastError() const {
    return last_error_;
}

void PlayerEngine::Notify(BaseComponent* sender, const std::string& event) {
    if (sender == &demuxer_) {
        if (event == "EOF") {
            // End of file
        }
    }
}
