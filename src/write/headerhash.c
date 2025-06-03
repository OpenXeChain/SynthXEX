// This file is part of SynthXEX, one component of the
// FreeChainXenon development toolchain
//
// Copyright (c) 2024-25 Aiden Isik
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

// TEMPORARY, TO BE REPLACED ELSEWHERE WHEN IMPORT HEADER IS IMPLEMENTED
void setImportsSha1(FILE *xex)
{
  fseek(xex, 0x0, SEEK_SET);
  while(get32BitFromXEX(xex) != 0x103FF){}
  fseek(xex, get32BitFromXEX(xex) + 0x4, SEEK_SET);
  
  uint8_t importCount[4];
  fseek(xex, 0x4, SEEK_CUR);
  fread(importCount, sizeof(uint8_t), 4, xex);
  fseek(xex, -0x8, SEEK_CUR);
  
  fseek(xex, get32BitFromXEX(xex) + 0x4, SEEK_CUR);
  uint32_t size = get32BitFromXEX(xex) - 0x4;
  
  uint8_t data[size];
  fread(data, sizeof(uint8_t), size, xex);
  
  struct sha1_ctx shaContext;
  sha1_init(&shaContext);
  sha1_update(&shaContext, size, data);

  uint8_t sha1[20];
  sha1_digest(&shaContext, 20, sha1);

  fseek(xex, 0x10, SEEK_SET);
  fseek(xex, get32BitFromXEX(xex) + 0x128, SEEK_SET);
  fwrite(importCount, sizeof(uint8_t), 4, xex);
  fwrite(sha1, sizeof(uint8_t), 20, xex);
}

// So, it would probably be more sensible to endian-swap all of the data back and determine which structure is where
// to determine the hash, but reading the file we just created is easier.
int setHeaderSha1(FILE *xex)
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
  fseek(xex, endOfImageInfo, SEEK_SET);
  fread(remainderOfHeaders, sizeof(uint8_t), remainingSize, xex);
  sha1_update(&shaContext, remainingSize, remainderOfHeaders);
  
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

  return SUCCESS;
}
