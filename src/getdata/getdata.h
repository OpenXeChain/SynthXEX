// This file is part of SynthXEX, one component of the
// FreeChainXenon development toolchain
//
// Copyright (c) 2024 Aiden Isik
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

#include <sys/stat.h>
#include "../common/common.h"
#include "../common/datastorage.h"

// Utility functions to get data from PE file, byte-swapping as required
uint32_t get32BitFromPE(FILE *pe);
uint32_t get16BitFromPE(FILE *pe);

// Returns true if PE is valid Xbox 360 PE, else false
bool validatePE(FILE *pe);

// Gets data required for XEX building from PE
int getHdrData(FILE *pe, struct peData *peData, uint8_t flags);
