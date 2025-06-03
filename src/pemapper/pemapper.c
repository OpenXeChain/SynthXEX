// This file is part of SynthXEX, one component of the
// FreeChainXenon development toolchain
//
// Copyright (c) 2025 Aiden Isik
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

#include "pemapper.h"

struct sectionInfo
{
  uint32_t virtualSize;
  uint32_t rva;
  uint32_t rawSize;
  uint32_t offset;
};

// Maps the PE file into the basefile (RVAs become offsets)
int mapPEToBasefile(FILE *pe, FILE *basefile, struct peData *peData)
{
  // Retrieve data for each section
  struct sectionInfo *sectionInfo = malloc(peData->numberOfSections * sizeof(struct sectionInfo));
  fseek(pe, (peData->headerSize - 1) + 0x8, SEEK_SET); // Seek to the first section in the section table at virtualSize

  for(uint16_t i = 0; i < peData->numberOfSections; i++)
    {
      sectionInfo[i].virtualSize = get32BitFromPE(pe);
      sectionInfo[i].rva = get32BitFromPE(pe);
      sectionInfo[i].rawSize = get32BitFromPE(pe);
      sectionInfo[i].offset = get32BitFromPE(pe);      
      fseek(pe, 0x18, SEEK_CUR); // Seek to next entry at virtualSize
    }
  
  // Copy the PE header and section table to the basefile verbatim
  fseek(pe, 0, SEEK_SET);
  uint8_t *buffer = malloc(peData->headerSize + peData->sectionTableSize);
  if(buffer == NULL) {return ERR_OUT_OF_MEM;}

  fread(buffer, sizeof(uint8_t), peData->headerSize + peData->sectionTableSize, pe);
  fwrite(buffer, sizeof(uint8_t), peData->headerSize + peData->sectionTableSize, basefile);

  // Now map the sections and write them
  for(uint16_t i = 0; i < peData->numberOfSections; i++)
    {
      buffer = realloc(buffer, sectionInfo[i].rawSize * sizeof(uint8_t));
      if(buffer == NULL) {return ERR_OUT_OF_MEM;}
      
      fseek(pe, sectionInfo[i].offset, SEEK_SET);
      fread(buffer, sizeof(uint8_t), sectionInfo[i].rawSize, pe);

      fseek(basefile, sectionInfo[i].rva, SEEK_SET);
      fwrite(buffer, sizeof(uint8_t), sectionInfo[i].rawSize, basefile);
    }

  // Pad the rest of the final page with zeroes, we can achieve this by seeking
  // to the end and placing a single zero there (unless the data runs all the way up to the end!)
  uint32_t currentOffset = ftell(basefile);
  uint32_t nextAligned = getNextAligned(currentOffset, peData->pageSize) - 0x1;

  if(nextAligned != currentOffset)
    {
      buffer = realloc(buffer, 1 * sizeof(uint8_t));
      if(buffer == NULL) {return ERR_OUT_OF_MEM;}
      buffer[0] = 0;
      fseek(basefile, nextAligned, SEEK_SET);
      fwrite(buffer, sizeof(uint8_t), 1, basefile);
    }

  // Make sure to update the PE (basefile) size
  peData->size = ftell(basefile);
  
  // We're done with these, free them
  nullAndFree((void**)&buffer);
  nullAndFree((void**)&sectionInfo);

  return SUCCESS;
}
