#pragma once

#include <memory>
#include <string>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/error.h>
#include <libavutil/frame.h>
#include <libavutil/samplefmt.h>
}

namespace ff {

std::string errStr(int err);

//packet删除器
struct PacketDeleter {
    void operator()(AVPacket* p) const;
};

//frame删除器
struct FrameDeleter {
    void operator()(AVFrame* f) const;
};

//pakcet和frame的智能指针
using PacketPtr = std::shared_ptr<AVPacket>;
using FramePtr = std::shared_ptr<AVFrame>;

//分配内存
PacketPtr makePacket();
FramePtr makeFrame();

}  // namespace ff
