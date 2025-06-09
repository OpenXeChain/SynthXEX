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

// TODO: Harden all functions against bad data, unallocated data (check for NULL), etc.
// Some functions are not robust as I wanted things up and running as quickly as possible.

#include "common/common.h"
#include "common/datastorage.h"
#include "pemapper/pemapper.h"
#include "getdata/gethdrdata.h"
#include "getdata/getimports.h"
#include "setdata/populateheaders.h"
#include "setdata/pagedescriptors.h"
#include "setdata/optheaders.h"
#include "placer/placer.h"
#include "write/writexex.h"
#include "write/headerhash.h"

// Using the standalone getopt bundled, as getopt_long is not POSIX, so we can't rely on system <getopt.h>.
#include <getopt_port/getopt.h>

void dispLibs()
{
  printf("\nLibraries utilised by SynthXEX:\n\n");

  printf("----- GETOPT_PORT -----\n\n");
  printf("Copyright (c) 2012-2023, Kim Grasman <kim.grasman@gmail.com>\n");
  printf("All rights reserved.\n\n");

  printf("Redistribution and use in source and binary forms, with or without\n");
  printf("modification, are permitted provided that the following conditions are met:\n");
  printf("* Redistributions of source code must retain the above copyright\n");
  printf("  notice, this list of conditions and the following disclaimer.\n");
  printf("* Redistributions in binary form must reproduce the above copyright\n");
  printf("  notice, this list of conditions and the following disclaimer in the\n");
  printf("  documentation and/or other materials provided with the distribution.\n");
  printf("* Neither the name of Kim Grasman nor the\n");
  printf("  names of contributors may be used to endorse or promote products\n");
  printf("  derived from this software without specific prior written permission.\n\n");

  printf("THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS \"AS IS\" AND\n");
  printf("ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED\n");
  printf("WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE\n");
  printf("DISCLAIMED. IN NO EVENT SHALL KIM GRASMAN BE LIABLE FOR ANY\n");
  printf("DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES\n");
  printf("(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;\n");
  printf("LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND\n");
  printf("ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT\n");
  printf("(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS\n");
  printf("SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.\n\n\n");

  printf("----- GNU NETTLE (SHA-1) -----\n\n");
  printf("Copyright (C) 2001, 2013 Niels Möller\n");
  printf("This file is part of GNU Nettle.\n\n");

  printf("GNU Nettle is free software: you can redistribute it and/or\n");
  printf("modify it under the terms of either:\n\n");
  printf("* the GNU Lesser General Public License as published by the Free\n");
  printf("  Software Foundation; either version 3 of the License, or (at your\n");
  printf("  option) any later version.\n\n");
  printf("or\n\n");
  printf("* the GNU General Public License as published by the Free\n");
  printf("  Software Foundation; either version 2 of the License, or (at your\n");
  printf("  option) any later version.\n\n");
  printf("or both in parallel, as here.\n\n");

  printf("GNU Nettle is distributed in the hope that it will be useful,\n");
  printf("but WITHOUT ANY WARRANTY; without even the implied warranty of\n");
  printf("MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU\n");
  printf("General Public License for more details.\n\n");
  printf("You should have received copies of the GNU General Public License and\n");
  printf("the GNU Lesser General Public License along with this program. If\n");
  printf("not, see http://www.gnu.org/licenses/.\n\n");
}

void dispVer()
{
  printf("\n%s\n\nThe XEX builder of the FreeChainXenon project\n\n", VERSION_STRING);
  printf("Copyright (c) %s Aiden Isik\n\nThis program is free software: you can redistribute it and/or modify\n", COPYRIGHT);
  printf("it under the terms of the GNU Affero General Public License as published by\n");
  printf("the Free Software Foundation, either version 3 of the License, or\n");
  printf("(at your option) any later version.\n\nThis program is distributed in the hope that it will be useful,\n");
  printf("but WITHOUT ANY WARRANTY; without even the implied warranty of\nMERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the\n");
  printf("GNU Affero General Public License for more details.\n\n");
  printf("You should have received a copy of the GNU Affero General Public License\n");
  printf("along with this program. If not, see <https://www.gnu.org/licenses/>.\n\n");
}

