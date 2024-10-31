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
int writeXEX(struct xexHeader *xexHeader, struct optHeaderEntries *optHeaderEntries, struct secInfoHeader *secInfoHeader, struct optHeaders *optHeaders, struct offsets *offsets, FILE *xex)
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

  // Optional header entries
#ifdef LITTLE_ENDIAN_SYSTEM
  // Endian swap opt header entries
  for(uint32_t i = 0; i < optHeaderEntries->count; i++)
    {
      optHeaderEntries->optHeaderEntry[i].id = htonl(optHeaderEntries->optHeaderEntry[i].id);
      optHeaderEntries->optHeaderEntry[i].dataOrOffset = htonl(optHeaderEntries->optHeaderEntry[i].dataOrOffset);
    }
#endif

  fseek(xex, offsets->optHeaderEntries, SEEK_SET);

  for(int i = 0; i < optHeaderEntries->count; i++)
    {
      fwrite(&(optHeaderEntries->optHeaderEntry[i]), sizeof(uint8_t), sizeof(struct optHeaderEntry), xex);
    }

  free(optHeaderEntries->optHeaderEntry); // Alloc'd in setdata (optheaders). Now we're done with it.
  
  // Page descriptors
  fseek(xex, offsets->secInfoHeader + sizeof(struct secInfoHeader) - sizeof(void*), SEEK_SET);
  
  for(int i = 0; i < secInfoHeader->pageDescCount; i++)
    {
#ifdef LITTLE_ENDIAN_SYSTEM
      secInfoHeader->descriptors[i].sizeAndInfo = htonl(secInfoHeader->descriptors[i].sizeAndInfo);
#endif
      
      // Writing out current descriptor...
      fwrite(&(secInfoHeader->descriptors[i].sizeAndInfo), sizeof(uint32_t), 0x1, xex);
      fwrite(secInfoHeader->descriptors[i].sha1, sizeof(uint8_t), 0x14, xex);
    }

  free(secInfoHeader->descriptors); // calloc'd elsewhere, freeing now

  // Security Info
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
  
  // Optional headers
  uint32_t currentHeader = 0;
  
  if(optHeaders->basefileFormat.size != 0) // If not 0, it has data. Write it.
    {
      fseek(xex, offsets->optHeaders[currentHeader], SEEK_SET);

#ifdef LITTLE_ENDIAN_SYSTEM
      optHeaders->basefileFormat.size = htonl(optHeaders->basefileFormat.size);
      optHeaders->basefileFormat.encType = htons(optHeaders->basefileFormat.encType);
      optHeaders->basefileFormat.compType = htons(optHeaders->basefileFormat.compType);
      optHeaders->basefileFormat.dataSize = htonl(optHeaders->basefileFormat.dataSize);
      optHeaders->basefileFormat.zeroSize = htonl(optHeaders->basefileFormat.zeroSize);
#endif
      
      fwrite(&(optHeaders->basefileFormat), sizeof(uint8_t), sizeof(struct basefileFormat), xex);
      currentHeader++;
    }

  if(optHeaders->importLibraries.temp != 0)
    {
      fseek(xex, offsets->optHeaders[currentHeader], SEEK_SET);
      currentHeader++; // Skipping, don't know how to create yet...
    }

  if(optHeaders->tlsInfo.slotCount != 0)
    {
      fseek(xex, offsets->optHeaders[currentHeader], SEEK_SET);

#ifdef LITTLE_ENDIAN_SYSTEM
      optHeaders->tlsInfo.slotCount = htonl(optHeaders->tlsInfo.slotCount);
      optHeaders->tlsInfo.rawDataAddr = htonl(optHeaders->tlsInfo.rawDataAddr);
      optHeaders->tlsInfo.dataSize = htonl(optHeaders->tlsInfo.dataSize);
      optHeaders->tlsInfo.rawDataSize = htonl(optHeaders->tlsInfo.rawDataSize);
#endif

      fwrite(&(optHeaders->tlsInfo), sizeof(uint8_t), sizeof(struct tlsInfo), xex);
    }
  
  free(offsets->optHeaders); // Alloc'd in placer.
  return SUCCESS;
}
