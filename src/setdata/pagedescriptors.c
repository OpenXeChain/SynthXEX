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

#include "pagedescriptors.h"

uint8_t getRwx(struct secInfoHeader *secInfoHeader, struct peData *peData, uint32_t page)
{
  uint32_t pageSize = secInfoHeader->peSize / secInfoHeader->pageDescCount;
  uint32_t currentOffset = page * pageSize;
  
  for(int i = peData->sections.count - 1; i >= 0; i--)
    {
      if(currentOffset >= peData->sections.sectionPerms[i].rawOffset)
	{
	  return peData->sections.sectionPerms[i].permFlag;
	}
    }

  return XEX_SECTION_RODATA | 0b10000; // We're in the PE header, so RODATA
}

void setPageDescriptors(FILE *pe, struct peData *peData, struct secInfoHeader *secInfoHeader)
{
  uint32_t pageSize = secInfoHeader->peSize / secInfoHeader->pageDescCount;
  secInfoHeader->descriptors = calloc(secInfoHeader->pageDescCount, sizeof(struct pageDescriptor)); // The free() for this is called after written to XEX

  // Setting size/info data and calculating hashes for page descriptors
  for(int64_t i = secInfoHeader->pageDescCount - 1; i >= 0; i--)
    {      
      // Get page type (rwx)
      secInfoHeader->descriptors[i].sizeAndInfo = getRwx(secInfoHeader, peData, i);
      
      // Init sha1 hash
      struct sha1_ctx shaContext;
      sha1_init(&shaContext);
      
      fseek(pe, i * pageSize, SEEK_SET);
      uint8_t page[pageSize]; 
      fread(page, sizeof(uint8_t), pageSize, pe);

      // For little endian systems, swap into big endian for hashing, then back (to keep struct endianness consistent)
#ifdef LITTLE_ENDIAN_SYSTEM
      secInfoHeader->descriptors[i].sizeAndInfo = htonl(secInfoHeader->descriptors[i].sizeAndInfo);
#endif
      
      sha1_update(&shaContext, pageSize, page);
      sha1_update(&shaContext, 0x18, (uint8_t*)&secInfoHeader->descriptors[i]);

#ifdef LITTLE_ENDIAN_SYSTEM
      secInfoHeader->descriptors[i].sizeAndInfo = ntohl(secInfoHeader->descriptors[i].sizeAndInfo);
#endif
      
      if(i != 0)
	{
	  sha1_digest(&shaContext, 0x14, secInfoHeader->descriptors[i - 1].sha1);
	}
      else
	{
	  sha1_digest(&shaContext, 0x14, secInfoHeader->imageSha1);
	}
    }

  free(peData->sections.sectionPerms); // Alloc'd in getdata
}
