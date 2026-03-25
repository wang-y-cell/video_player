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
    if (!yuv420p || !texture_ || !renderer_ || !window_) {
        return;
    }
    SDL_UpdateYUVTexture(texture_, nullptr, 
        yuv420p->data[0], yuv420p->linesize[0], 
        yuv420p->data[1], yuv420p->linesize[1], 
        yuv420p->data[2], yuv420p->linesize[2]
    );

    int win_w = 0;
    int win_h = 0;
    //获得渲染窗口的尺寸
    SDL_GetWindowSize(window_, &win_w, &win_h);
    if(win_w <= 0 || win_h <= 0 || yuv420p->width <= 0 || yuv420p->height <= 0)
        return;

    //获得视频分辨率的长宽
    const float src_w = static_cast<float>(yuv420p->width);
    const float src_h = static_cast<float>(yuv420p->height);
    //计算高度和宽度与视频窗口的比例,取最小值,防止部分视频被截断
    const float scale_w = static_cast<float>(win_w) / src_w;
    const float scale_h = static_cast<float>(win_h) / src_h;
    const float scale = std::min(scale_w,scale_h);

    //目标的高和宽都需要乘以这个比例
    const float dst_w = src_w * scale;
    const float dst_h = src_h * scale;
    //获得将视频放在窗口中间的坐标
    const float dst_x = (static_cast<float>(win_w) - dst_w) * 0.5f;
    const float dst_y = (static_cast<float>(win_h) - dst_h) * 0.5f;

    //创建矩形
    const SDL_FRect dst = {dst_x, dst_y, dst_w, dst_h};

    SDL_RenderClear(renderer_);
    //如果最后一个参数时nullptr,就是填充所有频幕,这里是矩形的坐标和大小,用来适配窗口
    SDL_RenderTexture(renderer_, texture_, nullptr, &dst);
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
