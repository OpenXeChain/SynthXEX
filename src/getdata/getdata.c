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

#include "getdata.h"

// Validate PE. This isn't thorough, but it's enough to catch any non-PE/360 files.
// I was considering merging this with getHdrData and mapPEToBasefile as we're
// basically reading the same data twice, but I think it's beneficial to have a
// dedicated place where we validate the input.
bool validatePE(FILE *pe, bool skipMachineCheck) // True if valid, else false 
{
  // Check if we have at least the size of a DOS header, so we don't overrun the PE
  fseek(pe, 0, SEEK_END);
  size_t finalOffset = ftell(pe);

  if(finalOffset < 0x3C + 0x4)
    {
      return false;
    }
  
  // Check magic
  fseek(pe, 0, SEEK_SET);
  uint16_t magic = get16BitFromPE(pe);

  if(magic != 0x5A4D) // PE magic
    {
      return false;
    }

  // Check if pointer to PE header is valid
  fseek(pe, 0x3C, SEEK_SET);
  size_t peHeaderOffset = get32BitFromPE(pe);
  
  if(finalOffset < peHeaderOffset)
    {
      return false;
    }

  // Check if the file is big enough to get size of optional header, and therefore size of whole PE header 
  if(finalOffset < 0x14 + 0x2)
    {
      return false;
    }

  // Check section count
  fseek(pe, peHeaderOffset + 0x6, SEEK_SET);
  uint16_t sectionCount = get16BitFromPE(pe);
  
  if(sectionCount == 0)
    {
      return false;
    }
  
  // Check if the file is large enough to contain the whole PE header
  fseek(pe, peHeaderOffset + 0x14, SEEK_SET);
  uint16_t sizeOfOptHdr = get16BitFromPE(pe);

  // 0x18 == size of COFF header, 0x28 == size of one entry in section table
  if(finalOffset < peHeaderOffset + 0x18 + sizeOfOptHdr + (sectionCount * 0x28))
    {
      return false;
    }
  
  // Check machine ID
  fseek(pe, peHeaderOffset + 0x4, SEEK_SET);
  uint16_t machineID = get16BitFromPE(pe);

  if(machineID != 0x1F2 && !skipMachineCheck) // 0x1F2 == POWERPCBE
    {
      return false;
    }

  // Check subsystem
  fseek(pe, peHeaderOffset + 0x5C, SEEK_SET);
  uint16_t subsystem = get16BitFromPE(pe);

  if(subsystem != 0xE) // 0xE == XBOX
    {
      return false;
    }

  // Check page size/alignment
  fseek(pe, peHeaderOffset + 0x38, SEEK_SET);
  uint32_t pageSize = get32BitFromPE(pe);

  if(pageSize != 0x1000 && pageSize != 0x10000) // 4KiB and 64KiB are the only valid sizes
    {
      return false;
    }

  // Check each raw offset + raw size in section table
  fseek(pe, peHeaderOffset + 0x18 + sizeOfOptHdr + 0x10, SEEK_SET); // 0x10 == raw offset in entry

  for(uint16_t i = 0; i < sectionCount; i++)
    {
      // If raw size + raw offset exceeds file size, PE is invalid
      if(finalOffset < get32BitFromPE(pe) + get32BitFromPE(pe))
	{
	  return false;
	}

      fseek(pe, 0x20, SEEK_CUR); // Next entry
    }
  
  return true; // Checked enough, this is an Xbox 360 PE file
}

