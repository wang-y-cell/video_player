cmake_minimum_required(VERSION 3.12)
project(player VERSION 1.0)

# 1. 基础编译设置
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED YES)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_BUILD_TYPE Debug)

# 2. 自动查找库 (支持跨平台: Linux用pkg-config, Windows用vcpkg等标准find_package)
if(WIN32)
     set(FFMPEG_ROOT "F:\\wy\\ffmpeg-master-latest-win64-gpl-shared")
     set(SDL3_ROOT "F:\\wy\\SDL3")
     include_directories(${FFMPEG_ROOT}/include)
     include_directories(${SDL3_ROOT}/include);
     # 链接库文件位置
     link_directories(${FFMPEG_ROOT}/lib)
     link_directories(${SDL3_ROOT}/lib)
     set(FFMPEG_TARGET libavformat libavcodec libavutil libswscale libswresample)
     set(SDL3_TARGET SDL3)

     

else()
    # 在 Linux/macOS 上，使用原生的 pkg-config 最为方便可靠
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(FFMPEG REQUIRED IMPORTED_TARGET libavformat libavcodec libavutil libswscale libswresample)
    pkg_check_modules(SDL3 REQUIRED IMPORTED_TARGET sdl3)
    
    set(FFMPEG_TARGET PkgConfig::FFMPEG)
    set(SDL3_TARGET PkgConfig::SDL3)
endif()

find_package(Threads REQUIRED)

# 3. 收集所有源文件
file(GLOB_RECURSE SRC_FILES "src/*.cpp")

# 4. 创建可执行文件
add_executable(${PROJECT_NAME} ${SRC_FILES})

# 5. 设置头文件搜索路径
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include/Core
    ${CMAKE_CURRENT_SOURCE_DIR}/include/Decoders
    ${CMAKE_CURRENT_SOURCE_DIR}/include/Outputs
    ${CMAKE_CURRENT_SOURCE_DIR}/include/Utils
)

# 6. 链接库文件
target_link_libraries(${PROJECT_NAME} PRIVATE
    ${FFMPEG_TARGET}
    ${SDL3_TARGET}
    Threads::Threads
)

# 7. 指定编译好的程序输出到 bin/ 目录
set_target_properties(${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)
