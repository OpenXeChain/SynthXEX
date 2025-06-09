
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

#include "optheaders.h"

void setBasefileFormat(struct basefileFormat *basefileFormat, struct secInfoHeader *secInfoHeader)
{
  basefileFormat->size = (1 * 8) + 8; // (Block count * size of raw data descriptor) + size of data descriptor
  basefileFormat->encType = 0x0; // No encryption
  basefileFormat->compType = 0x1; // No compression
  basefileFormat->dataSize = secInfoHeader->peSize;
  basefileFormat->zeroSize = 0x0; // We aren't going to be removing any zeroes. TODO: implement this, it can make files much smaller
}

// STUB. TLS info not supported.
void setTLSInfo(struct tlsInfo *tlsInfo)
{
  tlsInfo->slotCount = 0x40;
  tlsInfo->rawDataAddr = 0x0;
  tlsInfo->dataSize = 0x0;
  tlsInfo->rawDataSize = 0x0;
}

int setImportLibsInfo(struct importLibraries *importLibraries, struct peImportInfo *peImportInfo, struct secInfoHeader *secInfoHeader)
{ 
  // Set table count and allocate enough memory for all tables
  importLibraries->tableCount = peImportInfo->tableCount;
  secInfoHeader->importTableCount = peImportInfo->tableCount;
  importLibraries->importTables = calloc(importLibraries->tableCount, sizeof(struct importTable));
  if(importLibraries->importTables == NULL) {return ERR_OUT_OF_MEM;}
  struct importTable *importTables = importLibraries->importTables; // Use this to avoid dereferencing an unaligned pointer
  
  // Initialise the size of the import libraries to just the size of the header (- 2 * sizeof(void*) to exclude addresses for internal use only)
  importLibraries->size = (sizeof(struct importLibraries) + importLibraries->nameTableSize) - (2 * sizeof(void*));

  // Space to store names for use after the loop
  char *names[importLibraries->tableCount];
  
  // Populate each table, then compute it's hash and store in the previous table
  for(int64_t i = importLibraries->tableCount - 1; i >= 0; i--)
    {
      // Set the table index field to the current index
      importTables[i].tableIndex = i;
      
      // Extract the name, target, and minimum versions from the name string
      // Major and minor version are always 2 and 0, respectively
      // Strings are definitely null-terminated as otherwise they couldn't have been read in
      const uint8_t majorVer = 2;
      const uint8_t minorVer = 0;
      char *targetBuildVerStr = NULL;
      char *targetHotfixVerStr = NULL;
      char *minimumBuildVerStr = NULL;
      char *minimumHotfixVerStr = NULL;
      uint16_t buildVer;
      uint8_t hotfixVer;
      char *strtoulRet;

      // Get the name
      uint32_t oldNameLen = strlen(peImportInfo->tables[i].name);
      names[i] = strtok(peImportInfo->tables[i].name, "@");
      if(strlen(names[i]) == oldNameLen || strlen(names[i]) < 1)
	{return ERR_INVALID_IMPORT_NAME;} // Encountered '\0', not '@'

      // Target versions first
      targetBuildVerStr = strtok(NULL, ".");
      if(targetBuildVerStr == NULL) {return ERR_INVALID_IMPORT_NAME;} // No more tokens
      if(strlen(names[i]) + 1 + strlen(targetBuildVerStr) == oldNameLen)
	{return ERR_INVALID_IMPORT_NAME;} // Encountered null terminator instead of '.'

      buildVer = (uint16_t)strtoul(targetBuildVerStr, &strtoulRet, 10);
      if(*strtoulRet != 0 || strtoulRet == targetBuildVerStr)
	{return ERR_INVALID_IMPORT_NAME;} // Encountered a non-number, or string was empty

      targetHotfixVerStr = strtok(NULL, "+");
      if(targetHotfixVerStr == NULL) {return ERR_INVALID_IMPORT_NAME;}
      if(strlen(names[i]) + 1 + strlen(targetBuildVerStr) + 1 + strlen(targetHotfixVerStr) == oldNameLen)
	{return ERR_INVALID_IMPORT_NAME;}

      hotfixVer = (uint8_t)strtoul(targetHotfixVerStr, &strtoulRet, 10);
      if(*strtoulRet != 0 || strtoulRet == targetHotfixVerStr)
	{return ERR_INVALID_IMPORT_NAME;}

      // Now pack these into the target version bitfield
      importTables[i].targetVer =
	((majorVer & 0xF) << (32 - 4))
	| ((minorVer & 0xF) << (32 - 4 - 4))
	| (buildVer << (32 - 4 - 4 - 16))
	| hotfixVer;

      // Now onto the minimum versions, this works much the same
      minimumBuildVerStr = strtok(NULL, ".");
      if(minimumBuildVerStr == NULL) {return ERR_INVALID_IMPORT_NAME;} // No more tokens
      if(strlen(names[i]) + 1 + strlen(targetBuildVerStr) + 1 + strlen(targetHotfixVerStr)
	 + 1 + strlen(minimumBuildVerStr) == oldNameLen)
	{return ERR_INVALID_IMPORT_NAME;} // Encountered null terminator instead of '.'

      buildVer = (uint16_t)strtoul(minimumBuildVerStr, &strtoulRet, 10);
      if(*strtoulRet != 0 || strtoulRet == minimumBuildVerStr)
	{return ERR_INVALID_IMPORT_NAME;} // Encountered a non-number, or string was empty

      minimumHotfixVerStr = strtok(NULL, "\0");
      if(minimumHotfixVerStr == NULL) {return ERR_INVALID_IMPORT_NAME;}

      hotfixVer = (uint8_t)strtoul(minimumHotfixVerStr, &strtoulRet, 10);
      if(*strtoulRet != 0 || strtoulRet == minimumHotfixVerStr)
	{return ERR_INVALID_IMPORT_NAME;}

      // Now pack these into the minimum version bitfield
      importTables[i].minimumVer =
	((majorVer & 0xF) << (32 - 4))
	| ((minorVer & 0xF) << (32 - 4 - 4))
	| (buildVer << (32 - 4 - 4 - 16))
	| hotfixVer;

      // Hardcode a currently unknown value. TODO: find out how this is calculated.
      if(strcmp(names[i], "xboxkrnl.exe") == 0)
	{
	  importTables[i].unknown = 0x45DC17E0;
	}
      else if(strcmp(names[i], "xam.xex") == 0)
	{
	  importTables[i].unknown = 0xFCA15C76;
	}
      else if(strcmp(names[i], "xbdm.xex") == 0)
	{
	  importTables[i].unknown = 0xECEB8109;
	}
      else
	{
	  return ERR_UNSUPPORTED_STRUCTURE;
	}

      // Determine the number of addresses (2 for functions, 1 for everything else)
      importTables[i].addressCount =
	(peImportInfo->tables[i].branchStubCount * 2)
	+ (peImportInfo->tables[i].importCount - peImportInfo->tables[i].branchStubCount);

      // Allocate enough memory for the addresses
      importTables[i].addresses = calloc(importTables[i].addressCount, sizeof(uint32_t));
      if(importTables[i].addresses == NULL) {return ERR_OUT_OF_MEM;}
      uint32_t *addresses = importTables[i].addresses; // Use this to avoid dereferencing an unaligned pointer
      
      // Populate the addresses
      uint16_t currentAddr = 0;
      
      for(uint16_t j = 0; j < peImportInfo->tables[i].importCount; j++)
	{
	  if(currentAddr >= importTables[i].addressCount)
	    {
	      return ERR_DATA_OVERFLOW;
	    }
	  
	  addresses[currentAddr] = peImportInfo->tables[i].imports[j].iatAddr;
	  currentAddr++;

	  if(peImportInfo->tables[i].imports[j].branchStubAddr != 0)
	    {
	      if(currentAddr >= importTables[i].addressCount)
		{
		  return ERR_DATA_OVERFLOW;
		}
	      
	      addresses[currentAddr] = peImportInfo->tables[i].imports[j].branchStubAddr;
	      currentAddr++;
	    }
	}

      // Determine the total size, in bytes, of the current table (- sizeof(void*) to exclude address to addresses at the end)
      importTables[i].size = (sizeof(struct importTable) - sizeof(void*) + (importTables[i].addressCount * sizeof(uint32_t)));
      importLibraries->size += importTables[i].size;

      // Init sha1 hash
      struct sha1_ctx shaContext;
      sha1_init(&shaContext);

      // On little endian this ensures the byteswapped address count doesn't cause any trouble when using it.
      // On big endian it's pointless but here so the code isn't too complex with differences between endianness.
      uint16_t addressCount = importTables[i].addressCount;
      
      // If we're on a little endian system, swap everything into big endian for hashing
#ifdef LITTLE_ENDIAN_SYSTEM
      importTables[i].size = __builtin_bswap32(importTables[i].size);
      importTables[i].unknown = __builtin_bswap32(importTables[i].unknown);
      importTables[i].targetVer = __builtin_bswap32(importTables[i].targetVer);
      importTables[i].minimumVer = __builtin_bswap32(importTables[i].minimumVer);
      importTables[i].addressCount = __builtin_bswap16(importTables[i].addressCount);

      // Byteswap the addresses
      for(uint16_t j = 0; j < addressCount; j++)
	{
	  addresses[j] = __builtin_bswap32(addresses[j]);
	}
#endif

      // - sizeof(void*) to exclude the address to the addresses at the end (not part of the XEX format)
      // +/- sizeof(uint32_t) to exclude table size from hash
      sha1_update(&shaContext, sizeof(struct importTable) - sizeof(void*) - sizeof(uint32_t), (void*)&(importTables[i]) + sizeof(uint32_t));
      sha1_update(&shaContext, addressCount * sizeof(uint32_t), (void*)addresses);

      // If we're on a little endian system, swap everything back into little endian
#ifdef LITTLE_ENDIAN_SYSTEM
      importTables[i].size = __builtin_bswap32(importTables[i].size);
      importTables[i].unknown = __builtin_bswap32(importTables[i].unknown);
      importTables[i].targetVer = __builtin_bswap32(importTables[i].targetVer);
      importTables[i].minimumVer = __builtin_bswap32(importTables[i].minimumVer);
      importTables[i].addressCount = __builtin_bswap16(importTables[i].addressCount);

      // Byteswap the addresses
      for(uint16_t j = 0; j < addressCount; j++)
	{
	  addresses[j] = __builtin_bswap32(addresses[j]);
	}
#endif
      
      if(i != 0)
	{
	  sha1_digest(&shaContext, 0x14, importTables[i - 1].sha1);
	}
      else
	{
	  sha1_digest(&shaContext, 0x14, secInfoHeader->importTableSha1);
	}
    }

  // Allocate enough memory for the name table
  uint32_t nameOffsets[importLibraries->tableCount];

  for(uint32_t i = 0; i < importLibraries->tableCount; i++)
    {
      nameOffsets[i] = importLibraries->nameTableSize;
      importLibraries->nameTableSize += getNextAligned(strlen(names[i]) + 1, sizeof(uint32_t));
    }

  importLibraries->size += importLibraries->nameTableSize;
  importLibraries->nameTable = calloc(importLibraries->nameTableSize, sizeof(char));
  if(importLibraries->nameTable == NULL) {return ERR_OUT_OF_MEM;}
  char *nameTable = importLibraries->nameTable; // Use this to avoid dereferencing an unaligned pointer

  // Populate the name table
  for(uint32_t i = 0; i < importLibraries->tableCount; i++)
    {
      strcpy(&(nameTable[nameOffsets[i]]), names[i]);
    }

  return SUCCESS;
}

