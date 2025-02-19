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

#include "elimzeroes.h"

// Specify which parts of the basefile are just zeroes, to be eliminated.
// This is somewhat inefficient and definitely an abuse of the read syscall,
// so if anyone has a better implementation, feel free to contribute.
int eliminateZeroes(FILE *basefile, struct basefileFormat *basefileFormat)
{
  bool continuingPrevLoop = false;
  
  fseek(basefile, 0, SEEK_SET);

  basefileFormat->zeroEliminations = malloc(1 * sizeof(struct zeroEliminations)); // free'd in writeXEX
  if(basefileFormat->zeroEliminations == NULL) {return ERR_OUT_OF_MEM;}
  
  for(int i = 0;; i++)
    {
      uint8_t nextByte = 1;
      basefileFormat->zeroEliminations = realloc(basefileFormat->zeroEliminations, (i + 1) * sizeof(struct zeroEliminations));
      if(basefileFormat->zeroEliminations == NULL) {return ERR_OUT_OF_MEM;}

      // If we've not already cleared the data for use in this set of fields, do so
      // Also, update the size for the basefile format and the number of sets of fields
      if(!continuingPrevLoop)
	{
	  basefileFormat->size += 0x8;
	  basefileFormat->zeroElimCount += 1;
	  basefileFormat->zeroEliminations[i].dataSize = 0;
	  basefileFormat->zeroEliminations[i].zeroSize = 0;
	}

      continuingPrevLoop = false;

      // Data size field
      while(nextByte != 0)
	{
	  if(fread(&nextByte, sizeof(uint8_t), 1, basefile) < 1)
	    {
	      return SUCCESS;
	    }

	  if(nextByte != 0)
	    {
	      basefileFormat->zeroEliminations[i].dataSize++;
	    }
	}

      fseek(basefile, -1, SEEK_CUR);
      
      // Zero size field
      while(nextByte == 0)
	{
	  if(fread(&nextByte, sizeof(uint8_t), 1, basefile) < 1)
	    {
	      return SUCCESS;
	    }

	  if(nextByte == 0)
	    {
	      basefileFormat->zeroEliminations[i].zeroSize++;
	    }
	}

      fseek(basefile, -1, SEEK_CUR);

      // Deciding whether to eliminate zeroes or not (is there less than 8 bytes worth of them?)
      if(basefileFormat->zeroEliminations[i].zeroSize <= 8) // If we don't have enough zeroes, continue in the current set of fields
	{
	  basefileFormat->zeroEliminations[i].dataSize += basefileFormat->zeroEliminations[i].zeroSize;
	  basefileFormat->zeroEliminations[i].zeroSize = 0;
	  i--;
	  continuingPrevLoop = true;
	  continue;
	}
    }
}
