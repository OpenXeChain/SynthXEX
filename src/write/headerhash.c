// This file is part of SynthXEX, one component of the
// OpenXeChain development toolchain
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

// So, it would probably be more sensible to endian-swap all of the data back and determine which structure is where
// to determine the hash, but reading the file we just created is easier.
int setHeaderSha1(FILE *xex)
{
    // Get basefile offset
    if(fseek(xex, 0x8, SEEK_SET) != 0)
    { return ERR_FILE_READ; }

    uint32_t basefileOffset = get32BitFromXEX(xex);

    if(errno != SUCCESS)
    { return errno; }

    // Get secinfo offset
    if(fseek(xex, 0x10, SEEK_SET) != 0)
    { return ERR_FILE_READ; }

    uint32_t secInfoOffset = get32BitFromXEX(xex);

    if(errno != SUCCESS)
    { return errno; }

    uint32_t endOfImageInfo = secInfoOffset + 0x8 + 0x174; // 0x8 == image info offset in security info, 0x174 == length of that
    uint32_t remainingSize = basefileOffset - endOfImageInfo; // How much data is between end of image info and basefile (we hash that too)

    // Init sha1 hash
    struct sha1_ctx shaContext;
    memset(&shaContext, 0, sizeof(shaContext));
    sha1_init(&shaContext);

    // Hash first part (remainder of headers is done first, then the start)
    uint8_t *remainderOfHeaders = malloc(remainingSize);

    if(!remainderOfHeaders)
    { return ERR_OUT_OF_MEM; }

    memset(remainderOfHeaders, 0, remainingSize);

    if(fseek(xex, endOfImageInfo, SEEK_SET) != 0)
    {
        nullAndFree((void **)&remainderOfHeaders);
        return ERR_FILE_READ;
    }

    if(fread(remainderOfHeaders, 1, remainingSize, xex) != remainingSize)
    {
        nullAndFree((void **)&remainderOfHeaders);
        return ERR_FILE_READ;
    }

    sha1_update(&shaContext, remainingSize, remainderOfHeaders);
    nullAndFree((void **)&remainderOfHeaders);

    // Hash from start up to image info (0x8 into security header)
    uint32_t headersLen = secInfoOffset + 0x8;
    uint8_t *headersStart = malloc(headersLen);

    if(!headersStart)
    { return ERR_OUT_OF_MEM; }

    memset(headersStart, 0, headersLen);

    if(fseek(xex, 0, SEEK_SET) != 0)
    {
        nullAndFree((void **)&headersStart);
        return ERR_FILE_READ;
    }

    if(fread(headersStart, 1, headersLen, xex) != headersLen)
    {
        nullAndFree((void **)&headersStart);
        return ERR_FILE_READ;
    }

    sha1_update(&shaContext, headersLen, headersStart);
    nullAndFree((void **)&headersStart);

    // Get final hash
    uint8_t headerHash[20];
    memset(headerHash, 0, sizeof(headerHash));
    sha1_digest(&shaContext, 20, headerHash);

    // Finally, write it out
    if(fseek(xex, secInfoOffset + 0x164, SEEK_SET) != 0) // 0x164 == offset in secinfo of header hash
    { return ERR_FILE_READ; }

    if(fwrite(headerHash, 1, 20, xex) != 20)
    { return ERR_FILE_READ; }

    return SUCCESS;
}
