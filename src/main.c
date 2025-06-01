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

#include "common/common.h"
#include "common/datastorage.h"
#include "pemapper/pemapper.h"
#include "getdata/getdata.h"
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
  printf("-o,\t--output,\t\tSpecify output XEX file path\n\n");
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
    {0, 0, 0, 0}
  };

  int optIndex = 0;
  int option = 0;

  bool gotInput = false;
  bool gotOutput = false;
  bool skipMachineCheck = false;
  
  char *pePath = NULL;
  char *xexfilePath = NULL;
  
  while((option = getopt_long(argc, argv, "hvlsi:o:", longOptions, &optIndex)) != -1)
    {      
      if(option == 'h' || option == '?' || (option == 0 && strcmp(longOptions[optIndex].name, "help") == 0))
	{
	  dispHelp(argv);
	  return SUCCESS;
	}
      else if(option == 'v' || (option == 0 && strcmp(longOptions[optIndex].name, "version") == 0))
	{
	  dispVer();
	  return SUCCESS;
	}
      else if(option == 'l' || (option == 0 && strcmp(longOptions[optIndex].name, "libs") == 0))
	{
	  dispLibs();
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
	      if(xexfilePath != NULL) {free(xexfilePath);}
	      return -1;
	    }
	  
	  strncpy(pePath, optarg, strlen(optarg) + 1);
	}
      else if(option == 'o' || (option == 0 && strcmp(longOptions[optIndex].name, "output") == 0))
	{
	  gotOutput = true;
	  xexfilePath = malloc(strlen(optarg) + 1);

	  if(xexfilePath == NULL)
	    {
	      printf("%s ERROR: Out of memory. Aborting.\n", PRINT_STEM);
	      if(pePath != NULL) {free(pePath);}
	      return -1;
	    }
	  
	  strncpy(xexfilePath, optarg, strlen(optarg) + 1);
	}
    }

  printf("%s This is %s. Copyright (c) %s Aiden Isik.\n", PRINT_STEM, VERSION_STRING, COPYRIGHT);
  printf("%s This program is free/libre software. Run \"%s --version\" for info.\n\n", PRINT_STEM, argv[0]);
  
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

  if(pe == NULL)
    {
      printf("%s ERROR: Failed to open PE file. Do you have read permissions? Aborting.\n", PRINT_STEM);
      free(pePath);
      free(xexfilePath);
      return -1;
    }

  free(pePath);
  
  FILE *xex = fopen(xexfilePath, "wb+");

  if(xex == NULL)
    {
      printf("%s ERROR: Failed to create XEX file. Do you have write permissions? Aborting.\n", PRINT_STEM);
      fclose(pe);
      free(xexfilePath);
      return -1;
    }

  // Don't free this yet. We need it to determine where we can put the mapped basefile.
  // There *are* ways to get the file path from file pointer, but none of them are portable.
  //free(xexfilePath);

  int ret;
  
  struct offsets offsets;
  struct xexHeader xexHeader;
  struct secInfoHeader secInfoHeader;
  struct peData peData;
  struct optHeaderEntries optHeaderEntries;
  struct optHeaders optHeaders;

  // Make sure the import library size is initially zero
  optHeaders.importLibraries.size = 0;

  printf("%s Validating PE file...\n", PRINT_STEM);
  
  if(!validatePE(pe, skipMachineCheck))
    {
      printf("%s ERROR: Input PE is not Xbox 360 PE. Aborting.\n", PRINT_STEM);
      free(xexfilePath);
      fclose(pe);
      fclose(xex);
      return -1;
    }

  printf("%s PE valid!\n", PRINT_STEM);

  // Reading in header data from PE
  printf("%s Retrieving header data from PE...\n", PRINT_STEM);
  ret = getHdrData(pe, &peData, 0);
  
  if(ret == ERR_UNKNOWN_DATA_REQUEST)
    {
      printf("%s ERROR: Internal error getting data from PE file. THIS IS A BUG, please report it. Aborting.\n", PRINT_STEM);
      fclose(pe);
      fclose(xex);
      return -1;
    }
  else if(ret == ERR_FILE_READ)
    {
      printf("%s ERROR: Failed to read data from PE file. Aborting.\n", PRINT_STEM);
      fclose(pe);
      fclose(xex);
      return -1;
    }
  else if(ret == ERR_OUT_OF_MEM)
    {
      printf("%s ERROR: Out of memory. Aborting.\n", PRINT_STEM);
      fclose(pe);
      fclose(xex);
      return -1;
    }
  else if(ret == ERR_MISSING_SECTION_FLAG)
    {
      printf("%s ERROR: R/W/X flag missing from PE section. Aborting.\n", PRINT_STEM);
      fclose(pe);
      fclose(xex);
      return -1;
    }
  else if(ret == ERR_UNSUPPORTED_STRUCTURE)
    {
      printf("%s ERROR: Encountered an unsupported data structure in PE. Aborting.\n", PRINT_STEM);
      fclose(pe);
      fclose(xex);
      return -1;
    }

  printf("%s Got header data from PE!\n", PRINT_STEM);

  // Process imports
  printf("%s Retrieving import data from PE...\n", PRINT_STEM);
  ret = getImports(pe, &peData);

  if(ret != SUCCESS)
    {
      printf("TEMPORARY ERROR HANDLING: IMPORT DATA FAILED: %d\n", ret);
      exit(-1);
    }
  
  printf("%s Got import data from PE!\n", PRINT_STEM);
  
  // Determine the path we should save the basefile at and open it.
  printf("%s Creating basefile from PE...\n", PRINT_STEM);
  char *basefilePath = malloc((strlen(xexfilePath) + strlen(".basefile") + 1) * sizeof(char));

  if(basefilePath == NULL)
    {
      printf("%s ERROR: Out of memory. Aborting.\n", PRINT_STEM);
      free(xexfilePath);
      fclose(pe);
      fclose(xex);
      return -1;
    }

  strcpy(basefilePath, xexfilePath);
  strcat(basefilePath, ".basefile");
  
  FILE* basefile = fopen(basefilePath, "wb+");
  free(xexfilePath); // *Now* we're done with it.
  free(basefilePath);
  
  if(basefile == NULL)
    {
      printf("%s ERROR: Could not create basefile. Do you have write permission? Aborting.\n", PRINT_STEM);
      fclose(pe);
      fclose(xex);
      return -1;
    }

  // Map the PE into the basefile (RVAs become offsets)
  ret = mapPEToBasefile(pe, basefile, &peData);
  fclose(pe);

  if(ret == ERR_OUT_OF_MEM)
    {
      printf("%s ERROR: Out of memory. Aborting.\n", PRINT_STEM);
      fclose(basefile);
      fclose(xex);
      return -1;
    }
  
  printf("%s Created basefile!\n", PRINT_STEM);  
  
  // Setting final XEX data structs
  printf("%s Building XEX header...\n", PRINT_STEM);
  setXEXHeader(&xexHeader, &peData);
  
  printf("%s Building security header...\n", PRINT_STEM);
  setSecInfoHeader(&secInfoHeader, &peData);
  
  printf("%s Setting page descriptors...\n", PRINT_STEM);
  ret = setPageDescriptors(basefile, &peData, &secInfoHeader);

  if(ret == ERR_OUT_OF_MEM)
    {
      printf("%s ERROR: Out of memory. Aborting.\n", PRINT_STEM);
      fclose(basefile);
      fclose(xex);
      return -1;
    }
  
  printf("%s Building optional headers...\n", PRINT_STEM);
  ret = setOptHeaders(&secInfoHeader, &peData, &optHeaderEntries, &optHeaders);

  if(ret == ERR_OUT_OF_MEM)
    {
      printf("%s ERROR: Out of memory. Aborting.\n", PRINT_STEM);
      fclose(basefile);
      fclose(xex);
      return -1;
    }
  
  // Setting data positions...
  printf("%s Aligning data...\n", PRINT_STEM);
  ret = placeStructs(&offsets, &xexHeader, &optHeaderEntries, &secInfoHeader, &optHeaders);

  if(ret == ERR_OUT_OF_MEM)
    {
      printf("%s ERROR: Out of memory. Aborting.\n", PRINT_STEM);
      fclose(basefile);
      fclose(xex);
      return -1;
    }
  
  // Write out all of the XEX data to file
  printf("%s Writing data to XEX file...\n", PRINT_STEM);
  ret = writeXEX(&xexHeader, &optHeaderEntries, &secInfoHeader, &optHeaders, &offsets, basefile, xex);
  
  if(ret == ERR_OUT_OF_MEM)
    {
      printf("%s ERROR: Out of memory. Aborting.\n", PRINT_STEM);
      fclose(basefile);
      fclose(xex);
      return -1;
    }

  printf("%s Main data written to XEX file!\n", PRINT_STEM);

  // Final pass (sets & writes header hash)
  printf("%s Calculating and writing header SHA1...\n", PRINT_STEM);
  setHeaderSha1(xex);
  printf("%s Header SHA1 written!\n", PRINT_STEM);
  
  fclose(basefile);
  fclose(xex);
  
  printf("%s XEX built. Have a nice day!\n\n", PRINT_STEM);
  return SUCCESS;
}
