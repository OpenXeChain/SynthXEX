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

#include "pagedescriptors.h"

uint8_t getRwx(struct secInfoHeader *secInfoHeader, struct peData *peData, uint32_t page)
{
    uint32_t pageSize = secInfoHeader->peSize / secInfoHeader->pageDescCount;
    uint32_t currentOffset = page * pageSize;

    for(int32_t i = peData->sections.count - 1; i >= 0; i--)
        if(currentOffset >= peData->sections.section[i].rva)
        { return peData->sections.section[i].permFlag; }

    return XEX_SECTION_RODATA | 0b10000; // We're in the PE header, so RODATA
}

int setPageDescriptors(FILE *pe, struct peData *peData, struct secInfoHeader *secInfoHeader)
{
    uint32_t pageSize = secInfoHeader->peSize / secInfoHeader->pageDescCount;

    secInfoHeader->descriptors = calloc(secInfoHeader->pageDescCount, sizeof(struct pageDescriptor));

    if(!secInfoHeader->descriptors)
    { return ERR_OUT_OF_MEM; }

    struct pageDescriptor *descriptors = secInfoHeader->descriptors;

    for(int64_t i = secInfoHeader->pageDescCount - 1; i >= 0; i--)
    {
        descriptors[i].sizeAndInfo = getRwx(secInfoHeader, peData, i);

        struct sha1_ctx shaContext;
        sha1_init(&shaContext);

        if(fseek(pe, i * pageSize, SEEK_SET) != 0)
        { return ERR_FILE_READ; }

        uint8_t *page = malloc(pageSize);

        if(!page)
        { return ERR_OUT_OF_MEM; }

        if(fread(page, 1, pageSize, pe) != pageSize)
        {
            nullAndFree((void **)&page);
            return ERR_FILE_READ;
        }

#ifdef LITTLE_ENDIAN_SYSTEM
        descriptors[i].sizeAndInfo = __builtin_bswap32(descriptors[i].sizeAndInfo);
#endif

        sha1_update(&shaContext, pageSize, page);
        sha1_update(&shaContext, 0x18, (uint8_t *)&descriptors[i]);

#ifdef LITTLE_ENDIAN_SYSTEM
        descriptors[i].sizeAndInfo = __builtin_bswap32(descriptors[i].sizeAndInfo);
#endif

        if(i != 0)
        { sha1_digest(&shaContext, 0x14, descriptors[i - 1].sha1); }
        else
        { sha1_digest(&shaContext, 0x14, secInfoHeader->imageSha1); }

        nullAndFree((void **)&page);
    }

    return SUCCESS;
}
