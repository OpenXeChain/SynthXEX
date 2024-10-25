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

#include "getdata.h"

uint32_t get32BitFromPE(FILE *pe)
{
  uint8_t store[4];
  fread(store, sizeof(uint8_t), 4, pe);
  
  uint32_t result = 0;

  for(int i = 0; i < 4; i++)
    {
      result |= store[i] << i * 8;
    }

  // If system is big endian, swap endianness (PE is LE)
#ifdef BIG_ENDIAN_SYSTEM
  result = htonl(result);
#endif
  
  return result;
}

uint32_t get16BitFromPE(FILE *pe)
{
  uint8_t store[2];
  fread(store, sizeof(uint8_t), 2, pe);
  
  uint16_t result = 0;

  for(int i = 0; i < 2; i++)
    {
      result |= store[i] << i * 8;
    }

  // If system is big endian, swap endianness (PE is LE)
#ifdef BIG_ENDIAN_SYSTEM
  result = htons(result);
#endif
  
  return result;
}

// Validate PE. This isn't thorough, but it's enough to catch any non-PE/360 files
bool validatePE(FILE *pe) // True if valid, else false 
{
  // Check magic
  fseek(pe, 0, SEEK_SET);
  uint16_t magic = get16BitFromPE(pe);

  if(magic != 0x5A4D) // PE magic
    {
      return false;
    }

  // Check if pointer to PE header is valid
  fseek(pe, 0, SEEK_END);
  size_t finalOffset = ftell(pe) - 1;
  fseek(pe, 0x3C, SEEK_SET);
  size_t peHeaderOffset = get32BitFromPE(pe);

  if(finalOffset < peHeaderOffset)
    {
      return false;
    }

  // Check machine ID
  fseek(pe, peHeaderOffset + 0x4, SEEK_SET);
  uint16_t machineID = get16BitFromPE(pe);

  if(machineID != 0x1F2) // 0x1F2 == POWERPCBE
    {
      return false;
    }

  // Check subsystem
  if(finalOffset < peHeaderOffset + 0x5C)
    {
      return false;
    }
  
  fseek(pe, peHeaderOffset + 0x5C, SEEK_SET);
  uint16_t subsystem = get16BitFromPE(pe);

  if(subsystem != 0xE) // 0xE == XBOX
    {
      return false;
    }
  
  return true; // Checked enough, this is an Xbox 360 PE file
}

int getHdrData(FILE *pe, struct peData *peData, uint8_t flags)
{
  // Get header data required for ANY XEX
  // PE size
  struct stat peStat;
  fstat(fileno(pe), &peStat);
  peData->size = peStat.st_size;

  // Getting PE header offset before we go any further..
  fseek(pe, 0x3C, SEEK_SET);
  uint32_t peOffset = get32BitFromPE(pe);

  // Base address
  fseek(pe, peOffset + 0x34, SEEK_SET);
  peData->baseAddr = get32BitFromPE(pe);

  // Entry point (RVA)
  fseek(pe, peOffset + 0x28, SEEK_SET);
  peData->entryPoint = get32BitFromPE(pe);

  // TLS status (PE TLS is currently UNSUPPORTED, so if we find it, we'll need to abort (handled elsewhere))
  fseek(pe, peOffset + 0xC0, SEEK_SET);
  peData->tlsAddr = get32BitFromPE(pe);
  peData->tlsSize = get32BitFromPE(pe);

  // No flags supported at this time (will be used for getting additional info, for e.g. other optional headers)
  if(flags)
    {
      return ERR_UNKNOWN_DATA_REQUEST;
    }
  
  return SUCCESS;
}
