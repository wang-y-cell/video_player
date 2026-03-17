#include <SDL3/SDL.h>
#include <queue>
#include <iostream>
#include <memory>
#include <condition_variable>
#include <string>


extern "C"{
#include <libavutil/log.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}

auto freeCtx = [](AVCodecContext* fmt){avcodec_free_context(&fmt);};
auto free_texture = [](SDL_Texture* texture) {SDL_DestroyTexture(texture);};
auto fmtDelete = [](AVFormatContext* ctx) {avformat_close_input(&ctx);};
auto free_pkt = [](AVPacket* pkt){av_packet_free(&pkt);};
auto free_frame = [](AVFrame* frame){av_frame_free(&frame);};
auto free_renderer = [](SDL_Renderer* renderer){SDL_DestroyRenderer(renderer);};
auto free_win = [](SDL_Window* win){SDL_DestroyWindow(win);};

std::string ErrStr(int ret) {
    char errbuf[AV_ERROR_MAX_STRING_SIZE] = {0};
    av_strerror(ret, errbuf, sizeof(errbuf));
    return std::string(errbuf);
}

struct PacketQueue {
    using AVPacketPtr = std::unique_ptr<AVPacket,decltype(free_pkt)>;
    std::queue<AVPacketPtr> pkts;
    int nb_packets = 0;
    int size = 0;
    int64_t duration = 0;

    std::condition_variable cond;
    std::mutex mtx;
};

struct VideoState {
    AVCodecContext* actx = nullptr;
    AVPacket* apkt = nullptr;
    AVFrame* aframe = nullptr;

    struct SwrContext* swr_ctx = nullptr;

    uint8_t* audio_buf = nullptr;     //解码后的数据保存的位置
    uint audio_buf_size = 0;    //buf大小
    int audio_buf_index = 0;    //我们使用了多少数据
    //还没有使用的数据: size - index
    //如果index >= size: 没有数据了,需要对音频包解码,解码后的数据再保存在buf中


    AVCodecContext* vctx = nullptr;
    AVPacket* vpkt = nullptr;
    AVFrame* vframe = nullptr;

    SDL_Texture* texture = nullptr;
    SDL_AudioStream* stream = nullptr;

    PacketQueue audioq; //音频队列
};

std::unique_ptr<SDL_Window,decltype(free_win)> win(nullptr,free_win);

std::unique_ptr<SDL_Renderer,decltype(free_renderer)> renderer(nullptr,free_renderer);

static int w_width = 360;
static int w_height = 640;


int main(int argc, char* argv[]) {
    av_log_set_level(AV_LOG_INFO);

    int ret = 0;
    int flag = 0;
    VideoState *is;

    if(argc < 2) {
        fprintf(stderr, "Usage: command <file>\n");
        exit(-1);
    }

    char* input_filename = argv[1];

    flag = SDL_INIT_VIDEO | SDL_INIT_AUDIO;    

    //初始化SDL3,默认有timer
    if(!SDL_Init(flag)) {
        av_log(nullptr, AV_LOG_FATAL, "Could not initialize SDL - %s\n",SDL_GetError());
        return -1;
    }

    //绘制窗口
    win.reset(SDL_CreateWindow("Simple Player",w_width, w_height,SDL_WINDOW_VULKAN));

    if(win) {
        //根据窗口生成渲染器
        renderer.reset(SDL_CreateRenderer(win.get(), nullptr));
    }

    //测试窗口
    std::cin.get();

}
