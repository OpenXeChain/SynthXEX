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

// Strips the ordinal flags from IAT entries, and swaps them to big endian
// Also adds module indexes to them
int xenonifyIAT(FILE *basefile, struct peData *peData)
{
    // Loop through each import table and handle their IAT entries
    for(uint32_t i = 0; i < peData->peImportInfo.tableCount; i++)
    {
        // Seek to the first IAT entry for this table
        if(fseek(basefile, peData->peImportInfo.tables[i].rva, SEEK_SET) != 0)
        { return ERR_FILE_READ; }

        // Loop through each import and handle it's IAT entry
        for(uint32_t j = 0; j < peData->peImportInfo.tables[i].importCount; j++)
        {
            uint32_t iatEntry = get32BitFromPE(basefile);

            if(errno != SUCCESS)
            { return ERR_FILE_READ; }

            // Seek back so we can write back to the same place
            if(fseek(basefile, -0x4, SEEK_CUR) != 0)
            { return ERR_FILE_READ; }

            // Xenonify the IAT entry
            iatEntry &= ~PE_IMPORT_ORDINAL_FLAG; // Strip the import by ordinal flag
            iatEntry |= (i & 0x000000FF) << 16; // Add the module index

            // Write back out as big endian (TODO: make a utility function for this like get32BitFromPE)
#ifdef LITTLE_ENDIAN_SYSTEM
            iatEntry = __builtin_bswap32(iatEntry);
#endif

            if(fwrite(&iatEntry, sizeof(uint32_t), 1, basefile) < 1)
            { return ERR_FILE_WRITE; }
        }
    }

    return SUCCESS;
}

// Maps the PE file into the basefile (RVAs become offsets)
int mapPEToBasefile(FILE *pe, FILE *basefile, struct peData *peData)
{
    // Retrieve the section info from PE (TODO: use the data we get from getHdrData for this, right now we're duplicating work)
    struct sectionInfo *sectionInfo = malloc(peData->numberOfSections *sizeof(struct sectionInfo));

    if(!sectionInfo)
    { return ERR_OUT_OF_MEM; }

    // Seek to the first section in the section table at virtualSize
    if(fseek(pe, (peData->headerSize - 1) + 0x8, SEEK_SET) != 0)
    { return ERR_FILE_READ; }

    for(uint16_t i = 0; i < peData->numberOfSections; i++)
    {
        sectionInfo[i].virtualSize = get32BitFromPE(pe);

        if(errno != SUCCESS)
        { return errno; }

        sectionInfo[i].rva = get32BitFromPE(pe);

        if(errno != SUCCESS)
        { return errno; }

        sectionInfo[i].rawSize = get32BitFromPE(pe);

        if(errno != SUCCESS)
        { return errno; }

        sectionInfo[i].offset = get32BitFromPE(pe);

        if(errno != SUCCESS)
        { return errno; }

        // Seek to the next entry at virtualSize
        if(fseek(pe, 0x18, SEEK_CUR) != 0)
        { return ERR_FILE_READ; }
    }

    if(fseek(pe, 0, SEEK_SET) != 0)
    { return ERR_FILE_READ; }

    // Copy the PE header and section table to the basefile verbatim
    uint8_t *buffer = malloc(peData->headerSize + peData->sectionTableSize);

    if(!buffer)
    { return ERR_OUT_OF_MEM; }

    size_t totalHeader = peData->headerSize + peData->sectionTableSize;

    if(fread(buffer, 1, totalHeader, pe) != totalHeader)
    { return ERR_FILE_READ; }

    if(fwrite(buffer, 1, totalHeader, basefile) != totalHeader)
    { return ERR_FILE_READ; }

    // Now map the sections and write them
    for(uint16_t i = 0; i < peData->numberOfSections; i++)
    {
        buffer = realloc(buffer, sectionInfo[i].rawSize);

        if(!buffer)
        { return ERR_OUT_OF_MEM; }

        if(fseek(pe, sectionInfo[i].offset, SEEK_SET) != 0)
        { return ERR_FILE_READ; }

        if(fread(buffer, 1, sectionInfo[i].rawSize, pe) != sectionInfo[i].rawSize)
        { return ERR_FILE_READ; }

        if(fseek(basefile, sectionInfo[i].rva, SEEK_SET) != 0)
        { return ERR_FILE_READ; }

        if(fwrite(buffer, 1, sectionInfo[i].rawSize, basefile) != sectionInfo[i].rawSize)
        { return ERR_FILE_READ; }
    }

    // Pad the rest of the final page with zeroes, we can achieve this by seeking
    // to the end and placing a single zero there (unless the data runs all the way up to the end!)
    uint32_t currentOffset = ftell(basefile);
    uint32_t nextAligned = getNextAligned(currentOffset, peData->pageSize) - 1;

    if(nextAligned != currentOffset)
    {
        buffer = realloc(buffer, 1);

        if(!buffer)
        { return ERR_OUT_OF_MEM; }

        buffer[0] = 0;

        if(fseek(basefile, nextAligned, SEEK_SET) != 0)
        { return ERR_FILE_READ; }

        if(fwrite(buffer, 1, 1, basefile) != 1)
        { return ERR_FILE_READ; }
    }

    // Make sure to update the PE (basefile) size
    peData->size = ftell(basefile);

    // We're done with these now, free them
    nullAndFree((void **)&buffer);
    nullAndFree((void **)&sectionInfo);

    // While we're writing the basefile, let's do the required modifications to the IAT.
    // We can skip the return check because this is the last function call.
    // The outcome of this is the outcome of the whole function.
    return xenonifyIAT(basefile, peData);
}
