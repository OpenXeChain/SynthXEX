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

#include "headerhash.h"

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

// So, it would probably be more sensible to endian-swap all of the data back and determine which structure is where
// to determine the hash, but reading the file we just created is easier.
void setHeaderSha1(FILE *xex)
{
  fseek(xex, 0x8, SEEK_SET); // Basefile offset
  uint32_t basefileOffset = get32BitFromXEX(xex);
  
  fseek(xex, 0x10, SEEK_SET); // Secinfo offset
  uint32_t secInfoOffset = get32BitFromXEX(xex);

  uint32_t endOfImageInfo = secInfoOffset + 0x8 + 0x174; // 0x8 == image info offset in security info, 0x174 == length of that
  uint32_t remainingSize = basefileOffset - endOfImageInfo; // How much data is between end of image info and basefile (we hash that too)

  // Init sha1 hash
  struct sha1_ctx shaContext;
  sha1_init(&shaContext);

  // Hash first part (remainder of headers is done first, then the start)
  uint8_t remainderOfHeaders[remainingSize];
  fseek(xex, basefileOffset, SEEK_SET);
  fread(remainderOfHeaders, sizeof(uint8_t), remainingSize, xex);
  sha1_update(&shaContext, remainingSize, remainderOfHeaders);
  
  // Second part (XEX header)
  uint8_t headersStart[secInfoOffset + 0x8]; // Hash from start up to image info (0x8 into security header)
  fseek(xex, 0, SEEK_SET);
 
  fread(headersStart, sizeof(uint8_t), secInfoOffset + 0x8, xex);  
  sha1_update(&shaContext, secInfoOffset + 0x8, headersStart);  
  
  // Get final hash
  uint8_t headerHash[20];
  sha1_digest(&shaContext, 20, headerHash);

  // Finally, write it out
  fseek(xex, secInfoOffset + 0x164, SEEK_SET); // 0x164 == offset in secinfo of header hash
  fwrite(headerHash, sizeof(uint8_t), 20, xex);
}
