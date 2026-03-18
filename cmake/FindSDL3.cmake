set(SDL3_FOUND FALSE)

set(SDL3_ROOT "" CACHE PATH "")

if(SDL3_ROOT)
  list(APPEND _sdl3_hints "${SDL3_ROOT}")
  set(_sdl3_use_pkg_config OFF)
else()
  set(_sdl3_use_pkg_config ON)
endif()
if(DEFINED ENV{SDL3_ROOT})
  list(APPEND _sdl3_hints "$ENV{SDL3_ROOT}")
  set(_sdl3_use_pkg_config OFF)
endif()
if(DEFINED ENV{SDL3_DIR})
  list(APPEND _sdl3_hints "$ENV{SDL3_DIR}")
  set(_sdl3_use_pkg_config OFF)
endif()

if(_sdl3_use_pkg_config)
  find_package(PkgConfig QUIET)
  if(PkgConfig_FOUND)
    if(SDL3_USE_STATIC_LIBS)
      pkg_check_modules(PC_SDL3 QUIET STATIC IMPORTED_TARGET sdl3)
    else()
      pkg_check_modules(PC_SDL3 QUIET IMPORTED_TARGET sdl3)
    endif()
  endif()
endif()

set(SDL3_INCLUDE_DIRS "")
set(SDL3_LIBRARIES "")
set(SDL3_LIBRARY_DIRS "")

if(PC_SDL3_FOUND)
  set(SDL3_INCLUDE_DIRS ${PC_SDL3_INCLUDE_DIRS})
  set(SDL3_LIBRARY_DIRS ${PC_SDL3_LIBRARY_DIRS})
  set(SDL3_LIBRARIES PkgConfig::PC_SDL3)
  set(SDL3_FOUND TRUE)
else()
  if(SDL3_ROOT OR ENV{SDL3_ROOT} OR ENV{SDL3_DIR})
    set(_sdl3_no_default NO_DEFAULT_PATH)
  endif()

  find_path(SDL3_INCLUDE_DIR
    NAMES SDL3/SDL.h
    HINTS ${_sdl3_hints}
    PATH_SUFFIXES include
    ${_sdl3_no_default}
  )

  if(SDL3_USE_STATIC_LIBS)
      set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
  endif()

  find_library(SDL3_LIBRARY
    NAMES SDL3
    HINTS ${_sdl3_hints}
    PATH_SUFFIXES lib lib64
    ${_sdl3_no_default}
  )

  if(SDL3_INCLUDE_DIR)
    set(SDL3_INCLUDE_DIRS ${SDL3_INCLUDE_DIR})
  endif()
  if(SDL3_LIBRARY)
    set(SDL3_LIBRARIES ${SDL3_LIBRARY})
  endif()

  if(SDL3_LIBRARY)
    get_filename_component(_sdl3_libdir "${SDL3_LIBRARY}" DIRECTORY)
    set(SDL3_LIBRARY_DIRS "${_sdl3_libdir}")
  endif()

  if(SDL3_INCLUDE_DIRS AND SDL3_LIBRARIES)
    set(SDL3_FOUND TRUE)
  endif()
endif()

if(SDL3_FOUND AND NOT TARGET SDL3::SDL3)
  if(TARGET PkgConfig::PC_SDL3)
    add_library(SDL3::SDL3 ALIAS PkgConfig::PC_SDL3)
  else()
    add_library(SDL3::SDL3 UNKNOWN IMPORTED)
    set_target_properties(SDL3::SDL3 PROPERTIES
      IMPORTED_LOCATION "${SDL3_LIBRARIES}"
      INTERFACE_INCLUDE_DIRECTORIES "${SDL3_INCLUDE_DIRS}"
    )
  endif()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(SDL3
  REQUIRED_VARS SDL3_INCLUDE_DIRS SDL3_LIBRARIES
)
