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

#include "writexex.h"

// TEMPORARY WRITE TESTING
int writeXEX(struct xexHeader *xexHeader, struct secInfoHeader *secInfoHeader, struct offsets *offsets, FILE *xex)
{
  // XEX Header
#ifdef LITTLE_ENDIAN_SYSTEM
  // Endian-swap XEX header before writing
  xexHeader->moduleFlags = htonl(xexHeader->moduleFlags);
  xexHeader->peOffset = htonl(xexHeader->peOffset);
  xexHeader->secInfoOffset = htonl(xexHeader->secInfoOffset);
  xexHeader->optHeaderCount = htonl(xexHeader->optHeaderCount);
#endif

  fseek(xex, offsets->xexHeader, SEEK_SET);
  fwrite(xexHeader, sizeof(uint8_t), sizeof(struct xexHeader), xex);

  // Security Info
  // Get page descriptor count before endian-swapping (we need it for page descriptors)
  int pageDescCount = secInfoHeader->pageDescCount; // Maybe re-order write order so this isn't necessary?
  
#ifdef LITTLE_ENDIAN_SYSTEM
  // Endian-swap secinfo header
  secInfoHeader->headerSize = htonl(secInfoHeader->headerSize);
  secInfoHeader->peSize = htonl(secInfoHeader->peSize);
  secInfoHeader->imageInfoSize = htonl(secInfoHeader->imageInfoSize);
  secInfoHeader->imageFlags = htonl(secInfoHeader->imageFlags);
  secInfoHeader->baseAddr = htonl(secInfoHeader->baseAddr);
  secInfoHeader->importTableCount = htonl(secInfoHeader->importTableCount);
  secInfoHeader->exportTableAddr = htonl(secInfoHeader->exportTableAddr);
  secInfoHeader->gameRegion = htonl(secInfoHeader->gameRegion);
  secInfoHeader->mediaTypes = htonl(secInfoHeader->mediaTypes);
  secInfoHeader->pageDescCount = htonl(secInfoHeader->pageDescCount);
#endif

  fseek(xex, offsets->secInfoHeader, SEEK_SET);
  fwrite(secInfoHeader, sizeof(uint8_t), sizeof(struct secInfoHeader) - sizeof(void*), xex); // sizeof(void*) == size of page descriptor pointer at end

  // Page descriptors
  for(int i = 0; i < pageDescCount; i++)
    {
#ifdef LITTLE_ENDIAN_SYSTEM
      secInfoHeader->descriptors[i].sizeAndInfo = htonl(secInfoHeader->descriptors[i].sizeAndInfo);
#endif
      
      // Writing out current descriptor...
      fwrite(&(secInfoHeader->descriptors[i].sizeAndInfo), sizeof(uint32_t), 0x1, xex);
      fwrite(secInfoHeader->descriptors[i].sha1, sizeof(uint8_t), 0x14, xex);
    }

  free(secInfoHeader->descriptors); // calloc'd elsewhere, freeing now
  
  return SUCCESS;
}