// STUB. TODO: Dynamically select, and/or allow user to select, flags
void setSysFlags(uint32_t *flags)
{
  *flags = XEX_SYS_GAMEPAD_DISCONNECT |
           XEX_SYS_INSECURE_SOCKETS |
           XEX_SYS_XAM_HOOKS |
           XEX_SYS_BACKGROUND_DL |
           XEX_SYS_ALLOW_CONTROL_SWAP;
}

int setOptHeaders(struct secInfoHeader *secInfoHeader, struct peData *peData, struct optHeaderEntries *optHeaderEntries, struct optHeaders *optHeaders)
{
  bool importsPresent = (peData->peImportInfo.totalImportCount > 0) ? true : false;

  optHeaderEntries->count = 4;

  if(importsPresent)
    {
      optHeaderEntries->count++;
    }
  
  optHeaderEntries->optHeaderEntry = calloc(optHeaderEntries->count, sizeof(struct optHeaderEntry));
  if(optHeaderEntries->optHeaderEntry == NULL) {return ERR_OUT_OF_MEM;}

  uint32_t currentHeader = 0;

  // NOTE: Make sure that these headers are handled IN ORDER OF ID. The loader will reject the XEX if they are not.

  // Basefile format (0x003FF)
  setBasefileFormat(&(optHeaders->basefileFormat), secInfoHeader);
  optHeaderEntries->optHeaderEntry[currentHeader].id = XEX_OPT_ID_BASEFILE_FORMAT;
  currentHeader++;

  // Entrypoint (0x10100)
  optHeaderEntries->optHeaderEntry[currentHeader].id = XEX_OPT_ID_ENTRYPOINT;
  optHeaderEntries->optHeaderEntry[currentHeader].dataOrOffset = secInfoHeader->baseAddr + peData->entryPoint;
  currentHeader++;

  // Import libraries (0x103FF)
  if(importsPresent)
    {
      optHeaderEntries->optHeaderEntry[currentHeader].id = XEX_OPT_ID_IMPORT_LIBS;
      int ret = setImportLibsInfo(&(optHeaders->importLibraries), &(peData->peImportInfo), secInfoHeader);
      if(ret != SUCCESS) {return ret;}
      currentHeader++;
    }

  // TLS info (0x20104)
  optHeaderEntries->optHeaderEntry[currentHeader].id = XEX_OPT_ID_TLS_INFO;
  setTLSInfo(&(optHeaders->tlsInfo));
  currentHeader++;

  // System flags (0x30000)
  optHeaderEntries->optHeaderEntry[currentHeader].id = XEX_OPT_ID_SYS_FLAGS;

  // We're using the name of the first element of the struct for clarity,
  // it can never be an unaligned access, so ignore that warning.
  // Also works for Clang.
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Waddress-of-packed-member"
  setSysFlags(&(optHeaderEntries->optHeaderEntry[currentHeader].dataOrOffset));
  #pragma GCC diagnostic pop

  //currentHeader++;

  return SUCCESS;
}
