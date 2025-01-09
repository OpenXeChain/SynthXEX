// This file is part of SynthXEX, one component of the
// FreeChainXenon development toolchain
//
// Copyright (c) 2024 Aiden Isik
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
  // TODO: Dynamically select optional headers to use, instead of hard-coding
  optHeaderEntries->count = 4;
  optHeaderEntries->optHeaderEntry = calloc(5, sizeof(struct optHeaderEntry));
  if(optHeaderEntries->optHeaderEntry == NULL) {return ERR_OUT_OF_MEM;}
  
  // First optional header (basefile format)
  setBasefileFormat(&(optHeaders->basefileFormat), secInfoHeader);
  optHeaderEntries->optHeaderEntry[0].id = XEX_OPT_ID_BASEFILE_FORMAT;

  // Second optional header (entrypoint)
  optHeaderEntries->optHeaderEntry[1].id = XEX_OPT_ID_ENTRYPOINT;
  optHeaderEntries->optHeaderEntry[1].dataOrOffset = secInfoHeader->baseAddr + peData->entryPoint;

  // Third optional header (import libs)
  //optHeaderEntries->optHeaderEntry[2].id = XEX_OPT_ID_IMPORT_LIBS;

  // Fourth optional header (tls info)
  optHeaderEntries->optHeaderEntry[2].id = XEX_OPT_ID_TLS_INFO;
  setTLSInfo(&(optHeaders->tlsInfo));

  // Fifth optional header (system flags)
  optHeaderEntries->optHeaderEntry[3].id = XEX_OPT_ID_SYS_FLAGS;
  setSysFlags(&(optHeaderEntries->optHeaderEntry[3].dataOrOffset));
}
