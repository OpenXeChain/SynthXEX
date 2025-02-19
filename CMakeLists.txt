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

# This version of CMake is available on Debian bullseye backports. I think that's a reasonably old version that most people should have.
cmake_minimum_required(VERSION 3.25.1)
project(synthxex)

# Setting sources...
set(allsources ${CMAKE_SOURCE_DIR}/src/main.c)

list(APPEND allsources ${CMAKE_SOURCE_DIR}/src/common/common.h)
list(APPEND allsources ${CMAKE_SOURCE_DIR}/src/common/crypto.h)
list(APPEND allsources ${CMAKE_SOURCE_DIR}/src/common/datastorage.h)
list(APPEND allsources ${CMAKE_SOURCE_DIR}/src/common/datastorage.c)

list(APPEND allsources ${CMAKE_SOURCE_DIR}/src/getdata/getdata.h)
list(APPEND allsources ${CMAKE_SOURCE_DIR}/src/getdata/getdata.c)

list(APPEND allsources ${CMAKE_SOURCE_DIR}/src/pemapper/pemapper.h)
list(APPEND allsources ${CMAKE_SOURCE_DIR}/src/pemapper/pemapper.c)

list(APPEND allsources ${CMAKE_SOURCE_DIR}/src/placer/placer.h)
list(APPEND allsources ${CMAKE_SOURCE_DIR}/src/placer/placer.c)

list(APPEND allsources ${CMAKE_SOURCE_DIR}/src/setdata/optheaders.h)
list(APPEND allsources ${CMAKE_SOURCE_DIR}/src/setdata/pagedescriptors.h)
list(APPEND allsources ${CMAKE_SOURCE_DIR}/src/setdata/populateheaders.h)
list(APPEND allsources ${CMAKE_SOURCE_DIR}/src/setdata/elimzeroes.h)
list(APPEND allsources ${CMAKE_SOURCE_DIR}/src/setdata/optheaders.c)
list(APPEND allsources ${CMAKE_SOURCE_DIR}/src/setdata/pagedescriptors.c)
list(APPEND allsources ${CMAKE_SOURCE_DIR}/src/setdata/populateheaders.c)
list(APPEND allsources ${CMAKE_SOURCE_DIR}/src/setdata/elimzeroes.c)


list(APPEND allsources ${CMAKE_SOURCE_DIR}/src/write/headerhash.h)
list(APPEND allsources ${CMAKE_SOURCE_DIR}/src/write/writexex.h)
list(APPEND allsources ${CMAKE_SOURCE_DIR}/src/write/headerhash.c)
list(APPEND allsources ${CMAKE_SOURCE_DIR}/src/write/writexex.c)

list(APPEND allsources ${CMAKE_SOURCE_DIR}/include/getopt_port/getopt.h)
list(APPEND allsources ${CMAKE_SOURCE_DIR}/include/getopt_port/getopt.c)

# Setting compilation settings...
add_executable(synthxex ${allsources})
target_include_directories(synthxex PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(synthxex PRIVATE libnettle.a) # SynthXEX uses the nettle library for it's cryptography
target_compile_options(synthxex PRIVATE -Wno-multichar) # Shut up the torrent of warnings from the endian check

# If we're doing a debug build, compile with debugging and git commit hash
if(NOT ("${CMAKE_BUILD_TYPE}" STREQUAL "Release"))
	target_compile_options(synthxex PRIVATE -O0 -g -fsanitize=address)
	target_link_options(synthxex PRIVATE -fsanitize=address)

	execute_process(
			COMMAND git rev-parse --short HEAD
			WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
			OUTPUT_VARIABLE GIT_COMMIT
			OUTPUT_STRIP_TRAILING_WHITESPACE
		)

	add_compile_definitions(GIT_COMMIT="${GIT_COMMIT}")
endif()

# Setting install target settings...
install(TARGETS synthxex DESTINATION bin)
