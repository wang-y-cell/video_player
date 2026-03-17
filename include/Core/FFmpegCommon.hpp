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
inline std::string errStr(int err) {
    char buf[AV_ERROR_MAX_STRING_SIZE] = {};
    av_strerror(err, buf, sizeof(buf));
    return std::string(buf);
}

struct PacketDeleter {
    void operator()(AVPacket* p) const {
        if (!p) {
            return;
        }
        AVPacket* tmp = p;
        av_packet_free(&tmp);
    }
};

struct FrameDeleter {
    void operator()(AVFrame* f) const {
        if (!f) {
            return;
        }
        AVFrame* tmp = f;
        av_frame_free(&tmp);
    }
};

using PacketPtr = std::shared_ptr<AVPacket>;
using FramePtr = std::shared_ptr<AVFrame>;

inline PacketPtr makePacket() { return PacketPtr(av_packet_alloc(), PacketDeleter{}); }
inline FramePtr makeFrame() { return FramePtr(av_frame_alloc(), FrameDeleter{}); }
}  // namespace ff
