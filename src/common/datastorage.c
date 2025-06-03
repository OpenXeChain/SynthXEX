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

// Frees memory and sets the pointer to NULL
// If the pointer is currently NULL, do nothing (avoid double-free)
// Disable pointer type checking here to make this easier to use
void nullAndFree(void **ptr)
{
  if(*ptr != NULL)
    {
      freeOnlyUseThisFunctionInTheNullAndFreeFunctionNowhereElse(*ptr);
      *ptr = NULL;
    }
}

// These functions together handle freeing all of the main structs.
// They can also be called individually, and it doesn't matter if you call them twice.
// Each function checks if the struct going to be accessed is NULL, so it doesn't try
// to access unallocated data.
//
// DO NOT CALL THESE OUTWITH MAIN. Those which take double pointers will replace
// the pointer given to them with null ONLY in the function they're called in, not it's callers
// (including main), which WILL result in a double free.
//
// Even if it doesn't access data right now, it's still there so it's not forgotten
// about if it becomes necessary with future additions.
void freeOffsetsStruct(struct offsets **offsets)
{
  if(*offsets != NULL)
    {
      nullAndFree((void**)&((*offsets)->optHeaders));
      nullAndFree((void**)offsets);
    }
}

void freeXexHeaderStruct(struct xexHeader **xexHeader)
{
  if(*xexHeader != NULL)
    {
      nullAndFree((void**)xexHeader);
    }
}

void freeSecInfoHeaderStruct(struct secInfoHeader **secInfoHeader)
{
  if(*secInfoHeader != NULL)
    {
      // Ignore the unaligned pointer warning here, every field before descriptors is a multiple of 8.
      // Also works for Clang.
      #pragma GCC diagnostic push
      #pragma GCC diagnostic ignored "-Waddress-of-packed-member"
      nullAndFree((void**)&((*secInfoHeader)->descriptors));
      #pragma GCC diagnostic pop
      
      nullAndFree((void**)secInfoHeader);
    }
}

void freeSectionsStruct(struct sections *sections)
{
  nullAndFree((void**)&(sections->section));
}

void freePeImportInfoStruct(struct peImportInfo *peImportInfo)
{
  if(peImportInfo->tables != NULL)
    {
      // Free the imports within each table first, then the tables,
      // otherwise we'll have a memory leak
      for(uint32_t i = 0; i < peImportInfo->tableCount; i++)
	{
	  nullAndFree((void**)&(peImportInfo->tables[i].name));
	  nullAndFree((void**)&(peImportInfo->tables[i].imports));
	}

      nullAndFree((void**)&(peImportInfo->tables));
    }
}

 
void freePeDataStruct(struct peData **peData)
{
  if(*peData != NULL)
    {
      freeSectionsStruct(&((*peData)->sections));
      freePeImportInfoStruct(&((*peData)->peImportInfo));
      nullAndFree((void**)peData);
    }
}

void freeOptHeaderEntriesStruct(struct optHeaderEntries **optHeaderEntries)
{
  if(*optHeaderEntries != NULL)
    {
      nullAndFree((void**)&((*optHeaderEntries)->optHeaderEntry));
      nullAndFree((void**)optHeaderEntries);
    }
}

void freeOptHeadersStruct(struct optHeaders **optHeaders)
{
  if(*optHeaders != NULL)
    {
      nullAndFree((void**)optHeaders);
    }
}

void freeAllMainStructs(struct offsets **offsets, struct xexHeader **xexHeader, struct secInfoHeader **secInfoHeader,
		    struct peData **peData, struct optHeaderEntries **optHeaderEntries, struct optHeaders **optHeaders)
{
  freeOffsetsStruct(offsets);
  freeXexHeaderStruct(xexHeader);
  freeSecInfoHeaderStruct(secInfoHeader);
  freePeDataStruct(peData);
  freeOptHeaderEntriesStruct(optHeaderEntries);
  freeOptHeadersStruct(optHeaders);
}

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