void dispHelp(char **argv)
{
  printf("\nUsage: %s [OPTION] <ARG>\n\n", argv[0]);
  printf("Options:\n");
  printf("-h,\t--help,\t\t\tShow this information\n");
  printf("-v,\t--version,\t\tShow version and licensing information\n");
  printf("-l,\t--libs,\t\t\tShow licensing information of libraries used\n");
  printf("-s,\t--skip-machine-check,\tSkip the PE file machine ID check\n");
  printf("-i,\t--input,\t\tSpecify input PE file path\n");
  printf("-o,\t--output,\t\tSpecify output XEX file path\n");
  printf("-t,\t--type,\t\t\tOverride automatic executable type detection\n\t\t\t\t(options: title, titledll, sysdll, dll)\n\n");
}

int main(int argc, char **argv)
{
  static struct option longOptions[] =
  {
    {"help", no_argument, 0, 0},
    {"version", no_argument, 0, 0},
    {"libs", no_argument, 0, 0},
    {"skip-machine-check", no_argument, 0, 0},
    {"input", required_argument, 0, 0},
    {"output", required_argument, 0, 0},
    {"type", required_argument, 0, 0},
    {0, 0, 0, 0}
  };

  struct offsets *offsets = calloc(1, sizeof(struct offsets));
  struct xexHeader *xexHeader = calloc(1, sizeof(struct xexHeader));
  struct secInfoHeader *secInfoHeader = calloc(1, sizeof(struct secInfoHeader));
  struct peData *peData = calloc(1, sizeof(struct peData));
  struct optHeaderEntries *optHeaderEntries = calloc(1, sizeof(struct optHeaderEntries));
  struct optHeaders *optHeaders = calloc(1, sizeof(struct optHeaders));

  if(offsets == NULL || xexHeader == NULL || secInfoHeader == NULL || peData == NULL
     || optHeaderEntries == NULL || optHeaders == NULL)
    {
      printf("%s ERROR: Out of memory. Aborting\n", PRINT_STEM);
      freeAllMainStructs(&offsets, &xexHeader, &secInfoHeader, &peData, &optHeaderEntries, &optHeaders);
      return -1;
    }

  int optIndex = 0;
  int option = 0;

  bool gotInput = false;
  bool gotOutput = false;
  bool skipMachineCheck = false;
  
  char *pePath = NULL;
  char *xexfilePath = NULL;
  
  while((option = getopt_long(argc, argv, "hvlsi:o:t:", longOptions, &optIndex)) != -1)
    {      
      if(option == 'h' || option == '?' || (option == 0 && strcmp(longOptions[optIndex].name, "help") == 0))
	{
	  dispHelp(argv);
	  freeAllMainStructs(&offsets, &xexHeader, &secInfoHeader, &peData, &optHeaderEntries, &optHeaders);
	  return SUCCESS;
	}
      else if(option == 'v' || (option == 0 && strcmp(longOptions[optIndex].name, "version") == 0))
	{
	  dispVer();
	  freeAllMainStructs(&offsets, &xexHeader, &secInfoHeader, &peData, &optHeaderEntries, &optHeaders);
	  return SUCCESS;
	}
      else if(option == 'l' || (option == 0 && strcmp(longOptions[optIndex].name, "libs") == 0))
	{
	  dispLibs();
	  freeAllMainStructs(&offsets, &xexHeader, &secInfoHeader, &peData, &optHeaderEntries, &optHeaders);
	  return SUCCESS;
	}
      else if(option == 's' || (option == 0 && strcmp(longOptions[optIndex].name, "skip-machine-check") == 0))
	{
	  printf("%s WARNING: Skipping machine ID check.\n", PRINT_STEM);
	  skipMachineCheck = true;
	}
      else if(option == 'i' || (option == 0 && strcmp(longOptions[optIndex].name, "input") == 0))
	{
	  gotInput = true;
	  pePath = malloc(strlen(optarg) + 1);

	  if(pePath == NULL)
	    {
	      printf("%s ERROR: Out of memory. Aborting.\n", PRINT_STEM);
	      nullAndFree((void**)&xexfilePath);
	      freeAllMainStructs(&offsets, &xexHeader, &secInfoHeader, &peData, &optHeaderEntries, &optHeaders);
	      return -1;
	    }
	  
	  strncpy(pePath, optarg, strlen(optarg) + 1);
	  pePath[strlen(optarg)] = '\0';
	}
      else if(option == 'o' || (option == 0 && strcmp(longOptions[optIndex].name, "output") == 0))
	{
	  gotOutput = true;
	  xexfilePath = malloc(strlen(optarg) + 1);

	  if(xexfilePath == NULL)
	    {
	      printf("%s ERROR: Out of memory. Aborting.\n", PRINT_STEM);
	      nullAndFree((void**)&pePath);
	      freeAllMainStructs(&offsets, &xexHeader, &secInfoHeader, &peData, &optHeaderEntries, &optHeaders);
	      return -1;
	    }
	  
	  strncpy(xexfilePath, optarg, strlen(optarg) + 1);
	  xexfilePath[strlen(optarg)] = '\0';
	}
      else if(option == 't' || (option == 0 && strcmp(longOptions[optIndex].name, "type") == 0))
	{
	  if(strcmp(optarg, "title") == 0)
	    {
	      // Overriding type with "title"
	      xexHeader->moduleFlags = XEX_MOD_FLAG_TITLE;
	    }
	  else if(strcmp(optarg, "titledll") == 0)
	    {
	      // Overriding type with "titledll"
	      xexHeader->moduleFlags = XEX_MOD_FLAG_TITLE | XEX_MOD_FLAG_DLL;
	    }
	  else if(strcmp(optarg, "sysdll") == 0)
	    {
	      // Overriding type with "sysdll"
	      xexHeader->moduleFlags = XEX_MOD_FLAG_EXPORTS | XEX_MOD_FLAG_DLL;
	    }
	  else if(strcmp(optarg, "dll") == 0)
	    {
	      // Overriding type with "dll"
	      xexHeader->moduleFlags = XEX_MOD_FLAG_DLL;
	    }
	  else
	    {
	      printf("%s ERROR: Invalid type override \"%s\" (valid: title, titledll, sysdll, dll). Aborting.\n", PRINT_STEM, optarg);
	      nullAndFree((void**)&pePath);
	      nullAndFree((void**)&xexfilePath);
	      freeAllMainStructs(&offsets, &xexHeader, &secInfoHeader, &peData, &optHeaderEntries, &optHeaders);
	      return -1;
	    }
	}
    }

  printf("%s This is %s. Copyright (c) %s Aiden Isik.\n", PRINT_STEM, VERSION_STRING, COPYRIGHT);
  printf("%s This program is free/libre software. Run \"%s --version\" for info.\n\n", PRINT_STEM, argv[0]);
  
  // Check we got everything we need
  if(!gotInput)
    {
      if(gotOutput)
	{
	  nullAndFree((void**)&xexfilePath);
	}
      
      printf("%s ERROR: PE input expected but not found. Aborting.\n", PRINT_STEM);
      return -1;
    }
  else if(!gotOutput)
    {
      if(gotInput)
	{
	  nullAndFree((void**)&pePath);
	}

      printf("%s ERROR: XEX file output expected but not found. Aborting.\n", PRINT_STEM);
      return -1;
    }

  // Opening the files now that they've been validated
  FILE *pe = fopen(pePath, "rb");

  if(pe == NULL)
    {
      printf("%s ERROR: Failed to open PE file. Do you have read permissions? Aborting.\n", PRINT_STEM);
      nullAndFree((void**)&pePath);
      nullAndFree((void**)&xexfilePath);
      return -1;
    }

  nullAndFree((void**)&pePath);
  
  FILE *xex = fopen(xexfilePath, "wb+");

  if(xex == NULL)
    {
      printf("%s ERROR: Failed to create XEX file. Do you have write permissions? Aborting.\n", PRINT_STEM);
      fclose(pe);
      nullAndFree((void**)&xexfilePath);
      return -1;
    }

  // Don't free this yet. We need it to determine where we can put the mapped basefile.
  // There *are* ways to get the file path from file pointer, but none of them are portable.
  //free(xexfilePath);

  int ret;
  printf("%s Validating PE file...\n", PRINT_STEM);
  
  if(!validatePE(pe, skipMachineCheck))
    {
      printf("%s ERROR: Input PE is not Xbox 360 PE. Aborting.\n", PRINT_STEM);
      nullAndFree((void**)&xexfilePath);
      freeAllMainStructs(&offsets, &xexHeader, &secInfoHeader, &peData, &optHeaderEntries, &optHeaders);
      fclose(pe);
      fclose(xex);
      return -1;
    }

  printf("%s PE valid!\n", PRINT_STEM);

  // Reading in header data from PE
  printf("%s Retrieving header data from PE...\n", PRINT_STEM);
  ret = getHdrData(pe, peData, 0);

  if(ret != SUCCESS)
    {
      if(ret == ERR_UNKNOWN_DATA_REQUEST)
	{
	  printf("%s ERROR: Internal error getting data from PE file. THIS IS A BUG, please report it. Aborting.\n", PRINT_STEM);
	}
      else if(ret == ERR_FILE_READ)
	{
	  printf("%s ERROR: Failed to read data from PE file. Aborting.\n", PRINT_STEM);
	}
      else if(ret == ERR_OUT_OF_MEM)
	{
	  printf("%s ERROR: Out of memory. Aborting.\n", PRINT_STEM);
	}
      else if(ret == ERR_MISSING_SECTION_FLAG)
	{
	  printf("%s ERROR: R/W/X flag missing from PE section. Aborting.\n", PRINT_STEM);
	}
      else if(ret == ERR_UNSUPPORTED_STRUCTURE)
	{
	  printf("%s ERROR: Encountered an unsupported data structure in PE. Aborting.\n", PRINT_STEM);
	}
      else
	{
	  printf("%s ERROR: Unknown error: %d. Aborting.\n", PRINT_STEM, ret);
	}

      nullAndFree((void**)&xexfilePath);
      freeAllMainStructs(&offsets, &xexHeader, &secInfoHeader, &peData, &optHeaderEntries, &optHeaders);
      fclose(pe);
      fclose(xex);
      return -1;
    }

  printf("%s Got header data from PE!\n", PRINT_STEM);

  // Process imports
  printf("%s Retrieving import data from PE...\n", PRINT_STEM);
  ret = getImports(pe, peData);

  if(ret != SUCCESS)
    {
      if(ret == ERR_OUT_OF_MEM)
	{
	  printf("%s ERROR: Out of memory. Aborting.\n", PRINT_STEM);
	}
      else if(ret == ERR_FILE_READ)
	{
	  printf("%s ERROR: Failed to read data from PE file. Aborting.\n", PRINT_STEM);
	}
      else if(ret == ERR_INVALID_RVA_OR_OFFSET)
	{
	  printf("%s ERROR: Invalid RVA or offset encountered (malformed PE). Aborting.\n", PRINT_STEM);
	}
      else if(ret == ERR_UNSUPPORTED_STRUCTURE) // Does import by name actually need special treatment or can this go??
	{
	  printf("%s ERROR: Import by name detected. This is currently unsupported. Aborting.\n", PRINT_STEM);
	}
      else
	{
	  printf("%s ERROR: Unknown error: %d. Aborting.\n", PRINT_STEM, ret);
	}

      nullAndFree((void**)&xexfilePath);
      freeAllMainStructs(&offsets, &xexHeader, &secInfoHeader, &peData, &optHeaderEntries, &optHeaders);
      fclose(pe);
      fclose(xex);
      return -1;
    }
  
  printf("%s Got import data from PE!\n", PRINT_STEM);
  
  // Determine the path we should save the basefile at and open it.
  printf("%s Creating basefile from PE...\n", PRINT_STEM);
  char *basefilePath = malloc((strlen(xexfilePath) + strlen(".basefile") + 1) * sizeof(char));

  if(basefilePath == NULL)
    {
      printf("%s ERROR: Out of memory. Aborting.\n", PRINT_STEM);
      freeAllMainStructs(&offsets, &xexHeader, &secInfoHeader, &peData, &optHeaderEntries, &optHeaders);
      nullAndFree((void**)&xexfilePath);
      fclose(pe);
      fclose(xex);
      return -1;
    }

  strcpy(basefilePath, xexfilePath);
  strcat(basefilePath, ".basefile");
  
  FILE* basefile = fopen(basefilePath, "wb+");
  nullAndFree((void**)&xexfilePath); // *Now* we're done with it.
  nullAndFree((void**)&basefilePath);
  
  if(basefile == NULL)
    {
      printf("%s ERROR: Could not create basefile. Do you have write permission? Aborting.\n", PRINT_STEM);
      freeAllMainStructs(&offsets, &xexHeader, &secInfoHeader, &peData, &optHeaderEntries, &optHeaders);
      fclose(pe);
      fclose(xex);
      return -1;
    }

  // Map the PE into the basefile (RVAs become offsets)
  ret = mapPEToBasefile(pe, basefile, peData);
  fclose(pe);

  if(ret != SUCCESS)
    {
      if(ret == ERR_OUT_OF_MEM)
	{
	  printf("%s ERROR: Out of memory. Aborting.\n", PRINT_STEM);
	}
      else
	{
	  printf("%s ERROR: Unknown error: %d. Aborting.\n", PRINT_STEM, ret);
	}

      freeAllMainStructs(&offsets, &xexHeader, &secInfoHeader, &peData, &optHeaderEntries, &optHeaders);
      fclose(basefile);
      fclose(xex);
      return -1;
    }
  
  printf("%s Created basefile!\n", PRINT_STEM);  
  
  // Setting final XEX data structs 
  printf("%s Building security header...\n", PRINT_STEM);
  ret = setSecInfoHeader(secInfoHeader, peData);

  if(ret != SUCCESS)
    {
      printf("%s ERROR: Unknown error: %d. Aborting.\n", PRINT_STEM, ret);
      freeAllMainStructs(&offsets, &xexHeader, &secInfoHeader, &peData, &optHeaderEntries, &optHeaders);
      fclose(basefile);
      fclose(xex);
      return -1;
    }
  
  printf("%s Setting page descriptors...\n", PRINT_STEM);
  ret = setPageDescriptors(basefile, peData, secInfoHeader);

  if(ret != SUCCESS)
    {
      if(ret == ERR_OUT_OF_MEM)
	{
	  printf("%s ERROR: Out of memory. Aborting.\n", PRINT_STEM);
	}
      else
	{
	  printf("%s ERROR: Unknown error: %d. Aborting.\n", PRINT_STEM, ret);
	}

      freeAllMainStructs(&offsets, &xexHeader, &secInfoHeader, &peData, &optHeaderEntries, &optHeaders);
      fclose(basefile);
      fclose(xex);
      return -1;
    }

  // Done with this now
  freeSectionsStruct(&(peData->sections));
  
  printf("%s Building optional headers...\n", PRINT_STEM);
  ret = setOptHeaders(secInfoHeader, peData, optHeaderEntries, optHeaders);

  if(ret != SUCCESS)
    {
      if(ret == ERR_OUT_OF_MEM)
	{
	  printf("%s ERROR: Out of memory. Aborting.\n", PRINT_STEM);
	}
      else
	{
	  printf("%s ERROR: Unknown error: %d. Aborting.\n", PRINT_STEM, ret);
	}
      
      freeAllMainStructs(&offsets, &xexHeader, &secInfoHeader, &peData, &optHeaderEntries, &optHeaders);
      fclose(basefile);
      fclose(xex);
      return -1;
    }

  printf("%s Building XEX header...\n", PRINT_STEM);
  ret = setXEXHeader(xexHeader, optHeaderEntries, peData);

  if(ret != SUCCESS)
    {
      printf("%s ERROR: Unknown error: %d. Aborting.\n", PRINT_STEM, ret);
      freeAllMainStructs(&offsets, &xexHeader, &secInfoHeader, &peData, &optHeaderEntries, &optHeaders);
      fclose(basefile);
      fclose(xex);
      return -1;
    }

  // Done with this now
  freePeImportInfoStruct(&(peData->peImportInfo));
  
  // Setting data positions...
  printf("%s Aligning data...\n", PRINT_STEM);
  ret = placeStructs(offsets, xexHeader, optHeaderEntries, secInfoHeader, optHeaders);

  if(ret != SUCCESS)
    {
      if(ret == ERR_OUT_OF_MEM)
	{
	  printf("%s ERROR: Out of memory. Aborting.\n", PRINT_STEM);
	}
      else
	{
	  printf("%s ERROR: Unknown error: %d. Aborting.\n", PRINT_STEM, ret);
	}
      
      freeAllMainStructs(&offsets, &xexHeader, &secInfoHeader, &peData, &optHeaderEntries, &optHeaders);
      fclose(basefile);
      fclose(xex);
      return -1;
    }

  // We're done with this now
  freePeDataStruct(&peData);

  // Write out all of the XEX data to file
  printf("%s Writing data to XEX file...\n", PRINT_STEM);
  ret = writeXEX(xexHeader, optHeaderEntries, secInfoHeader, optHeaders, offsets, basefile, xex);

  if(ret != SUCCESS)
    {
      if(ret == ERR_OUT_OF_MEM)
	{
	  printf("%s ERROR: Out of memory. Aborting.\n", PRINT_STEM);
	}
      else
	{
	  printf("%s ERROR: Unknown error: %d. Aborting.\n", PRINT_STEM, ret);
	}
      
      freeAllMainStructs(&offsets, &xexHeader, &secInfoHeader, &peData, &optHeaderEntries, &optHeaders);
      fclose(basefile);
      fclose(xex);
      return -1;
    }

  printf("%s Main data written to XEX file!\n", PRINT_STEM);

  // Final pass (sets & writes header hash)
  printf("%s Calculating and writing header SHA1...\n", PRINT_STEM);
  ret = setHeaderSha1(xex);

  if(ret != SUCCESS)
    {
      printf("%s ERROR: Unknown error: %d. Aborting.\n", PRINT_STEM, ret);
      freeAllMainStructs(&offsets, &xexHeader, &secInfoHeader, &peData, &optHeaderEntries, &optHeaders);
      fclose(basefile);
      fclose(xex);
      return -1;
    }

  printf("%s Header SHA1 written!\n", PRINT_STEM);
  
  fclose(basefile);
  fclose(xex);
  
  printf("%s XEX built. Have a nice day!\n\n", PRINT_STEM);

  // Free everything left and exit
  freeAllMainStructs(&offsets, &xexHeader, &secInfoHeader, &peData, &optHeaderEntries, &optHeaders);
  return SUCCESS;
}
