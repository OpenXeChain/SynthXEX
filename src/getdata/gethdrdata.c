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

#include "gethdrdata.h"

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
    { return false; }

    // Check magic
    fseek(pe, 0, SEEK_SET);
    uint16_t magic = get16BitFromPE(pe);

    if(magic != 0x5A4D || errno != SUCCESS) // PE magic
    { return false; }

    // Check if pointer to PE header is valid
    fseek(pe, 0x3C, SEEK_SET);
    size_t peHeaderOffset = get32BitFromPE(pe);

    if(finalOffset < peHeaderOffset || errno != SUCCESS)
    { return false; }

    // Check if the file is big enough to get size of optional header, and therefore size of whole PE header
    if(finalOffset < 0x14 + 0x2)
    { return false; }

    // Check section count
    fseek(pe, peHeaderOffset + 0x6, SEEK_SET);
    uint16_t sectionCount = get16BitFromPE(pe);

    if(sectionCount == 0 || errno != SUCCESS)
    { return false; }

    // Check if the file is large enough to contain the whole PE header
    fseek(pe, peHeaderOffset + 0x14, SEEK_SET);
    uint16_t sizeOfOptHdr = get16BitFromPE(pe);

    // 0x18 == size of COFF header, 0x28 == size of one entry in section table
    if(finalOffset < peHeaderOffset + 0x18 + sizeOfOptHdr + (sectionCount * 0x28) || errno != SUCCESS)
    { return false; }

    // Check machine ID
    fseek(pe, peHeaderOffset + 0x4, SEEK_SET);

    // 0x1F2 == POWERPCBE
    uint16_t machineID = get16BitFromPE(pe);

    if((machineID != 0x1F2 && !skipMachineCheck) || errno != SUCCESS)
    { return false; }

    // Check subsystem
    fseek(pe, peHeaderOffset + 0x5C, SEEK_SET);

    uint16_t subsystem = get16BitFromPE(pe);

    if(subsystem != 0xE || errno != SUCCESS) // 0xE == XBOX
    { return false; }

    // Check page size/alignment
    fseek(pe, peHeaderOffset + 0x38, SEEK_SET);

    // 4KiB and 64KiB are the only valid sizes
    uint32_t pageSize = get32BitFromPE(pe);

    if((pageSize != 0x1000 && pageSize != 0x10000) || errno != SUCCESS)
    { return false; }

    // Check each raw offset + raw size in section table
    fseek(pe, peHeaderOffset + 0x18 + sizeOfOptHdr + 0x10, SEEK_SET); // 0x10 == raw offset in entry

    for(uint16_t i = 0; i < sectionCount; i++)
    {
        // If raw size + raw offset exceeds file size, PE is invalid
        uint32_t rawSize = get32BitFromPE(pe);

        if(errno != SUCCESS)
        { return false; }

        uint32_t rawOffset = get32BitFromPE(pe);

        if(errno != SUCCESS)
        { return false; }

        if(finalOffset < rawSize + rawOffset)
        { return false; }

        // Next entry
        fseek(pe, 0x20, SEEK_CUR);
    }

    return true; // Checked enough, this is an Xbox 360 PE file
}

int getSectionInfo(FILE *pe, struct sections *sections)
{
    fseek(pe, 0x3C, SEEK_SET);
    uint32_t peOffset = get32BitFromPE(pe);

    if(errno != SUCCESS)
    { return errno; }

    fseek(pe, peOffset + 0x6, SEEK_SET); // 0x6 == section count
    sections->count = get16BitFromPE(pe);

    if(errno != SUCCESS)
    { return errno; }

    sections->section = calloc(sections->count, sizeof(struct section)); // free() is called for this in setdata

    if(sections->section == NULL)
    { return ERR_OUT_OF_MEM; }

    fseek(pe, peOffset + 0xF8, SEEK_SET); // 0xF8 == beginning of section table

    for(uint16_t i = 0; i < sections->count; i++)
    {
        fseek(pe, 0x8, SEEK_CUR); // Seek to virtual size of section
        sections->section[i].virtualSize = get32BitFromPE(pe);

        if(errno != SUCCESS)
        { return errno; }

        sections->section[i].rva = get32BitFromPE(pe);

        if(errno != SUCCESS)
        { return errno; }

        sections->section[i].rawSize = get32BitFromPE(pe);

        if(errno != SUCCESS)
        { return errno; }

        sections->section[i].offset = get32BitFromPE(pe);

        if(errno != SUCCESS)
        { return errno; }

        fseek(pe, 0xC, SEEK_CUR); // Now progress to characteristics, where we will check flags
        uint32_t characteristics = get32BitFromPE(pe);

        if(errno != SUCCESS)
        { return errno; }

        if(characteristics & PE_SECTION_FLAG_EXECUTE)
        {
            sections->section[i].permFlag = XEX_SECTION_CODE | 0b10000; // | 0b(1)0000 == include size of 1
        }
        else if(characteristics & PE_SECTION_FLAG_WRITE || characteristics & PE_SECTION_FLAG_DISCARDABLE)
        { sections->section[i].permFlag = XEX_SECTION_RWDATA | 0b10000; }
        else if(characteristics & PE_SECTION_FLAG_READ)
        { sections->section[i].permFlag = XEX_SECTION_RODATA | 0b10000; }
        else
        { return ERR_MISSING_SECTION_FLAG; }
    }

    // Don't need to progress any more to get to beginning of next entry, as characteristics is last field
    return SUCCESS;
}

