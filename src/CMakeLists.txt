set(FFMPEG_INCLUDE_DIR /usr/local/ffmpeg/include)
set(FFMPEG_LIB_DIR /usr/local/ffmpeg/lib)
set(SDL_INCLUDE_DIR /usr/local/include)
set(SDL_LIB_DIR /usr/local/lib)

set(filename main.cpp)

set(FILE_NAME ${CMAKE_CURRENT_SOURCE_DIR}/${filename})

include_directories(${CMAKE_SOURCE_DIR}/include ${FFMPEG_INCLUDE_DIR} ${SDL_INCLUDE_DIR})

link_directories(${FFMPEG_LIB_DIR} ${SDL_LIB_DIR})

get_filename_component(TARGET_NAME ${FILE_NAME} NAME_WE)

add_executable(${TARGET_NAME} ${FILE_NAME})

target_link_libraries(
    ${TARGET_NAME}
    SDL3
    avutil
    avcodec
    avformat
    swscale
    swresample
)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin)

find_package(Threads REQUIRED)
target_link_libraries(${TARGET_NAME} Threads::Threads)
