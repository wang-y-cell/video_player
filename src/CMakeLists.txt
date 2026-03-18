set(filename main.cpp)

set(FILE_NAME ${CMAKE_CURRENT_SOURCE_DIR}/${filename})

# 关闭静态库开关，使用动态库
set(FFMPEG_USE_STATIC_LIBS OFF)
set(SDL3_USE_STATIC_LIBS OFF)

# 指定 3rdparty 目录下的库路径 - 使用 CACHE FORCE 确保可见性
set(FFMPEG_ROOT ${CMAKE_SOURCE_DIR}/3rdparty/ffmpeg CACHE PATH "FFmpeg root" FORCE)
set(SDL3_ROOT ${CMAKE_SOURCE_DIR}/3rdparty/sdl3 CACHE PATH "SDL3 root" FORCE)
set(FFmpeg_ROOT ${FFMPEG_ROOT} CACHE PATH "FFmpeg root for CMake" FORCE) # 兼容 CMP0074/0144

find_package(FFmpeg REQUIRED COMPONENTS avformat avcodec avutil swscale swresample)
find_package(SDL3 REQUIRED)

# 设置 RPATH，使得生成的可执行文件可以在运行时找到这些动态库
# $ORIGIN 代表可执行文件所在目录
# 我们假设 libs 会放在 bin/../3rdparty/... 或者直接放在 bin 同级
# 这里设置 RPATH 指向构建目录的相对路径，以及安装后的相对路径
set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
set(CMAKE_INSTALL_RPATH "$ORIGIN/../3rdparty/ffmpeg/lib:$ORIGIN/../3rdparty/sdl3/lib")

include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/Core
    ${CMAKE_SOURCE_DIR}/include/Decoders
    ${CMAKE_SOURCE_DIR}/include/Outputs
    ${CMAKE_SOURCE_DIR}/include/Utils
    ${FFmpeg_INCLUDE_DIRS}
    ${SDL3_INCLUDE_DIRS})

link_directories(${SDL3_LIBRARY_DIRS} ${FFmpeg_LIBRARY_DIRS})

get_filename_component(TARGET_NAME ${filename} NAME_WE)

file(GLOB_RECURSE SRC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

add_executable(${TARGET_NAME} ${SRC_FILES})

target_link_libraries(
    ${TARGET_NAME}
    SDL3::SDL3
    ${FFmpeg_LIBRARIES}
)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin)

find_package(Threads REQUIRED)
target_link_libraries(${TARGET_NAME} Threads::Threads)
