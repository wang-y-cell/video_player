#pragma once

#include <string>

#include <SDL3/SDL.h>

extern "C" {
#include <libavutil/frame.h>
}

#include "IVideoOutput.hpp"

class SDL3Output final : public IVideoOutput {
public:
    SDL3Output() = default;
    ~SDL3Output() override;

    //确保视频窗口的初始化
    bool ensureInit(int w, int h) override;

    //渲染纹理,并播放
    void render(const AVFrame* yuv420p) override;

    void destroy() override;

    const std::string& lastError() const override;

private:
    SDL_Window* window_ = nullptr;
    SDL_Renderer* renderer_ = nullptr;
    SDL_Texture* texture_ = nullptr;
    std::string last_error_;
};
