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

void setOptHeaderOffsets(struct offsets *offsets, struct optHeaderEntries *optHeaderEntries, struct optHeaders *optHeaders, uint32_t *currentOffset)
{
  offsets->optHeaders = calloc(optHeaderEntries->count, sizeof(uint32_t)); // Calloc because 0 values will be used to determine if a header is not present.
  uint32_t sepHeader = 0; // Separate header iterator, i.e. one with it's data outwith the entries
  
  for(uint32_t i = 0; i < optHeaderEntries->count; i++)
    {
      *currentOffset = getNextAligned(*currentOffset, 0x8);
      offsets->optHeaders[sepHeader] = *currentOffset;
      
      switch(optHeaderEntries->optHeaderEntry[i].id)
	{
	case XEX_OPT_ID_BASEFILE_FORMAT:
	  *currentOffset += sizeof(struct basefileFormat);
	  optHeaderEntries->optHeaderEntry[i].dataOrOffset = *currentOffset;
	  sepHeader++;
	  break;

	case XEX_OPT_ID_IMPORT_LIBS:
	  *currentOffset += sizeof(struct importLibraries);
	  optHeaderEntries->optHeaderEntry[i].dataOrOffset = *currentOffset;
	  sepHeader++;
	  break;

	case XEX_OPT_ID_TLS_INFO:
	  *currentOffset += sizeof(struct tlsInfo);
	  optHeaderEntries->optHeaderEntry[i].dataOrOffset = *currentOffset;
	  sepHeader++;
	  break;
	}
    }
}

// Todo in future: implement a dynamic optional header selection mechanism instead of hard-coding the basic 5
void placeStructs(struct offsets *offsets, struct xexHeader *xexHeader, struct optHeaderEntries *optHeaderEntries, struct secInfoHeader *secInfoHeader, struct optHeaders *optHeaders)
{
  // XEX Header
  uint32_t currentOffset = 0x0;
  offsets->xexHeader = currentOffset;
  currentOffset += sizeof(struct xexHeader);

  // Optional header entries (no alignment, they immediately follow XEX header)
  offsets->optHeaderEntries = currentOffset;
  currentOffset += optHeaderEntries->count * sizeof(struct optHeaderEntry);
  
  // Security header
  currentOffset = getNextAligned(currentOffset, 0x8); // 8-byte alignment for these headers etc
  offsets->secInfoHeader = currentOffset;
  xexHeader->secInfoOffset = currentOffset;
  currentOffset += (sizeof(struct secInfoHeader) - sizeof(void*)) + (secInfoHeader->pageDescCount * sizeof(struct pageDescriptor));

  // Optional headers
  setOptHeaderOffsets(offsets, optHeaderEntries, optHeaders, &currentOffset);
  
  // PE basefile
  currentOffset = getNextAligned(currentOffset, 0x1000); // 4KiB alignment for basefile
  offsets->basefile = currentOffset;
  xexHeader->peOffset = currentOffset;
}
