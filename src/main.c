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

#include <getopt.h>
#include "common/common.h"
#include "common/datastorage.h"
#include "pemapper/pemapper.h"
#include "getdata/getdata.h"
#include "setdata/populateheaders.h"
#include "setdata/pagedescriptors.h"
#include "setdata/optheaders.h"
#include "placer/placer.h"
#include "write/writexex.h"
#include "write/headerhash.h"

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
  printf("-i,\t--input,\tSpecify input PE file path\n");
  printf("-o,\t--output,\tSpecify output XEX file path\n\n");
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

  char *pePath;
  char *xexfilePath;
  
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
	  pePath = malloc(strlen(optarg) + 1);
	  strncpy(pePath, optarg, strlen(optarg) + 1);
	}
      else if(option == 'o' || (option == 0 && strcmp(longOptions[optIndex].name, "output") == 0))
	{
	  gotOutput = true;
	  xexfilePath = malloc(strlen(optarg) + 1);
	  strncpy(xexfilePath, optarg, strlen(optarg) + 1);
	}
    }

  printf("%s This is SynthXEX, %s. Copyright (c) %s Aiden Isik.\n", PRINT_STEM, VERSION, COPYRIGHT);
  printf("%s This program is free/libre software. Run \"%s --version\" for license info.\n\n", PRINT_STEM, argv[0]);
  
  // Check we got everything we need
  if(!gotInput)
    {
      if(gotOutput)
	{
	  free(xexfilePath);
	}
      
      printf("%s ERROR: PE input expected but not found. Aborting.\n", PRINT_STEM);
      return -1;
    }
  else if(!gotOutput)
    {
      if(gotInput)
	{
	  free(pePath);
	}

      printf("%s ERROR: XEX file output expected but not found. Aborting.\n", PRINT_STEM);
      return -1;
    }

  // Opening the files now that they've been validated
  FILE *pe = fopen(pePath, "rb");
  FILE *xex = fopen(xexfilePath, "wb+");

  free(pePath);

  // Don't free this yet. We need it to determine where we can put the mapped basefile.
  // There *are* ways to get the file path from file pointer, but none of them are portable.
  //free(xexfilePath);
  
  struct offsets offsets;
  struct xexHeader xexHeader;
  struct secInfoHeader secInfoHeader;
  struct peData peData;
  struct optHeaderEntries optHeaderEntries;
  struct optHeaders optHeaders;

  printf("%s Validating PE file...\n", PRINT_STEM);
  
  if(!validatePE(pe))
    {
      printf("%s ERROR: Input PE is not Xbox 360 PE. Aborting.\n", PRINT_STEM);
      fclose(pe);
      fclose(xex);
      return -1;
    }

  printf("%s PE valid!\n", PRINT_STEM);
  printf("%s Creating basefile from PE...\n", PRINT_STEM);
  
  // Determine the path we should save the basefile at and open it.
  char *basefilePath = malloc((strlen(xexfilePath) + strlen(".basefile") + 1) * sizeof(char));
  strcpy(basefilePath, xexfilePath);
  strcat(basefilePath, ".basefile");
  
  FILE* basefile = fopen(basefilePath, "wb+");
  free(xexfilePath); // *Now* we're done with it.
  free(basefilePath);
  
  if(basefile == NULL)
    {
      printf("%s ERROR: Could not create basefile. Do you have write permissions in the output directory? Aborting.\n", PRINT_STEM);
      fclose(pe);
      fclose(xex);
      free(xexfilePath);
      free(basefilePath);
      return -1;
    }
  
  mapPEToBasefile(pe, basefile);
  fclose(pe);
  
  printf("%s Created basefile!\n", PRINT_STEM);
  printf("%s Retrieving header data from basefile...\n", PRINT_STEM);

  if(getHdrData(basefile, &peData, 0) != 0)
    {
      printf("%s ERROR: Error in data retrieval from basefile. Aborting.\n", PRINT_STEM);
      fclose(basefile);
      fclose(xex);
      return -1;
    }

  printf("%s Got header data from basefile!\n", PRINT_STEM);

  // TEMPORARY: READ IN IMPORT HEADER DATA
//  FILE *importData = fopen("./import.bin", "r");
//
//  struct stat importStat;
//  fstat(fileno(importData), &importStat);
//  uint32_t importLen = importStat.st_size;
//
//  fread(&(optHeaders.importLibraries.size), sizeof(uint8_t), 4, importData);
//
//#ifdef LITTLE_ENDIAN_SYSTEM
//  optHeaders.importLibraries.size = ntohl(optHeaders.importLibraries.size);
//#endif
  
  //optHeaders.importLibraries.data = malloc(importLen - 0x4 * sizeof(uint8_t));
  //fread(optHeaders.importLibraries.data, sizeof(uint8_t), importLen - 0x4, importData);
  
  // Setting final XEX data structs
  printf("%s Building XEX header...\n", PRINT_STEM);
  setXEXHeader(&xexHeader);
  printf("%s Building security header...\n", PRINT_STEM);
  setSecInfoHeader(&secInfoHeader, &peData);
  printf("%s Setting page descriptors...\n", PRINT_STEM);
  setPageDescriptors(basefile, &peData, &secInfoHeader);
  printf("%s Building optional headers...\n", PRINT_STEM);
  setOptHeaders(&secInfoHeader, &peData, &optHeaderEntries, &optHeaders);
  
  // Setting data positions...
  printf("%s Aligning data...\n", PRINT_STEM);
  placeStructs(&offsets, &xexHeader, &optHeaderEntries, &secInfoHeader, &optHeaders);
  
  // Write out all of the XEX data to file
  printf("%s Writing data to XEX file...\n", PRINT_STEM);
  
  if(writeXEX(&xexHeader, &optHeaderEntries, &secInfoHeader, &optHeaders, &offsets, basefile, xex) != 0)
    {
      printf("%s ERROR: Unknown error in XEX write routine. Aborting.\n", PRINT_STEM);
      fclose(basefile);
      fclose(xex);
      return -1;
    }

  printf("%s Main data written to XEX file!\n", PRINT_STEM);
  
  //free(optHeaders.importLibraries.data);

  // Final pass (sets & writes header hash)
  printf("%s Calculating and writing header SHA1...\n", PRINT_STEM);
  setHeaderSha1(xex);
  printf("%s Header SHA1 written!\n", PRINT_STEM);

  // TEMPORARY: Hashing will be moved to earlier on when imports are implemented
  //setImportsSha1(xex);
  
  fclose(basefile);
  fclose(xex);
  
  printf("%s XEX built. Have a nice day!\n\n", PRINT_STEM);
  return 0;
}
