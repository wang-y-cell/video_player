#include "SDLAudioOutput.hpp"

SDLAudioOutput::~SDLAudioOutput() {
    close();
}

bool SDLAudioOutput::init(int rate, int channels) {
    close(); //关闭之前可能打开的音频流设备,避免重复打开

    SDL_AudioSpec spec; //准备一个结构体，告诉 SDL：你希望输出的音频格式是什么。
    spec.format = SDL_AUDIO_S16; //输出样本格式设为 16-bit signed（对应你上游重采样输出的 AV_SAMPLE_FMT_S16 ）。
    spec.channels = channels; //输出通道数设为 2（对应你上游重采样输出的 AV_CH_LAYOUT_STEREO ）。
    spec.freq = rate; //设置输出采样率，比如 44100 或 48000 Hz。

    //打开默认的播放设备，并创建一个 SDL_AudioStream* 。
    //之后会通过 SDL_PutAudioStreamData 往这个 stream 里写 PCM 数据
    sdl_stream_ = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, nullptr, nullptr);
    if (!sdl_stream_) {
        last_error_ = SDL_GetError();
        return false;
    }

    //启动/恢复音频设备播放（让设备开始从 stream 里取数据输出）。
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

void SDLAudioOutput::flush() {
    if (sdl_stream_) {
        SDL_ClearAudioStream(sdl_stream_);
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
