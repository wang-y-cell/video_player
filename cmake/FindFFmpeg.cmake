set(_ffmpeg_components ${FFmpeg_FIND_COMPONENTS})
if(NOT _ffmpeg_components)
  set(_ffmpeg_components avformat avcodec avutil swscale swresample)
endif()

set(FFmpeg_FOUND FALSE)

set(FFMPEG_ROOT "" CACHE PATH "")

if(FFMPEG_ROOT)
  list(APPEND _ffmpeg_hints "${FFMPEG_ROOT}")
  list(APPEND _ffmpeg_hints "${FFMPEG_ROOT}/ffmpeg")
endif()

if(DEFINED ENV{FFMPEG_ROOT})
  list(APPEND _ffmpeg_hints "$ENV{FFMPEG_ROOT}")
endif()
if(DEFINED ENV{FFMPEG_DIR})
  list(APPEND _ffmpeg_hints "$ENV{FFMPEG_DIR}")
endif()

find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
  pkg_check_modules(PC_FFMPEG QUIET libavformat libavcodec libavutil libswscale libswresample)
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
  find_path(FFmpeg_INCLUDE_DIR
    NAMES libavformat/avformat.h
    HINTS ${_ffmpeg_hints}
    PATH_SUFFIXES include
  )

  if(FFmpeg_INCLUDE_DIR)
    set(FFmpeg_INCLUDE_DIRS ${FFmpeg_INCLUDE_DIR})
  endif()

  foreach(_c IN LISTS _ffmpeg_components)
    find_library(FFmpeg_${_c}_LIBRARY
      NAMES ${_c}
      HINTS ${_ffmpeg_hints}
      PATH_SUFFIXES lib lib64
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
