#pragma once

//这个提供了播放界面ui的接口,将代码的UI和逻辑分开
//以后如果想要将sdl换为qt会更加方便


//这个函数作为播放器界面的基类,主要作用是提供了公用的接口

#include <string>

struct AVFrame;

class IVideoOutput {
public:
    virtual ~IVideoOutput() = default;  //默认析构函数
    virtual bool ensureInit(int w, int h) = 0; //判断播放器窗口,渲染器,纹理是否已经创建
    virtual void render(const AVFrame* yuv420p) = 0; //渲染纹理的接口
    virtual void destroy() = 0; //销毁接口,可用于手动销毁窗口
    virtual const std::string& lastError() const = 0; //最近出现的窗口相关错误
};
