set(_ffmpeg_components ${FFmpeg_FIND_COMPONENTS})
if(NOT _ffmpeg_components)
  set(_ffmpeg_components avformat avcodec avutil swscale swresample)
endif()

set(FFmpeg_FOUND FALSE)

set(FFMPEG_ROOT "" CACHE PATH "")

if(FFMPEG_ROOT)
  list(APPEND _ffmpeg_hints "${FFMPEG_ROOT}")
  list(APPEND _ffmpeg_hints "${FFMPEG_ROOT}/ffmpeg")
  set(_ffmpeg_use_pkg_config OFF)
else()
  set(_ffmpeg_use_pkg_config ON)
endif()

if(DEFINED ENV{FFMPEG_ROOT})
  list(APPEND _ffmpeg_hints "$ENV{FFMPEG_ROOT}")
  set(_ffmpeg_use_pkg_config OFF)
endif()
if(DEFINED ENV{FFMPEG_DIR})
  list(APPEND _ffmpeg_hints "$ENV{FFMPEG_DIR}")
  set(_ffmpeg_use_pkg_config OFF)
endif()

if(_ffmpeg_use_pkg_config)
  find_package(PkgConfig QUIET)
  if(PkgConfig_FOUND)
    if(FFMPEG_USE_STATIC_LIBS)
      pkg_check_modules(PC_FFMPEG QUIET STATIC libavformat libavcodec libavutil libswscale libswresample)
    else()
      pkg_check_modules(PC_FFMPEG QUIET libavformat libavcodec libavutil libswscale libswresample)
    endif()
  endif()
endif()

set(FFmpeg_INCLUDE_DIRS "")
set(FFmpeg_LIBRARIES "")
set(FFmpeg_LIBRARY_DIRS "")

if(PC_FFMPEG_FOUND)
  set(FFmpeg_INCLUDE_DIRS ${PC_FFMPEG_INCLUDE_DIRS})
  set(FFmpeg_LIBRARY_DIRS ${PC_FFMPEG_LIBRARY_DIRS})
  set(FFmpeg_LIBRARIES ${PC_FFMPEG_LIBRARIES} ${PC_FFMPEG_LDFLAGS_OTHER})
  set(FFmpeg_FOUND TRUE)
else()
  if(FFMPEG_ROOT OR ENV{FFMPEG_ROOT} OR ENV{FFMPEG_DIR})
    set(_ffmpeg_no_default NO_DEFAULT_PATH)
    message(STATUS "Using custom FFmpeg root: ${FFMPEG_ROOT}")
    message(STATUS "FFmpeg hints: ${_ffmpeg_hints}")
  endif()

  find_path(FFmpeg_INCLUDE_DIR
    NAMES libavformat/avformat.h
    HINTS ${_ffmpeg_hints}
    PATH_SUFFIXES include
    ${_ffmpeg_no_default}
  )

  if(FFmpeg_INCLUDE_DIR)
    set(FFmpeg_INCLUDE_DIRS ${FFmpeg_INCLUDE_DIR})
  endif()

  if(FFMPEG_USE_STATIC_LIBS)
      set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
  endif()

  foreach(_c IN LISTS _ffmpeg_components)
    find_library(FFmpeg_${_c}_LIBRARY
      NAMES ${_c}
      HINTS ${_ffmpeg_hints}
      PATH_SUFFIXES lib lib64
      ${_ffmpeg_no_default}
    )
    if(FFmpeg_${_c}_LIBRARY)
      list(APPEND FFmpeg_LIBRARIES ${FFmpeg_${_c}_LIBRARY})
    endif()
  endforeach()

  set(_ffmpeg_missing "")
  if(NOT FFmpeg_INCLUDE_DIRS)
    list(APPEND _ffmpeg_missing "headers")
  endif()
  foreach(_c IN LISTS _ffmpeg_components)
    if(NOT FFmpeg_${_c}_LIBRARY)
      list(APPEND _ffmpeg_missing "${_c}")
    endif()
  endforeach()

  if(NOT _ffmpeg_missing)
    set(FFmpeg_FOUND TRUE)
  endif()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(FFmpeg
  REQUIRED_VARS FFmpeg_INCLUDE_DIRS FFmpeg_LIBRARIES
)
