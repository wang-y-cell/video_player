#include <cstdarg> //处理可变参数
#include <cstdio>
#include <filesystem>
#include <string>

#include <SDL3/SDL.h>

#include "Logger.hpp"
#include "PlayerEngine.hpp"
#include "ThreadPool.hpp"

extern "C" {
#include <libavutil/log.h>
}

namespace {
void ffmpegLogCallback(void*, int level, const char* fmt, va_list args) {
    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    Logger::instance().logFfmpeg(level, buffer);
}
}

int main(int argc, char** argv) {
    auto& logger = Logger::instance();
    logger.enableConsole(false);
#ifdef NDEBUG
    logger.setLevel(Logger::Level::Info);
    av_log_set_level(AV_LOG_INFO);
#else
    logger.setLevel(Logger::Level::Debug);
    av_log_set_level(AV_LOG_DEBUG);
#endif
    //打开日志文件
    const std::filesystem::path executable_path(argv[0]);
    const std::filesystem::path log_path = (executable_path.parent_path() / ".." / "logs" / "player.log").lexically_normal();
    if (!logger.setFile(log_path.string())) {
        LOG_WARN("Main", "打开日志文件失败: " << log_path.string());
    }
    av_log_set_callback(ffmpegLogCallback);

    if (argc < 2) {
        LOG_ERROR("Main", "用法: " << argv[0] << " <输入文件>");
        return 1;
    }

    const std::string input = argv[1];
    LOG_INFO("Main", "启动播放器，输入文件: " << input);

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_EVENTS)) {
        LOG_ERROR("Main", "SDL 初始化失败: " << SDL_GetError());
        return 1;
    }

    {
        thread_pool pool(4);
        //配置中介,设置音频播放方式,视频播放方式
        PlayerEngine engine(pool);
        if (!engine.prepare(input)) {
            LOG_ERROR("Main", "播放器准备失败: " << engine.lastError());
            SDL_Quit();
            return 1;
        }
        if (!engine.play()) {
            LOG_ERROR("Main", "播放器启动失败: " << engine.lastError());
            SDL_Quit();
            return 1;
        }
        engine.run();
    }

    SDL_Quit();
    LOG_INFO("Main", "播放器已退出");
    logger.closeFile();
    return 0;
}
