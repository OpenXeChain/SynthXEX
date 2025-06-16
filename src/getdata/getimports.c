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

#include "getimports.h"

int checkForBranchStub(FILE *pe, struct peData *peData)
{
    uint32_t expectedInstructions[] =
    {
        // lis r<x>, <upper half of addr>
        // Mask 0xFC000000 for instruction (we're just comparing in-place, no need for shifts)
        // Mask 0x03E00000 and shift right 21 bits for register 1
        // Mask 0x001F0000 and shift right 16 bits for register 2 (should be 0 for lis)
        // Mask 0x0000FFFF for value
        // https://www.ibm.com/docs/en/aix/7.2.0?topic=set-addis-cau-add-immediate-shifted-instruction
        0x3C000000,

        // lwz r<x>, <lower half of addr>(r<x>)
        // Mask 0xFC000000 for instruction
        // Mask 0x03E00000 and shift right 21 bits for register 1
        // Mask 0x001F0000 and shift right 16 bits for register 2
        // Mask 0x0000FFFF for value
        // https://www.ibm.com/docs/en/aix/7.2.0?topic=set-lwz-l-load-word-zero-instruction
        0x80000000,

        // mtctr r<x>
        // Mask 0xFC1FFFFF for instruction (mtspr CTR)
        // Mask 0x03E00000 and shift right 21 bits for register
        // https://www.ibm.com/docs/en/aix/7.2.0?topic=set-mtspr-move-special-purpose-register-instruction
        0x7C0903A6,

        // bctr
        // An assembler told me this, apparently IBM doesn't have documentation on their site for it
        0x4E800420
    };

    // Read in the set of instructions to check
    uint32_t *instructionBuffer = malloc(4 * sizeof(uint32_t));

    if (instructionBuffer == NULL)
    {
        return ERR_OUT_OF_MEM;
    }

    memset(instructionBuffer, 0, sizeof(uint32_t) * 4);

    if (fread(instructionBuffer, sizeof(uint32_t), 4, pe) < 4)
    {
        nullAndFree((void**)&instructionBuffer);
        return ERR_FILE_READ;
    }

#ifdef LITTLE_ENDIAN_SYSTEM

    // Byteswap the instructions
    for (uint8_t i = 0; i < 4; i++)
    {
        instructionBuffer[i] = __builtin_bswap32(instructionBuffer[i]);
    }

#endif

    // Check if each instruction matches
    uint8_t expectedReg = (instructionBuffer[0] & 0x03E00000) >> 21;

    if (((instructionBuffer[0] & 0xFC000000) == expectedInstructions[0]) &&
            (((instructionBuffer[0] & 0x001F0000) >> 16) == 0))
    {
        if (((instructionBuffer[1] & 0xFC000000) == expectedInstructions[1]) &&
                (((instructionBuffer[1] & 0x03E00000) >> 21) == expectedReg) &&
                (((instructionBuffer[1] & 0x001F0000) >> 16) == expectedReg))
        {
            if (((instructionBuffer[2] & 0xFC1FFFFF) == expectedInstructions[2]) &&
                    (((instructionBuffer[2] & 0x03E00000) >> 21) == expectedReg))
            {
                if (instructionBuffer[3] == expectedInstructions[3])
                {
                    uint32_t currentLoadAddr = ((instructionBuffer[0] & 0x0000FFFF) << 16) |
                                               (instructionBuffer[1] & 0x0000FFFF);

                    for (uint32_t i = 0; i < peData->peImportInfo.tableCount; i++)
                    {
                        if (peData->peImportInfo.tables[i].importCount ==
                                peData->peImportInfo.tables[i].branchStubCount)
                        {
                            continue;
                        }

                        for (uint32_t j = 0; j < peData->peImportInfo.tables[i].importCount; j++)
                        {
                            if (peData->peImportInfo.tables[i].imports[j].branchStubAddr != 0)
                            {
                                continue;
                            }

                            if (peData->peImportInfo.tables[i].imports[j].iatAddr == currentLoadAddr)
                            {
                                uint32_t currentBranchStubRVA = offsetToRVA(ftell(pe) - 16, &(peData->sections));

                                if (currentBranchStubRVA == 0)
                                {
                                    nullAndFree((void**)&instructionBuffer);
                                    return ERR_INVALID_RVA_OR_OFFSET;
                                }

                                peData->peImportInfo.tables[i].imports[j].branchStubAddr =
                                    peData->baseAddr + currentBranchStubRVA;
                                peData->peImportInfo.tables[i].branchStubCount++;
                                peData->peImportInfo.totalBranchStubCount++;

                                nullAndFree((void**)&instructionBuffer);
                                return SUCCESS;
                            }
                        }
                    }
                }
            }
        }
    }

    nullAndFree((void**)&instructionBuffer);
    return fseek(pe, -12, SEEK_CUR) != 0 ? ERR_FILE_READ : SUCCESS;
}

