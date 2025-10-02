// This file is part of SynthXEX, one component of the
// OpenXeChain development toolchain
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

#include "getimports.h"

int getImports(FILE *pe, struct peData *peData)
{
    // Make sure the peImportInfo struct is blank, except the IDT RVA
    memset(&(peData->peImportInfo.tableCount), 0, sizeof(struct peImportInfo) - sizeof(uint32_t));

    // If there is no IDT, just skip handling imports
    if(peData->peImportInfo.idtRVA == 0)
    { return SUCCESS; }

    // Seek to the IDT and read the first entry
    uint32_t idtOffset = rvaToOffset(peData->peImportInfo.idtRVA, &(peData->sections));

    if(idtOffset == 0)
    { return ERR_INVALID_RVA_OR_OFFSET; }

    if(fseek(pe, idtOffset, SEEK_SET) != 0)
    { return ERR_FILE_READ; }

    uint32_t *currentIDT = malloc(5 * sizeof(uint32_t));

    if(currentIDT == NULL)
    { return ERR_OUT_OF_MEM; }

    uint32_t *blankIDT = calloc(5, sizeof(uint32_t)); // Blank IDT for comparisons

    if(blankIDT == NULL)
    {
        nullAndFree((void **)&currentIDT);
        return ERR_OUT_OF_MEM;
    }

    if(fread(currentIDT, sizeof(uint32_t), 5, pe) < 5)
    {
        nullAndFree((void **)&currentIDT);
        nullAndFree((void **)&blankIDT);
        return ERR_FILE_READ;
    }

    // While the IDT is not blank, process it
    for(uint32_t i = 0; memcmp(currentIDT, blankIDT, 5 * sizeof(uint32_t)) != 0; i++)
    {
        // Allocate space for the current table data
        peData->peImportInfo.tableCount++;
        peData->peImportInfo.tables = realloc(peData->peImportInfo.tables, (i + 1) * sizeof(struct peImportTable));

        if(peData->peImportInfo.tables == NULL)
        {
            nullAndFree((void **)&currentIDT);
            nullAndFree((void **)&blankIDT);
            return ERR_OUT_OF_MEM;
        }

        memset(&(peData->peImportInfo.tables[i]), 0, sizeof(struct peImportTable)); // Make sure it's blank

#ifdef BIG_ENDIAN_SYSTEM

        // Byteswap the IDT fields
        for(uint8_t j = 0; j < 5; j++)
        { currentIDT[j] = __builtin_bswap32(currentIDT[j]); }

#endif

        // Retrieve the name pointed to by the table
        uint32_t savedOffset = ftell(pe);
        uint32_t tableNameOffset = rvaToOffset(currentIDT[3], &(peData->sections));

        if(tableNameOffset == 0)
        {
            nullAndFree((void **)&currentIDT);
            nullAndFree((void **)&blankIDT);
            return ERR_INVALID_RVA_OR_OFFSET;
        }

        if(fseek(pe, tableNameOffset, SEEK_SET) != 0)
        {
            nullAndFree((void **)&currentIDT);
            nullAndFree((void **)&blankIDT);
            return ERR_FILE_READ;
        }

        // Allocating is expensive, go 16 bytes at a time to avoid excessive realloc calls
        for(uint32_t j = 0;; j += 16)
        {
            peData->peImportInfo.tables[i].name = realloc(peData->peImportInfo.tables[i].name, (j + 16) * sizeof(char));

            if(peData->peImportInfo.tables[i].name == NULL)
            {
                nullAndFree((void **)&currentIDT);
                nullAndFree((void **)&blankIDT);
                return ERR_OUT_OF_MEM;
            }

            if(fread(peData->peImportInfo.tables[i].name + j, sizeof(char), 16, pe) < 16)
            {
                nullAndFree((void **)&currentIDT);
                nullAndFree((void **)&blankIDT);
                return ERR_FILE_READ;
            }

            // Check for null terminator
            for(uint32_t k = j; k < j + 16; k++)
                if(peData->peImportInfo.tables[i].name[k] == '\0')
                { goto doneGettingTableName; }
        }

    doneGettingTableName:
        // Store the IAT RVA for this table
        peData->peImportInfo.tables[i].rva = currentIDT[4];

        // Seek to the IAT and read the first entry
        uint32_t iatOffset = rvaToOffset(currentIDT[4], &(peData->sections));

        if(iatOffset == 0)
        {
            nullAndFree((void **)&currentIDT);
            nullAndFree((void **)&blankIDT);
            return ERR_INVALID_RVA_OR_OFFSET;
        }

        if(fseek(pe, iatOffset, SEEK_SET) != 0)
        {
            nullAndFree((void **)&currentIDT);
            nullAndFree((void **)&blankIDT);
            return ERR_FILE_READ;
        }

        uint32_t currentImport;

        if(fread(&currentImport, sizeof(uint32_t), 1, pe) < 1)
        {
            nullAndFree((void **)&currentIDT);
            nullAndFree((void **)&blankIDT);
            return ERR_FILE_READ;
        }

        // While the import is not blank, process it
        for(int j = 0; currentImport != 0; j++)
        {
#ifdef BIG_ENDIAN_SYSTEM
            // Byteswap the import
            currentImport = __builtin_bswap32(currentImport);
#endif
            // Allocate space for the current import
            peData->peImportInfo.tables[i].importCount++;
            peData->peImportInfo.tables[i].imports = realloc(peData->peImportInfo.tables[i].imports, (j + 1) * sizeof(struct peImport));

            // Store the address of the current import entry in iatAddr
            uint32_t currentImportRVA = offsetToRVA(ftell(pe) - 4, &(peData->sections));

            if(currentImportRVA == 0)
            {
                nullAndFree((void **)&currentIDT);
                nullAndFree((void **)&blankIDT);
                return ERR_INVALID_RVA_OR_OFFSET;
            }

            peData->peImportInfo.tables[i].imports[j].iatAddr = peData->baseAddr + currentImportRVA;

            // Read the next import
            if(fread(&currentImport, sizeof(uint32_t), 1, pe) < 1)
            {
                nullAndFree((void **)&currentIDT);
                nullAndFree((void **)&blankIDT);
                return ERR_FILE_READ;
            }
        }

        // Add table's import count to total and return to next IDT entry
        peData->peImportInfo.totalImportCount += peData->peImportInfo.tables[i].importCount;

        if(fseek(pe, savedOffset, SEEK_SET) != 0)
        {
            nullAndFree((void **)&currentIDT);
            nullAndFree((void **)&blankIDT);
            return ERR_FILE_READ;
        }

        // Read next IDT
        if(fread(currentIDT, sizeof(uint32_t), 5, pe) < 5)
        { return ERR_FILE_READ; }
    }

    nullAndFree((void **)&currentIDT);
    nullAndFree((void **)&blankIDT);
    return SUCCESS;
}