int getSectionRwxFlags(FILE *pe, struct sections *sections)
{
  fseek(pe, 0x3C, SEEK_SET);
  uint32_t peOffset = get32BitFromPE(pe);

  fseek(pe, peOffset + 0x6, SEEK_SET); // 0x6 == section count
  sections->count = get16BitFromPE(pe);
  
  sections->sectionPerms = calloc(sections->count, sizeof(struct sectionPerms)); // free() is called for this in setdata
  if(sections->sectionPerms == NULL) {return ERR_OUT_OF_MEM;}
  fseek(pe, peOffset + 0xF8, SEEK_SET); // 0xF8 == beginning of section table

  for(uint32_t i = 0; i < sections->count; i++)
    {
      // Check if we're part of the ELF section. If we are, handle it's segment permission flags.
      char sectionName[9] = {0}; // 8 chars, followed by null terminator
      fread(sectionName, sizeof(char), 0x8, pe);

      if(!strcmp(sectionName, ".elf")) // .elf section
	{
	  fseek(pe, 0x4, SEEK_CUR);
	  uint32_t elfRVA = get32BitFromPE(pe); // Store the base RVA for the ELF

	  // Get the offset in the PE of the ELF
	  uint32_t elfSize = get32BitFromPE(pe); // Store the size of the ELF
	  uint32_t elfBaseOffset = get32BitFromPE(pe);

	  // Set the end of the current section entry as the offset to return to when we're done with the ELF
	  uint32_t returnToOffset = ftell(pe) + 0x10;

	  // Verify that the .elf section actually contains an ELF. If it doesn't, just treat it like any other section.
	  fseek(pe, elfBaseOffset, SEEK_SET);
	  uint8_t elfMagic[4] = {0x7F, 'E', 'L', 'F'};
	  uint8_t readMagic[4] = {0};
	  fread(readMagic, sizeof(uint8_t), 0x4, pe);

	  // If we're NOT an ELF, handle this section like any other
	  if(memcmp(elfMagic, readMagic, 0x4 * sizeof(uint8_t)))
	    {
	      goto notElfInSection;
	    }

	  // Get the section table offset
	  fseek(pe, elfBaseOffset + 0x20, SEEK_SET);
	  uint32_t sectionTableOffset = get32BitFromXEX(pe); // Should probably rename this function... Used for any big-endian value.

	  // Get the number of sections
	  fseek(pe, elfBaseOffset + 0x30, SEEK_SET);
	  uint16_t sectionCount = get16BitFromXEX(pe);

	  // Check that the data we got makes sense (smaller than .elf section)
	  if(sectionTableOffset + (sectionCount * 0x28) > elfSize || sectionCount < 2) // 0x28 == size of each entry
	    {
	      goto notElfInSection;
	    }

	  // The offsets and sizes seem to check out, start processing the permissions
	  fseek(pe, elfBaseOffset + sectionTableOffset + 0x28, SEEK_SET); // +0x28 to skip to index 1 (0 is empty)
	  sections->count += sectionCount;
	  sections->sectionPerms = realloc(sections->sectionPerms, sections->count * sizeof(struct sectionPerms));
	  if(sections->sectionPerms == NULL) {return ERR_OUT_OF_MEM;}

	  for(uint32_t j = i; i < j + sectionCount; i++)
	    {
	      // Get the flags
	      fseek(pe, 0x8, SEEK_CUR);
	      uint32_t flags = get32BitFromXEX(pe);

	      // Calculate the RVA of the current section
	      fseek(pe, 0x4, SEEK_CUR);
	      sections->sectionPerms[i].rva = get32BitFromXEX(pe) + elfRVA;

	      // Translate ELF permissions to XEX permissions
	      if(flags & ELF_SECTION_FLAG_EXECUTABLE)
		{
		  sections->sectionPerms[i].permFlag = XEX_SECTION_CODE | 0b10000;
		}
	      else if(flags & ELF_SECTION_FLAG_WRITE)
		{
		  sections->sectionPerms[i].permFlag = XEX_SECTION_RWDATA | 0b10000;
		}
	      else if(flags & ELF_SECTION_FLAG_ALLOC)
		{
		  sections->sectionPerms[i].permFlag = XEX_SECTION_RODATA | 0b10000;
		}
	      else
		{
		  // If an ELF section doesn't have any of the above flags, it's a non-runtime section.
		  // Ignore it and move onto the next one.
		  sectionCount--;
		  sections->count--;
		  sections->sectionPerms = realloc(sections->sectionPerms, sections->count * sizeof(struct sectionPerms));
		  if(sections->sectionPerms == NULL) {return ERR_OUT_OF_MEM;}
		  i--;
		}

	      // Move to end of current entry
	      fseek(pe, 0x14, SEEK_CUR);

	      continue;

	      // This code is here so we only ever trigger it when we explicitly jump to it
	    notElfInSection:
	      // Seek back to the start of the section entry
	      fseek(pe, returnToOffset - 0x28, SEEK_SET);
  
	      // Jump to the code to handle normal sections
	      goto normalSectionHandling;
	    }

	  // Return to the PE section table at the start of the next entry
	  fseek(pe, returnToOffset, SEEK_SET);
	}
      else // Not the .elf section
	{
	normalSectionHandling:
	  fseek(pe, 0x4, SEEK_CUR); // Seek to RVA of section
	  sections->sectionPerms[i].rva = get32BitFromPE(pe);

	  fseek(pe, 0x14, SEEK_CUR); // Now progress to characteristics, where we will check flags
	  uint32_t characteristics = get32BitFromPE(pe);

	  if(characteristics & PE_SECTION_FLAG_EXECUTE)
	    {
	      sections->sectionPerms[i].permFlag = XEX_SECTION_CODE | 0b10000; // | 0b(1)0000 == include size of 1
	    }
	  else if(characteristics & PE_SECTION_FLAG_WRITE || characteristics & PE_SECTION_FLAG_DISCARDABLE)
	    {
	      sections->sectionPerms[i].permFlag = XEX_SECTION_RWDATA | 0b10000;
	    }
	  else if(characteristics & PE_SECTION_FLAG_READ)
	    {
	      sections->sectionPerms[i].permFlag = XEX_SECTION_RODATA | 0b10000;
	    }
	  else
	    {
	      return ERR_MISSING_SECTION_FLAG;
	    }

	  // Don't need to progress any more to get to beginning of next entry, as characteristics is last field
	}
    }
  
  return SUCCESS;
}

