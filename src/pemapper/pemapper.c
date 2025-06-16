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
  struct sectionInfo *sectionInfo = malloc(peData->numberOfSections * sizeof(struct sectionInfo));
  if (!sectionInfo)
    return ERR_OUT_OF_MEM;

  if (fseek(pe, (peData->headerSize - 1) + 0x8, SEEK_SET) != 0)
    return ERR_FILE_READ;

  for (uint16_t i = 0; i < peData->numberOfSections; i++) {
    sectionInfo[i].virtualSize = get32BitFromPE(pe);
    sectionInfo[i].rva = get32BitFromPE(pe);
    sectionInfo[i].rawSize = get32BitFromPE(pe);
    sectionInfo[i].offset = get32BitFromPE(pe);

    if (fseek(pe, 0x18, SEEK_CUR) != 0)
      return ERR_FILE_READ;
  }

  if (fseek(pe, 0, SEEK_SET) != 0)
    return ERR_FILE_READ;

  uint8_t *buffer = malloc(peData->headerSize + peData->sectionTableSize);
  if (!buffer)
    return ERR_OUT_OF_MEM;

  size_t totalHeader = peData->headerSize + peData->sectionTableSize;

  if (fread(buffer, 1, totalHeader, pe) != totalHeader)
    return ERR_FILE_READ;
  if (fwrite(buffer, 1, totalHeader, basefile) != totalHeader)
    return ERR_FILE_READ;

  for (uint16_t i = 0; i < peData->numberOfSections; i++) {
    buffer = realloc(buffer, sectionInfo[i].rawSize);
    if (!buffer)
      return ERR_OUT_OF_MEM;

    if (fseek(pe, sectionInfo[i].offset, SEEK_SET) != 0)
      return ERR_FILE_READ;
    if (fread(buffer, 1, sectionInfo[i].rawSize, pe) != sectionInfo[i].rawSize)
      return ERR_FILE_READ;

    if (fseek(basefile, sectionInfo[i].rva, SEEK_SET) != 0)
      return ERR_FILE_READ;
    if (fwrite(buffer, 1, sectionInfo[i].rawSize, basefile) != sectionInfo[i].rawSize)
      return ERR_FILE_READ;
  }

  uint32_t currentOffset = ftell(basefile);
  uint32_t nextAligned = getNextAligned(currentOffset, peData->pageSize) - 1;

  if (nextAligned != currentOffset) {
    buffer = realloc(buffer, 1);
    if (!buffer)
      return ERR_OUT_OF_MEM;
    buffer[0] = 0;

    if (fseek(basefile, nextAligned, SEEK_SET) != 0)
      return ERR_FILE_READ;
    if (fwrite(buffer, 1, 1, basefile) != 1)
      return ERR_FILE_READ;
  }

  peData->size = ftell(basefile);

  nullAndFree((void**)&buffer);
  nullAndFree((void**)&sectionInfo);

  return SUCCESS;
}
