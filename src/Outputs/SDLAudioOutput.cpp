#include "SDLAudioOutput.hpp"

SDLAudioOutput::~SDLAudioOutput() {
    close();
}

bool SDLAudioOutput::init(int rate, int channels) {
    close();

    SDL_AudioSpec spec;
    spec.format = SDL_AUDIO_S16;
    spec.channels = channels;
    spec.freq = rate;

    sdl_stream_ = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, nullptr, nullptr);
    if (!sdl_stream_) {
        last_error_ = SDL_GetError();
        return false;
    }

    if (!SDL_ResumeAudioStreamDevice(sdl_stream_)) {
        last_error_ = SDL_GetError();
        return false;
    }

    return true;
}

void SDLAudioOutput::write(const uint8_t* data, int size) {
    if (sdl_stream_ && data && size > 0) {
        SDL_PutAudioStreamData(sdl_stream_, data, size);
    }
}

int SDLAudioOutput::getQueuedSize() const {
    if (sdl_stream_) {
        return SDL_GetAudioStreamAvailable(sdl_stream_);
    }
    return 0;
}

void SDLAudioOutput::pause() {
    if (sdl_stream_) {
        SDL_PauseAudioStreamDevice(sdl_stream_);
    }
}

void SDLAudioOutput::resume() {
    if (sdl_stream_) {
        SDL_ResumeAudioStreamDevice(sdl_stream_);
    }
}

void SDLAudioOutput::setSpeed(double speed) {
    if (sdl_stream_) {
        SDL_SetAudioStreamFrequencyRatio(sdl_stream_, static_cast<float>(speed));
    }
}

void SDLAudioOutput::close() {
    if (sdl_stream_) {
        SDL_DestroyAudioStream(sdl_stream_);
        sdl_stream_ = nullptr;
    }
}

const std::string& SDLAudioOutput::lastError() const {
    return last_error_;
}
