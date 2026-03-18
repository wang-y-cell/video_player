#include "SDL3Output.hpp"

SDL3Output::~SDL3Output() {
    destroy();
}

bool SDL3Output::ensureInit(int w, int h) {
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

void SDL3Output::render(const AVFrame* yuv420p) {
    if (!yuv420p || !texture_ || !renderer_) {
        return;
    }
    SDL_UpdateYUVTexture(texture_, nullptr, yuv420p->data[0], yuv420p->linesize[0], yuv420p->data[1], yuv420p->linesize[1], yuv420p->data[2],
                         yuv420p->linesize[2]);
    SDL_RenderClear(renderer_);
    SDL_RenderTexture(renderer_, texture_, nullptr, nullptr);
    SDL_RenderPresent(renderer_);
}

void SDL3Output::destroy() {
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

const std::string& SDL3Output::lastError() const {
    return last_error_;
}
