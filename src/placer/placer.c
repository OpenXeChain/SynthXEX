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

#include "placer.h"

uint32_t getNextAligned(uint32_t offset, uint32_t alignment)
{
  if(offset % alignment) // If offset not aligned
    {
      return offset + (alignment - (offset % alignment)); // Align
    }

  return offset; // Offset already aligned
}

// Todo in future: implement a dynamic optional header selection mechanism instead of hard-coding the basic 5
void placeStructs(struct offsets *offsets, struct xexHeader *xexHeader, struct secInfoHeader *secInfoHeader)
{
  // XEX Header
  uint32_t currentOffset = 0x0;
  offsets->xexHeader = currentOffset;
  currentOffset += sizeof(struct xexHeader);

  // Security header
  currentOffset = getNextAligned(currentOffset, 0x8); // 8-byte alignment for these headers etc
  offsets->secInfoHeader = currentOffset;
  xexHeader->secInfoOffset = currentOffset;
  currentOffset += (sizeof(struct secInfoHeader) - sizeof(void*)) + (secInfoHeader->pageDescCount * sizeof(struct pageDescriptor));

  // PE basefile
  currentOffset = getNextAligned(currentOffset, 0x1000); // 4KiB alignment for basefile
  offsets->basefile = currentOffset;
  xexHeader->peOffset = currentOffset;
}
