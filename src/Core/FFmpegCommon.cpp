#include "FFmpegCommon.hpp"

namespace ff {

std::string errStr(int err) {
    char buf[AV_ERROR_MAX_STRING_SIZE] = {};
    av_strerror(err, buf, sizeof(buf));
    return std::string(buf);
}

void PacketDeleter::operator()(AVPacket* p) const {
    if (!p) {
        return;
    }
    AVPacket* tmp = p;
    av_packet_free(&tmp);
}

void FrameDeleter::operator()(AVFrame* f) const {
    if (!f) {
        return;
    }
    AVFrame* tmp = f;
    av_frame_free(&tmp);
}

PacketPtr makePacket() {
    return PacketPtr(av_packet_alloc(), PacketDeleter{});
}

FramePtr makeFrame() {
    return FramePtr(av_frame_alloc(), FrameDeleter{});
}

}  // namespace ff
