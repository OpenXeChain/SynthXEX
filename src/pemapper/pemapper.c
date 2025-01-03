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
int mapPEToBasefile(FILE *pe, FILE *basefile)
{
  // Get data about the header

  // Get PE header offset
  fseek(pe, 0x3C, SEEK_SET);
  uint32_t peHeaderOffset = get32BitFromPE(pe);

  // Get size of optional header
  fseek(pe, peHeaderOffset + 0x14, SEEK_SET);
  uint16_t optHeaderSize = get16BitFromPE(pe);

  // Calculate total header size (0x18 == size of COFF headers)
  uint32_t headerSize = (peHeaderOffset + 1) + 0x18 + optHeaderSize;

  // Get page size
  fseek(pe, peHeaderOffset + 0x38, SEEK_SET);
  uint32_t pageSize = get32BitFromPE(pe);

  // Get number of sections and calculate section table size
  fseek(pe, peHeaderOffset + 0x6, SEEK_SET);
  uint16_t numberOfSections = get16BitFromPE(pe);
  uint32_t sectionTableSize = numberOfSections * 0x28;
  
  // Retrieve data for each section
  struct sectionInfo *sectionInfo = malloc(numberOfSections * sizeof(struct sectionInfo));
  fseek(pe, (headerSize - 1) + 0x8, SEEK_SET); // Seek to the first section in the section table at virtualSize

  for(uint16_t i = 0; i < numberOfSections; i++)
    {
      sectionInfo[i].virtualSize = get32BitFromPE(pe);
      sectionInfo[i].rva = get32BitFromPE(pe);
      sectionInfo[i].rawSize = get32BitFromPE(pe);
      sectionInfo[i].offset = get32BitFromPE(pe);
      fseek(pe, 0x18, SEEK_CUR); // Seek to next entry at virtualSize
    }
  
  // Copy the PE header and section table to the basefile verbatim
  fseek(pe, 0, SEEK_SET);
  uint8_t *buffer = malloc(headerSize + sectionTableSize);
  fread(buffer, sizeof(uint8_t), headerSize + sectionTableSize, pe);
  fwrite(buffer, sizeof(uint8_t), headerSize + sectionTableSize, basefile);

  // Now map the sections and write them
  for(uint16_t i = 0; i < numberOfSections; i++)
    {
      buffer = realloc(buffer, sectionInfo[i].rawSize * sizeof(uint8_t));

      fseek(pe, sectionInfo[i].offset, SEEK_SET);
      fread(buffer, sizeof(uint8_t), sectionInfo[i].rawSize, pe);

      fseek(basefile, sectionInfo[i].rva, SEEK_SET);
      fwrite(buffer, sizeof(uint8_t), sectionInfo[i].rawSize, basefile);
    }

  // Pad the rest of the final page with zeroes, we can achieve this by seeking
  // to the end and placing a single zero there (unless the data runs all the way up to the end!)
  uint32_t currentOffset = ftell(basefile);
  uint32_t nextAligned = getNextAligned(currentOffset, pageSize) - 0x1;

  if(nextAligned != currentOffset)
    {
      buffer = realloc(buffer, 1 * sizeof(uint8_t));
      buffer[0] = 0;
      fseek(basefile, nextAligned, SEEK_SET);
      fwrite(buffer, sizeof(uint8_t), 1, basefile);
    }
  
  // We're done with these, free them
  free(buffer);
  free(sectionInfo);

  return SUCCESS;
}