int getImports(FILE *pe, struct peData *peData)
{
    // Make sure the peImportInfo struct is blank, except the IDT RVA
    memset(&(peData->peImportInfo.tableCount), 0, sizeof(struct peImportInfo) - sizeof(uint32_t));

    // If there is no IDT, just skip handling imports
    if (peData->peImportInfo.idtRVA == 0)
    {
        return SUCCESS;
    }

    // Seek to the IDT and read the first entry
    uint32_t idtOffset = rvaToOffset(peData->peImportInfo.idtRVA, &(peData->sections));

    if (idtOffset == 0)
    {
        return ERR_INVALID_RVA_OR_OFFSET;
    }

    if (fseek(pe, idtOffset, SEEK_SET) != 0)
    {
        return ERR_FILE_READ;
    }

    uint32_t *currentIDT = malloc(5 * sizeof(uint32_t));

    if (currentIDT == NULL)
    {
        return ERR_OUT_OF_MEM;
    }

    uint32_t *blankIDT = calloc(5, sizeof(uint32_t));  // Blank IDT for comparisons

    if (blankIDT == NULL)
    {
        nullAndFree((void**)&currentIDT);
        return ERR_OUT_OF_MEM;
    }

    if (fread(currentIDT, sizeof(uint32_t), 5, pe) < 5)
    {
        nullAndFree((void**)&currentIDT);
        nullAndFree((void**)&blankIDT);
        return ERR_FILE_READ;
    }

    // While the IDT is not blank, process it
    for (uint32_t i = 0; memcmp(currentIDT, blankIDT, 5 * sizeof(uint32_t)) != 0; i++)
    {
        // Allocate space for the current table data
        peData->peImportInfo.tableCount++;
        peData->peImportInfo.tables = realloc(peData->peImportInfo.tables, (i + 1) * sizeof(struct peImportTable));

        if (peData->peImportInfo.tables == NULL)
        {
            nullAndFree((void**)&currentIDT);
            nullAndFree((void**)&blankIDT);
            return ERR_OUT_OF_MEM;
        }

        memset(&(peData->peImportInfo.tables[i]), 0, sizeof(struct peImportTable));  // Make sure it's blank

#ifdef BIG_ENDIAN_SYSTEM

        // Byteswap the IDT fields
        for (uint8_t j = 0; j < 5; j++)
        {
            currentIDT[j] = __builtin_bswap32(currentIDT[j]);
        }

#endif

        // Retrieve the name pointed to by the table
        uint32_t savedOffset = ftell(pe);
        uint32_t tableNameOffset = rvaToOffset(currentIDT[3], &(peData->sections));

        if (tableNameOffset == 0)
        {
            nullAndFree((void**)&currentIDT);
            nullAndFree((void**)&blankIDT);
            return ERR_INVALID_RVA_OR_OFFSET;
        }

        if (fseek(pe, tableNameOffset, SEEK_SET) != 0)
        {
            nullAndFree((void**)&currentIDT);
            nullAndFree((void**)&blankIDT);
            return ERR_FILE_READ;
        }

        // Allocating is expensive, go 16 bytes at a time to avoid excessive realloc calls
        for (uint32_t j = 0;; j += 16)
        {
            peData->peImportInfo.tables[i].name = realloc(peData->peImportInfo.tables[i].name, (j + 16) * sizeof(char));

            if (peData->peImportInfo.tables[i].name == NULL)
            {
                nullAndFree((void**)&currentIDT);
                nullAndFree((void**)&blankIDT);
                return ERR_OUT_OF_MEM;
            }

            if (fread(peData->peImportInfo.tables[i].name + j, sizeof(char), 16, pe) < 16)
            {
                nullAndFree((void**)&currentIDT);
                nullAndFree((void**)&blankIDT);
                return ERR_FILE_READ;
            }

            // Check for null terminator
            for (uint32_t k = j; k < j + 16; k++)
                if (peData->peImportInfo.tables[i].name[k] == '\0')
                {
                    goto doneGettingTableName;
                }
        }

    doneGettingTableName:
        // Seek to the IAT and read the first entry
        uint32_t iatOffset = rvaToOffset(currentIDT[4], &(peData->sections));

        if (iatOffset == 0)
        {
            nullAndFree((void**)&currentIDT);
            nullAndFree((void**)&blankIDT);
            return ERR_INVALID_RVA_OR_OFFSET;
        }

        if (fseek(pe, iatOffset, SEEK_SET) != 0)
        {
            nullAndFree((void**)&currentIDT);
            nullAndFree((void**)&blankIDT);
            return ERR_FILE_READ;
        }

        uint32_t currentImport;

        if (fread(&currentImport, sizeof(uint32_t), 1, pe) < 1)
        {
            nullAndFree((void**)&currentIDT);
            nullAndFree((void**)&blankIDT);
            return ERR_FILE_READ;
        }

        // While the import is not blank, process it
        for (int j = 0; currentImport != 0; j++)
        {
#ifdef BIG_ENDIAN_SYSTEM
            // Byteswap the import
            currentImport = __builtin_bswap32(currentImport);
#endif
            // Allocate space for the current import
            peData->peImportInfo.tables[i].importCount++;
            peData->peImportInfo.tables[i].imports = realloc(peData->peImportInfo.tables[i].imports, (j + 1) * sizeof(struct peImport));
            peData->peImportInfo.tables[i].imports[j].branchStubAddr = 0;  // Zero it for later

            // Store the address of the current import entry in iatAddr
            uint32_t currentImportRVA = offsetToRVA(ftell(pe) - 4, &(peData->sections));

            if (currentImportRVA == 0)
            {
                nullAndFree((void**)&currentIDT);
                nullAndFree((void**)&blankIDT);
                return ERR_INVALID_RVA_OR_OFFSET;
            }

            peData->peImportInfo.tables[i].imports[j].iatAddr = peData->baseAddr + currentImportRVA;

            // Read the next import
            if (fread(&currentImport, sizeof(uint32_t), 1, pe) < 1)
            {
                nullAndFree((void**)&currentIDT);
                nullAndFree((void**)&blankIDT);
                return ERR_FILE_READ;
            }
        }

        // Add table's import count to total and return to next IDT entry
        peData->peImportInfo.totalImportCount += peData->peImportInfo.tables[i].importCount;

        if (fseek(pe, savedOffset, SEEK_SET) != 0)
        {
            nullAndFree((void**)&currentIDT);
            nullAndFree((void**)&blankIDT);
            return ERR_FILE_READ;
        }

        // Read next IDT
        if (fread(currentIDT, sizeof(uint32_t), 5, pe) < 5)
        {
            return ERR_FILE_READ;
        }
    }

    nullAndFree((void**)&currentIDT);
    nullAndFree((void**)&blankIDT);

    // Find the branch stubs
    for (uint16_t i = 0; i < peData->sections.count; i++)
    {
        if (peData->peImportInfo.totalBranchStubCount == peData->peImportInfo.totalImportCount)
        {
            break;    // All branch stubs found
        }

        if (!(peData->sections.section[i].permFlag & XEX_SECTION_CODE))
        {
            continue;    // Skip non-executable section
        }

        // Seek to the start of the current section
        if (fseek(pe, peData->sections.section[i].offset, SEEK_SET) != 0)
        {
            return ERR_FILE_READ;
        }

        // While inside section and at least 4 instructions left (15 bytes)
        while (ftell(pe) < (peData->sections.section[i].offset + peData->sections.section[i].rawSize) - 15)
        {
            int ret = checkForBranchStub(pe, peData);

            if (ret != SUCCESS)
            {
                return ret;
            }
        }
    }

    return SUCCESS;
}

