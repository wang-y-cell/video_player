#pragma once

#include <string>

#include <SDL3/SDL.h>

extern "C" {
#include <libavutil/frame.h>
}

#include "./../Outputs/IVideoOutput.hpp"

class SDL3Output final : public IVideoOutput {
public:
    SDL3Output() = default;
    ~SDL3Output() override { destroy(); }

    bool ensureInit(int w, int h) override {
        if (renderer_) {
            return true;
        }
        window_ = SDL_CreateWindow("player", w, h, SDL_WINDOW_RESIZABLE);
        if (!window_) {
            last_error_ = SDL_GetError();
            return false;
        }
        renderer_ = SDL_CreateRenderer(window_, nullptr);
        if (!renderer_) {
            last_error_ = SDL_GetError();
            return false;
        }
        texture_ = SDL_CreateTexture(renderer_, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, w, h);
        if (!texture_) {
            last_error_ = SDL_GetError();
            return false;
        }
        SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 255);
        return true;
    }

    void render(const AVFrame* yuv420p) override {
        if (!yuv420p || !texture_ || !renderer_) {
            return;
        }
        SDL_UpdateYUVTexture(texture_, nullptr, yuv420p->data[0], yuv420p->linesize[0], yuv420p->data[1], yuv420p->linesize[1], yuv420p->data[2],
                             yuv420p->linesize[2]);
        SDL_RenderClear(renderer_);
        SDL_RenderTexture(renderer_, texture_, nullptr, nullptr);
        SDL_RenderPresent(renderer_);
    }

    void destroy() override {
        if (texture_) {
            SDL_DestroyTexture(texture_);
            texture_ = nullptr;
        }
        if (renderer_) {
            SDL_DestroyRenderer(renderer_);
            renderer_ = nullptr;
        }
        if (window_) {
            SDL_DestroyWindow(window_);
            window_ = nullptr;
        }
        last_error_.clear();
    }

    const std::string& lastError() const override { return last_error_; }

private:
    SDL_Window* window_ = nullptr;
    SDL_Renderer* renderer_ = nullptr;
    SDL_Texture* texture_ = nullptr;
    std::string last_error_;
};

