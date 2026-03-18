#include "PlayerEngine.hpp"

PlayerEngine::PlayerEngine(thread_pool& pool) : pool_(pool), video_output_(new SDL3Output()) {
    demuxer_.set_mediator(this);
    audio_.set_mediator(this);
    video_.set_mediator(this);
}

PlayerEngine::PlayerEngine(thread_pool& pool, std::unique_ptr<IVideoOutput> output) : pool_(pool), video_output_(std::move(output)) {
    demuxer_.set_mediator(this);
    audio_.set_mediator(this);
    video_.set_mediator(this);
}

PlayerEngine::~PlayerEngine() {
    stop();
}

bool PlayerEngine::prepare(const std::string& url) {
    prepared_ = false;
    last_error_.clear();
    clock_.reset();

    if (!demuxer_.open(url)) {
        last_error_ = demuxer_.lastError();
        return false;
    }
    if (!prepared_) {
        last_error_ = last_error_.empty() ? "prepare failed" : last_error_;
    }
    return prepared_;
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
            } else if (ev.type == SDL_EVENT_KEY_DOWN) {
                if (ev.key.key == SDLK_ESCAPE) {
                    stop();
                } else if (ev.key.key == SDLK_SPACE) {
                    togglePause();
                }
            }
        }

        if (paused_.load()) {
            SDL_Delay(10);
            continue;
        }

        if (!video_frames_.popFor(vf, 10)) {
            continue;
        }
        if (!vf) {
            stop();
            break;
        }

        if (!video_inited) {
            if (!video_output_->ensureInit(video_.width(), video_.height())) {
                last_error_ = video_output_->lastError();
                stop();
                break;
            }
            video_inited = true;
        }

        const double audio_clock = clock_.getAudioClock();
        const double diff = vf->pts_seconds - audio_clock;
        if (diff > 0.0) {
            const uint32_t delay_ms = static_cast<uint32_t>(std::min(200.0, diff * 1000.0));
            if (delay_ms > 0) {
                SDL_Delay(delay_ms);
            }
        } else if (diff < -0.25) {
            continue;
        }

        video_output_->render(vf->frame.get());
    }
}

void PlayerEngine::stop() {
    if (!running_.exchange(false)) {
        return;
    }
    abort_.store(true);
    audio_packets_.abort();
    video_packets_.abort();
    video_frames_.abort();

    audio_.setPaused(true);
    if (video_output_) {
        video_output_->destroy();
    }
}

bool PlayerEngine::isRunning() const { return running_.load(); }
bool PlayerEngine::isPaused() const { return paused_.load(); }
const std::string& PlayerEngine::lastError() const { return last_error_; }

void PlayerEngine::Notify(BaseComponent* sender, const std::string& event) {
    (void)sender;
    if (event != "StreamReady") {
        return;
    }

    const bool audio_ok = (demuxer_.audioIndex() >= 0)
                              ? audio_.init(demuxer_.context(), demuxer_.audioIndex(), demuxer_.audioTimeBase(), &clock_)
                              : true;
    const bool video_ok = (demuxer_.videoIndex() >= 0) ? video_.init(demuxer_.context(), demuxer_.videoIndex(), demuxer_.videoTimeBase()) : false;

    if (!audio_ok) {
        last_error_ = audio_.lastError();
        prepared_ = false;
        return;
    }
    if (!video_ok) {
        last_error_ = video_.lastError();
        prepared_ = false;
        return;
    }
    prepared_ = true;
}
