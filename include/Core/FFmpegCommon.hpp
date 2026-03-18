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

struct PacketDeleter {
    void operator()(AVPacket* p) const;
};

struct FrameDeleter {
    void operator()(AVFrame* f) const;
};

using PacketPtr = std::shared_ptr<AVPacket>;
using FramePtr = std::shared_ptr<AVFrame>;

PacketPtr makePacket();
FramePtr makeFrame();

}  // namespace ff
