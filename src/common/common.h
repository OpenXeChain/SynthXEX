// This file is part of SynthXEX, one component of the
// OpenXeChain development toolchain
//
// Copyright (c) 2024-25 Aiden Isik
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#pragma once

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

// Forbid the use of the free function
#define freeOnlyUseThisFunctionInTheNullAndFreeFunctionNowhereElse free
#pragma GCC poison free

// Program identifiers (version set in CMakeLists.txt)
#if defined(DEBUG) || defined(_DEBUG)
    #define SYNTHXEX_NAME "SynthXEX-Debug"
#else
    #define SYNTHXEX_NAME "SynthXEX"
#endif

#define SYNTHXEX_COPYRIGHT "2024-2025"
#define SYNTHXEX_VERSION_STRING SYNTHXEX_NAME " " SYNTHXEX_VERSION

// Print constants
#define SYNTHXEX_PRINT_STEM SYNTHXEX_NAME ">"

// Return values
#define SUCCESS 0

#define ERR_UNKNOWN_DATA_REQUEST -1
#define ERR_MISSING_SECTION_FLAG -2
#define ERR_FILE_OPEN -3
#define ERR_FILE_READ -4
#define ERR_FILE_WRITE -5
#define ERR_OUT_OF_MEM -6
#define ERR_UNSUPPORTED_STRUCTURE -7
#define ERR_INVALID_RVA_OR_OFFSET -8
#define ERR_INVALID_IMPORT_NAME -9
#define ERR_DATA_OVERFLOW -10
