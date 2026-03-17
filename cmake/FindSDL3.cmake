set(SDL3_FOUND FALSE)

set(SDL3_ROOT "" CACHE PATH "")

if(SDL3_ROOT)
  list(APPEND _sdl3_hints "${SDL3_ROOT}")
endif()
if(DEFINED ENV{SDL3_ROOT})
  list(APPEND _sdl3_hints "$ENV{SDL3_ROOT}")
endif()
if(DEFINED ENV{SDL3_DIR})
  list(APPEND _sdl3_hints "$ENV{SDL3_DIR}")
endif()

find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
  pkg_check_modules(PC_SDL3 QUIET IMPORTED_TARGET sdl3)
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
  find_path(SDL3_INCLUDE_DIR
    NAMES SDL3/SDL.h
    HINTS ${_sdl3_hints}
    PATH_SUFFIXES include
  )

  find_library(SDL3_LIBRARY
    NAMES SDL3
    HINTS ${_sdl3_hints}
    PATH_SUFFIXES lib lib64
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
