#include "PlayerEngine.hpp"
#include "SDL3Output.hpp"
#include "SDLAudioOutput.hpp"
#include <algorithm>

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
}

void PlayerEngine::resume() {
    if (!running_.load()) {
        return;
    }
    paused_.store(false);
    audio_.setPaused(false);
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
                    setSpeed(getSpeed() + 0.25);
                } else if (ev.key.key == SDLK_DOWN) {
                    setSpeed(std::max(0.25, getSpeed() - 0.25));
                } else if (ev.key.key == SDLK_R) {
                    setSpeed(1.0);
                }
            }
        }

        if (paused_.load()) {
            SDL_Delay(10);
            continue;
        }

        if (!video_frames_.pop(vf)) {
            if (abort_.load()) break;
            SDL_Delay(2); // Wait for frames
            continue;
        }
        
        if (!vf) {
            break; // End of stream
        }

        if (!video_inited) {
            if (!video_output_->ensureInit(vf->frame->width, vf->frame->height)) {
                last_error_ = video_output_->lastError();
                stop();
                break;
            }
            video_inited = true;
        }

        // Sync logic
        const double pts = vf->pts_seconds;
        double delay = pts - clock_.getAudioClock();
        
        // Handle sync
        if (delay > 0) {
             // If video is ahead of audio, wait
             // Consider speed: The delay we need to wait in real-world time 
             // is shorter if we are playing faster.
             double speed = getSpeed();
             if (speed > 0) {
                 delay /= speed;
             }
             
             if (delay > 0.1) {
                 // Large delay, probably seeking or startup, just wait up to 100ms
                 delay = 0.1;
             }
             
             uint32_t delay_ms = static_cast<uint32_t>(delay * 1000);
             if (delay_ms > 0) {
                 SDL_Delay(delay_ms);
             }
        } else if (delay < -0.5) {
            // Video is way behind audio (e.g. slow decoding), drop frame
            // But for now we just render it late
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
