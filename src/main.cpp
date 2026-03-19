#include <iostream>

#include <string>

#include <SDL3/SDL.h>

#include "PlayerEngine.hpp"
#include "ThreadPool.hpp"

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "usage: " << argv[0] << " <input>\n";
        return 1;
    }

    const std::string input = argv[1];

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_EVENTS)) {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << "\n";
        return 1;
    }

    {
        thread_pool pool(4);
        PlayerEngine engine(pool);
        if (!engine.prepare(input)) {
            std::cerr << "prepare failed: " << engine.lastError() << "\n";
            SDL_Quit();
            return 1;
        }
        if (!engine.play()) {
            std::cerr << "play failed: " << engine.lastError() << "\n";
            SDL_Quit();
            return 1;
        }
        engine.run();
    }

    SDL_Quit();
    return 0;
}
