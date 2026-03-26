#pragma once

#include "IAudioOutput.hpp"
#include <SDL3/SDL.h>
#include <string>

class SDLAudioOutput : public IAudioOutput {
public:
    SDLAudioOutput() = default;
    ~SDLAudioOutput() override;

    bool init(int rate, int channels) override;
    void write(const uint8_t* data, int size) override;
    //获得音频播放器剩余没有播放的字节数
    int getQueuedSize() const override;
    void pause() override;
    void resume() override;
    void setSpeed(double speed) override;
    void flush() override;
    void close() override;
    const std::string& lastError() const override;

private:
    SDL_AudioStream* sdl_stream_ = nullptr;
    std::string last_error_;
};