int getHdrData(FILE *pe, struct peData *peData, uint8_t flags)
{
  // No flags supported at this time (will be used for getting additional info, for e.g. other optional headers)
  if(flags)
    {
      return ERR_UNKNOWN_DATA_REQUEST;
    }
  
  // Get header data required for ANY XEX
  // PE size
  fseek(pe, 0, SEEK_SET); // If we don't do this, the size returned is wrong (?)
  struct stat peStat;
  fstat(fileno(pe), &peStat);
  peData->size = peStat.st_size;
  
  // Getting PE header offset before we go any further..
  fseek(pe, 0x3C, SEEK_SET);
  peData->peHeaderOffset = get32BitFromPE(pe);

  // Number of sections
  fseek(pe, peData->peHeaderOffset + 0x6, SEEK_SET);
  peData->numberOfSections = get16BitFromPE(pe);

  // Size of section table
  peData->sectionTableSize = peData->numberOfSections * 0x28;
  
  // Size of header
  // 0x18 == size of COFF header, get16BitFromPE value == size of optional header
  fseek(pe, peData->peHeaderOffset + 0x14, SEEK_SET);
  peData->headerSize = (peData->peHeaderOffset + 1) + 0x18 + get16BitFromPE(pe);
  
  // Entry point (RVA)
  fseek(pe, peData->peHeaderOffset + 0x28, SEEK_SET);
  peData->entryPoint = get32BitFromPE(pe);
  
  // Base address
  fseek(pe, peData->peHeaderOffset + 0x34, SEEK_SET);
  peData->baseAddr = get32BitFromPE(pe);

  // Page alignment/size
  fseek(pe, peData->peHeaderOffset + 0x38, SEEK_SET);
  peData->pageSize = get32BitFromPE(pe);

  // TLS status (PE TLS is currently UNSUPPORTED, so if we find it, we'll need to abort)
  fseek(pe, peData->peHeaderOffset + 0xC0, SEEK_SET);
  peData->tlsAddr = get32BitFromPE(pe);
  peData->tlsSize = get32BitFromPE(pe);
  if(peData->tlsAddr != 0 || peData->tlsSize != 0) {return ERR_UNSUPPORTED_STRUCTURE;}

  // Page RWX flags
  int ret = getSectionRwxFlags(pe, &(peData->sections));
  if(ret != 0) {return ret;}

  return SUCCESS;
}
