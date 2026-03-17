set(SDL_INCLUDE_DIR /usr/local/include)
set(SDL_LIB_DIR /usr/local/lib)

set(filename main.cpp)

set(FILE_NAME ${CMAKE_CURRENT_SOURCE_DIR}/${filename})

find_package(FFmpeg REQUIRED COMPONENTS avformat avcodec avutil swscale swresample)

include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/Core
    ${CMAKE_SOURCE_DIR}/include/Decoders
    ${CMAKE_SOURCE_DIR}/include/Outputs
    ${CMAKE_SOURCE_DIR}/include/Utils
    ${FFmpeg_INCLUDE_DIRS}
    ${SDL_INCLUDE_DIR})

link_directories(${SDL_LIB_DIR} ${FFmpeg_LIBRARY_DIRS})

get_filename_component(TARGET_NAME ${FILE_NAME} NAME_WE)

add_executable(${TARGET_NAME} ${FILE_NAME})

target_link_libraries(
    ${TARGET_NAME}
    SDL3
    ${FFmpeg_LIBRARIES}
)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin)

find_package(Threads REQUIRED)
target_link_libraries(${TARGET_NAME} Threads::Threads)
