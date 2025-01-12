cmake_minimum_required(VERSION 3.28)
project(SimpleVideoPlayer C)

set(CMAKE_C_STANDARD 11)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(FFMPEG_DEV_ROOT D:/app/ffmpeg-shared-4.4)
set(SDL_DEV_ROOT D:/app/SDL2-devel-2.0.8-mingw/SDL2-2.0.8/x86_64-w64-mingw32)


# set ffmpeg root directory
if(NOT FFMPEG_DEV_ROOT)
    message(FATAL_ERROR "set FFMPEG_DEV_ROOT to use ffmpeg libraries")
endif()

# set ffmpeg develop environment
include_directories(${FFMPEG_DEV_ROOT}/include ${SDL_DEV_ROOT}/include)
link_directories(${FFMPEG_DEV_ROOT}/lib ${SDL_DEV_ROOT}/lib)
link_libraries(
        avcodec
        avformat
        avfilter
        avdevice
        swresample
        swscale
        avutil
        SDL2
        SDL2main
)


# copy dlls
file(GLOB ffmpeg_shared_libries ${FFMPEG_DEV_ROOT}/bin/*dll)
file(COPY ${ffmpeg_shared_libries} DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

# copy sdl dlls
file(GLOB sdl_shared_libries ${SDL_DEV_ROOT}/bin/*dll)
file(COPY ${sdl_shared_libries} DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

# copy test file
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/Titanic.ts DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/cuc_ieschool.flv DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/ds.mov DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/sintel_640_360.yuv DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/test_yuv420p_320x180.yuv DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/屌丝男士.mov DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})


add_executable(TestDecoder test_decoder.c)
add_executable(TestSdl test_sdl.c)
add_executable(TestSdlThread test_sdl_thread.c)
add_executable(TestYuvSdl test_yuv_sdl.c)
