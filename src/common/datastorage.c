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

// TODO: Combine all of these into a single function

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

uint16_t get16BitFromPE(FILE *pe)
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

uint32_t get32BitFromXEX(FILE *xex)
{
  uint8_t store[4];
  fread(store, sizeof(uint8_t), 4, xex);
  
  uint32_t result = 0;

  for(int i = 0; i < 4; i++)
    {
      result |= store[i] << i * 8;
    }

  // If system and file endianness don't match we need to change it
#ifdef LITTLE_ENDIAN_SYSTEM
  result = ntohl(result);
#endif
  
  return result;
}
