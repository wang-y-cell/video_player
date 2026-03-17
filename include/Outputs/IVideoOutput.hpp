#pragma once

#include <string>

struct AVFrame;

class IVideoOutput {
public:
    virtual ~IVideoOutput() = default;
    virtual bool ensureInit(int w, int h) = 0;
    virtual void render(const AVFrame* yuv420p) = 0;
    virtual void destroy() = 0;
    virtual const std::string& lastError() const = 0;
};
