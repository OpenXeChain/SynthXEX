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

#include <getopt.h>
#include "common/common.h"
#include "common/datastorage.h"
#include "getdata/getdata.h"
#include "setdata/populateheaders.h"
#include "setdata/pagedescriptors.h"
#include "placer/placer.h"
#include "write/writexex.h"

void dispVer()
{
  printf("\nSynthXEX %s\n\nThe XEX builder of the FreeChainXenon project\n\n", VERSION);
  printf("Copyright (c) %s Aiden Isik\n\nThis program is free software: you can redistribute it and/or modify\n", COPYRIGHT);
  printf("it under the terms of the GNU Affero General Public License as published by\n");
  printf("the Free Software Foundation, either version 3 of the License, or\n");
  printf("(at your option) any later version.\n\nThis program is distributed in the hope that it will be useful,\n");
  printf("but WITHOUT ANY WARRANTY; without even the implied warranty of\nMERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n");
  printf("GNU Affero General Public License for more details.\n\n");
  printf("You should have received a copy of the GNU Affero General Public License\n");
  printf("along with this program.  If not, see <https://www.gnu.org/licenses/>.\n\n");
}

void dispHelp(char **argv)
{
  printf("\nUsage: %s [OPTION] <ARG>\n\n", argv[0]);
  printf("Options:\n");
  printf("-h,\t--help,\t\tShow this information\n");
  printf("-v,\t--version,\tShow version and copyright information\n");
  printf("-i,\t--input,\tSpecify input basefile path\n");
  printf("-o,\t--output,\tSpecify output xexfile path\n\n");
}

int main(int argc, char **argv)
{
  static struct option longOptions[] =
    {
      {"help", no_argument, 0, 0},
      {"version", no_argument, 0, 0},
      {"input", required_argument, 0, 0},
      {"output", required_argument, 0, 0},
      {0, 0, 0, 0}
    };

  int optIndex = 0;
  int option = 0;

  bool gotInput = false;
  bool gotOutput = false;
  char *basefilePath, *xexfilePath;
  
  while((option = getopt_long_only(argc, argv, "hvi:o:", longOptions, &optIndex)) != -1)
    {      
      if(option == 'h' || (option == 0 && strcmp(longOptions[optIndex].name, "help") == 0))
	{
	  dispHelp(argv);
	  return 0;
	}
      else if(option == 'v' || (option == 0 && strcmp(longOptions[optIndex].name, "version") == 0))
	{
	  dispVer();
	  return 0;
	}
      else if(option == 'i' || (option == 0 && strcmp(longOptions[optIndex].name, "input") == 0))
	{
	  gotInput = true;
	  basefilePath = malloc(strlen(optarg) + 1);
	  strncpy(basefilePath, optarg, strlen(optarg) + 1);
	}
      else if(option == 'o' || (option == 0 && strcmp(longOptions[optIndex].name, "output") == 0))
	{
	  gotOutput = true;
	  xexfilePath = malloc(strlen(optarg) + 1);
	  strncpy(xexfilePath, optarg, strlen(optarg) + 1);
	}
    }

  // Check we got everything we need
  if(!gotInput)
    {
      if(gotOutput)
	{
	  free(xexfilePath);
	}
      
      infoPrint("ERROR: Basefile input expected but not found. Aborting.");
      return -1;
    }
  else if(!gotOutput)
    {
      if(gotInput)
	{
	  free(basefilePath);
	}
      
      infoPrint("ERROR: Xexfile output expected but not found. Aborting.");
      return -1;
    }

  // Opening the files now that they've been validated
  FILE *pe = fopen(basefilePath, "r");
  FILE *xex = fopen(xexfilePath, "w+");

  free(basefilePath);
  free(xexfilePath);
  
  struct offsets offsets;
  struct xexHeader xexHeader;
  struct secInfoHeader secInfoHeader;
  struct peData peData;

  if(!validatePE(pe))
    {
      infoPrint("ERROR: Basefile is not Xbox 360 PE. Aborting.");
      fclose(pe);
      fclose(xex);
      return -1;
    }
  
  if(getHdrData(pe, &peData, 0) != 0)
    {
      infoPrint("ERROR: Unknown error in data retrieval from basefile. Aborting.");
      fclose(pe);
      fclose(xex);
      return -1;
    }

  // Setting final XEX data structs
  setXEXHeader(&xexHeader);
  setSecInfoHeader(&secInfoHeader, &peData, 0x2, 0x823DFC64); // TEMP IMPORT TABLE COUNT & EXPORT TABLE ADDR
  setPageDescriptors(pe, &peData, &secInfoHeader);

  // Setting data positions...
  placeStructs(&offsets, &xexHeader, &secInfoHeader);
  
  // Finally, write out all of the XEX data to file
  if(writeXEX(&xexHeader, &secInfoHeader, &offsets, xex) != 0)
    {
      infoPrint("ERROR: Unknown error in XEX write routine. Aborting.");
      fclose(pe);
      fclose(xex);
      return -1;
    }

  fclose(pe);
  fclose(xex);
  return 0;
}
