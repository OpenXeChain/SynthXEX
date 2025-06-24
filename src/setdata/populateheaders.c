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

#include "populateheaders.h"

int setXEXHeader(struct xexHeader *xexHeader, struct optHeaderEntries *optHeaderEntries, struct peData *peData)
{
    // Writing data into XEX header.
    strncpy(xexHeader->magic, "XEX2", sizeof(char) * 4); // Magic

    // Module flags (type of executable)
    // Not sure if this is correct, for DLLs specifically the type overrides should probably be used instead
    if(xexHeader->moduleFlags == 0)
    {
        if(peData->characteristics & PE_CHAR_FLAG_DLL)
        {
            xexHeader->moduleFlags |= XEX_MOD_FLAG_DLL;    // The executable is a DLL
        }
        else
        {
            xexHeader->moduleFlags |= XEX_MOD_FLAG_TITLE;    // The executable is a regular title
        }

        if(peData->peExportInfo.count > 0)
        {
            xexHeader->moduleFlags |= XEX_MOD_FLAG_EXPORTS;    // The executable exports functions
        }
    }

    xexHeader->optHeaderCount = optHeaderEntries->count;

    return SUCCESS;
}

int setSecInfoHeader(struct secInfoHeader *secInfoHeader, struct peData *peData)
{
    // Writing data into security info header (much of this is derived from info in PE)
    secInfoHeader->peSize = peData->size;

    // Setting signature (just a SynthXEX version identifier)
    strcpy(secInfoHeader->signature, SYNTHXEX_VERSION_STRING);

    secInfoHeader->imageInfoSize = 0x174; // Image info size is always 0x174
    secInfoHeader->imageFlags = (peData->pageSize == 0x1000 ? XEX_IMG_FLAG_4KIB_PAGES : 0) | XEX_IMG_FLAG_REGION_FREE; // If page size is 4KiB (small pages), set that flag
    secInfoHeader->baseAddr = peData->baseAddr;
    //memset(secInfoHeader->mediaID, 0, sizeof(secInfoHeader->mediaID)); // Null media ID (no longer need to use memset, as we use calloc for these structs now)
    //memset(secInfoHeader->aesKey, 0, sizeof(secInfoHeader->aesKey)); // No encryption, null AES key
    //secInfoHeader->exportTableAddr = TEMPEXPORTADDR;
    secInfoHeader->exportTableAddr = 0;
    secInfoHeader->gameRegion = XEX_REG_FLAG_REGION_FREE;
    secInfoHeader->mediaTypes = 0xFFFFFFFF; // All flags set, can load from any type.
    secInfoHeader->pageDescCount = secInfoHeader->peSize / peData->pageSize; // Number of page descriptors following security info (same number of pages)
    secInfoHeader->headerSize = (secInfoHeader->pageDescCount * sizeof(struct pageDescriptor)) + (sizeof(struct secInfoHeader) - sizeof(void *)); // Page descriptor total size + length of rest of secinfo header (subtraction of sizeof void* is to remove pointer at end of struct from calculation)

    return SUCCESS;
}