int getHdrData(FILE *pe, struct peData *peData, uint8_t flags)
{
    // No flags supported at this time (will be used for getting additional info, for e.g. other optional headers)
    if(flags)
    { return ERR_UNKNOWN_DATA_REQUEST; }

    // Getting PE header offset before we go any further..
    fseek(pe, 0x3C, SEEK_SET);
    peData->peHeaderOffset = get32BitFromPE(pe);

    if(errno != SUCCESS)
    { return errno; }

    // Number of sections
    fseek(pe, peData->peHeaderOffset + 0x6, SEEK_SET);
    peData->numberOfSections = get16BitFromPE(pe);

    if(errno != SUCCESS)
    { return errno; }

    // Size of section table
    peData->sectionTableSize = peData->numberOfSections * 0x28;

    // Size of header
    // 0x18 == size of COFF header, get16BitFromPE value == size of optional header
    fseek(pe, peData->peHeaderOffset + 0x14, SEEK_SET);
    peData->headerSize = (peData->peHeaderOffset + 1) + 0x18 + get16BitFromPE(pe);

    if(errno != SUCCESS)
    { return errno; }

    // PE characteristics
    fseek(pe, peData->peHeaderOffset + 0x16, SEEK_SET);
    peData->characteristics = get16BitFromPE(pe);

    if(errno != SUCCESS)
    { return errno; }

    // Entry point (RVA)
    fseek(pe, peData->peHeaderOffset + 0x28, SEEK_SET);
    peData->entryPoint = get32BitFromPE(pe);

    if(errno != SUCCESS)
    { return errno; }

    // Base address
    fseek(pe, peData->peHeaderOffset + 0x34, SEEK_SET);
    peData->baseAddr = get32BitFromPE(pe);

    if(errno != SUCCESS)
    { return errno; }

    // Page alignment/size
    fseek(pe, peData->peHeaderOffset + 0x38, SEEK_SET);
    peData->pageSize = get32BitFromPE(pe);

    if(errno != SUCCESS)
    { return errno; }

    // Export tables
    fseek(pe, peData->peHeaderOffset + 0x78, SEEK_SET);
    peData->peExportInfo.count = (get32BitFromPE(pe) == 0 ? 0 : 1); // TODO: Actually read the data

    if(errno != SUCCESS)
    { return errno; }

    // Import tables
    fseek(pe, peData->peHeaderOffset + 0x80, SEEK_SET);
    peData->peImportInfo.idtRVA = get32BitFromPE(pe);

    if(errno != SUCCESS)
    { return errno; }

    // TLS status (PE TLS is currently UNSUPPORTED, so if we find it, we'll need to abort)
    fseek(pe, peData->peHeaderOffset + 0xC0, SEEK_SET);
    peData->tlsAddr = get32BitFromPE(pe);

    if(errno != SUCCESS)
    { return errno; }

    peData->tlsSize = get32BitFromPE(pe);

    if(errno != SUCCESS)
    { return errno; }

    if(peData->tlsAddr != 0 || peData->tlsSize != 0)
    { return ERR_UNSUPPORTED_STRUCTURE; }

    // Section info
    int ret = getSectionInfo(pe, &(peData->sections));

    if(ret != 0)
    { return ret; }

    return SUCCESS;
}
