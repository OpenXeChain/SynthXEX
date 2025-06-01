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

#include "datastorage.h"

uint32_t getNextAligned(uint32_t offset, uint32_t alignment)
{
  if(offset % alignment) // If offset not aligned
    {
      return offset + (alignment - (offset % alignment)); // Align
    }

  return offset; // Offset already aligned
}

uint32_t rvaToOffset(uint32_t rva, struct sections *sections)
{
  if((sections->count > 0)
     && (rva >= sections->section[sections->count - 1].rva + sections->section[sections->count - 1].virtualSize))
    {
      return 0; // Not found (beyond end of PE)
    }
  
  for(int32_t i = sections->count - 1; i >= 0; i--)
    {
      if(rva >= sections->section[i].rva)
	{
	  return (rva - sections->section[i].rva) + sections->section[i].offset;
	}
    }

  return 0; // Not found
}

uint32_t offsetToRVA(uint32_t offset, struct sections *sections)
{
  if((sections->count > 0)
     && (offset >= sections->section[sections->count - 1].offset + sections->section[sections->count - 1].rawSize))
    {
      return 0; // Not found (beyond end of PE)
    }
  
  for(int32_t i = sections->count - 1; i >= 0; i--)
    {
      if(offset >= sections->section[i].offset)
	{
	  return (offset - sections->section[i].offset) + sections->section[i].rva;
	}
    }

  return 0; // Not found
}

uint32_t get32BitFromPE(FILE *pe)
{
  uint32_t result;
  fread(&result, sizeof(uint32_t), 1, pe);

  // If system is big endian, swap endianness (PE is LE)
#ifdef BIG_ENDIAN_SYSTEM
  return __builtin_bswap32(result);
#else
  return result;
#endif
}

uint16_t get16BitFromPE(FILE *pe)
{
  uint16_t result;
  fread(&result, sizeof(uint16_t), 1, pe);

  // If system is big endian, swap endianness (PE is LE)
#ifdef BIG_ENDIAN_SYSTEM
  return __builtin_bswap16(result);
#else
  return result;
#endif
}

uint32_t get32BitFromXEX(FILE *xex)
{
  uint32_t result;
  fread(&result, sizeof(uint32_t), 1, xex);

  // If system and file endianness don't match we need to change it
#ifdef LITTLE_ENDIAN_SYSTEM
  return __builtin_bswap32(result);
#else  
  return result;
#endif
}

uint16_t get16BitFromXEX(FILE *xex)
{
  uint16_t result;
  fread(&result, sizeof(uint16_t), 1, xex);

  // If system and file endianness don't match we need to change it
#ifdef LITTLE_ENDIAN_SYSTEM
  return __builtin_bswap16(result);
#else
  return result;
#endif
}
