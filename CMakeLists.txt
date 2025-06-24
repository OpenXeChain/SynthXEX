# This file is part of SynthXEX, one component of the
# FreeChainXenon development toolchain
#
# Copyright (c) 2025 Aiden Isik
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Affero General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

cmake_minimum_required(VERSION 3.25.1)
project(synthxex)

if(MSVC)
  message(SEND_ERROR "Compiling with MSVC is not supported. Please use GCC or Clang via the -DCMAKE_C_COMPILER flag or CC environment variable. (Example: cmake -DCMAKE_C_COMPILER=\"gcc\" ..).")
  return()
endif()

# Gather all source and header files
file(GLOB_RECURSE allsources
  ${CMAKE_SOURCE_DIR}/src/*.c
  ${CMAKE_SOURCE_DIR}/src/*.h
  ${CMAKE_SOURCE_DIR}/include/getopt_port/*.c
  ${CMAKE_SOURCE_DIR}/include/getopt_port/*.h
  ${CMAKE_SOURCE_DIR}/include/nettle/*.c
  ${CMAKE_SOURCE_DIR}/include/nettle/*.h
)

# Setting compilation settings
add_executable(synthxex ${allsources})
target_include_directories(synthxex PRIVATE ${CMAKE_SOURCE_DIR}/include)

# Debug/release build handling
if(GENERATOR_IS_MULTI_CONFIG)
  set(SYNTHXEX_BUILD_TYPE "MultiConf") # Multi-config generators handle debug build options themselves, don't apply ours
else()
  if(CMAKE_BUILD_TYPE)
    set(SYNTHXEX_BUILD_TYPE ${CMAKE_BUILD_TYPE})
  else()
    set(SYNTHXEX_BUILD_TYPE "Debug")
  endif()
endif()

if(${SYNTHXEX_BUILD_TYPE} MATCHES "Deb")
  add_compile_definitions(_DEBUG=1)

  if(NOT MINGW)
    target_compile_options(synthxex PRIVATE -O0 -g -fsanitize=address -fsanitize=undefined)
    target_link_options(synthxex PRIVATE -lasan -lubsan -fsanitize=address -fsanitize=undefined)
  else()
    target_compile_options(synthxex PRIVATE -O0 -g)
  endif()
endif()

# Set the version
execute_process(
  COMMAND git describe HEAD
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_VARIABLE VERSION_STRING
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

# Only use the result from the git command if it's a valid version string
if(${VERSION_STRING} MATCHES "^v[0-9]+\.[0-9]+\.[0-9]+(-[0-9]+-[0-9a-z]+)?$")
  add_compile_definitions(SYNTHXEX_VERSION="${VERSION_STRING}")
else()
  add_compile_definitions(SYNTHXEX_VERSION="v0.0.3") # Only used as a fallback
endif()

# Setting install target settings...
install(TARGETS synthxex DESTINATION bin)
