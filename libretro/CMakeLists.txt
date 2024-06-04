# Copyright (c) 2024, SDLPAL development team.
# All rights reserved.
#
# This file is part of SDLPAL.
#
# SDLPAL is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 3
# as published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

cmake_minimum_required(VERSION 3.7)
project(sdlpal-libretro C CXX)

option(LIBRETRO_STATIC "Statically link the libretro core" OFF)

file(COPY sdl-libretro/SDL_config.h
  DESTINATION ${CMAKE_SOURCE_DIR}/../3rd/SDL/include
)

file(GLOB SOURCES
  ../*.c ../*.cpp
  ../adplug/*.c ../adplug/*.cpp
  ../timidity/*.c
  ../3rd/SDL/src/*.c
  ../3rd/SDL/src/audio/*.c
  ../3rd/SDL/src/cpuinfo/*.c
  ../3rd/SDL/src/events/*.c
  ../3rd/SDL/src/file/*.c
  ../3rd/SDL/src/joystick/*.c
  ../3rd/SDL/src/stdlib/*.c
  ../3rd/SDL/src/thread/*.c
  ../3rd/SDL/src/thread/pthread/*.c
  ../3rd/SDL/src/timer/*.c
  ../3rd/SDL/src/timer/unix/*.c
  ../3rd/SDL/src/video/*.c
  ../3rd/SDL/src/cdrom/*.c
  ../3rd/SDL/src/cdrom/dummy/*.c
  ../3rd/SDL/src/joystick/dummy/*.c
  ../3rd/SDL/src/loadso/dummy/*.c
  sdl-libretro/*.c
  libretro.c
)

if (LIBRETRO_STATIC)
  add_library(sdlpal_libretro STATIC ${SOURCES})
else ()
  add_library(sdlpal_libretro SHARED ${SOURCES})
  target_link_options(sdlpal_libretro PRIVATE
    -Wl,--version-script=${CMAKE_SOURCE_DIR}/link.T
    -Wl,--no-undefined
  )
endif ()

set_property(TARGET sdlpal_libretro PROPERTY C_STANDARD 99)
set_property(TARGET sdlpal_libretro PROPERTY CXX_STANDARD 11)
target_include_directories(sdlpal_libretro PRIVATE
  .
  ..
  sdl-libretro
  ../3rd/SDL/include
  ../timidity
)


# Follow naming conventions for libretro cores
set_target_properties(sdlpal_libretro PROPERTIES PREFIX "")
if (ANDROID)
  set_target_properties(sdlpal_libretro PROPERTIES SUFFIX "_android.so")
elseif (EMSCRIPTEN)
  set_target_properties(sdlpal_libretro PROPERTIES SUFFIX "${LIBRETRO_SUFFIX}.bc")
elseif (LIBRETRO_STATIC)
  set_target_properties(sdlpal_libretro PROPERTIES SUFFIX "${LIBRETRO_SUFFIX}.a")
endif ()
